#ifndef NEON_EXTENSION_HPP
#define NEON_EXTENSION_HPP

// An extension in C++, over the tables of neon-extension.h. See
// docs/extensions.md.
//
// Nothing here crosses the boundary: this header is compiled into the
// extension and turns classes, templates, and lambdas into the plain calls
// of the C file. It is to that file what godot-cpp is to GDExtension. An
// extension that would rather make the calls itself needs none of it.
//
//     struct Spinner
//     {
//       float speed = 90.0f;
//       float angle = 0.0f;
//     };
//
//     class Spinning final : public neon::extension::System
//     {
//       neon::extension::Query<Spinner> _spinners;
//
//     public:
//       void Start(neon::extension::World &world) override { _spinners = world.CreateQuery<Spinner>(); }
//
//       void FixedUpdate(neon::extension::World &world, const double step) override
//       {
//         _spinners.Each([step](neon::extension::Entity, Spinner &spinner) { spinner.angle += spinner.speed * step; });
//       }
//     };
//
//     class Game final : public neon::extension::Extension
//     {
//     public:
//       bool Initialize(neon::extension::World &world) override
//       {
//         AddSystem<Spinning>("Spinning");
//         return true;
//       }
//
//       void RegisterComponents(neon::extension::World &world) override
//       {
//         using neon::extension::Field;
//         world.RegisterComponent<Spinner>("Spinner", "Turns what carries it", {
//           Field("speed", &Spinner::speed, "How far it turns in a second, in degrees"),
//           Field("angle", &Spinner::angle),
//         });
//       }
//     };
//
//     NEON_EXTENSION(Game)
//
// An exception must not leave a function the application calls. What throws
// is caught inside the extension.

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "neon-extension.h"

namespace neon::extension
{
  using Entity = NeonEntity;
  using Vector2 = NeonVector2;
  using Vector3 = NeonVector3;
  using Vector4 = NeonVector4;
  using IntegerVector2 = NeonIntegerVector2;
  using IntegerVector3 = NeonIntegerVector3;
  using Color = NeonColor;
  using Quaternion = NeonQuaternion;

  /// The version of the C file this header needs of the application.
  inline constexpr std::uint32_t needed_abi_version = 6;

  /// The kind of a field from its type, and its value as the numbers a
  /// description carries. A type of the extension's own that lies in memory
  /// as one of the kinds, such as a vector of a mathematics library, is made
  /// known by a specialization of its own.
  template<typename T>
  struct FieldKindOf;

  template<>
  struct FieldKindOf<bool>
  {
    static constexpr NeonFieldKind kind = NEON_FIELD_BOOLEAN;
    static void Numbers(const bool &value, double *numbers) { numbers[0] = value ? 1.0 : 0.0; }
  };

  /// A kind that holds one number.
  template<typename T, NeonFieldKind Kind>
  struct NumberKind
  {
    static constexpr NeonFieldKind kind = Kind;
    static void Numbers(const T &value, double *numbers) { numbers[0] = static_cast<double>(value); }
  };

  template<> struct FieldKindOf<std::int32_t> : NumberKind<std::int32_t, NEON_FIELD_INTEGER> {};
  template<> struct FieldKindOf<float> : NumberKind<float, NEON_FIELD_FLOAT> {};
  template<> struct FieldKindOf<double> : NumberKind<double, NEON_FIELD_DOUBLE> {};
  template<> struct FieldKindOf<std::uint8_t> : NumberKind<std::uint8_t, NEON_FIELD_BYTE> {};
  template<> struct FieldKindOf<std::int16_t> : NumberKind<std::int16_t, NEON_FIELD_SHORT> {};
  template<> struct FieldKindOf<std::uint16_t> : NumberKind<std::uint16_t, NEON_FIELD_UNSIGNED_SHORT> {};
  template<> struct FieldKindOf<std::uint32_t> : NumberKind<std::uint32_t, NEON_FIELD_UNSIGNED_INTEGER> {};
  template<> struct FieldKindOf<std::int64_t> : NumberKind<std::int64_t, NEON_FIELD_LONG> {};
  template<> struct FieldKindOf<std::uint64_t> : NumberKind<std::uint64_t, NEON_FIELD_UNSIGNED_LONG> {};

