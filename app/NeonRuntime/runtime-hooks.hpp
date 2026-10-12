#ifndef RUNTIME_HOOKS_HPP
#define RUNTIME_HOOKS_HPP

#include <optional>
#include <string>

#include <neon/runtime/composition-hooks.hpp>

#include "neon-runtime.hpp"

/// NeonRuntime's own choices on the shared start-up, see
/// RuntimeComposition: the runtime registers no option set beyond the
/// runtime's, runs the project that is put together next to it, and takes
/// the settings as they are. It owns the NeonRuntime that is run.
class RuntimeHooks final : public neon::CompositionHooks
{
  std::optional<NeonRuntime> _runtime;

public:
  /// The runtime ships no game, so one started as it was built has none.
  [[nodiscard]] std::string AdviseWithoutAProject(const SettingsConfig &settings) const override;

  neon::Runtime &MakeRuntime(const neon::RuntimeSystems &systems) override;
};

#endif //RUNTIME_HOOKS_HPP
