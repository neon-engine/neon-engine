#include "script-component-layout.hpp"

#include <cctype>
#include <cstring>
#include <new>
#include <utility>

#include <neon/reflection/field-documents.hpp>

namespace neon
{
  // Helpers of ScriptComponentLayout, for this file alone.
  namespace
  {
    std::size_t size_of(const FieldKind kind)
    {
      switch (kind)
      {
        case FieldKind::Boolean: return sizeof(bool);
        case FieldKind::Integer: return sizeof(int);
        case FieldKind::Float: return sizeof(float);
        case FieldKind::Double: return sizeof(double);
        case FieldKind::String: return sizeof(std::string);
        case FieldKind::Vector3: return sizeof(glm::vec3);
        case FieldKind::Color: return sizeof(Color);
        case FieldKind::Byte: return sizeof(std::uint8_t);
        case FieldKind::Char: return sizeof(char);
        case FieldKind::Short: return sizeof(std::int16_t);
        case FieldKind::UnsignedShort: return sizeof(std::uint16_t);
        case FieldKind::UnsignedInteger: return sizeof(std::uint32_t);
        case FieldKind::Long: return sizeof(std::int64_t);
        case FieldKind::UnsignedLong: return sizeof(std::uint64_t);
        case FieldKind::Vector2: return sizeof(glm::vec2);
        case FieldKind::Vector4: return sizeof(glm::vec4);
        case FieldKind::IntegerVector2: return sizeof(glm::ivec2);
        case FieldKind::IntegerVector3: return sizeof(glm::ivec3);
        case FieldKind::Quaternion: return sizeof(glm::quat);
        case FieldKind::Matrix3: return sizeof(glm::mat3);
        case FieldKind::Matrix4: return sizeof(glm::mat4);
        default: return 0;
      }
    }

    std::size_t alignment_of(const FieldKind kind)
    {
      switch (kind)
      {
        case FieldKind::Boolean: return alignof(bool);
        case FieldKind::Integer: return alignof(int);
        case FieldKind::Float: return alignof(float);
        case FieldKind::Double: return alignof(double);
        case FieldKind::String: return alignof(std::string);
        case FieldKind::Vector3: return alignof(glm::vec3);
        case FieldKind::Color: return alignof(Color);
        case FieldKind::Byte: return alignof(std::uint8_t);
        case FieldKind::Char: return alignof(char);
        case FieldKind::Short: return alignof(std::int16_t);
        case FieldKind::UnsignedShort: return alignof(std::uint16_t);
        case FieldKind::UnsignedInteger: return alignof(std::uint32_t);
        case FieldKind::Long: return alignof(std::int64_t);
        case FieldKind::UnsignedLong: return alignof(std::uint64_t);
        case FieldKind::Vector2: return alignof(glm::vec2);
        case FieldKind::Vector4: return alignof(glm::vec4);
        case FieldKind::IntegerVector2: return alignof(glm::ivec2);
        case FieldKind::IntegerVector3: return alignof(glm::ivec3);
        case FieldKind::Quaternion: return alignof(glm::quat);
        case FieldKind::Matrix3: return alignof(glm::mat3);
        case FieldKind::Matrix4: return alignof(glm::mat4);
        default: return 1;
      }
    }

    std::size_t align_up(const std::size_t offset, const std::size_t alignment)
    {
      return (offset + alignment - 1) / alignment * alignment;
    }

    /// Whether a name can be a field: letters, digits, and underscores, and
    /// not a digit first, so that a script reaches it as `door.speed`.
    bool is_identifier(const std::string &name)
    {
      if (name.empty() || std::isdigit(static_cast<unsigned char>(name.front())) != 0) { return false; }
      for (const char c : name)
      {
        if (std::isalnum(static_cast<unsigned char>(c)) == 0 && c != '_') { return false; }
      }
      return true;
    }

    template<typename T>
    T &at(void *object, const std::size_t offset)
    {
      return *static_cast<T *>(static_cast<void *>(static_cast<char *>(object) + offset));
    }

