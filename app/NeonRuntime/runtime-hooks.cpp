#include "runtime-hooks.hpp"

std::string RuntimeHooks::AdviseWithoutAProject(const SettingsConfig &) const
{
  return "NeonRuntime runs the project that is put together next to it, and has none of its own. "
         "Start a project instead, such as the sandbox of the engine's build, bin/<build>/<platform>/sandbox, "
         "see docs/projects.md";
}

neon::Runtime &RuntimeHooks::MakeRuntime(const neon::RuntimeSystems &systems)
{
  return _runtime.emplace(
    *systems.settings,
    systems.window,
    systems.input,
    systems.render,
    systems.render_pipeline,
    systems.logging,
    systems.world,
    systems.logger);
}
