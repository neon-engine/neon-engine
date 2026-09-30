#include "flecs-entity-store.hpp"

#include <algorithm>
#include <stdexcept>

// the C interface is all that is used
#define FLECS_NO_CPP
#include <flecs.h>

namespace neon
{
  namespace
  {
    // Components live under an entity of their own, so that their names do
    // not collide with the names of entities in a scene.
    const char *components_scope_name = "neon-components";

    // Flecs reports through one function for the whole process
    std::shared_ptr<Logger> flecs_logger;

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    void Log(const int32_t level, const char *file, const int32_t line, const char *message)
    {
      if (flecs_logger == nullptr) { return; }

      if (level <= -3)
      {
        flecs_logger->Error("{} ({}:{})", message, file, line);
      } else if (level == -2)
      {
        flecs_logger->Warn("{} ({}:{})", message, file, line);
      } else
      {
        flecs_logger->Debug("{}", message);
      }
    }

    const ComponentInfo &InfoOf(const ecs_type_info_t *type_info)
    {
      return *static_cast<const ComponentInfo *>(type_info->hooks.ctx);
    }

    void Construct(void *at, const int32_t count, const ecs_type_info_t *type_info)
    {
      InfoOf(type_info).construct(at, static_cast<std::size_t>(count));
    }

    void Destruct(void *at, const int32_t count, const ecs_type_info_t *type_info)
    {
      InfoOf(type_info).destruct(at, static_cast<std::size_t>(count));
    }

    void Copy(void *to, const void *from, const int32_t count, const ecs_type_info_t *type_info)
    {
      InfoOf(type_info).copy(to, from, static_cast<std::size_t>(count));
    }

    void Move(void *to, void *from, const int32_t count, const ecs_type_info_t *type_info)
    {
      InfoOf(type_info).move(to, from, static_cast<std::size_t>(count));
    }

    void OnRemove(ecs_iter_t *it)
    {
      const auto &info = *static_cast<const ComponentInfo *>(it->ctx);
      auto *components = static_cast<unsigned char *>(ecs_field_w_size(it, info.size, 0));

      for (int32_t i = 0; i < it->count; i++)
      {
        info.on_remove(it->entities[i], components + static_cast<std::size_t>(i) * info.size);
      }
    }
  }

  Flecs_EntityStore::Flecs_EntityStore(const std::shared_ptr<Logger> &logger)
  {
    _logger = logger;
  }

  Flecs_EntityStore::~Flecs_EntityStore()
  {
    CleanUp();
  }

  void Flecs_EntityStore::Initialize()
  {
    if (_world != nullptr) { return; }

    _logger->Info("Initializing Flecs");

    flecs_logger = _logger;
    ecs_os_set_api_defaults();
    auto os_api = ecs_os_get_api();
    os_api.log_ = Log;
    ecs_os_set_api(&os_api);

    // the core alone. The addons of Flecs are not used
    _world = ecs_mini();
    if (_world == nullptr)
    {
      throw std::runtime_error("Failed to create the Flecs world");
    }

    ecs_entity_desc_t scope = {};
    scope.name = components_scope_name;
    scope.sep = "";
    _components_scope = ecs_entity_init(_world, &scope);

    _own_entities.clear();
    _own_entities = GetChildren(No_Entity);
  }

  void Flecs_EntityStore::CleanUp()
  {
    if (_world == nullptr) { return; }

    _logger->Info("Cleaning up Flecs");

    for (const auto &[query, sizes, parents_first] : _queries)
    {
      ecs_query_fini(query);
    }
    _queries.clear();

    // tells every component that it is removed, which needs what is in
    // _components. So those are released afterwards
    ecs_fini(_world);
    _world = nullptr;

    _components_by_id.clear();
    _components_by_name.clear();
    _components.clear();
    _components_scope = No_Entity;
    _own_entities.clear();

    flecs_logger = nullptr;
  }

  const Flecs_EntityStore::Component *Flecs_EntityStore::FindComponentById(const ComponentId component) const
  {
    const auto it = _components_by_id.find(component);
    if (it == _components_by_id.end())
    {
      throw std::runtime_error("A component was used that the store does not know");
    }
    return it->second;
  }

