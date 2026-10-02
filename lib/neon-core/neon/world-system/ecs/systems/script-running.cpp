#include "script-running.hpp"

#include <utility>

namespace neon
{
  ScriptRunning::ScriptRunning(
    ScriptContext *scripts,
    PhysicsContext *physics,
    ComponentFormats *formats,
    std::string folder,
    const std::shared_ptr<Logger> &logger)
    : _scripts(scripts), _physics(physics), _formats(formats), _folder(std::move(folder)), _logger(logger) {}

  void ScriptRunning::Register(EntityStore &store)
  {
    if (!_scripts->LoadScripts(_folder, store, *_formats))
    {
      _logger->Info("No scripts: there is no folder {}", _folder);
      return;
    }

    const std::size_t components = _scripts->GetComponentCount();
    const std::size_t systems = _scripts->GetSystemCount();
    _logger->Info("Scripts under {} declare {} components and {} systems", _folder, components, systems);
  }

  void ScriptRunning::Initialize(EntityStore &store)
  {
    _scripts->Start(store);
  }

  void ScriptRunning::Update(EntityStore &store, const double delta_time)
  {
    if (_physics != nullptr) { _scripts->DispatchPhysicsEvents(store, _physics->GetFrameEvents()); }
    _scripts->Update(store, delta_time);
  }

  void ScriptRunning::FixedUpdate(EntityStore &store, const double fixed_delta_time)
  {
    _scripts->FixedUpdate(store, fixed_delta_time);
  }
} // neon
