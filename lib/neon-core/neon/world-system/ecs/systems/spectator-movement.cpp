#include "spectator-movement.hpp"

#include <algorithm>

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/spectator.hpp>

namespace neon
{
  SpectatorMovement::SpectatorMovement(InputContext *input_context)
  {
    _input_context = input_context;
  }

  void SpectatorMovement::Initialize(EntityStore &store)
  {
    _query = store.Query<Transform, Spectator>();
  }

  void SpectatorMovement::Update(EntityStore &store, const double delta_time)
  {
    // the actions of the input map, which the project binds: `move` is to
    // the right and forward, `look` to the right and up
    const glm::vec2 move = _input_context->ActionAxis("move");
    const glm::vec2 look = _input_context->ActionAxis("look");

    store.Each(_query, [&](const EntityBlock &block)
    {
      auto *transforms = block.Column<Transform>(0);
      const auto *spectators = block.Column<Spectator>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &transform = transforms[i];
        const auto &spectator = spectators[i];

        const glm::vec3 move_direction = (transform.Forward() * move.y + transform.Right() * move.x) *
                                         spectator.move_speed;

        if (look.x != 0.0f || look.y != 0.0f)
        {
          transform.rotation.yaw -= look.x * spectator.look_speed;
          transform.rotation.pitch += look.y * spectator.look_speed;
          transform.rotation.pitch = std::clamp(transform.rotation.pitch, -89.f, 89.f);
        }

        transform.position += move_direction * static_cast<float>(delta_time);
      }
    });
  }
} // neon
