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
  ///   6. building the meshes of entities with a Geometry that have none
  ///      yet, then handing the camera, the lights, and what is visible to
  ///      the renderer
  ///   7. the systems that were added with AddSystemAfterPlacing, in the
  ///      order they were added
  ///
  /// So a system that was added sees the input of the frame, and what it
  /// moves is drawn where it moved it to. A system that was added after
  /// placing sees every entity where it is drawn in this frame.
  class EntityWorld final : public WorldSystem
  {
    EntityStore *_store;
    Scene *_scene;

    std::vector<std::unique_ptr<EntitySystem>> _before;
    std::vector<std::unique_ptr<EntitySystem>> _added;
    std::vector<std::unique_ptr<EntitySystem>> _placing;
    std::vector<std::unique_ptr<EntitySystem>> _after;
    std::vector<std::unique_ptr<EntitySystem>> _added_after_placing;

    FixedClock _fixed_clock;

    bool _initialized = false;
    bool _paused = false;

    // the scene the world started with could not be read at all
    bool _has_no_scene = false;

    // the scene asked for, taken at the start of the next frame, so that a
    // change never happens in the middle of one
    std::string _scene_to_load;

    // the entities with a SceneExit, looked at after every frame
    QueryId _exits = 0;

    void RegisterComponents() const;

    /// Reads the scene that was asked for, then destroys every entity that
    /// does not stay and places the scene. A scene that cannot be read at
    /// all changes nothing.
    void ChangeScene();

    /// Draws the frame, and after the first one of a scene that took the
    /// place of another asks the renderer to free what nothing shows any
    /// more.
    void RenderFrame();

    bool _frees_unused_after_frame = false;

    /// Asks for the scene of a SceneExit whose Trigger has a body inside.
    void CheckExits();

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

    /// Adds behavior of a game. Call it before Initialize.
    void AddSystem(std::unique_ptr<EntitySystem> system);

    /// Adds a system that needs every entity where it is drawn, such as
    /// one that plays sounds where their entities are. It moves nothing,
    /// since nothing would be placed again before the frame is drawn. It is
    /// updated while the world is paused too, with no time passing. Call it
    /// before Initialize.
    void AddSystemAfterPlacing(std::unique_ptr<EntitySystem> system);

    /// Decides when the world takes a step. It is where the number of steps
    /// per second is set, and what a system asks that needs to know about
    /// the steps.
    [[nodiscard]] FixedClock &GetFixedClock();

    /// Asks for another scene, by its virtual path. It is read at the start
    /// of the next frame, and when it can be read, every entity of the
    /// world is destroyed, except those with a Persistent component, which
    /// stay with their children, and then the scene fills the store. When
    /// it cannot be read at all, the log says why and the world stays as it
    /// is. The systems stay as
    /// they are, so what they hold for a destroyed entity is released
    /// through the store, as when an entity goes away in play.
    void LoadScene(const std::string &file_path) override;

    /// Whether a scene was asked for and not read yet.
    [[nodiscard]] bool IsChangingScene() const;

    /// Places a prefab below `parent` at once, through the scene, so that
    /// what is spawned is what the scene would have placed: it carries the
    /// Prefab component, and the file is read once for the life of the
    /// scene. A world that is not initialized, a path that is empty, and a
    /// parent that is not alive are refused and said in the log. See
    /// WorldSystem::Spawn.
    Entity Spawn(const std::string &path, Entity parent, const DataValue &overrides) override;

    void Initialize() override;

    void Update() override;

    void SetPaused(bool paused) override;

    [[nodiscard]] bool IsPaused() const override;

    /// Whether the scene the world started with could not be read at all. A
    /// scene it is asked to change to and cannot read leaves it where it
    /// is.
    [[nodiscard]] bool HasNoScene() const override;

    void CleanUp() override;
  };
} // neon

#endif //ENTITY_WORLD_HPP
