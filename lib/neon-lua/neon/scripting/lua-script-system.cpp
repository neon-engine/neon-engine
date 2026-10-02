#include "lua-script-system.hpp"

#include <algorithm>
#include <cctype>

#include "lua-api.hpp"
#include "lua-classes.hpp"
#include "lua-color-handle.hpp"
#include "lua-component-handle.hpp"
#include "lua-entity-handle.hpp"
#include "lua-libraries.hpp"
#include "lua-sandbox.hpp"
#include "lua-vec3-handle.hpp"

namespace neon
{
  // Helpers of Lua_ScriptSystem, for this file alone.
  namespace
  {
    /// What a file's name says it returns.
    enum class FileKind
    {
      /// `name.lua`: a component, systems, both, or a module.
      Plain,

      /// `name.component.lua`: one component.
      Component,

      /// `name.system.lua`: systems.
      System
    };

    /// The name of a file without its folder and its `.lua`.
    std::string stem_of(const std::string &path)
    {
      const std::size_t slash = path.rfind('/');
      std::string stem = slash == std::string::npos ? path : path.substr(slash + 1);
      if (stem.ends_with(".lua")) { stem = stem.substr(0, stem.size() - 4); }
      return stem;
    }

    FileKind kind_of(const std::string &stem)
    {
      if (stem.ends_with(".component")) { return FileKind::Component; }
      if (stem.ends_with(".system")) { return FileKind::System; }
      return FileKind::Plain;
    }

    /// The name of a component from the name of its file: `door` and
    /// `door.component` are `Door`, `turret_gun` and `turret-gun` are
    /// `TurretGun`.
    std::string component_name_of(const std::string &stem)
    {
      std::string base = stem;
      if (const std::size_t dot = base.find('.'); dot != std::string::npos) { base = base.substr(0, dot); }

      std::string name;
      bool upper = true;
      for (const char c : base)
      {
        if (c == '_' || c == '-')
        {
          upper = true;
          continue;
        }
        name += upper ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
        upper = false;
      }
      return name;
    }

    /// Whether a name can be a component's: letters and digits, a capital
    /// first, as the engine's are.
    bool is_component_name(const std::string &name)
    {
      if (name.empty() || std::isupper(static_cast<unsigned char>(name.front())) == 0) { return false; }
      return std::ranges::all_of(name, [](const char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0; });
    }

    /// The extra arguments of a hook beyond the entity and the components.
    constexpr int extra_of(const LuaHook hook)
    {
      switch (hook)
      {
        case LuaHook::Update:
        case LuaHook::Step:
        case LuaHook::TriggerEnter:
        case LuaHook::TriggerExit: return 1;
        case LuaHook::Collision: return 3;
        default: return 0;
      }
    }
  }

  Lua_ScriptSystem::Lua_ScriptSystem(FileSystemContext *file_system, const std::shared_ptr<Logger> &logger)
  {
    _host.file_system = file_system;
    _host.logger = logger;
  }

  Lua_ScriptSystem::~Lua_ScriptSystem()
  {
    CleanUp();
  }

  void Lua_ScriptSystem::SetInput(InputContext *input)
  {
    _host.input = input;
  }

  void Lua_ScriptSystem::SetWorld(WorldSystem *world)
  {
    _host.world = world;
  }

  void Lua_ScriptSystem::Initialize()
  {
    if (_lua != nullptr) { return; }

    _lua = luaL_newstate();
    set_host(_lua, &_host);

    open_sandbox(_lua);
    open_entity_handles(_lua);
    open_component_handles(_lua);
    open_vec3_handles(_lua);
    open_color_handles(_lua);
    open_classes(_lua);
    open_world_library(_lua);
    open_input_library(_lua);
    open_log_library(_lua);
    open_scene_library(_lua);
    open_math_extras(_lua);

    const std::string version = LUA_VERSION_MAJOR "." LUA_VERSION_MINOR;
    _host.logger->Info("Lua {} is ready for the scripts", version);
  }

  void Lua_ScriptSystem::CleanUp()
  {
    if (_lua == nullptr) { return; }

    lua_close(_lua);
    _lua = nullptr;
    _components.clear();
    _systems.clear();
    _host.queries.clear();
    _host.store = nullptr;
    _started = false;
  }