  template<>
  struct FieldKindOf<Vector2>
  {
    static constexpr NeonFieldKind kind = NEON_FIELD_VECTOR2;
    static void Numbers(const Vector2 &value, double *numbers)
    {
      numbers[0] = value.x;
      numbers[1] = value.y;
    }
  };

  template<>
  struct FieldKindOf<Vector3>
  {
    static constexpr NeonFieldKind kind = NEON_FIELD_VECTOR3;
    static void Numbers(const Vector3 &value, double *numbers)
    {
      numbers[0] = value.x;
      numbers[1] = value.y;
      numbers[2] = value.z;
    }
  };

  template<>
  struct FieldKindOf<Vector4>
  {
    static constexpr NeonFieldKind kind = NEON_FIELD_VECTOR4;
    static void Numbers(const Vector4 &value, double *numbers)
    {
      numbers[0] = value.x;
      numbers[1] = value.y;
      numbers[2] = value.z;
      numbers[3] = value.w;
    }
  };

  template<>
  struct FieldKindOf<IntegerVector2>
  {
    static constexpr NeonFieldKind kind = NEON_FIELD_INTEGER_VECTOR2;
    static void Numbers(const IntegerVector2 &value, double *numbers)
    {
      numbers[0] = value.x;
      numbers[1] = value.y;
    }
  };

  template<>
  struct FieldKindOf<IntegerVector3>
  {
    static constexpr NeonFieldKind kind = NEON_FIELD_INTEGER_VECTOR3;
    static void Numbers(const IntegerVector3 &value, double *numbers)
    {
      numbers[0] = value.x;
      numbers[1] = value.y;
      numbers[2] = value.z;
    }
  };

  template<>
  struct FieldKindOf<Color>
  {
    static constexpr NeonFieldKind kind = NEON_FIELD_COLOR;
    static void Numbers(const Color &value, double *numbers)
    {
      numbers[0] = value.r;
      numbers[1] = value.g;
      numbers[2] = value.b;
      numbers[3] = value.a;
    }
  };

  template<>
  struct FieldKindOf<Quaternion>
  {
    static constexpr NeonFieldKind kind = NEON_FIELD_QUATERNION;
    static void Numbers(const Quaternion &value, double *numbers)
    {
      numbers[0] = value.x;
      numbers[1] = value.y;
      numbers[2] = value.z;
      numbers[3] = value.w;
    }
  };

  /// Describes a field of a component from the member itself: its kind
  /// from its type, where it lies from where the member is, and what it
  /// starts with from what a component holds that was just made, so that a
  /// default is written once, in the struct.
  template<typename T, typename M>
  NeonFieldDescription Field(const char *name, M T::*member, const char *description = nullptr)
  {
    const T standard{};

    NeonFieldDescription field{};
    field.name = name;
    field.kind = FieldKindOf<M>::kind;
    field.offset = static_cast<std::uint64_t>(
      reinterpret_cast<const char *>(&(standard.*member)) - reinterpret_cast<const char *>(&standard));
    FieldKindOf<M>::Numbers(standard.*member, field.standard);
    field.description = description;
    return field;
  }

  /// What a type was registered as, for this extension.
  template<typename T>
  struct ComponentOf
  {
    static inline NeonComponent id = 0;
  };

  /// A field of a component that is not the extension's own, read and
  /// written in place in the column of a block, see World::PlaceField. T is
  /// the type the kind of the field has, such as Vector3 for the position
  /// of a Transform.
  template<typename T>
  class FieldPlace
  {
    std::uint64_t _offset = 0;
    std::uint64_t _stride = 0;

  public:
    FieldPlace() = default;

