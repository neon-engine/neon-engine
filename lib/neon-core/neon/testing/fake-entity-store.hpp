#ifndef FAKE_ENTITY_STORE_HPP
#define FAKE_ENTITY_STORE_HPP

#include <set>
#include <algorithm>
#include <cstddef>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

#include <neon/world-system/ecs/entity-store.hpp>

namespace neon::testing
{
  /// An entity store that is as simple as one can be, for testing what works
  /// on entities without a backend. The tests of neon-core use it, since they
  /// must not depend on Flecs.
  ///
  /// It does what EntityStore promises, with these differences to a real
  /// store:
  ///   - every block a query hands over holds one entity
  ///   - changes that are made during a query take effect at once, they are
  ///     not held back until the query is done
  ///
  /// Like the store of Flecs, a query with QueryOrder::ParentsFirst names as
  /// parent of a block the nearest entity above that carries the first
  /// component of the query.
  class FakeEntityStore final : public EntityStore
  {
    struct Record
    {
      std::string name;
      Entity parent = No_Entity;
      std::map<ComponentId, void *> components;
    };

    // ordered, so that entities are handed over in the order they were
    // created in
    std::map<Entity, Record> _entities;
    std::map<ComponentId, ComponentInfo> _components;

    // the components that are turned off, by their entity
    std::set<std::pair<Entity, ComponentId>> _disabled;
    std::vector<QueryInfo> _queries;

    Entity _next_entity = 1000;
    ComponentId _next_component = 1;
    bool _initialized = false;
    int _initialize_count = 0;
    int _clean_up_count = 0;

    [[nodiscard]] const ComponentInfo &InfoOf(const ComponentId component) const
    {
      const auto it = _components.find(component);
      if (it == _components.end())
      {
        throw std::runtime_error("A component was used that the store does not know");
      }
      return it->second;
    }

    void Release(const Entity entity, Record &record, const ComponentId component)
    {
      const auto it = record.components.find(component);
      if (it == record.components.end()) { return; }

      const auto &info = InfoOf(component);
      if (info.on_remove) { info.on_remove(entity, it->second); }

      info.destruct(it->second, 1);
      ::operator delete(it->second, std::align_val_t(info.alignment));
      record.components.erase(it);
    }

    [[nodiscard]] std::size_t DepthOf(Entity entity) const
    {
      std::size_t depth = 0;
      for (auto it = _entities.find(entity); it != _entities.end() && it->second.parent != No_Entity;
           it = _entities.find(it->second.parent))
      {
        depth++;
      }
      return depth;
    }

    [[nodiscard]] Entity NearestAboveWith(const Entity entity, const ComponentId component) const
    {
      auto it = _entities.find(entity);
      while (it != _entities.end() && it->second.parent != No_Entity)
      {
        const Entity parent = it->second.parent;
        it = _entities.find(parent);
        if (it != _entities.end() && it->second.components.contains(component)) { return parent; }
      }
      return No_Entity;
    }

  public:
    FakeEntityStore() = default;

    FakeEntityStore(const FakeEntityStore &) = delete;

    FakeEntityStore &operator=(const FakeEntityStore &) = delete;

    ~FakeEntityStore()
    {
      Clear();
    }

    /// How often Initialize was called.
    [[nodiscard]] int InitializeCount() const { return _initialize_count; }

    /// How often CleanUp was called.
    [[nodiscard]] int CleanUpCount() const { return _clean_up_count; }

    /// Number of entities that are alive.
    [[nodiscard]] std::size_t EntityCount() const { return _entities.size(); }

    void Initialize() override
    {
      _initialize_count++;
      _initialized = true;
    }

    void CleanUp() override
    {
      _clean_up_count++;
      Clear();
    }

    /// Removes everything, as CleanUp does, without counting as a call of it.
    void Clear()
    {
      for (auto &[entity, record] : _entities)
      {
        while (!record.components.empty()) { Release(entity, record, record.components.begin()->first); }
      }
      _entities.clear();
      _components.clear();
      _queries.clear();
      _initialized = false;
    }