  ComponentId Flecs_EntityStore::RegisterComponent(const ComponentInfo &info)
  {
    if (const auto it = _components_by_name.find(info.name); it != _components_by_name.end())
    {
      if (it->second->info.size != info.size || it->second->info.alignment != info.alignment)
      {
        throw std::runtime_error("Component '" + info.name + "' was registered before with another size");
      }
      return it->second->id;
    }

    if (info.size == 0 || info.construct == nullptr || info.destruct == nullptr
        || info.copy == nullptr || info.move == nullptr)
    {
      throw std::runtime_error("Component '" + info.name + "' is not described completely");
    }

    auto &component = _components.emplace_back(std::make_unique<Component>());
    component->info = info;

    ecs_entity_desc_t entity = {};
    entity.name = component->info.name.c_str();
    entity.sep = "";
    entity.parent = _components_scope;

    ecs_component_desc_t desc = {};
    desc.entity = ecs_entity_init(_world, &entity);
    desc.type.size = static_cast<ecs_size_t>(info.size);
    desc.type.alignment = static_cast<ecs_size_t>(info.alignment);
    desc.type.hooks.ctor = Construct;
    desc.type.hooks.dtor = Destruct;
    desc.type.hooks.copy = Copy;
    desc.type.hooks.move = Move;
    desc.type.hooks.ctx = &component->info;
    if (info.on_remove)
    {
      desc.type.hooks.on_remove = OnRemove;
    }

    component->id = ecs_component_init(_world, &desc);
    if (component->id == 0)
    {
      _components.pop_back();
      throw std::runtime_error("Flecs did not accept component '" + info.name + "'");
    }

    _components_by_id[component->id] = component.get();
    _components_by_name[info.name] = component.get();

    _logger->Debug("Registered component {}", info.name);
    return component->id;
  }

  ComponentId Flecs_EntityStore::FindComponent(const std::string &name)
  {
    const auto it = _components_by_name.find(name);
    return it == _components_by_name.end() ? No_Component : it->second->id;
  }

  Entity Flecs_EntityStore::CreateEntity(const std::string &name, const Entity parent)
  {
    ecs_entity_desc_t desc = {};
    desc.parent = parent;
    if (!name.empty())
    {
      desc.name = name.c_str();
      // a name is taken as it is, and not as a path
      desc.sep = "";
    }

    return ecs_entity_init(_world, &desc);
  }

  void Flecs_EntityStore::DestroyEntity(const Entity entity)
  {
    if (!IsAlive(entity)) { return; }
    ecs_delete(_world, entity);
  }

  bool Flecs_EntityStore::IsAlive(const Entity entity)
  {
    return entity != No_Entity && ecs_is_alive(_world, entity);
  }

  Entity Flecs_EntityStore::FindEntity(const std::string &path)
  {
    if (path.empty()) { return No_Entity; }
    return ecs_lookup_path_w_sep(_world, 0, path.c_str(), "/", nullptr, false);
  }

  std::string Flecs_EntityStore::GetName(const Entity entity)
  {
    if (!IsAlive(entity)) { return {}; }

    const auto *name = ecs_get_name(_world, entity);
    return name == nullptr ? std::string{} : std::string{name};
  }

  void Flecs_EntityStore::SetParent(const Entity entity, const Entity parent)
  {
    if (!IsAlive(entity)) { return; }

    if (parent == No_Entity)
    {
      ecs_remove_id(_world, entity, ecs_pair(EcsChildOf, EcsWildcard));
      return;
    }

    // an entity has one parent, so this replaces the one it had
    ecs_add_id(_world, entity, ecs_pair(EcsChildOf, parent));
  }

  Entity Flecs_EntityStore::GetParent(const Entity entity)
  {
    if (!IsAlive(entity)) { return No_Entity; }
    return ecs_get_parent(_world, entity);
  }

  std::vector<Entity> Flecs_EntityStore::GetChildren(const Entity entity)
  {
    std::vector<Entity> children;
    if (entity != No_Entity && !IsAlive(entity)) { return children; }

    auto it = ecs_children(_world, entity);
    while (ecs_children_next(&it))
    {
      for (int32_t i = 0; i < it.count; i++)
      {
        const auto child = it.entities[i];
        if (std::ranges::find(_own_entities, child) != _own_entities.end()) { continue; }
        children.push_back(child);
      }
    }

    // Flecs hands the children over table by table, that is grouped by the
    // components they carry, and not in the order they were created.
    //
    // An id of Flecs has 64 bits. ECS_ENTITY_MASK keeps the lower 32, the
    // index of the entity, which counts up as entities are created. The 16
    // bits above count how often that index was used again after an entity
    // was destroyed, and the highest bits are flags. Sorting by the index
    // alone lists the children in the order they were created, except that a
    // child which reuses the index of a destroyed entity takes its place.
    std::ranges::sort(children, [](const Entity left, const Entity right)
    {
      return (left & ECS_ENTITY_MASK) < (right & ECS_ENTITY_MASK);
    });

    return children;
  }