    FieldPlace(const std::uint64_t offset, const std::uint64_t stride)
    {
      _offset = offset;
      _stride = stride;
    }

    /// Whether the field has a place, which a field that is worked out
    /// when it is read has not.
    [[nodiscard]] bool IsValid() const { return _stride != 0; }

    /// The field of the component at a place in a column.
    [[nodiscard]] T &At(void *column, const std::uint64_t index) const
    {
      return *reinterpret_cast<T *>(static_cast<char *>(column) + index * _stride + _offset);
    }
  };

  /// The entities that carry all of some components, whoever the components
  /// are of, handed over block by block, see World::CreateBlockQuery.
  class BlockQuery
  {
    const NeonExtensionHost *_host = nullptr;
    NeonQuery _query = 0;

  public:
    BlockQuery() = default;

    BlockQuery(const NeonExtensionHost *host, const NeonQuery query)
    {
      _host = host;
      _query = query;
    }

    [[nodiscard]] bool IsValid() const { return _query != 0; }

    /// Calls `visit(const NeonEntityBlock &)` for every block of entities
    /// that match. Its columns are in the order the query named them.
    template<typename Visit>
    void Each(Visit &&visit) const
    {
      if (_host == nullptr || _query == 0) { return; }

      using Stored = std::remove_reference_t<Visit>;
      _host->each(
        _host->context,
        _query,
        [](void *user, const NeonEntityBlock *block) { (*static_cast<Stored *>(user))(*block); },
        const_cast<void *>(static_cast<const void *>(&visit)));
    }
  };

  /// The entities that carry all of some components, see World::CreateQuery.
  template<typename... Components>
  class Query
  {
    const NeonExtensionHost *_host = nullptr;
    NeonQuery _query = 0;

    template<typename Visit, std::size_t... Indices>
    static void VisitBlock(Visit &visit, const NeonEntityBlock *block, std::index_sequence<Indices...>)
    {
      for (std::uint64_t i = 0; i < block->count; i++)
      {
        visit(block->entities[i], static_cast<Components *>(block->columns[Indices])[i]...);
      }
    }

  public:
    Query() = default;

    Query(const NeonExtensionHost *host, const NeonQuery query)
    {
      _host = host;
      _query = query;
    }

    /// Whether the query was created.
    [[nodiscard]] bool IsValid() const { return _query != 0; }

    /// Calls `visit(entity, component &...)` for every entity that matches.
    /// What is written to the components takes effect at once.
    template<typename Visit>
    void Each(Visit &&visit) const
    {
      if (_host == nullptr) { return; }

      using Stored = std::remove_reference_t<Visit>;
      _host->each(
        _host->context,
        _query,
        [](void *user, const NeonEntityBlock *block)
        {
          VisitBlock(*static_cast<Stored *>(user), block, std::index_sequence_for<Components...>{});
        },
        const_cast<void *>(static_cast<const void *>(&visit)));
    }
  };

  /// What the application offers, as C++. It is handed to every function of
  /// an extension and of its systems.
  class World
  {
    const NeonExtensionHost *_host;

  public:
    explicit World(const NeonExtensionHost *host) { _host = host; }

    /// The table itself, for a call this class has no function for.
    [[nodiscard]] const NeonExtensionHost *GetHost() const { return _host; }

    void Log(const NeonLogLevel level, const std::string &message) const
    {
      _host->log(_host->context, level, message.c_str());
    }

    void Info(const std::string &message) const { Log(NEON_LOG_INFO, message); }
    void Warn(const std::string &message) const { Log(NEON_LOG_WARN, message); }
    void Error(const std::string &message) const { Log(NEON_LOG_ERROR, message); }

