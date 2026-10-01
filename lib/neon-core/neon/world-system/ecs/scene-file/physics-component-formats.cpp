#include "component-format.hpp"

#include <format>

#include <neon/world-system/ecs/components/character-body.hpp>
#include <neon/world-system/ecs/components/collider.hpp>
#include <neon/world-system/ecs/components/rigid-body.hpp>
#include <neon/world-system/ecs/components/trigger.hpp>

// How the components of the physics are written in a scene file follows
// from their descriptions, which are next to the components. What is left
// here is what sets them apart from the other components of the engine.

namespace neon
{
  namespace
  {
    /// The format of a component that the physics registers. A world
    /// without physics does not know the component, which is said instead
    /// of giving the entity something the store cannot hold.
    template<typename T>
    ComponentFormat PhysicsFormat()
    {
      auto format = ComponentFormat::Of<T>();

      format.read = [type = format.type](const DataReader &reader, EntityStore &store, const Entity entity)
      {
        // read first, so that what is wrong with it is said as well
        T component{};
        ReadFields(*type, reader, &component);

        if (store.FindComponent(type->name) == No_Component)
        {
          reader.Report({}, std::format(
                          "{} needs the physics, which is not part of this world",
                          reader.GetWhere()));
          return;
        }

        store.Set(entity, component);
      };

      return format;
    }
  }

  void ComponentFormats::AddPhysicsComponents()
  {
    Add(PhysicsFormat<RigidBody>());
    Add(PhysicsFormat<Trigger>());
    Add(PhysicsFormat<CharacterBody>());
    Add(PhysicsFormat<Collider>());
  }
} // neon