    template<typename T>
    const T &at(const void *object, const std::size_t offset)
    {
      return *static_cast<const T *>(static_cast<const void *>(static_cast<const char *>(object) + offset));
    }

    /// Memory for one component outside the store, aligned for any field.
    using Buffer = std::vector<std::max_align_t>;

    Buffer buffer_for(const std::size_t size)
    {
      return Buffer((size + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t));
    }
  }

  bool ScriptComponentLayout::Supports(const FieldKind kind)
  {
    return size_of(kind) != 0;
  }

  ScriptComponentLayout::ScriptComponentLayout(
    const std::string &name,
    const std::string &description,
    const std::vector<ScriptField> &fields)
  {
    auto shape = std::make_shared<Shape>();
    shape->name = name;

    auto type = std::make_shared<TypeInfo>();
    type->name = name;
    type->description = description;

    std::size_t offset = 0;
    for (const auto &field : fields)
    {
      if (!is_identifier(field.name))
      {
        _problem = "The field '" + field.name + "' needs a name of letters, digits, and underscores, not starting with a digit";
        return;
      }
      if (type->Find(field.name) != nullptr)
      {
        _problem = "The field '" + field.name + "' is declared twice";
        return;
      }
      if (!Supports(field.kind))
      {
        _problem = "The field '" + field.name + "' holds " + Describe(field.kind) + ", which a script's component cannot";
        return;
      }
      if (!Holds(field.standard, field.kind))
      {
        _problem = "The default of '" + field.name + "' is not " + Describe(field.kind);
        return;
      }

      offset = align_up(offset, alignment_of(field.kind));
      const Slot slot{field.kind, offset, field.standard};
      shape->slots.push_back(slot);
      shape->alignment = std::max(shape->alignment, alignment_of(field.kind));
      offset += size_of(field.kind);

      FieldInfo info;
      info.name = field.name;
      info.kind = field.kind;
      info.description = field.description;
      info.get = [slot](const void *object) { return Get(slot, object); };
      info.set = [slot](void *object, const FieldValue &value) { Set(slot, object, value); };
      type->fields.push_back(info);
    }

    // a component without fields marks an entity, and a store keeps nothing
    // of size zero
    shape->size = std::max<std::size_t>(align_up(offset, shape->alignment), 1);

    _shape = shape;
    _type = type;
  }

  const std::string &ScriptComponentLayout::GetProblem() const
  {
    return _problem;
  }

  const std::string &ScriptComponentLayout::GetName() const
  {
    return _shape->name;
  }

  std::size_t ScriptComponentLayout::GetSize() const
  {
    return _shape->size;
  }

  std::size_t ScriptComponentLayout::GetAlignment() const
  {
    return _shape->alignment;
  }