    /// Makes a struct known as a component, with the fields it has. Only in
    /// Extension::RegisterComponents. Returns false when it was refused,
    /// which the log says why.
    template<typename T>
    bool RegisterComponent(
      const char *name,
      const char *description,
      const std::initializer_list<NeonFieldDescription> fields)
    {
      static_assert(std::is_trivially_copyable_v<T>, "A component of an extension is a struct of plain fields");

      NeonComponentDescription described{};
      described.name = name;
      described.description = description;
      described.size = sizeof(T);
      described.fields = fields.begin();
      described.field_count = fields.size();

      ComponentOf<T>::id = _host->register_component(_host->context, &described);
      return ComponentOf<T>::id != 0;
    }

    [[nodiscard]] Entity CreateEntity(const std::string &name = "", const Entity parent = 0) const
    {
      return _host->create_entity(_host->context, name.c_str(), parent);
    }

    void DestroyEntity(const Entity entity) const { _host->destroy_entity(_host->context, entity); }

    [[nodiscard]] bool IsAlive(const Entity entity) const { return _host->is_alive(_host->context, entity) != 0; }

    [[nodiscard]] Entity FindEntity(const std::string &path) const
    {
      return _host->find_entity(_host->context, path.c_str());
    }

    void SetParent(const Entity entity, const Entity parent) const { _host->set_parent(_host->context, entity, parent); }

    [[nodiscard]] Entity GetParent(const Entity entity) const { return _host->get_parent(_host->context, entity); }

    template<typename T>
    void Set(const Entity entity, const T &value) const
    {
      _host->set_component(_host->context, entity, ComponentOf<T>::id, &value);
    }

    /// The component of the entity, or nullptr. Valid until the entity
    /// gains or loses a component.
    template<typename T>
    [[nodiscard]] T *Get(const Entity entity) const
    {
      return static_cast<T *>(_host->get_component(_host->context, entity, ComponentOf<T>::id));
    }

    template<typename T>
    [[nodiscard]] bool Has(const Entity entity) const
    {
      return _host->has_component(_host->context, entity, ComponentOf<T>::id) != 0;
    }

    template<typename T>
    void Remove(const Entity entity) const
    {
      _host->remove_component(_host->context, entity, ComponentOf<T>::id);
    }

    /// The component registered under a name, by anyone, or 0.
    [[nodiscard]] NeonComponent FindComponent(const std::string &name) const
    {
      return _host->find_component(_host->context, name.c_str());
    }

    /// Prepares a query over components by what they were registered as,
    /// for one that names a component that is not the extension's own:
    /// CreateBlockQuery({ComponentOf<Faller>::id, world.FindComponent("Transform")}).
    [[nodiscard]] BlockQuery CreateBlockQuery(
      const std::initializer_list<NeonComponent> components,
      const NeonQueryOrder order = NEON_QUERY_ANY) const
    {
      for (const NeonComponent component : components)
      {
        if (component == 0) { return {}; }
      }
      return {_host, _host->create_query(_host->context, components.begin(), components.size(), order)};
    }

    /// Where a field lies in its component, for reading and writing it in
    /// the columns of a BlockQuery without a call for each entity. T is the
    /// type of its kind. The place is not valid for a field of another
    /// kind, or one that is worked out when it is read.
    template<typename T>
    [[nodiscard]] FieldPlace<T> PlaceField(const std::string &component, const std::string &path) const
    {
      NeonFieldLayout layout{};
      const NeonField field = FindField(component, path);
      if (field == 0 || _host->get_field_layout(_host->context, field, &layout) == 0) { return {}; }
      if (layout.kind != FieldKindOf<T>::kind) { return {}; }
      return {layout.offset, layout.stride};
    }

    /// Sets a value that the files of the user interface show as `{name}`.
    void SetUiNumber(const std::string &name, const double number) const
    {
      _host->set_ui_number(_host->context, name.c_str(), number);
    }

    void SetUiText(const std::string &name, const std::string &text) const
    {
      _host->set_ui_text(_host->context, name.c_str(), text.c_str());
    }

