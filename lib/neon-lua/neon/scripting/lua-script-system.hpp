#ifndef LUA_SCRIPT_SYSTEM_HPP
#define LUA_SCRIPT_SYSTEM_HPP

#include <array>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <neon/scripting/script-component-layout.hpp>
#include <neon/scripting/script-system.hpp>

#include "lua-host.hpp"
#include "lua-system-declaration.hpp"

struct lua_State;

namespace neon
{
  /// The scripts of a game in Lua, run by LuaJIT.
  ///
  /// Every `*.lua` under the scripts' folder is run once when the scripts
  /// are loaded, in the order of the paths. A file declares a component
  /// with `Component:extend`, a system with `System:extend`, or both, and
  /// returns them; a file whose name says a kind, `*.component.lua` or
  /// `*.system.lua`, has to return that kind. A file that returns a table
  /// of its own is a module for `require`. The name of a component is the
  /// name of its file, as `door.lua` or `door.component.lua` is `Door`.
  ///
  /// A system's hooks run over every entity that carries the components it
  /// named, with the system's class as `self`, the entity, one handle per
  /// component, and what the hook adds. A hook that fails is reported once
  /// and switched off, so that the game goes on.
  // ReSharper disable once CppInconsistentNaming
  class Lua_ScriptSystem final : public ScriptSystem
  {
    /// A component a script declared, registered with the store.
    struct DeclaredComponent
    {
      std::string name;
      std::string file;
      std::unique_ptr<ScriptComponentLayout> layout;
      std::shared_ptr<const TypeInfo> type;
      ComponentId id = No_Component;
    };

    /// A system a script declared, over components of the store.
    struct DeclaredSystem
    {
      LuaSystemDeclaration declaration;
      std::string file;

      std::vector<ComponentId> components;
      std::vector<const TypeInfo *> types;
      std::vector<std::size_t> sizes;
      QueryId query = 0;

      /// Which hooks were switched off after a failure.
      std::array<bool, 7> off{};

      /// The entities the system has seen, for `ready` and `removed`.
      std::unordered_set<Entity> seen;

      /// The handles the hooks are called with, kept in the registry and
      /// filled in before every call, so that no call allocates.
      int entity_ref = -1;
      std::vector<int> component_refs;

      /// The function that runs a block, made once, whose upvalue is the
      /// run of the moment.
      int run_ref = -1;
    };

    /// One run of a hook over a block, inside one protected call: what the
    /// loop needs, and where it is, so that a failure names the entity.
    struct BlockRun
    {
      DeclaredSystem *system = nullptr;
      const EntityBlock *block = nullptr;
      LuaHook hook = LuaHook::Update;
      double seconds = 0.0;
      std::size_t index = 0;
    };

    lua_State *_lua = nullptr;

    bool _jit = true;
    LuaHost _host;
    std::vector<DeclaredComponent> _components;
    std::vector<DeclaredSystem> _systems;
    bool _started = false;

    /// Runs one file and takes what it returns. Returns false when the file
    /// was refused, which has been reported.
    bool LoadFile(const std::string &path, EntityStore &store, ComponentFormats &formats);

    /// Reads what a file returned, from `first` to the top of the stack.
    bool TakeResults(const std::string &path, int first, EntityStore &store, ComponentFormats &formats);

    bool DeclareComponent(const std::string &path, int index, EntityStore &store, ComponentFormats &formats);

    bool DeclareSystem(const std::string &path, int index);

    /// Finds the components a system named, once every file is read.
    bool ResolveSystem(DeclaredSystem &system, EntityStore &store);

    /// The description of a component, from the scripts or the formats.
    [[nodiscard]] const TypeInfo *TypeOf(const std::string &name) const;

    /// Fills the handles of a system in for an entity. `pointers` are the
    /// components in the block, or null to have the handles ask the store.
    void FillHandles(const DeclaredSystem &system, Entity entity, const std::vector<void *> &pointers);

    /// Calls a hook with the handles filled in and `extra` values pushed on
    /// the stack by `push_extra`. Returns false when it failed, which has
    /// been reported and switched the hook off.
    bool CallHook(DeclaredSystem &system, LuaHook hook, Entity entity, int extra);

    /// Runs a hook over every entity of a system's query, pushing `seconds`
    /// as the last argument: one protected call per block, inside which
    /// the loop calls the hook plainly for each entity, so that a thousand
    /// entities cost a thousand calls and one error handler, not a
    /// thousand.
    void RunOver(DeclaredSystem &system, LuaHook hook, EntityStore &store, double seconds);

    /// The loop of one block, run inside the protected call. The run is
    /// its upvalue.
    static int RunBlock(lua_State *lua);

    /// Calls `ready` for an entity the system had not seen and `removed`
    /// for one it no longer sees.
    void TrackLifetimes(DeclaredSystem &system, EntityStore &store);

    void ReportError(const DeclaredSystem &system, LuaHook hook, Entity entity, EntityStore &store);

    /// Tells the state whether to compile.
    void ApplyJit();

  public:
    Lua_ScriptSystem(FileSystemContext *file_system, const std::shared_ptr<Logger> &logger);

    ~Lua_ScriptSystem();

    /// What `input` reads. May be left out for a run without input.
    void SetInput(InputContext *input);

    /// Whether LuaJIT compiles the scripts as they run; off runs them in
    /// its interpreter. On by default.
    /// Takes effect at once, on a state that is ready or on the next one.
    void SetJit(bool enabled);

    /// What `scene.load` asks. May be left out for a world of its own.
    void SetWorld(WorldSystem *world);

    void Initialize() override;

    void CleanUp() override;

    bool LoadScripts(const std::string &folder, EntityStore &store, ComponentFormats &formats) override;

    void Start(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;

    void FixedUpdate(EntityStore &store, double fixed_delta_time) override;

    void DispatchPhysicsEvents(EntityStore &store, const std::vector<PhysicsEvent> &events) override;

    [[nodiscard]] std::size_t GetComponentCount() const override;

    [[nodiscard]] std::size_t GetSystemCount() const override;
  };
} // neon

#endif //LUA_SCRIPT_SYSTEM_HPP