  void ScriptComponentLayout::Construct(const Shape &shape, void *at)
  {
    for (const auto &slot : shape.slots)
    {
      void *place = static_cast<char *>(at) + slot.offset;
      switch (slot.kind)
      {
        case FieldKind::Boolean: new(place) bool(std::get<bool>(slot.standard));
          break;
        case FieldKind::Integer: new(place) int(std::get<int>(slot.standard));
          break;
        case FieldKind::Float: new(place) float(std::get<float>(slot.standard));
          break;
        case FieldKind::Double: new(place) double(std::get<double>(slot.standard));
          break;
        case FieldKind::String: new(place) std::string(std::get<std::string>(slot.standard));
          break;
        case FieldKind::Vector3: new(place) glm::vec3(std::get<glm::vec3>(slot.standard));
          break;
        case FieldKind::Color: new(place) Color(std::get<Color>(slot.standard));
          break;
        case FieldKind::Byte: new(place) std::uint8_t(std::get<std::uint8_t>(slot.standard));
          break;
        case FieldKind::Char: new(place) char(std::get<char>(slot.standard));
          break;
        case FieldKind::Short: new(place) std::int16_t(std::get<std::int16_t>(slot.standard));
          break;
        case FieldKind::UnsignedShort: new(place) std::uint16_t(std::get<std::uint16_t>(slot.standard));
          break;
        case FieldKind::UnsignedInteger: new(place) std::uint32_t(std::get<std::uint32_t>(slot.standard));
          break;
        case FieldKind::Long: new(place) std::int64_t(std::get<std::int64_t>(slot.standard));
          break;
        case FieldKind::UnsignedLong: new(place) std::uint64_t(std::get<std::uint64_t>(slot.standard));
          break;
        case FieldKind::Vector2: new(place) glm::vec2(std::get<glm::vec2>(slot.standard));
          break;
        case FieldKind::Vector4: new(place) glm::vec4(std::get<glm::vec4>(slot.standard));
          break;
        case FieldKind::IntegerVector2: new(place) glm::ivec2(std::get<glm::ivec2>(slot.standard));
          break;
        case FieldKind::IntegerVector3: new(place) glm::ivec3(std::get<glm::ivec3>(slot.standard));
          break;
        case FieldKind::Quaternion: new(place) glm::quat(std::get<glm::quat>(slot.standard));
          break;
        case FieldKind::Matrix3: new(place) glm::mat3(std::get<glm::mat3>(slot.standard));
          break;
        case FieldKind::Matrix4: new(place) glm::mat4(std::get<glm::mat4>(slot.standard));
          break;
        default: break;
      }
    }
  }

  void ScriptComponentLayout::Destruct(const Shape &shape, void *at)
  {
    for (const auto &slot : shape.slots)
    {
      // only text holds anything outside the component
      if (slot.kind == FieldKind::String) { neon::at<std::string>(at, slot.offset).~basic_string(); }
    }
  }

  void ScriptComponentLayout::Copy(const Shape &shape, void *to, const void *from)
  {
    for (const auto &slot : shape.slots)
    {
      switch (slot.kind)
      {
        case FieldKind::Boolean: at<bool>(to, slot.offset) = at<bool>(from, slot.offset);
          break;
        case FieldKind::Integer: at<int>(to, slot.offset) = at<int>(from, slot.offset);
          break;
        case FieldKind::Float: at<float>(to, slot.offset) = at<float>(from, slot.offset);
          break;
        case FieldKind::Double: at<double>(to, slot.offset) = at<double>(from, slot.offset);
          break;
        case FieldKind::String: at<std::string>(to, slot.offset) = at<std::string>(from, slot.offset);
          break;
        case FieldKind::Vector3: at<glm::vec3>(to, slot.offset) = at<glm::vec3>(from, slot.offset);
          break;
        case FieldKind::Color: at<Color>(to, slot.offset) = at<Color>(from, slot.offset);
          break;
        default:
          // every other kind a layout holds is plain bytes
          if (slot.kind != FieldKind::String && size_of(slot.kind) != 0)
          {
            std::memcpy(static_cast<char *>(to) + slot.offset, static_cast<const char *>(from) + slot.offset, size_of(slot.kind));
          }
          break;
      }
    }
  }

  void ScriptComponentLayout::Move(const Shape &shape, void *to, void *from)
  {
    for (const auto &slot : shape.slots)
    {
      if (slot.kind == FieldKind::String)
      {
        at<std::string>(to, slot.offset) = std::move(at<std::string>(from, slot.offset));
      }
    }
    // everything else is copied as it is
    Shape rest = shape;
    std::erase_if(rest.slots, [](const Slot &slot) { return slot.kind == FieldKind::String; });
    Copy(rest, to, from);
  }

