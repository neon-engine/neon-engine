#ifndef ENTITY_WORLD_HPP
#define ENTITY_WORLD_HPP

#include <memory>
#include <vector>

#include <neon/world-system/world-system.hpp>

#include "entity-store.hpp"
#include "entity-system.hpp"
#include "fixed-clock.hpp"
#include "scene.hpp"

namespace neon
{
  /// The world as entities, components, and systems.
  ///
  /// Every frame runs in this order:
  ///   1. the steps of the world that the time of the frame asks for, which
  ///      can be none. Every step calls FixedUpdate of every system, in the
  ///      order of 2 and 3
  ///   2. the systems of the engine that react to input
  ///   3. the systems that were added with AddSystem, in the order they were
  ///      added
  ///   4. placing every entity in the world
  ///   5. Interpolate of every system, which places what is drawn between
  ///      the last two steps
  ///   6. handing the camera, the lights, and what is visible to the renderer
  ///
  /// So a system that was added sees the input of the frame, and what it
  /// moves is drawn where it moved it to.
  class EntityWorld final : public WorldSystem
  {
    EntityStore *_store;
    Scene *_scene;

    std::vector<std::unique_ptr<EntitySystem>> _before;
    std::vector<std::unique_ptr<EntitySystem>> _added;
    std::vector<std::unique_ptr<EntitySystem>> _placing;
    std::vector<std::unique_ptr<EntitySystem>> _after;

    FixedClock _fixed_clock;

    bool _initialized = false;
    bool _paused = false;

    void RegisterComponents() const;

  public:
    EntityWorld(
      EntityStore *store,
      Scene *scene,
      RenderPipeline *render_pipeline,
      InputContext *input_context,
      WindowContext *window_context,
      const std::shared_ptr<Logger> &logger);

    /// Cleans up, so that the store is not left with components that refer
    /// to a world that is gone.
    ~EntityWorld();

    /// Adds behaviour of a game. Call it before Initialize.
    void AddSystem(std::unique_ptr<EntitySystem> system);

    /// Decides when the world takes a step. It is where the number of steps
    /// per second is set, and what a system asks that needs to know about
    /// the steps.
    [[nodiscard]] FixedClock &GetFixedClock();

    void Initialize() override;

    void Update() override;

    void SetPaused(bool paused) override;

    [[nodiscard]] bool IsPaused() const override;

    void CleanUp() override;
  };
} // neon

#endif //ENTITY_WORLD_HPP
