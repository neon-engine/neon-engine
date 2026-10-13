#ifndef WORLD_SYSTEM_HPP
#define WORLD_SYSTEM_HPP

#include <string>

#include <neon/data/data-value.hpp>
#include <neon/input/input-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/render-pipeline.hpp>
#include <neon/window/window-context.hpp>
#include <neon/world-system/ecs/entity.hpp>
#include <neon/world-system/possession-listener.hpp>

namespace neon
{
  class WorldSystem
  {
  protected:
    WindowContext *_window_context;
    InputContext *_input_context;
    RenderPipeline *_render_pipeline;
    std::shared_ptr<Logger> _logger;

    ~WorldSystem() = default;

  public:
    WorldSystem(
      RenderPipeline *render_pipeline,
      InputContext *input_context,
      WindowContext *window_context,
      const std::shared_ptr<Logger> &logger)
    {
      _render_pipeline = render_pipeline;
      _input_context = input_context;
      _window_context = window_context;
      _logger = logger;
    }

    virtual void Initialize() = 0;

    virtual void Update() = 0;

    virtual void CleanUp() = 0;

    /// Holds the world still while a menu is shown: nothing moves and no
    /// time passes for it, and it is still drawn. It goes on with false.
    virtual void SetPaused(bool paused) {}

    [[nodiscard]] virtual bool IsPaused() const
    {
      return false;
    }

    /// Whether the scene the world started with could not be read at all,
    /// its file missing or no document, so that there is nothing to run. The
    /// log says which scene and why. A world without scenes always has one.
    [[nodiscard]] virtual bool HasNoScene() const
    {
      return false;
    }

    /// Asks for the scene at that virtual path to take the place of what is
    /// there, which happens at the next Update(). A world that cannot
    /// change scene ignores it.
    virtual void LoadScene(const std::string &file_path)
    {
    }

    /// Places the prefab recipe at that virtual path below `parent`, which
    /// is No_Entity for the top, while the game plays, as a scene places
    /// it. `overrides` is read on top, in the form of the `components` of
    /// an entity in a scene file, or nothing to override none. Returns the
    /// entity, or No_Entity when nothing was spawned, which is in the log.
    /// A world that cannot spawn returns No_Entity.
    virtual Entity Spawn(const std::string &path, Entity parent, const DataValue &overrides)
    {
      return No_Entity;
    }

    /// Possesses the entity, see docs/possession.md: the input reaches it
    /// alone from now on and the view follows it, the entity that was
    /// possessed is released, and the hooks of both are called. An entity
    /// that is gone or carries no Possessable is refused, with the reason
    /// in the log. Returns whether the entity is possessed. A world that
    /// knows no possession refuses.
    virtual bool Possess(Entity entity)
    {
      return false;
    }

    /// Releases the possessed entity, so that nothing is possessed.
    virtual void Unpossess()
    {
    }

    /// The entity that is possessed, or No_Entity when nothing is.
    [[nodiscard]] virtual Entity GetPossessed() const
    {
      return No_Entity;
    }

    /// Whoever is told when an entity is possessed or released, which is
    /// how the scripts and the extensions hear of it. A world that knows
    /// no possession tells nobody.
    virtual void AddPossessionListener(PossessionListener *listener)
    {
    }
  };
}

#endif // WORLD_SYSTEM_HPP
