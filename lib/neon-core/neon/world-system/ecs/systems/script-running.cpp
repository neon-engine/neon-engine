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

  void ScriptRunning::AddFolder(const std::string &folder)
  {
    _more_folders.push_back(folder);
  }

  void ScriptRunning::SetUi(UiContext *ui)
  {
    _ui = ui;
  }

  void ScriptRunning::Register(EntityStore &store)
  {
    if (!_scripts->LoadScripts(_folder, store, *_formats))
    {
      _logger->Info("No scripts: there is no folder {}", _folder);
    } else
    {
      const std::size_t components = _scripts->GetComponentCount();
      const std::size_t systems = _scripts->GetSystemCount();
      _logger->Info("Scripts under {} declare {} components and {} systems", _folder, components, systems);
    }

    for (const auto &folder : _more_folders)
    {
      if (!_scripts->LoadScripts(folder, store, *_formats)) { continue; }

      const std::size_t components = _scripts->GetComponentCount();
      const std::size_t systems = _scripts->GetSystemCount();
      _logger->Info("With the scripts under {}, {} components and {} systems are declared", folder, components, systems);
    }
  }

  void ScriptRunning::Initialize(EntityStore &store)
  {
    _scripts->Start(store);
  }

  void ScriptRunning::Update(EntityStore &store, const double delta_time)
  {
    if (_physics != nullptr) { _scripts->DispatchPhysicsEvents(store, _physics->GetFrameEvents()); }

    if (_ui != nullptr)
    {
      // taken first: a function that is called may load and unload files
      _calls.clear();
      for (const UiEvent &event : _ui->GetEvents())
      {
        if (event.call.IsEmpty()) { continue; }

        // the event says who shows the file and who pointed at the screen,
        // as the loading and the pointing told the user interface; either
        // may be gone by now
        auto showing = static_cast<Entity>(event.owner);
        auto instigator = static_cast<Entity>(event.pointed_by);
        if (showing != No_Entity && !store.IsAlive(showing)) { showing = No_Entity; }
        if (instigator != No_Entity && !store.IsAlive(instigator)) { instigator = No_Entity; }
        _calls.push_back({.entity = showing, .instigator = instigator, .event = event});
      }

      if (!_calls.empty()) { _scripts->DispatchUiCalls(store, _calls); }
    }

    _scripts->Update(store, delta_time);
  }

  void ScriptRunning::FixedUpdate(EntityStore &store, const double fixed_delta_time)
  {
    _scripts->FixedUpdate(store, fixed_delta_time);
  }
} // neon