    ComponentId RegisterComponent(const ComponentInfo &info) override
    {
      for (const auto &[id, known] : _components)
      {
        if (known.name != info.name) { continue; }

        if (known.size != info.size || known.alignment != info.alignment)
        {
          throw std::runtime_error("Component '" + info.name + "' was registered before with another size");
        }
        return id;
      }

      if (info.size == 0 || info.construct == nullptr || info.destruct == nullptr
          || info.copy == nullptr || info.move == nullptr)
      {
        throw std::runtime_error("Component '" + info.name + "' is not described completely");
      }

      _components[_next_component] = info;
      return _next_component++;
    }

    ComponentId FindComponent(const std::string &name) override
    {
      for (const auto &[id, known] : _components)
      {
        if (known.name == name) { return id; }
      }
      return No_Component;
    }

    std::size_t GetComponentSize(const ComponentId component) override
    {
      const auto it = _components.find(component);
      return it == _components.end() ? 0 : it->second.size;
    }

    using EntityStore::CreateEntity;

    Entity CreateEntity(const std::string &name, const Entity parent) override
    {
      if (!name.empty())
      {
        for (const auto &[entity, record] : _entities)
        {
          if (record.parent == parent && record.name == name) { return entity; }
        }
      }

      _entities[_next_entity] = Record{.name = name, .parent = parent};
      return _next_entity++;
    }

    void DestroyEntity(const Entity entity) override
    {
      const auto it = _entities.find(entity);
      if (it == _entities.end()) { return; }

      std::vector<Entity> children;
      for (const auto &[candidate, record] : _entities)
      {
        if (record.parent == entity) { children.push_back(candidate); }
      }
      for (const Entity child : children) { DestroyEntity(child); }

      while (!it->second.components.empty()) { Release(entity, it->second, it->second.components.begin()->first); }
      _entities.erase(it);
    }

    bool IsAlive(const Entity entity) override
    {
      return _entities.contains(entity);
    }

    Entity FindEntity(const std::string &path) override
    {
      if (path.empty()) { return No_Entity; }

      Entity current = No_Entity;
      std::size_t start = 0;
      while (start <= path.size())
      {
        std::size_t end = path.find('/', start);
        if (end == std::string::npos) { end = path.size(); }
        const std::string name = path.substr(start, end - start);
        start = end + 1;

        Entity found = No_Entity;
        for (const auto &[entity, record] : _entities)
        {
          if (record.parent == current && record.name == name) { found = entity; }
        }
        if (found == No_Entity) { return No_Entity; }
        current = found;
      }
      return current;
    }

    std::string GetName(const Entity entity) override
    {
      const auto it = _entities.find(entity);
      return it == _entities.end() ? std::string{} : it->second.name;
    }

    void SetName(const Entity entity, const std::string &name) override
    {
      if (const auto it = _entities.find(entity); it != _entities.end()) { it->second.name = name; }
    }

    void SetParent(const Entity entity, const Entity parent) override
    {
      if (const auto it = _entities.find(entity); it != _entities.end()) { it->second.parent = parent; }
    }

    Entity GetParent(const Entity entity) override
    {
      const auto it = _entities.find(entity);
      return it == _entities.end() ? No_Entity : it->second.parent;
    }

    std::vector<Entity> GetChildren(const Entity entity) override
    {
      // ids count up, and the map keeps them in order, so this is the order
      // the entities were created in
      std::vector<Entity> children;
      for (const auto &[id, record] : _entities)
      {
        if (record.parent == entity) { children.push_back(id); }
      }
      return children;
    }

    void SetComponent(const Entity entity, const ComponentId component, const void *value) override
    {
      const auto &info = InfoOf(component);

      const auto it = _entities.find(entity);
      if (it == _entities.end()) { return; }

      auto &memory = it->second.components[component];
      if (memory == nullptr)
      {
        memory = ::operator new(info.size, std::align_val_t(info.alignment));
        info.construct(memory, 1);
      }
      info.copy(memory, value, 1);
    }