    /// Places a prefab at a position, turned by pitch, yaw, and roll in
    /// degrees, both relative to the parent.
    [[nodiscard]] Entity SpawnAt(
      const std::string &prefab_path,
      const Vector3 &position,
      const Vector3 &rotation = {0.0f, 0.0f, 0.0f},
      const Entity parent = 0) const
    {
      return _host->spawn_at(_host->context, prefab_path.c_str(), parent, &position, &rotation);
    }

    /// Finds a field of any component by the names a recipe writes it with,
    /// as FindField("Transform", "position"). It is how a component that is
    /// not the extension's own is read and changed. In Start, once, and
    /// kept. Returns 0 when there is none.
    [[nodiscard]] NeonField FindField(const std::string &component, const std::string &path) const
    {
      return _host->find_field(_host->context, component.c_str(), path.c_str());
    }

    /// Reads a field of one number, or `otherwise` when the entity has no
    /// such component.
    [[nodiscard]] double GetNumber(const Entity entity, const NeonField field, const double otherwise = 0.0) const
    {
      double numbers[4] = {};
      return _host->get_field(_host->context, entity, field, numbers) >= 1 ? numbers[0] : otherwise;
    }

    bool SetNumber(const Entity entity, const NeonField field, const double number) const
    {
      const double numbers[4] = {number};
      return _host->set_field(_host->context, entity, field, numbers) != 0;
    }

    [[nodiscard]] bool GetBoolean(const Entity entity, const NeonField field) const
    {
      return GetNumber(entity, field) != 0.0;
    }

    bool SetBoolean(const Entity entity, const NeonField field, const bool value) const
    {
      return SetNumber(entity, field, value ? 1.0 : 0.0);
    }

    /// Reads a field of three numbers, or zeros when the entity has no such
    /// component.
    [[nodiscard]] Vector3 GetVector3(const Entity entity, const NeonField field) const
    {
      double numbers[4] = {};
      _host->get_field(_host->context, entity, field, numbers);
      return {static_cast<float>(numbers[0]), static_cast<float>(numbers[1]), static_cast<float>(numbers[2])};
    }

    bool SetVector3(const Entity entity, const NeonField field, const Vector3 &value) const
    {
      const double numbers[4] = {value.x, value.y, value.z};
      return _host->set_field(_host->context, entity, field, numbers) != 0;
    }

    /// Reads a field of four numbers: a vector, a colour as r, g, b, a, or
    /// a quaternion as x, y, z, w.
    [[nodiscard]] Vector4 GetVector4(const Entity entity, const NeonField field) const
    {
      double numbers[4] = {};
      _host->get_field(_host->context, entity, field, numbers);
      return {
        static_cast<float>(numbers[0]), static_cast<float>(numbers[1]), static_cast<float>(numbers[2]),
        static_cast<float>(numbers[3])
      };
    }

    bool SetVector4(const Entity entity, const NeonField field, const Vector4 &value) const
    {
      const double numbers[4] = {value.x, value.y, value.z, value.w};
      return _host->set_field(_host->context, entity, field, numbers) != 0;
    }

    /// Reads a field of text, or the word of a choice.
    [[nodiscard]] std::string GetText(const Entity entity, const NeonField field) const
    {
      std::string text(_host->get_field_text(_host->context, entity, field, nullptr, 0), '\0');
      if (!text.empty()) { _host->get_field_text(_host->context, entity, field, text.data(), text.size() + 1); }
      return text;
    }

    bool SetText(const Entity entity, const NeonField field, const std::string &text) const
    {
      return _host->set_field_text(_host->context, entity, field, text.c_str()) != 0;
    }

    /// The actions of the input map, as the game reads them.
    [[nodiscard]] bool IsActionDown(const std::string &action) const
    {
      return _host->is_action_down(_host->context, action.c_str()) != 0;
    }

    [[nodiscard]] bool WasActionPressed(const std::string &action) const
    {
      return _host->was_action_pressed(_host->context, action.c_str()) != 0;
    }

    [[nodiscard]] float ActionAxis(const std::string &action) const
    {
      return _host->action_axis(_host->context, action.c_str());
    }