  bool Lua_ScriptSystem::LoadScripts(const std::string &folder, EntityStore &store, ComponentFormats &formats)
  {
    if (_lua == nullptr)
    {
      _host.logger->Error("The scripts cannot be loaded before the script system is initialized");
      return false;
    }

    std::vector<std::string> paths;
    if (!_host.file_system->ListFiles(folder, paths)) { return false; }

    _host.folder = folder;
    while (!_host.folder.empty() && _host.folder.back() == '/' && !_host.folder.ends_with("://")) { _host.folder.pop_back(); }
    _host.store = &store;
    _host.formats = &formats;

    for (const std::string &path : paths)
    {
      // a definitions file for an editor is not a script
      if (!path.ends_with(".lua") || path.ends_with(".d.lua")) { continue; }
      LoadFile(path, store, formats);
    }

    for (DeclaredSystem &system : _systems) { ResolveSystem(system, store); }

    _host.store = nullptr;
    return true;
  }

  bool Lua_ScriptSystem::LoadFile(const std::string &path, EntityStore &store, ComponentFormats &formats)
  {
    std::string text;
    if (!_host.file_system->ReadText(path, text))
    {
      _host.logger->Error("The script {} cannot be read", path);
      return false;
    }

    const int base = lua_gettop(_lua);
    const int results = run_script(_lua, path, text);
    if (results < 0)
    {
      const std::string message = lua_tostring(_lua, -1);
      _host.logger->Error("The script {} failed: {}", path, message);
      lua_settop(_lua, base);
      return false;
    }

    const bool taken = TakeResults(path, base + 1, store, formats);
    lua_settop(_lua, base);
    return taken;
  }

  bool Lua_ScriptSystem::TakeResults(
    const std::string &path,
    const int first,
    EntityStore &store,
    ComponentFormats &formats)
  {
    const int last = lua_gettop(_lua);
    const std::string stem = stem_of(path);
    const FileKind kind = kind_of(stem);

    int components = 0;
    int systems = 0;
    for (int index = first; index <= last; index++)
    {
      if (is_component_class(_lua, index)) { components++; }
      else if (is_system_class(_lua, index)) { systems++; }
      else if (kind == FileKind::Plain && last == first)
      {
        // a module for require: kept under its name, so that require finds
        // it without running the file again
        lua_getfield(_lua, LUA_REGISTRYINDEX, "neon.modules");
        lua_pushvalue(_lua, index);
        lua_setfield(_lua, -2, module_name_of(_host.folder, path).c_str());
        lua_pop(_lua, 1);
        return true;
      }
      else
      {
        const std::string type = luaL_typename(_lua, index);
        _host.logger->Error(
          "The script {} returns {}, which is neither a component nor a system; return what Component:extend or System:extend made",
          path,
          type);
        return false;
      }
    }

    if (kind == FileKind::Component && (components != 1 || systems != 0))
    {
      _host.logger->Error("The script {} is named as a component, so it has to return one component and nothing else", path);
      return false;
    }
    if (kind == FileKind::System && (components != 0 || systems == 0))
    {
      _host.logger->Error("The script {} is named as a system, so it has to return systems and nothing else", path);
      return false;
    }
    if (components > 1)
    {
      _host.logger->Error("The script {} returns {} components; a file declares at most one, named after the file", path, components);
      return false;
    }
    if (components == 0 && systems == 0 && kind == FileKind::Plain)
    {
      // a file that returns nothing is a library run for what it defines
      return true;
    }

    // the component first, whichever order the file returns them in, so
    // that a system over it finds its name
    bool fine = true;
    for (int index = first; index <= last; index++)
    {
      if (is_component_class(_lua, index)) { fine = DeclareComponent(path, index, store, formats) && fine; }
    }
    for (int index = first; index <= last; index++)
    {
      if (is_system_class(_lua, index)) { fine = DeclareSystem(path, index) && fine; }
    }
    return fine;
  }