    void *GetComponent(const Entity entity, const ComponentId component) override
    {
      // one that is turned off is not there for whoever asks
      if (_disabled.contains({entity, component})) { return nullptr; }
      return GetComponentData(entity, component);
    }

    void *GetComponentData(const Entity entity, const ComponentId component) override
    {
      const auto it = _entities.find(entity);
      if (it == _entities.end()) { return nullptr; }

      const auto found = it->second.components.find(component);
      return found == it->second.components.end() ? nullptr : found->second;
    }

    // the ones by type, of EntityStore, next to the ones by id below
    using EntityStore::IsEnabled;
    using EntityStore::SetEnabled;

    bool IsEnabled(const Entity entity, const ComponentId component) override
    {
      return GetComponent(entity, component) != nullptr;
    }

    void SetEnabled(const Entity entity, const ComponentId component, const bool enabled) override
    {
      void *data = GetComponentData(entity, component);
      if (data == nullptr || enabled != _disabled.contains({entity, component})) { return; }

      if (enabled) { _disabled.erase({entity, component}); }
      else { _disabled.insert({entity, component}); }

      if (const auto known = _components.find(component); known != _components.end() && known->second.on_toggle)
      {
        known->second.on_toggle(entity, data, enabled);
      }
    }

    bool HasComponent(const Entity entity, const ComponentId component) override
    {
      return GetComponent(entity, component) != nullptr;
    }

    void RemoveComponent(const Entity entity, const ComponentId component) override
    {
      _disabled.erase({entity, component});
      if (const auto it = _entities.find(entity); it != _entities.end()) { Release(entity, it->second, component); }
    }

    QueryId CreateQuery(const QueryInfo &info) override
    {
      if (info.components.empty() || info.components.size() > EntityBlock::Max_Components)
      {
        throw std::runtime_error(
          "A query names between 1 and " + std::to_string(EntityBlock::Max_Components) + " components");
      }
      for (const auto component : info.components) { (void) InfoOf(component); }

      _queries.push_back(info);
      return _queries.size() - 1;
    }

    void Each(const QueryId query, const std::function<void(const EntityBlock &)> &visit) override
    {
      if (query >= _queries.size())
      {
        throw std::runtime_error("A query was used that the store does not know");
      }

      // a copy, because a query that is created during the visit moves them
      const QueryInfo info = _queries[query];

      std::vector<Entity> matches;
      for (const auto &[entity, record] : _entities)
      {
        // one that is turned off is passed over, as it is not there
        const Entity owner = entity;
        const bool carries_all = std::ranges::all_of(info.components, [&](const ComponentId component)
        {
          return record.components.contains(component) && !_disabled.contains({owner, component});
        });
        if (carries_all) { matches.push_back(entity); }
      }

      if (info.order == QueryOrder::ParentsFirst)
      {
        std::ranges::stable_sort(matches, [this](const Entity left, const Entity right)
        {
          return DepthOf(left) < DepthOf(right);
        });
      }

      for (const Entity entity : matches)
      {
        // the visit may have destroyed it, or taken a component away
        const auto it = _entities.find(entity);
        if (it == _entities.end()) { continue; }

        EntityBlock block;
        block.count = 1;
        block.entities = &entity;

        bool carries_all = true;
        for (std::size_t i = 0; i < info.components.size(); i++)
        {
          const auto found = it->second.components.find(info.components[i]);
          if (found == it->second.components.end())
          {
            carries_all = false;
            break;
          }
          block.columns[i] = found->second;
        }
        if (!carries_all) { continue; }

        if (info.order == QueryOrder::ParentsFirst)
        {
          block.parent = NearestAboveWith(entity, info.components[0]);
        }

        visit(block);
      }
    }
  };
} // neon::testing

#endif //FAKE_ENTITY_STORE_HPP
