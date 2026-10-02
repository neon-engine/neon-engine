#include "player-movement.hpp"

#include <algorithm>
#include <cmath>

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/character-body.hpp>
#include <neon/world-system/ecs/components/collider.hpp>
#include <neon/world-system/ecs/components/player.hpp>

namespace neon
{
  // Helpers of PlayerMovement: where the feet of the body are.
  namespace
  {
    /// How far the lowest point of a collider lies below the origin of its
    /// entity, in the units of the entity. The physics stands the body on
    /// that point, so the eyes are measured from it.
    float BottomOf(const Collider &collider)
    {
      switch (collider.shape)
      {
        case ShapeKind::Box: return collider.offset.y - collider.size.y * 0.5f;
        case ShapeKind::Sphere: return collider.offset.y - collider.radius;
        case ShapeKind::Capsule:
        case ShapeKind::Cylinder:
        case ShapeKind::TaperedCapsule:
        case ShapeKind::TaperedCylinder: return collider.offset.y - collider.height * 0.5f;
        default: return collider.offset.y;
      }
    }

    /// Where the camera sits on its body: `eye_height` above the feet, in
    /// the units of the body, which its scale sizes.
    float EyeOf(const Player &player, const Transform &body, const Collider *collider)
    {
      const float scale = body.scale.y != 0.0f ? body.scale.y : 1.0f;
      const float feet = collider != nullptr ? BottomOf(*collider) : 0.0f;
      return feet + player.eye_height / scale;
    }
  }

  PlayerMovement::PlayerMovement(InputContext *input_context)
  {
    _input_context = input_context;
  }

  void PlayerMovement::Initialize(EntityStore &store)
  {
    _players = store.Query<Transform, Player, CharacterBody>();
    _cameras = store.Query<Transform, Camera>();
  }

  void PlayerMovement::Update(EntityStore &store, const double delta_time)
  {
    // the actions of the input map, which the project binds: `move` is to
    // the right and forward, `look` to the right and up, in pixels
    const glm::vec2 move = _input_context->ActionAxis("move");
    const glm::vec2 look = _input_context->ActionAxis("look");
    const bool run = _input_context->IsActionDown("run");
    const bool jump = _input_context->WasActionPressed("jump");

    store.Each(_players, [&](const EntityBlock &block)
    {
      auto *transforms = block.Column<Transform>(0);
      const auto *players = block.Column<Player>(1);
      auto *bodies = block.Column<CharacterBody>(2);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &transform = transforms[i];
        const auto &player = players[i];
        auto &body = bodies[i];

        // the turn comes first, so that a step goes the way the player
        // looks after it. The body turns around its up alone, the pitch
        // belongs to the camera
        transform.rotation.yaw -= glm::degrees(look.x * player.look_speed);

        const float yaw = glm::radians(transform.rotation.yaw);
        const glm::vec3 forward{-std::sin(yaw), 0.0f, -std::cos(yaw)};
        const glm::vec3 right{std::cos(yaw), 0.0f, -std::sin(yaw)};

        // two keys together walk no faster than one, and a stick pushed
        // halfway walks at half the speed
        glm::vec3 direction = forward * move.y + right * move.x;
        if (const float length = glm::length(direction); length > 1.0f) { direction /= length; }

        body.velocity = direction * (run ? player.run_speed : player.walk_speed);

        // a jump replaces what falling added; the physics lets it fall back
        if (jump && body.on_floor) { body.fall_velocity = {0.0f, player.jump_speed, 0.0f}; }
      }
    });

    store.Each(_cameras, [&](const EntityBlock &block)
    {
      auto *transforms = block.Column<Transform>(0);

      for (std::size_t i = 0; i < block.count; i++)
      {
        const Entity parent = store.GetParent(block.entities[i]);
        if (parent == No_Entity) { continue; }

        const auto *player = store.Get<Player>(parent);
        if (player == nullptr) { continue; }

        const auto *body = store.Get<Transform>(parent);
        if (body == nullptr) { continue; }

        auto &transform = transforms[i];
        transform.position = {0.0f, EyeOf(*player, *body, store.Get<Collider>(parent)), 0.0f};
        transform.rotation.pitch = std::clamp(
          transform.rotation.pitch + glm::degrees(look.y * player->look_speed),
          -player->max_pitch,
          player->max_pitch);
      }
    });
  }
} // neon
