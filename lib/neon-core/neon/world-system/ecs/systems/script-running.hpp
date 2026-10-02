#ifndef SCRIPT_RUNNING_HPP
#define SCRIPT_RUNNING_HPP

#include <memory>
#include <string>

#include <neon/logging/logger.hpp>
#include <neon/physics/physics-context.hpp>
#include <neon/scripting/script-context.hpp>
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
  /// it happened in.
  class ScriptRunning final : public EntitySystem
  {
    ScriptContext *_scripts;
    PhysicsContext *_physics;
    ComponentFormats *_formats;
    std::string _folder;
    std::shared_ptr<Logger> _logger;

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

    void Register(EntityStore &store) override;

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;

    void FixedUpdate(EntityStore &store, double fixed_delta_time) override;
  };
} // neon

#endif //SCRIPT_RUNNING_HPP
