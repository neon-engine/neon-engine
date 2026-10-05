#include "pool-system.hpp"

#include <algorithm>
#include <format>

#include <neon/data/data-value.hpp>
#include <neon/world-system/ecs/components/pool-manager.hpp>
#include <neon/world-system/ecs/components/pool-instance.hpp>

namespace neon
{
  // Helpers of PoolSystem, for this file alone.
  namespace
  {
    /// The PoolManager of an entity, on or off, or nullptr.
    PoolManager *pool_of(EntityStore &store, const Entity entity)
    {
      const ComponentId id = store.FindComponent("PoolManager");
      if (id == No_Component) { return nullptr; }
      return static_cast<PoolManager *>(store.GetComponentData(entity, id));
    }

    PoolInstance *pooled_of(EntityStore &store, const Entity entity)
    {
      const ComponentId id = store.FindComponent("PoolInstance");
      if (id == No_Component) { return nullptr; }
      return static_cast<PoolInstance *>(store.GetComponentData(entity, id));
    }

    /// The kind of a name in a pool, made when `makes` says so, or nullptr.
    PoolKind *kind_of(PoolManager &pool, const std::string &name, const bool makes, std::size_t &index)
    {
      for (index = 0; index < pool.kinds.size(); index++)
      {
        if (pool.kinds[index].name == name) { return &pool.kinds[index]; }
      }
      if (!makes) { return nullptr; }

      pool.kinds.push_back(PoolKind{.name = name});
      return &pool.kinds.back();
    }

    /// What a prefab is called without its folders and its endings:
    /// `nail` for `assets://prefabs/nail.prefab.yml`.
    std::string short_name_of(const std::string &path)
    {
      const std::size_t slash = path.find_last_of('/');
      std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
      if (const std::size_t dot = name.find('.'); dot != std::string::npos) { name.resize(dot); }
      return name;
    }

    /// Takes an entity into a kind of a pool: marked, turned off, waiting.
    void take_in(EntityStore &store, const Entity pool, PoolKind &kind, const std::size_t index, const Entity instance)
    {
      store.SetComponent(instance, store.FindComponent("PoolInstance"), &static_cast<const PoolInstance &>(PoolInstance{pool, index, false}));
      PoolSystem::SetInUse(store, instance, false);
      kind.instances.push_back(instance);
      kind.free.push_back(instance);
    }
  }

  PoolSystem::PoolSystem(WorldSystem *world, const std::shared_ptr<Logger> &logger) : _world(world), _logger(logger) {}

  void PoolSystem::Register(EntityStore &store)
  {
    store.Register<PoolManager>("PoolManager");
    store.Register<PoolInstance>("PoolInstance");
  }

  void PoolSystem::Initialize(EntityStore &store)
  {
    _pools = store.Query<PoolManager>();
  }

  void PoolSystem::Update(EntityStore &store, double)
  {
    // the pools first, and then each is filled: filling makes entities,
    // which a query must not see while it is walked
    std::vector<Entity> unfilled;
    std::vector<Entity> asked_in_vain;
    store.Each(_pools, [&](const EntityBlock &block)
    {
      const auto *pools = block.Column<PoolManager>(0);
      for (std::size_t i = 0; i < block.count; i++)
      {
        const PoolManager &pool = pools[i];
        if (!pool.is_filled) { unfilled.push_back(block.entities[i]); }

        const bool missed = std::ranges::any_of(pool.kinds, [](const PoolKind &kind) { return kind.missed > 0; });
        if (missed || pool.unknown_said < pool.unknown.size()) { asked_in_vain.push_back(block.entities[i]); }
      }
    });

    for (const Entity pool : unfilled) { Fill(store, pool); }

    // what was asked of a pool in vain since the last update
    for (const Entity pool : asked_in_vain) { Report(store, pool); }
  }

  void PoolSystem::Fill(EntityStore &store, const Entity pool)
  {
    // a copy: spawning gives entities components, which may move the pool's
    PoolManager *held = pool_of(store, pool);
    if (held == nullptr) { return; }
    held->is_filled = true;
    const std::vector<PoolEntry> entries = held->entries;
    const std::string pool_name = store.GetName(pool);

    for (const PoolEntry &entry : entries)
    {
      if (entry.source.empty() || entry.count <= 0)
      {
        _logger->Error(
          "An entry of the pool '{}' is left out: it needs a 'source' and a 'count' above 0", pool_name);
        continue;
      }

      std::vector<Entity> made;
      if (entry.source.starts_with(kInstance_Scheme))
      {
        // An entity that is there already, by its path. It is the one
        // instance: making more of it takes cloning, which is #406.
        const std::string path = entry.source.substr(std::string(kInstance_Scheme).size());
        const Entity found = store.FindEntity(path);
        if (found == No_Entity)
        {
          _logger->Error("The pool '{}' finds no entity '{}' for {}", pool_name, path, entry.source);
          continue;
        }
        if (entry.count > 1)
        {
          _logger->Error(
            "The pool '{}' holds one instance of {} and not {}: an entity that is there is taken as it is, and "
            "making more of it is not there yet. Use a prefab, or hand the pool its instances in code",
            pool_name, entry.source, entry.count);
        }
        made.push_back(found);
      } else if (_world == nullptr)
      {
        _logger->Error("The pool '{}' cannot spawn {}: this world spawns no prefabs", pool_name, entry.source);
        continue;
      } else
      {
        const std::string name = short_name_of(entry.source);
        for (int number = 1; number <= entry.count; number++)
        {
          const Entity instance = _world->Spawn(entry.source, pool, DataValue{});
          if (instance == No_Entity) { break; }

          // a name is an address, so each has a number
          store.SetName(instance, std::format("{} {}", name, number));
          made.push_back(instance);
        }
        if (const std::size_t held_now = made.size(); held_now < static_cast<std::size_t>(entry.count))
        {
          _logger->Error(
            "The pool '{}' holds {} of {} and not {}: the prefab could not be spawned",
            pool_name, held_now, entry.source, entry.count);
        }
      }

      Add(store, pool, entry.source, made, _logger.get());
    }
  }