  FieldValue ScriptComponentLayout::Get(const Slot &slot, const void *object)
  {
    switch (slot.kind)
    {
      case FieldKind::Boolean: return at<bool>(object, slot.offset);
      case FieldKind::Integer: return at<int>(object, slot.offset);
      case FieldKind::Float: return at<float>(object, slot.offset);
      case FieldKind::Double: return at<double>(object, slot.offset);
      case FieldKind::String: return at<std::string>(object, slot.offset);
      case FieldKind::Vector3: return at<glm::vec3>(object, slot.offset);
      case FieldKind::Color: return at<Color>(object, slot.offset);
      case FieldKind::Byte: return at<std::uint8_t>(object, slot.offset);
      case FieldKind::Char: return at<char>(object, slot.offset);
      case FieldKind::Short: return at<std::int16_t>(object, slot.offset);
      case FieldKind::UnsignedShort: return at<std::uint16_t>(object, slot.offset);
      case FieldKind::UnsignedInteger: return at<std::uint32_t>(object, slot.offset);
      case FieldKind::Long: return at<std::int64_t>(object, slot.offset);
      case FieldKind::UnsignedLong: return at<std::uint64_t>(object, slot.offset);
      case FieldKind::Vector2: return at<glm::vec2>(object, slot.offset);
      case FieldKind::Vector4: return at<glm::vec4>(object, slot.offset);
      case FieldKind::IntegerVector2: return at<glm::ivec2>(object, slot.offset);
      case FieldKind::IntegerVector3: return at<glm::ivec3>(object, slot.offset);
      case FieldKind::Quaternion: return at<glm::quat>(object, slot.offset);
      case FieldKind::Matrix3: return at<glm::mat3>(object, slot.offset);
      case FieldKind::Matrix4: return at<glm::mat4>(object, slot.offset);
      default: return {};
    }
  }

  void ScriptComponentLayout::Set(const Slot &slot, void *object, const FieldValue &value)
  {
    // a value of another type than the kind holds is left out, as Check()
    // of the field says before anything is set
    switch (slot.kind)
    {
      case FieldKind::Boolean:
        if (const auto *held = std::get_if<bool>(&value)) { at<bool>(object, slot.offset) = *held; }
        break;
      case FieldKind::Integer:
        if (const auto *held = std::get_if<int>(&value)) { at<int>(object, slot.offset) = *held; }
        break;
      case FieldKind::Float:
        if (const auto *held = std::get_if<float>(&value)) { at<float>(object, slot.offset) = *held; }
        break;
      case FieldKind::Double:
        if (const auto *held = std::get_if<double>(&value)) { at<double>(object, slot.offset) = *held; }
        break;
      case FieldKind::String:
        if (const auto *held = std::get_if<std::string>(&value)) { at<std::string>(object, slot.offset) = *held; }
        break;
      case FieldKind::Vector3:
        if (const auto *held = std::get_if<glm::vec3>(&value)) { at<glm::vec3>(object, slot.offset) = *held; }
        break;
      case FieldKind::Color:
        if (const auto *held = std::get_if<Color>(&value)) { at<Color>(object, slot.offset) = *held; }
        break;
      case FieldKind::Byte:
        if (const auto *held = std::get_if<std::uint8_t>(&value)) { at<std::uint8_t>(object, slot.offset) = *held; }
        break;
      case FieldKind::Char:
        if (const auto *held = std::get_if<char>(&value)) { at<char>(object, slot.offset) = *held; }
        break;
      case FieldKind::Short:
        if (const auto *held = std::get_if<std::int16_t>(&value)) { at<std::int16_t>(object, slot.offset) = *held; }
        break;
      case FieldKind::UnsignedShort:
        if (const auto *held = std::get_if<std::uint16_t>(&value)) { at<std::uint16_t>(object, slot.offset) = *held; }
        break;
      case FieldKind::UnsignedInteger:
        if (const auto *held = std::get_if<std::uint32_t>(&value)) { at<std::uint32_t>(object, slot.offset) = *held; }
        break;
      case FieldKind::Long:
        if (const auto *held = std::get_if<std::int64_t>(&value)) { at<std::int64_t>(object, slot.offset) = *held; }
        break;
      case FieldKind::UnsignedLong:
        if (const auto *held = std::get_if<std::uint64_t>(&value)) { at<std::uint64_t>(object, slot.offset) = *held; }
        break;
      case FieldKind::Vector2:
        if (const auto *held = std::get_if<glm::vec2>(&value)) { at<glm::vec2>(object, slot.offset) = *held; }
        break;
      case FieldKind::Vector4:
        if (const auto *held = std::get_if<glm::vec4>(&value)) { at<glm::vec4>(object, slot.offset) = *held; }
        break;
      case FieldKind::IntegerVector2:
        if (const auto *held = std::get_if<glm::ivec2>(&value)) { at<glm::ivec2>(object, slot.offset) = *held; }
        break;
      case FieldKind::IntegerVector3:
        if (const auto *held = std::get_if<glm::ivec3>(&value)) { at<glm::ivec3>(object, slot.offset) = *held; }
        break;
      case FieldKind::Quaternion:
        if (const auto *held = std::get_if<glm::quat>(&value)) { at<glm::quat>(object, slot.offset) = *held; }
        break;
      case FieldKind::Matrix3:
        if (const auto *held = std::get_if<glm::mat3>(&value)) { at<glm::mat3>(object, slot.offset) = *held; }
        break;
      case FieldKind::Matrix4:
        if (const auto *held = std::get_if<glm::mat4>(&value)) { at<glm::mat4>(object, slot.offset) = *held; }
        break;
      default: break;
    }
  }

