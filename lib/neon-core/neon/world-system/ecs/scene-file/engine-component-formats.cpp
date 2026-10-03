#include "component-format.hpp"

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/light.hpp>
#include <neon/world-system/ecs/components/player.hpp>
#include <neon/world-system/ecs/components/geometry.hpp>
#include <neon/world-system/ecs/components/persistent.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>
#include <neon/world-system/ecs/components/rope.hpp>
#include <neon/world-system/ecs/components/scene-exit.hpp>
#include <neon/world-system/ecs/components/sky.hpp>
#include <neon/world-system/ecs/components/sound-listener.hpp>
#include <neon/world-system/ecs/components/sound-source.hpp>
#include <neon/world-system/ecs/components/spectator.hpp>

namespace neon
{
  void ComponentFormats::AddEngineComponents()
  {
    // how each is read and written follows from its description, which is
    // next to the component
    Add(ComponentFormat::Of<Transform>());
    Add(ComponentFormat::Of<Renderable>());
    Add(ComponentFormat::Of<Geometry>());
    Add(ComponentFormat::Of<Rope>());
    Add(ComponentFormat::Of<Camera>());
    Add(ComponentFormat::Of<Light>());
    Add(ComponentFormat::Of<Sky>());
    Add(ComponentFormat::Of<Spectator>());
    Add(ComponentFormat::Of<Player>());
    Add(ComponentFormat::Of<SoundSource>());
    Add(ComponentFormat::Of<SoundListener>());
    Add(ComponentFormat::Of<Persistent>());
    Add(ComponentFormat::Of<SceneExit>());

    // described as well, and told apart by needing the physics
    AddPhysicsComponents();
  }
} // neon
