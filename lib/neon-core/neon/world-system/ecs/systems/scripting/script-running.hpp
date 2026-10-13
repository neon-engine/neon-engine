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
#include <neon/world-system/possession-listener.hpp>
#include <neon/world-system/world-system.hpp>

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
  /// an element that names a function with `on_click`, or a change the
  /// player made to one that names it with `on_change`, calls it on the
  /// systems of the entity whose `Ui` or `UiSurface` shows the file. And
  /// so does what was possessed and released, see docs/possession.md: it
  /// is kept as the world tells it and handed to the hooks in the next
  /// Update, before `update`, since a hook may possess in turn.
  class ScriptRunning final : public EntitySystem, public PossessionListener
  {
    ScriptContext *_scripts;
    PhysicsContext *_physics;
    ComponentFormats *_formats;
    std::string _folder;

    // folders besides the first that are read when they are there, such as
    // the assets of the extensions
    std::vector<std::string> _more_folders;
    std::shared_ptr<Logger> _logger;

    // where the clicks come from
    UiContext *_ui = nullptr;
    std::vector<ScriptUiCall> _calls;

    // what was possessed and released since the scripts were told
    std::vector<PossessionEvent> _possession_events;


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

    /// The world whose possession the scripts hear of and ask for. Without
    /// one the hooks of possession are never called. Call it before the
    /// world is initialized.
    void SetWorld(WorldSystem *world);

    void OnPossession(const PossessionEvent &event) override;

    void Register(EntityStore &store) override;

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;

    void FixedUpdate(EntityStore &store, double fixed_delta_time) override;
  };
} // neon

#endif //SCRIPT_RUNNING_HPP