  bool Lua_ScriptSystem::DeclareComponent(
    const std::string &path,
    const int index,
    EntityStore &store,
    ComponentFormats &formats)
  {
    LuaComponentDeclaration declaration;
    std::string problem;
    if (!read_component_class(_lua, index, component_name_of(stem_of(path)), declaration, problem))
    {
      _host.logger->Error("The script {} is refused: {}", path, problem);
      return false;
    }

    if (!is_component_name(declaration.name))
    {
      _host.logger->Error(
        "The script {} is refused: '{}' is not a name for a component, which is letters and digits with a capital first, as Transform is",
        path,
        declaration.name);
      return false;
    }

    if (store.FindComponent(declaration.name) != No_Component || TypeOf(declaration.name) != nullptr)
    {
      const auto other = std::ranges::find(_components, declaration.name, &DeclaredComponent::name);
      const std::string where = other != _components.end() ? "declared by " + other->file : "of the engine";
      _host.logger->Error("The script {} is refused: a component called {} exists already, {}", path, declaration.name, where);
      return false;
    }

    auto layout = std::make_unique<ScriptComponentLayout>(declaration.name, "Declared by " + path, declaration.fields);
    if (!layout->GetProblem().empty())
    {
      _host.logger->Error("The script {} is refused: {}", path, layout->GetProblem());
      return false;
    }

    DeclaredComponent component;
    component.name = declaration.name;
    component.file = path;
    component.type = layout->GetTypeInfo();
    component.id = store.RegisterComponent(layout->GetComponentInfo());
    formats.Add(layout->GetComponentFormat());
    component.layout = std::move(layout);

    // the class knows its name from here on, for System:extend(Door)
    lua_pushstring(_lua, component.name.c_str());
    lua_setfield(_lua, index, "name");

    const std::size_t fields = declaration.fields.size();
    _host.logger->Debug("The script {} declares the component {} with {} fields", path, component.name, fields);
    _components.push_back(std::move(component));
    return true;
  }

  bool Lua_ScriptSystem::DeclareSystem(const std::string &path, const int index)
  {
    const std::string stem = stem_of(path);
    std::string name = component_name_of(stem) + "System";
    if (kind_of(stem) == FileKind::Plain && !_systems.empty())
    {
      // several systems of one file are told apart by their place
      name += std::to_string(_systems.size() + 1);
    }

    DeclaredSystem system;
    system.file = path;
    std::string problem;
    if (!read_system_class(_lua, index, name, system.declaration, problem))
    {
      _host.logger->Error("The script {} is refused: {}", path, problem);
      return false;
    }

    _host.logger->Debug("The script {} declares the system {}", path, system.declaration.name);
    _systems.push_back(std::move(system));
    return true;
  }

  const TypeInfo *Lua_ScriptSystem::TypeOf(const std::string &name) const
  {
    const auto component = std::ranges::find(_components, name, &DeclaredComponent::name);
    if (component != _components.end()) { return component->type.get(); }

    const ComponentFormat *format = _host.formats != nullptr ? _host.formats->Find(name) : nullptr;
    return format != nullptr ? format->type.get() : nullptr;
  }

  bool Lua_ScriptSystem::ResolveSystem(DeclaredSystem &system, EntityStore &store)
  {
    system.components.clear();
    system.types.clear();
    system.sizes.clear();

    for (const std::string &name : system.declaration.components)
    {
      const ComponentId id = store.FindComponent(name);
      if (id == No_Component)
      {
        std::string known;
        for (const DeclaredComponent &component : _components)
        {
          if (!known.empty()) { known += ", "; }
          known += component.name;
        }
        if (known.empty()) { known = "nothing"; }
        _host.logger->Error(
          "The system {} of {} runs over {}, which no script and nothing of the engine declares; the scripts declare {}",
          system.declaration.name,
          system.file,
          name,
          known);
        system.off.fill(true);
        return false;
      }

      system.components.push_back(id);
      system.types.push_back(TypeOf(name));

      const auto declared = std::ranges::find(_components, name, &DeclaredComponent::name);
      system.sizes.push_back(declared != _components.end() ? declared->layout->GetSize() : 0);
    }

    // the handles the hooks are called with
    push_entity(_lua, No_Entity);
    system.entity_ref = luaL_ref(_lua, LUA_REGISTRYINDEX);
    for (std::size_t i = 0; i < system.components.size(); i++)
    {
      LuaComponentHandle handle;
      handle.component = system.components[i];
      handle.type = system.types[i];
      push_component(_lua, handle);
      system.component_refs.push_back(luaL_ref(_lua, LUA_REGISTRYINDEX));
    }

    return true;
  }

