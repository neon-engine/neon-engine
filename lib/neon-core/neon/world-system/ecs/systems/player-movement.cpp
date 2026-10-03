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
  // Helpers of PlayerMovement: where the camera sits, and the motion between frames.
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

    /// Metres into the units of the body, which its scale sizes.
    glm::vec3 ScaleOf(const Transform &body)
    {
      return {
        body.scale.x != 0.0f ? body.scale.x : 1.0f,
        body.scale.y != 0.0f ? body.scale.y : 1.0f,
        body.scale.z != 0.0f ? body.scale.z : 1.0f
      };
    }

    /// Where the camera sits on its body: `eye_height` above the feet, less
    /// what the eyes still lag behind a step, moved by `camera_offset`, all
    /// in the units of the body.
    glm::vec3 CameraOf(const Player &player, const Transform &body, const Collider *collider)
    {
      const glm::vec3 scale = ScaleOf(body);
      const float feet = collider != nullptr ? BottomOf(*collider) : 0.0f;
      const glm::vec3 eyes{0.0f, feet, 0.0f};
      const glm::vec3 above{0.0f, player.eye_height + player.eye_glide, 0.0f};
      return eyes + (above + player.camera_offset) / scale;
    }

    /// The velocity the keys ask for in the air: the one the body left the
    /// ground with, pulled towards what the keys ask for by `air_control`.
    glm::vec3 SteeredInTheAir(const Player &player, const glm::vec3 &asked)
    {
      return player.ground_velocity + (asked - player.ground_velocity) * player.air_control;
    }

    /// How far the eyes lag behind the body after this frame: what they
    /// lagged before fades at `step_smoothing`, and what the body rose or
    /// sank by on the floor is added, so that a step is seen over a few
    /// frames. In the air, and after landing, nothing is added.
    float GlideOf(const Player &player, const CharacterBody &body, const float height, const double delta_time)
    {
      if (player.step_smoothing <= 0.0f) { return 0.0f; }

      float glide = player.eye_glide * std::exp(-player.step_smoothing * static_cast<float>(delta_time));
      if (body.on_floor && player.was_on_floor) { glide -= height - player.last_height; }
      return glide;
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
    const glm::vec2 move = _input_context->ActionAxis2("move");
    const glm::vec2 look = _input_context->ActionAxis2("look");
    const bool run = _input_context->IsActionDown("run");
    const bool jump = _input_context->WasActionPressed("jump");

    store.Each(_players, [&](const EntityBlock &block)
    {
      auto *transforms = block.Column<Transform>(0);
      auto *players = block.Column<Player>(1);
      auto *bodies = block.Column<CharacterBody>(2);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &transform = transforms[i];
        auto &player = players[i];
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

        const glm::vec3 asked = direction * (run ? player.run_speed : player.walk_speed);

        // on the ground the keys say where to go. In the air the body keeps
        // what it took off with, and the keys steer it by air_control, so
        // that a jump goes on when W is released over the gap
        if (body.on_floor)
        {
          body.velocity = asked;
          player.ground_velocity = asked;
        } else
        {
          body.velocity = SteeredInTheAir(player, asked);
        }

        // a jump replaces what falling added; the physics lets it fall back
        if (jump && body.on_floor) { body.fall_velocity = {0.0f, player.jump_speed, 0.0f}; }

        // the eyes glide after a step the physics took the body up or down
        const float height = transform.position.y;
        player.eye_glide = GlideOf(player, body, height, delta_time);
        player.last_height = height;
        player.was_on_floor = body.on_floor;
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
        transform.position = CameraOf(*player, *body, store.Get<Collider>(parent));
        transform.rotation.pitch = std::clamp(
          transform.rotation.pitch + glm::degrees(look.y * player->look_speed),
          -player->max_pitch,
          player->max_pitch);
      }
    });
  }
} // neon
