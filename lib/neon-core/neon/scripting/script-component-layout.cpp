#include "script-component-layout.hpp"

#include <cctype>
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
        case FieldKind::Bool: return sizeof(bool);
        case FieldKind::Whole: return sizeof(int);
        case FieldKind::Number: return sizeof(float);
        case FieldKind::Precise: return sizeof(double);
        case FieldKind::Text: return sizeof(std::string);
        case FieldKind::Vector: return sizeof(glm::vec3);
        case FieldKind::Color: return sizeof(Color);
        default: return 0;
      }
    }

    std::size_t alignment_of(const FieldKind kind)
    {
      switch (kind)
      {
        case FieldKind::Bool: return alignof(bool);
        case FieldKind::Whole: return alignof(int);
        case FieldKind::Number: return alignof(float);
        case FieldKind::Precise: return alignof(double);
        case FieldKind::Text: return alignof(std::string);
        case FieldKind::Vector: return alignof(glm::vec3);
        case FieldKind::Color: return alignof(Color);
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
        case FieldKind::Bool: new(place) bool(std::get<bool>(slot.standard));
          break;
        case FieldKind::Whole: new(place) int(std::get<int>(slot.standard));
          break;
        case FieldKind::Number: new(place) float(std::get<float>(slot.standard));
          break;
        case FieldKind::Precise: new(place) double(std::get<double>(slot.standard));
          break;
        case FieldKind::Text: new(place) std::string(std::get<std::string>(slot.standard));
          break;
        case FieldKind::Vector: new(place) glm::vec3(std::get<glm::vec3>(slot.standard));
          break;
        case FieldKind::Color: new(place) Color(std::get<Color>(slot.standard));
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
      if (slot.kind == FieldKind::Text) { neon::at<std::string>(at, slot.offset).~basic_string(); }
    }
  }

  void ScriptComponentLayout::Copy(const Shape &shape, void *to, const void *from)
  {
    for (const auto &slot : shape.slots)
    {
      switch (slot.kind)
      {
        case FieldKind::Bool: at<bool>(to, slot.offset) = at<bool>(from, slot.offset);
          break;
        case FieldKind::Whole: at<int>(to, slot.offset) = at<int>(from, slot.offset);
          break;
        case FieldKind::Number: at<float>(to, slot.offset) = at<float>(from, slot.offset);
          break;
        case FieldKind::Precise: at<double>(to, slot.offset) = at<double>(from, slot.offset);
          break;
        case FieldKind::Text: at<std::string>(to, slot.offset) = at<std::string>(from, slot.offset);
          break;
        case FieldKind::Vector: at<glm::vec3>(to, slot.offset) = at<glm::vec3>(from, slot.offset);
          break;
        case FieldKind::Color: at<Color>(to, slot.offset) = at<Color>(from, slot.offset);
          break;
        default: break;
      }
    }
  }

  void ScriptComponentLayout::Move(const Shape &shape, void *to, void *from)
  {
    for (const auto &slot : shape.slots)
    {
      if (slot.kind == FieldKind::Text)
      {
        at<std::string>(to, slot.offset) = std::move(at<std::string>(from, slot.offset));
      }
    }
    // everything else is copied as it is
    Shape rest = shape;
    std::erase_if(rest.slots, [](const Slot &slot) { return slot.kind == FieldKind::Text; });
    Copy(rest, to, from);
  }

  FieldValue ScriptComponentLayout::Get(const Slot &slot, const void *object)
  {
    switch (slot.kind)
    {
      case FieldKind::Bool: return at<bool>(object, slot.offset);
      case FieldKind::Whole: return at<int>(object, slot.offset);
      case FieldKind::Number: return at<float>(object, slot.offset);
      case FieldKind::Precise: return at<double>(object, slot.offset);
      case FieldKind::Text: return at<std::string>(object, slot.offset);
      case FieldKind::Vector: return at<glm::vec3>(object, slot.offset);
      case FieldKind::Color: return at<Color>(object, slot.offset);
      default: return {};
    }
  }

  void ScriptComponentLayout::Set(const Slot &slot, void *object, const FieldValue &value)
  {
    // a value of another type than the kind holds is left out, as Check()
    // of the field says before anything is set
    switch (slot.kind)
    {
      case FieldKind::Bool:
        if (const auto *held = std::get_if<bool>(&value)) { at<bool>(object, slot.offset) = *held; }
        break;
      case FieldKind::Whole:
        if (const auto *held = std::get_if<int>(&value)) { at<int>(object, slot.offset) = *held; }
        break;
      case FieldKind::Number:
        if (const auto *held = std::get_if<float>(&value)) { at<float>(object, slot.offset) = *held; }
        break;
      case FieldKind::Precise:
        if (const auto *held = std::get_if<double>(&value)) { at<double>(object, slot.offset) = *held; }
        break;
      case FieldKind::Text:
        if (const auto *held = std::get_if<std::string>(&value)) { at<std::string>(object, slot.offset) = *held; }
        break;
      case FieldKind::Vector:
        if (const auto *held = std::get_if<glm::vec3>(&value)) { at<glm::vec3>(object, slot.offset) = *held; }
        break;
      case FieldKind::Color:
        if (const auto *held = std::get_if<Color>(&value)) { at<Color>(object, slot.offset) = *held; }
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