    [[nodiscard]] Vector2 ActionAxis2(const std::string &action) const
    {
      return _host->action_axis2(_host->context, action.c_str());
    }

    [[nodiscard]] Vector3 ActionAxis3(const std::string &action) const
    {
      return _host->action_axis3(_host->context, action.c_str());
    }

    /// Whether a file can be read, at a virtual path.
    [[nodiscard]] bool FileExists(const std::string &path) const
    {
      return _host->file_exists(_host->context, path.c_str()) != 0;
    }

    /// Reads a whole file. Returns false when it cannot be read.
    bool ReadFile(const std::string &path, std::vector<std::uint8_t> &contents) const
    {
      return _host->read_file(
               _host->context,
               path.c_str(),
               [](void *user, const std::uint8_t *bytes, const std::uint64_t size)
               {
                 static_cast<std::vector<std::uint8_t> *>(user)->assign(bytes, bytes + size);
               },
               &contents) != 0;
    }

    /// Casts a ray into the world of the physics, at every layer and past
    /// triggers. Returns whether it hit something, which `hit` then says.
    /// `ignore` skips the bodies of an entity, such as the one that asks.
    bool CastRay(
      const Vector3 &origin,
      const Vector3 &direction,
      const float distance,
      NeonRayHit &hit,
      const Entity ignore = 0) const
    {
      const NeonRay ray{origin, direction, distance, 0xFFFFFFFFu, 0, ignore};
      return CastRay(ray, hit);
    }

    /// Casts a ray with everything it can say: the layers, and triggers.
    bool CastRay(const NeonRay &ray, NeonRayHit &hit) const
    {
      return _host->cast_ray(_host->context, &ray, &hit) != 0;
    }

    /// Places a prefab in the world, under a parent or at the top. Returns
    /// 0 when it could not be, which the log says why.
    [[nodiscard]] Entity Spawn(const std::string &prefab_path, const Entity parent = 0) const
    {
      return _host->spawn(_host->context, prefab_path.c_str(), parent);
    }

    /// Asks for another scene, which is loaded when the frame is done.
    void LoadScene(const std::string &scene_path) const
    {
      _host->load_scene(_host->context, scene_path.c_str());
    }

    /// Prepares a query for the entities that carry all of the types. In
    /// Start of an extension or of a system, once, and kept.
    template<typename... Components>
    [[nodiscard]] Query<Components...> CreateQuery(const NeonQueryOrder order = NEON_QUERY_ANY) const
    {
      const NeonComponent components[] = {ComponentOf<Components>::id...};
      return {_host, _host->create_query(_host->context, components, sizeof...(Components), order)};
    }
  };

  /// Behaviour an extension brings to the world, see EntitySystem of the
  /// engine for what belongs where. Added with Extension::AddSystem.
  class System
  {
  public:
    virtual ~System() = default;

    /// Called once, after every component is registered and before the
    /// scene is read. The place to create queries.
    virtual void Start(World &world) {}

    /// Called for everything that began or ended to touch in a frame,
    /// before Update.
    virtual void OnPhysicsEvent(World &world, const NeonPhysicsEvent &event) {}

    /// Called once per frame. `delta_time` is in seconds.
    virtual void Update(World &world, double delta_time) {}

    /// Called once per step of the world, with the length of a step.
    virtual void FixedUpdate(World &world, double fixed_delta_time) {}

    /// Called once per frame before it is drawn, with how far the frame
    /// lies between the last two steps.
    virtual void Interpolate(World &world, double blend) {}
  };

  /// An extension. A class of the extension derives from this, and
  /// NEON_EXTENSION names it.
  class Extension
  {
    /// A system with the world it is handed, which is what the application
    /// hands back to the functions below.
    struct Added
    {
      std::unique_ptr<System> system;
      World *world = nullptr;
    };

    World _world{nullptr};
    std::vector<std::unique_ptr<Added>> _systems;

    template<typename E>
    friend int Enter(const NeonExtensionHost *host, NeonExtension *table);