  void PoolSystem::SetInUse(EntityStore &store, const Entity instance, const bool in_use)
  {
    // The body before its collider when they are turned off, and after it
    // when they are turned on, so that the body keeps its shapes, see
    // PhysicsSimulation.
    static constexpr const char *off_order[] = {"RigidBody", "Collider", "Renderable"};
    static constexpr const char *on_order[] = {"Renderable", "Collider", "RigidBody"};

    std::vector<Entity> open{instance};
    while (!open.empty())
    {
      const Entity entity = open.back();
      open.pop_back();

      for (const char *name : in_use ? on_order : off_order)
      {
        if (const ComponentId id = store.FindComponent(name); id != No_Component) { store.SetEnabled(entity, id, in_use); }
      }
      for (const Entity child : store.GetChildren(entity)) { open.push_back(child); }
    }
  }

  bool PoolSystem::Add(
    EntityStore &store,
    const Entity pool,
    const std::string &kind,
    const std::vector<Entity> &instances,
    Logger *logger)
  {
    if (pool_of(store, pool) == nullptr || kind.empty()) { return false; }

    for (const Entity instance : instances)
    {
      if (!store.IsAlive(instance) || instance == pool || pooled_of(store, instance) != nullptr)
      {
        if (logger != nullptr)
        {
          const std::string pool_name = store.GetName(pool);
          logger->Warn(
            "The pool '{}' does not take an instance of {}: it is gone, or belongs to a pool already",
            pool_name, kind);
        }
        continue;
      }

      // found anew each time: giving an entity a component may move the pool
      std::size_t index = 0;
      PoolKind *held = kind_of(*pool_of(store, pool), kind, true, index);
      take_in(store, pool, *held, index, instance);
    }
    return true;
  }

  Entity PoolSystem::Acquire(EntityStore &store, const Entity pool, const std::string &kind)
  {
    PoolManager *held = pool_of(store, pool);
    if (held == nullptr) { return No_Entity; }

    std::size_t index = 0;
    PoolKind *found = kind_of(*held, kind, false, index);
    if (found == nullptr)
    {
      // kept, to be said once, see Report()
      if (std::ranges::find(held->unknown, kind) == held->unknown.end()) { held->unknown.push_back(kind); }
      return No_Entity;
    }

    // one that was destroyed while it waited is passed over
    while (!found->free.empty() && !store.IsAlive(found->free.front())) { found->free.pop_front(); }

    if (found->free.empty())
    {
      found->missed++;
      return No_Entity;
    }

    const Entity instance = found->free.front();
    found->free.pop_front();
    if (PoolInstance *pooled = pooled_of(store, instance); pooled != nullptr) { pooled->is_out = true; }
    SetInUse(store, instance, true);
    return instance;
  }

  void PoolSystem::Report(EntityStore &store, const Entity pool) const
  {
    PoolManager *held = pool_of(store, pool);
    if (held == nullptr) { return; }

    const std::string pool_name = store.GetName(pool);
    for (; held->unknown_said < held->unknown.size(); held->unknown_said++)
    {
      const std::string &kind = held->unknown[held->unknown_said];
      _logger->Warn("The pool '{}' was asked for {}, which it does not hold", pool_name, kind);
    }

    for (PoolKind &kind : held->kinds)
    {
      if (kind.missed == 0) { continue; }

      if (!kind.said_empty)
      {
        const std::size_t all = kind.instances.size();
        _logger->Warn(
          "The pool '{}' ran out of {}: all {} are out, and it hands out none until one is given back. "
          "It holds as many as it was told to",
          pool_name, kind.name, all);
        kind.said_empty = true;
      }
      kind.missed = 0;
    }
  }

  bool PoolSystem::Release(EntityStore &store, const Entity instance)
  {
    PoolInstance *pooled = pooled_of(store, instance);
    if (pooled == nullptr || !pooled->is_out) { return false; }

    PoolManager *held = pool_of(store, pooled->pool);
    if (held == nullptr || pooled->kind >= held->kinds.size()) { return false; }

    pooled->is_out = false;
    SetInUse(store, instance, false);

    PoolKind &kind = held->kinds[pooled->kind];
    kind.free.push_back(instance);
    kind.said_empty = false;
    return true;
  }

  std::vector<std::string> PoolSystem::GetKinds(EntityStore &store, const Entity pool)
  {
    std::vector<std::string> names;
    if (const PoolManager *held = pool_of(store, pool); held != nullptr)
    {
      for (const PoolKind &kind : held->kinds) { names.push_back(kind.name); }
    }
    return names;
  }

  std::size_t PoolSystem::GetCount(EntityStore &store, const Entity pool, const std::string &kind)
  {
    PoolManager *held = pool_of(store, pool);
    std::size_t index = 0;
    const PoolKind *found = held == nullptr ? nullptr : kind_of(*held, kind, false, index);
    return found == nullptr ? 0 : found->instances.size();
  }

  std::size_t PoolSystem::GetFreeCount(EntityStore &store, const Entity pool, const std::string &kind)
  {
    PoolManager *held = pool_of(store, pool);
    std::size_t index = 0;
    const PoolKind *found = held == nullptr ? nullptr : kind_of(*held, kind, false, index);
    return found == nullptr ? 0 : found->free.size();
  }
} // neon