  ComponentInfo ScriptComponentLayout::GetComponentInfo() const
  {
    const auto shape = _shape;

    ComponentInfo info;
    info.name = shape->name;
    info.size = shape->size;
    info.alignment = shape->alignment;

    info.construct = [shape](void *at, const std::size_t count)
    {
      for (std::size_t i = 0; i < count; i++) { Construct(*shape, static_cast<char *>(at) + i * shape->size); }
    };
    info.destruct = [shape](void *at, const std::size_t count)
    {
      for (std::size_t i = 0; i < count; i++) { Destruct(*shape, static_cast<char *>(at) + i * shape->size); }
    };
    info.copy = [shape](void *to, const void *from, const std::size_t count)
    {
      for (std::size_t i = 0; i < count; i++)
      {
        Copy(*shape, static_cast<char *>(to) + i * shape->size, static_cast<const char *>(from) + i * shape->size);
      }
    };
    info.move = [shape](void *to, void *from, const std::size_t count)
    {
      for (std::size_t i = 0; i < count; i++)
      {
        Move(*shape, static_cast<char *>(to) + i * shape->size, static_cast<char *>(from) + i * shape->size);
      }
    };

    return info;
  }

  std::shared_ptr<const TypeInfo> ScriptComponentLayout::GetTypeInfo() const
  {
    return _type;
  }

  ComponentFormat ScriptComponentLayout::GetComponentFormat() const
  {
    const auto shape = _shape;
    const auto type = _type;

    ComponentFormat format;
    format.name = shape->name;
    format.type = type;

    // reading starts from what the entity has, so that a file on top of a
    // prefab changes only what it names, and from the defaults otherwise
    format.read = [shape, type](const DataReader &reader, EntityStore &store, const Entity entity)
    {
      Buffer buffer = buffer_for(shape->size);
      Construct(*shape, buffer.data());

      const auto id = store.FindComponent(shape->name);
      if (id != No_Component)
      {
        if (const void *existing = store.GetComponent(entity, id)) { Copy(*shape, buffer.data(), existing); }
      }

      ReadFields(*type, reader, buffer.data());
      if (id != No_Component) { store.SetComponent(entity, id, buffer.data()); }

      Destruct(*shape, buffer.data());
    };

    format.write = [shape, type](EntityStore &store, const Entity entity, DataValue &value)
    {
      const auto id = store.FindComponent(shape->name);
      if (id == No_Component) { return false; }

      const void *existing = store.GetComponent(entity, id);
      if (existing == nullptr) { return false; }

      Buffer standard = buffer_for(shape->size);
      Construct(*shape, standard.data());

      value = DataValue::Map();
      WriteFields(*type, existing, standard.data(), value);

      Destruct(*shape, standard.data());
      return true;
    };

    return format;
  }
} // neon