  void Flecs_EntityStore::SetComponent(const Entity entity, const ComponentId component, const void *value)
  {
    const auto *known = FindComponentById(component);
    ecs_set_id(_world, entity, component, known->info.size, value);
  }

  void *Flecs_EntityStore::GetComponent(const Entity entity, const ComponentId component)
  {
    if (!IsAlive(entity)) { return nullptr; }
    return ecs_get_mut_id(_world, entity, component);
  }

  bool Flecs_EntityStore::HasComponent(const Entity entity, const ComponentId component)
  {
    return IsAlive(entity) && ecs_has_id(_world, entity, component);
  }

  void Flecs_EntityStore::RemoveComponent(const Entity entity, const ComponentId component)
  {
    if (!IsAlive(entity)) { return; }
    ecs_remove_id(_world, entity, component);
  }

  QueryId Flecs_EntityStore::CreateQuery(const QueryInfo &info)
  {
    if (info.components.empty() || info.components.size() > EntityBlock::Max_Components)
    {
      throw std::runtime_error(
        "A query names between 1 and " + std::to_string(EntityBlock::Max_Components) + " components");
    }

    Query query;
    query.parents_first = info.order == QueryOrder::ParentsFirst;

    ecs_query_desc_t desc = {};
    desc.cache_kind = EcsQueryCacheAuto;

    std::size_t term = 0;
    for (const auto component : info.components)
    {
      query.sizes.push_back(FindComponentById(component)->info.size);
      desc.terms[term].id = component;
      term++;
    }

    if (query.parents_first)
    {
      // Looks up the hierarchy for the first component. That sorts the
      // tables by their depth in the hierarchy, and tells which entity
      // above the component was found on.
      desc.terms[term].id = info.components[0];
      desc.terms[term].src.id = EcsCascade;
      desc.terms[term].oper = EcsOptional;
      desc.terms[term].inout = EcsIn;
    }

    // Flecs keeps an entity at the top for a query. It is not part of the
    // world, and would otherwise be handed over among the entities at the
    // top, and written into every scene that is saved.
    const auto before = GetChildren(No_Entity);

    query.query = ecs_query_init(_world, &desc);
    if (query.query == nullptr)
    {
      throw std::runtime_error("Flecs did not accept a query");
    }

    for (const auto entity : GetChildren(No_Entity))
    {
      if (std::ranges::find(before, entity) == before.end()) { _own_entities.push_back(entity); }
    }

    _queries.push_back(query);
    return _queries.size() - 1;
  }

  void Flecs_EntityStore::Each(const QueryId query, const std::function<void(const EntityBlock &)> &visit)
  {
    if (query >= _queries.size())
    {
      throw std::runtime_error("A query was used that the store does not know");
    }

    // by index, because a query that is created during the visit moves them
    const auto sizes = _queries[query].sizes;
    const auto parents_first = _queries[query].parents_first;
    const auto parent_field = static_cast<int8_t>(sizes.size());

    // Tables must not change while they are walked. Flecs holds changes back
    // until the end of the visit.
    ecs_defer_begin(_world);

    auto it = ecs_query_iter(_world, _queries[query].query);

    try
    {
      while (ecs_query_next(&it))
      {
        EntityBlock block;
        block.count = static_cast<std::size_t>(it.count);
        block.entities = it.entities;

        for (std::size_t i = 0; i < sizes.size(); i++)
        {
          block.columns[i] = ecs_field_w_size(&it, sizes[i], static_cast<int8_t>(i));
        }

        if (parents_first && ecs_field_is_set(&it, parent_field))
        {
          block.parent = ecs_field_src(&it, parent_field);
        }

        visit(block);
      }
    } catch (...)
    {
      ecs_iter_fini(&it);
      ecs_defer_end(_world);
      throw;
    }

    ecs_defer_end(_world);
  }
} // neon
