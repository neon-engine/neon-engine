#include "entity-fields.hpp"

#include <format>

namespace neon
{
  // Helpers of entity-fields.cpp, for this file alone.
  namespace
  {
    /// Finds the component and the field, or says what is missing.
    bool Find(
      EntityStore &store,
      const Entity entity,
      const TypeInfo &type,
      const std::string &path,
      void *&component,
      const FieldInfo *&field,
      std::string &error)
    {
      const auto id = store.FindComponent(type.name);
      if (id == No_Component)
      {
        error = std::format("Component {} is not registered", type.name);
        return false;
      }

      component = store.GetComponent(entity, id);
      if (component == nullptr)
      {
        error = std::format("The entity does not carry a {}", type.name);
        return false;
      }

      field = type.Find(path);
      if (field == nullptr)
      {
        std::string known;
        for (const auto &known_path : type.GetPaths())
        {
          if (!known.empty()) { known += ", "; }
          known += known_path;
        }

        error = std::format("'{}' is not known to {}. Known are: {}", path, type.name, known);
        return false;
      }

      return true;
    }
  }

  bool GetField(
    EntityStore &store,
    const Entity entity,
    const TypeInfo &type,
    const std::string &path,
    FieldValue &value,
    std::string &error)
  {
    void *component = nullptr;
    const FieldInfo *field = nullptr;
    if (!Find(store, entity, type, path, component, field, error)) { return false; }

    if (field->kind == FieldKind::Group)
    {
      error = std::format("'{}' of {} is a group and holds no value of its own", path, type.name);
      return false;
    }

    value = field->get(component);
    return true;
  }

  bool SetField(
    EntityStore &store,
    const Entity entity,
    const TypeInfo &type,
    const std::string &path,
    const FieldValue &value,
    std::string &error)
  {
    void *component = nullptr;
    const FieldInfo *field = nullptr;
    if (!Find(store, entity, type, path, component, field, error)) { return false; }

    error = field->Check(value, std::format("'{}' of {}", path, type.name));
    if (!error.empty()) { return false; }

    field->set(component, value);
    if (type.written) { type.written(component); }
    return true;
  }
} // neon
