#ifndef DEMO_SCENE_HPP
#define DEMO_SCENE_HPP

#include <neon/world-system/ecs/scene.hpp>

/// The scene NeonRuntime shows until it can load one from a project. It is
/// written in code.
class DemoScene final : public neon::Scene
{
public:
  void Populate(neon::EntityStore &store) override;
};

#endif //DEMO_SCENE_HPP
