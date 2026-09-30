#ifndef ENTITY_FIELDS_HPP
#define ENTITY_FIELDS_HPP

#include <string>

#include <neon/world-system/ecs/entity-store.hpp>

#include "type-info.hpp"

namespace neon
{
  /// Reads and changes the fields of the components of entities by name,
  /// for code that does not know the components it works with: the editor
  /// and scripts.
  ///
  /// A field of a group is named with the group in front and a dot between,
  /// such as `material.color`.

  /// Reads a field of the component of an entity. Returns false when the
  /// entity does not carry the component or the component has no such
  /// field, and says which in `error`.
  [[nodiscard]] bool GetField(
    EntityStore &store,
    Entity entity,
    const TypeInfo &type,
    const std::string &path,
    FieldValue &value,
    std::string &error);

  /// Changes a field of the component of an entity. Returns false when the
  /// value cannot be taken, and says why in `error`. The component is left
  /// as it is then.
  [[nodiscard]] bool SetField(
    EntityStore &store,
    Entity entity,
    const TypeInfo &type,
    const std::string &path,
    const FieldValue &value,
    std::string &error);
} // neon

#endif //ENTITY_FIELDS_HPP