  void Lua_ScriptSystem::Start(EntityStore &store)
  {
    if (_lua == nullptr) { return; }

    for (DeclaredSystem &system : _systems)
    {
      if (system.components.empty()) { continue; }
      system.query = store.CreateQuery(QueryInfo{.components = system.components});
    }
    _started = true;
  }

  void Lua_ScriptSystem::FillHandles(const DeclaredSystem &system, const Entity entity, const std::vector<void *> &pointers)
  {
    lua_rawgeti(_lua, LUA_REGISTRYINDEX, system.entity_ref);
    check_entity(_lua, -1).entity = entity;
    lua_pop(_lua, 1);

    for (std::size_t i = 0; i < system.component_refs.size(); i++)
    {
      lua_rawgeti(_lua, LUA_REGISTRYINDEX, system.component_refs[i]);
      LuaComponentHandle &handle = check_component(_lua, -1);
      handle.entity = entity;
      handle.pointer = i < pointers.size() ? pointers[i] : nullptr;
      lua_pop(_lua, 1);
    }
  }

  bool Lua_ScriptSystem::CallHook(DeclaredSystem &system, const LuaHook hook, const Entity entity, const int extra)
  {
    // the extra arguments are on the stack already; the call takes self,
    // the entity, the components, then them
    const auto index = static_cast<std::size_t>(hook);
    const int base = lua_gettop(_lua) - extra;

    lua_rawgeti(_lua, LUA_REGISTRYINDEX, system.declaration.ref);
    lua_getfield(_lua, -1, lua_hook_names[index].data());
    lua_insert(_lua, base + 1);
    lua_insert(_lua, base + 2);
    // stack: ..., function, self, extra...

    lua_rawgeti(_lua, LUA_REGISTRYINDEX, system.entity_ref);
    lua_insert(_lua, base + 3);

    int arguments = 2;
    if (hook != LuaHook::Removed)
    {
      for (std::size_t i = 0; i < system.component_refs.size(); i++)
      {
        lua_rawgeti(_lua, LUA_REGISTRYINDEX, system.component_refs[i]);
        lua_insert(_lua, base + 3 + static_cast<int>(i) + 1);
        arguments++;
      }
    }
    arguments += extra;

    if (lua_pcall(_lua, arguments, 0, 0) != LUA_OK)
    {
      ReportError(system, hook, entity, *_host.store);
      lua_pop(_lua, 1);
      system.off[index] = true;
      return false;
    }
    return true;
  }

  void Lua_ScriptSystem::ReportError(const DeclaredSystem &system, const LuaHook hook, const Entity entity, EntityStore &store)
  {
    const std::string name = store.IsAlive(entity) ? store.GetName(entity) : std::to_string(entity);
    const std::string hook_name(lua_hook_names[static_cast<std::size_t>(hook)]);
    const std::string message = lua_tostring(_lua, -1);
    _host.logger->Error(
      "The hook {} of {} failed for entity '{}' and is switched off until the scripts are loaded again: {}",
      hook_name,
      system.declaration.name,
      name,
      message);
  }

  void Lua_ScriptSystem::RunOver(DeclaredSystem &system, const LuaHook hook, EntityStore &store, const double seconds)
  {
    const auto index = static_cast<std::size_t>(hook);
    if (!system.declaration.hooks[index] || system.off[index] || system.components.empty()) { return; }

    // the handles point into the block while it is handed over, and ask the
    // store afterwards
    std::vector<void *> pointers(system.components.size(), nullptr);
    store.Each(system.query, [&](const EntityBlock &block)
    {
      for (std::size_t i = 0; i < block.count && !system.off[index]; i++)
      {
        for (std::size_t c = 0; c < system.components.size(); c++)
        {
          pointers[c] = system.sizes[c] == 0
                          ? nullptr
                          : static_cast<char *>(block.columns[c]) + i * system.sizes[c];
        }
        FillHandles(system, block.entities[i], pointers);
        lua_pushnumber(_lua, seconds);
        CallHook(system, hook, block.entities[i], 1);
      }
    });
    FillHandles(system, No_Entity, {});
  }

