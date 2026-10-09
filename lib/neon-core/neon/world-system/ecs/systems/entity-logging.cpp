#include "entity-logging.hpp"

#include <format>

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/character-body.hpp>
#include <neon/world-system/ecs/components/rigid-body.hpp>

namespace neon
{
  // Helpers of EntityLogging, for this file alone.
  namespace
  {
    /// A place as a scene writes one, `[x, y, z]`, to a millimeter.
    std::string Place(const glm::vec3 &position)
    {
      return std::format("[{:.3f}, {:.3f}, {:.3f}]", position.x, position.y, position.z);
    }
  }

  EntityLogging::EntityLogging(
    std::vector<std::string> paths,
    PhysicsContext *physics,
    std::shared_ptr<Logger> logger)
    : _paths(std::move(paths)), _physics(physics), _logger(std::move(logger))
  {
  }

  void EntityLogging::Initialize(EntityStore &store)
  {
    _transform = store.FindComponent("Transform");
    _rigid_body = store.FindComponent("RigidBody");
    _character_body = store.FindComponent("CharacterBody");
  }

  bool EntityLogging::FindBody(EntityStore &store, const Entity entity, glm::vec3 &position) const
  {
    if (_physics == nullptr) { return false; }

    if (_rigid_body != No_Component)
    {
      if (const auto *body = static_cast<const RigidBody *>(store.GetComponent(entity, _rigid_body));
          body != nullptr && body->body != No_Body)
      {
        BodyState state;
        if (!_physics->GetBodyState(body->body, state)) { return false; }
        position = state.position;
        return true;
      }
    }

    if (_character_body != No_Component)
    {
      if (const auto *body = static_cast<const CharacterBody *>(store.GetComponent(entity, _character_body));
          body != nullptr && body->character != No_Character)
      {
        return _physics->GetCharacterPosition(body->character, position);
      }
    }

    return false;
  }

  void EntityLogging::Update(EntityStore &store, double)
  {
    _frame++;

    for (const auto &path : _paths)
    {
      const Entity entity = store.FindEntity(path);
      if (entity == No_Entity)
      {
        // said once, and not in every frame. It is logged once it is there
        if (_missing.insert(path).second)
        {
          _logger->Warn("Frame {}: there is no entity {} to log the place of. It is logged once it is there",
                        _frame, path);
        }
        continue;
      }
      _missing.erase(path);

      const auto *transform = _transform == No_Component
                                ? nullptr
                                : static_cast<const Transform *>(store.GetComponent(entity, _transform));
      if (transform == nullptr)
      {
        if (_placeless.insert(path).second)
        {
          _logger->Warn("Frame {}: the entity {} has no Transform, so it has no place to log", _frame, path);
        }
        continue;
      }
      _placeless.erase(path);

      const std::string drawn = Place(glm::vec3(transform->world_coordinates[3]));

      if (glm::vec3 body; FindBody(store, entity, body))
      {
        const std::string simulated = Place(body);
        _logger->Info("Frame {}: {} is at {}, its body at {}", _frame, path, drawn, simulated);
      } else
      {
        _logger->Info("Frame {}: {} is at {}", _frame, path, drawn);
      }
    }
  }
} // neon
