#ifndef SCRIPT_RUNNING_HPP
#define SCRIPT_RUNNING_HPP

#include <memory>
#include <string>
#include <vector>

#include <neon/logging/logger.hpp>
#include <neon/physics/physics-context.hpp>
#include <neon/scripting/script-context.hpp>
#include <neon/ui/ui-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Runs the systems the scripts of a game declare, as one system of the
  /// world. The scripts are read when the components are registered, so
  /// that a scene can hold what they declare, and their systems run in the
  /// place this one was added at: before the physics, as the systems of a
  /// game do, so that what they ask for in a step is part of it.
  ///
  /// What the physics reported in the frame reaches the scripts before
  /// their `update`, so that a hook of a trigger or a touch sees the frame
  /// it happened in. So does what the user interface asked for: a click on
  /// an element that names a function with `on_click` calls it on the
  /// systems of the entity whose `Ui` or `UiSurface` shows the file.
  class ScriptRunning final : public EntitySystem
  {
    ScriptContext *_scripts;
    PhysicsContext *_physics;
    ComponentFormats *_formats;
    std::string _folder;

    // folders besides the first that are read when they are there, such as
    // the assets of the extensions
    std::vector<std::string> _more_folders;
    std::shared_ptr<Logger> _logger;

    // where the clicks come from, and the entities that show a file
    UiContext *_ui = nullptr;
    QueryId _views = 0;
    QueryId _surfaces = 0;
    std::vector<ScriptUiCall> _calls;

    /// The entity that shows the file the user interface knows by that
    /// number, or No_Entity, and in `instigator` who pointed at it, for a
    /// file on a surface in the world.
    [[nodiscard]] Entity FindShowing(EntityStore &store, int document, Entity &instigator) const;

  public:
    /// `physics` may be nullptr, for a world without physics: the hooks of
    /// triggers and touches are then never called. `formats` is where the
    /// scripts' components are added, so that the scene reads them.
    ScriptRunning(
      ScriptContext *scripts,
      PhysicsContext *physics,
      ComponentFormats *formats,
      std::string folder,
      const std::shared_ptr<Logger> &logger);

    /// Reads the scripts of another folder as well, after those of the
    /// first, when the folder is there; one that is not there is left out
    /// without a word. For what brings scripts besides the project, such as
    /// an extension. Call it before the world is initialized.
    void AddFolder(const std::string &folder);

    /// The user interface whose elements call functions of the scripts.
    /// Without one nothing is called. Call it before the world is
    /// initialized.
    void SetUi(UiContext *ui);

    void Register(EntityStore &store) override;

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;

    void FixedUpdate(EntityStore &store, double fixed_delta_time) override;
  };
} // neon

#endif //SCRIPT_RUNNING_HPP
