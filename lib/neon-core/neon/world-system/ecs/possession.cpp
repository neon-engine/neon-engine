#include "possession.hpp"

#include "components/first-person-controller.hpp"
#include "components/possessable.hpp"
#include "components/possessed.hpp"
#include "components/spectator.hpp"

namespace neon
{
  Possession::Possession(EntityStore *store, const std::shared_ptr<Logger> &logger)
  {
    _store = store;
    _logger = logger;
  }

  void Possession::AddListener(PossessionListener *listener)
  {
    if (listener != nullptr) { _listeners.push_back(listener); }
  }

  void Possession::Tell(const Entity entity, const bool possessed) const
  {
    const PossessionEvent event{.entity = entity, .possessed = possessed};
    for (auto *listener : _listeners) { listener->OnPossession(event); }
  }

  void Possession::Release()
  {
    const Entity released = GetPossessed();
    _possessed = No_Entity;
    if (released == No_Entity) { return; }

    _store->Remove<Possessed>(released);
    const std::string name = _store->GetName(released);
    _logger->Info("Released '{}'", name);
    Tell(released, false);
  }

  void Possession::Take(const Entity entity)
  {
    Release();
    _possessed = entity;
    _store->Set(entity, Possessed{});
    const std::string name = _store->GetName(entity);
    _logger->Info("Possessed '{}'", name);
    Tell(entity, true);
  }

  bool Possession::Possess(const Entity entity)
  {
    if (entity == No_Entity || !_store->IsAlive(entity))
    {
      _logger->Error("An entity that is gone cannot be possessed, nothing changes");
      return false;
    }
    if (entity == GetPossessed()) { return true; }

    if (!_store->Has<Possessable>(entity))
    {
      const std::string name = _store->GetName(entity);
      _logger->Error("'{}' cannot be possessed: it carries no Possessable, which marks what can be. Nothing changes", name);
      return false;
    }

    Take(entity);
    return true;
  }

  void Possession::Unpossess()
  {
    Release();
  }

  Entity Possession::GetPossessed() const
  {
    if (_possessed != No_Entity && !_store->IsAlive(_possessed)) { return No_Entity; }
    return _possessed;
  }

  Entity Possession::FirstWithBelow(const Entity parent, const ComponentId component) const
  {
    for (const Entity entity : _store->GetChildren(parent))
    {
      if (_store->HasComponent(entity, component)) { return entity; }
      if (const Entity below = FirstWithBelow(entity, component); below != No_Entity) { return below; }
    }
    return No_Entity;
  }

  Entity Possession::FirstWith(const ComponentId component) const
  {
    if (component == No_Component) { return No_Entity; }
    return FirstWithBelow(No_Entity, component);
  }

  void Possession::Start(const std::string &named)
  {
    if (!named.empty())
    {
      if (const Entity entity = _store->FindEntity(named); entity != No_Entity)
      {
        // the scene says so, whatever the entity carries
        if (entity != GetPossessed()) { Take(entity); }
        return;
      }
      _logger->Error("The scene possesses '{}', and there is no such entity. What would be possessed without a name is", named);
    }

    // what was possessed stayed through the change of scene
    if (GetPossessed() != No_Entity) { return; }
    _possessed = No_Entity;

    for (const ComponentId component : {
           _store->IdOf<Possessable>(), _store->IdOf<FirstPersonController>(), _store->IdOf<Spectator>()
         })
    {
      if (const Entity entity = FirstWith(component); entity != No_Entity)
      {
        Take(entity);
        return;
      }
    }

    _logger->Info("Nothing is possessed: the scene has no Possessable, no FirstPersonController, and no Spectator");
  }
} // neon
