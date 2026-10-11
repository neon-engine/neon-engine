#include "scene-exiting.hpp"

#include <neon/world-system/ecs/components/scene-exit.hpp>
#include <neon/world-system/ecs/components/trigger.hpp>

namespace neon
{
  SceneExiting::SceneExiting(WorldSystem *world)
  {
    _world = world;
  }

  void SceneExiting::Initialize(EntityStore &store)
  {
    _exits = store.Query<SceneExit>();

    // the physics registers Trigger, when there is one; it is looked up by
    // name so that a world without physics has exits that never open
    _trigger = store.FindComponent("Trigger");
  }

  void SceneExiting::Update(EntityStore &store, double delta_time)
  {
    if (_trigger == No_Component) { return; }

    std::string scene;
    store.Each(_exits, [&](const EntityBlock &block)
    {
      const auto *exits = block.Column<SceneExit>(0);
      for (std::size_t i = 0; i < block.count; i++)
      {
        const auto *trigger = static_cast<const Trigger *>(store.GetComponent(block.entities[i], _trigger));
        if (trigger == nullptr || trigger->inside == 0 || exits[i].scene.empty()) { continue; }
        scene = exits[i].scene;
      }
    });

    if (!scene.empty()) { _world->LoadScene(scene); }
  }
} // neon
