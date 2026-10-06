#include "script-running.hpp"

#include <utility>

#include <neon/world-system/ecs/components/ui-surface-view.hpp>
#include <neon/world-system/ecs/components/ui-view.hpp>

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
    if (_ui != nullptr)
    {
      _views = store.Query<UiView>();
      _surfaces = store.Query<UiSurfaceView>();
    }

    _scripts->Start(store);
  }

  Entity ScriptRunning::FindShowing(EntityStore &store, const int document, Entity &instigator) const
  {
    Entity showing = No_Entity;
    instigator = No_Entity;
    if (document < 0) { return showing; }

    store.Each(_views, [&](const EntityBlock &block)
    {
      const auto *views = block.Column<UiView>(0);
      for (std::size_t i = 0; i < block.count; i++)
      {
        if (views[i].document == document) { showing = block.entities[i]; }
      }
    });

    store.Each(_surfaces, [&](const EntityBlock &block)
    {
      const auto *views = block.Column<UiSurfaceView>(0);
      for (std::size_t i = 0; i < block.count; i++)
      {
        if (views[i].document != document) { continue; }

        showing = block.entities[i];
        instigator = views[i].pointed_by;
      }
    });

    // who pointed last may be gone by now
    if (instigator != No_Entity && !store.IsAlive(instigator)) { instigator = No_Entity; }

    return showing;
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
        Entity instigator = No_Entity;
        const Entity showing = FindShowing(store, event.document_id, instigator);
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