  void Lua_ScriptSystem::TrackLifetimes(DeclaredSystem &system, EntityStore &store)
  {
    constexpr auto ready = static_cast<std::size_t>(LuaHook::Ready);
    constexpr auto removed = static_cast<std::size_t>(LuaHook::Removed);
    if (!system.declaration.hooks[ready] && !system.declaration.hooks[removed]) { return; }
    if (system.components.empty()) { return; }

    std::vector<Entity> current;
    store.Each(system.query, [&](const EntityBlock &block)
    {
      current.insert(current.end(), block.entities, block.entities + block.count);
    });

    if (system.declaration.hooks[removed] && !system.off[removed])
    {
      std::vector<Entity> gone;
      for (const Entity entity : system.seen)
      {
        if (std::ranges::find(current, entity) == current.end()) { gone.push_back(entity); }
      }
      for (const Entity entity : gone)
      {
        FillHandles(system, entity, {});
        CallHook(system, LuaHook::Removed, entity, 0);
        system.seen.erase(entity);
        if (!store.IsAlive(entity)) { forget_private_fields(_lua, entity); }
      }
    }
    else
    {
      std::erase_if(system.seen, [&](const Entity entity) { return std::ranges::find(current, entity) == current.end(); });
    }

    for (const Entity entity : current)
    {
      if (system.seen.contains(entity)) { continue; }
      system.seen.insert(entity);
      if (system.declaration.hooks[ready] && !system.off[ready])
      {
        FillHandles(system, entity, {});
        CallHook(system, LuaHook::Ready, entity, 0);
      }
    }
  }

  void Lua_ScriptSystem::Update(EntityStore &store, const double delta_time)
  {
    if (_lua == nullptr || !_started) { return; }
    _host.store = &store;

    for (DeclaredSystem &system : _systems)
    {
      TrackLifetimes(system, store);
      RunOver(system, LuaHook::Update, store, delta_time);
    }

    _host.store = nullptr;
  }

  void Lua_ScriptSystem::FixedUpdate(EntityStore &store, const double fixed_delta_time)
  {
    if (_lua == nullptr || !_started) { return; }
    _host.store = &store;

    for (DeclaredSystem &system : _systems) { RunOver(system, LuaHook::Step, store, fixed_delta_time); }

    _host.store = nullptr;
  }

  void Lua_ScriptSystem::DispatchPhysicsEvents(EntityStore &store, const std::vector<PhysicsEvent> &events)
  {
    if (_lua == nullptr || !_started || events.empty()) { return; }
    _host.store = &store;

    for (DeclaredSystem &system : _systems)
    {
      if (system.components.empty()) { continue; }

      for (const PhysicsEvent &event : events)
      {
        // a trigger's hooks go to the trigger's entity; a touch to both
        const std::vector<Entity> receivers = event.trigger
                                                ? std::vector{event.first}
                                                : std::vector{event.first, event.second};
        for (const Entity entity : receivers)
        {
          if (!store.IsAlive(entity)) { continue; }

          const bool carries = std::ranges::all_of(
            system.components,
            [&](const ComponentId id) { return store.HasComponent(entity, id); });
          if (!carries) { continue; }

          const Entity other = entity == event.first ? event.second : event.first;
          LuaHook hook = LuaHook::Collision;
          if (event.trigger) { hook = event.kind == PhysicsEventKind::Began ? LuaHook::TriggerEnter : LuaHook::TriggerExit; }
          else if (event.kind != PhysicsEventKind::Began) { continue; }

          const auto index = static_cast<std::size_t>(hook);
          if (!system.declaration.hooks[index] || system.off[index]) { continue; }

          FillHandles(system, entity, {});
          push_entity(_lua, other);
          if (hook == LuaHook::Collision)
          {
            push_vec3(_lua, event.point);
            push_vec3(_lua, event.normal);
          }
          CallHook(system, hook, entity, extra_of(hook));
        }
      }
    }

    _host.store = nullptr;
  }

  std::size_t Lua_ScriptSystem::GetComponentCount() const
  {
    return _components.size();
  }

  std::size_t Lua_ScriptSystem::GetSystemCount() const
  {
    return _systems.size();
  }
} // neon