  protected:
    /// Adds a system to the world. In Initialize or RegisterComponents.
    /// The systems run in the order they were added.
    template<typename S, typename... Arguments>
    S &AddSystem(const char *name, Arguments &&... arguments)
    {
      auto added = std::make_unique<Added>();
      auto system = std::make_unique<S>(std::forward<Arguments>(arguments)...);
      S &reference = *system;
      added->system = std::move(system);
      added->world = &_world;

      NeonSystemDescription described{};
      described.name = name;
      described.user = added.get();
      described.update = [](void *user, const double delta_time)
      {
        const auto *of = static_cast<const Added *>(user);
        of->system->Update(*of->world, delta_time);
      };
      described.fixed_update = [](void *user, const double fixed_delta_time)
      {
        const auto *of = static_cast<const Added *>(user);
        of->system->FixedUpdate(*of->world, fixed_delta_time);
      };
      described.interpolate = [](void *user, const double blend)
      {
        const auto *of = static_cast<const Added *>(user);
        of->system->Interpolate(*of->world, blend);
      };

      _world.GetHost()->register_system(_world.GetHost()->context, &described);
      _world.GetHost()->listen_to_physics(
        _world.GetHost()->context,
        [](void *user, const NeonPhysicsEvent *event)
        {
          const auto *of = static_cast<const Added *>(user);
          of->system->OnPhysicsEvent(*of->world, *event);
        },
        added.get());
      _systems.push_back(std::move(added));
      return reference;
    }

  public:
    virtual ~Extension() = default;

    /// Called once, right after the library was opened. The place to add
    /// systems. Returns false when the extension cannot start, after saying
    /// why with world.Error. There is no world yet: the store is not
    /// reached from here.
    virtual bool Initialize(World &world) { return true; }

    /// The place to register components, and the only one.
    virtual void RegisterComponents(World &world) {}

    /// Called once, after every component is registered and before the
    /// scene is read, and before Start of the systems.
    virtual void Start(World &world) {}

    /// Called once before the library is closed.
    virtual void CleanUp(World &world) {}
  };

  /// What NEON_EXTENSION expands to: makes the one object of the class of
  /// the extension, and fills in the table with functions that call it.
  template<typename E>
  int Enter(const NeonExtensionHost *host, NeonExtension *table)
  {
    static_assert(std::is_base_of_v<Extension, E>, "NEON_EXTENSION names a class that derives from Extension");

    if (host->abi_version < needed_abi_version)
    {
      host->log(host->context, NEON_LOG_ERROR, "This application is older than the extension needs it to be");
      return 0;
    }

    // one for the library, which lives until the library is cleaned up
    static std::unique_ptr<Extension> extension;
    extension = std::make_unique<E>();
    extension->_world = World(host);

    table->abi_version = NEON_EXTENSION_ABI_VERSION;
    table->context = extension.get();

    table->register_components = [](void *context)
    {
      auto *of = static_cast<Extension *>(context);
      of->RegisterComponents(of->_world);
    };
    table->start = [](void *context)
    {
      auto *of = static_cast<Extension *>(context);
      of->Start(of->_world);
      for (const auto &added : of->_systems) { added->system->Start(of->_world); }
    };
    table->clean_up = [](void *context)
    {
      auto *of = static_cast<Extension *>(context);
      of->CleanUp(of->_world);
      extension.reset();
    };

    if (!extension->Initialize(extension->_world))
    {
      extension.reset();
      return 0;
    }
    return 1;
  }
} // neon::extension

/// Makes a class the extension of this library: the function the application
/// starts it by. Once in an extension, at the top level of a source file.
#define NEON_EXTENSION(Class) \
  extern "C" NEON_EXTENSION_EXPORT int neon_extension_initialize( \
    const NeonExtensionHost *host, \
    NeonExtension *extension) \
  { \
    return ::neon::extension::Enter<Class>(host, extension); \
  }

#endif //NEON_EXTENSION_HPP
