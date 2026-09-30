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
    const auto &input_state = _input_context->GetInputState();

    store.Each(_query, [&](const EntityBlock &block)
    {
      auto *transforms = block.Column<Transform>(0);
      const auto *spectators = block.Column<Spectator>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &transform = transforms[i];
        const auto &spectator = spectators[i];

        auto move_direction = glm::vec3{0.0f, 0.0f, 0.0f};

        if (input_state[Action::L_Up])
        {
          move_direction += transform.Forward() * spectator.move_speed;
        }

        if (input_state[Action::L_Down])
        {
          move_direction += transform.Forward() * -spectator.move_speed;
        }

        if (input_state[Action::L_Left])
        {
          move_direction += transform.Right() * -spectator.move_speed;
        }

        if (input_state[Action::L_Right])
        {
          move_direction += transform.Right() * spectator.move_speed;
        }

        if (input_state[Action::Mouse])
        {
          const auto x = input_state[Axis::Mouse].x;
          const auto y = input_state[Axis::Mouse].y;

          transform.rotation.yaw -= static_cast<float>(x) * spectator.look_speed;
          transform.rotation.pitch -= static_cast<float>(y) * spectator.look_speed;
          transform.rotation.pitch = std::clamp(transform.rotation.pitch, -89.f, 89.f);
        }

        transform.position += move_direction * static_cast<float>(delta_time);
      }
    });
  }
} // neon
