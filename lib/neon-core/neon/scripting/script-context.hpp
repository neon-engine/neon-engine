#ifndef SCRIPT_CONTEXT_HPP
#define SCRIPT_CONTEXT_HPP

#include <cstddef>
#include <string>
#include <vector>

#include <neon/physics/physics-types.hpp>
#include <neon/world-system/ecs/entity-store.hpp>
#include <neon/world-system/ecs/scene-file/component-format.hpp>

#include "script-ui-call.hpp"

namespace neon
{
  /// What the rest of the engine sees of the scripts of a game.
  ///
  /// A script declares components and systems, as the engine's own code
  /// does. The engine finds the scripts itself, under one folder of the
  /// project, registers what they declare, and runs their systems next to
  /// its own. Which language the scripts are in is a backend behind this
  /// interface; the first is Lua.
  class ScriptContext
  {
  protected:
    ~ScriptContext() = default;

  public:
    /// Reads every script under `folder` and below it, in the order of
    /// their paths, and registers the components they declare with the
    /// store and the formats, so that a recipe can hold them. A script with
    /// a problem is reported and left out; the rest are loaded. Returns
    /// false when the folder cannot be listed, which is not an error: a
    /// game without scripts has no folder.
    virtual bool LoadScripts(const std::string &folder, EntityStore &store, ComponentFormats &formats) = 0;

    /// Makes the queries of the systems. Called once every component is
    /// registered, the engine's as well as the scripts', and before the
    /// scene is read.
    virtual void Start(EntityStore &store) = 0;

    /// Runs the `update` of every system, once per frame.
    virtual void Update(EntityStore &store, double delta_time) = 0;

    /// Runs the `step` of every system, once per step of the world.
    virtual void FixedUpdate(EntityStore &store, double fixed_delta_time) = 0;

    /// Hands the systems what the physics reported in a frame: a body
    /// entering or leaving a trigger, two bodies touching or parting.
    virtual void DispatchPhysicsEvents(EntityStore &store, const std::vector<PhysicsEvent> &events) = 0;

    /// Calls the functions the user interface asked for in a frame, each
    /// on the systems of the entity that shows the user interface: an
    /// element that says `on_click: unlock` calls `unlock` of every system
    /// that runs over the entity and has a function of that name, with
    /// the entity as what it is called on, `self` in Lua, and what the file
    /// wrote as arguments.
    virtual void DispatchUiCalls(EntityStore &store, const std::vector<ScriptUiCall> &calls) = 0;

    /// How many components the scripts declared.
    [[nodiscard]] virtual std::size_t GetComponentCount() const = 0;

    /// How many systems the scripts declared.
    [[nodiscard]] virtual std::size_t GetSystemCount() const = 0;
  };
} // neon

#endif //SCRIPT_CONTEXT_HPP
