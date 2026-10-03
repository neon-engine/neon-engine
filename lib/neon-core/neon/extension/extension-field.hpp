#ifndef EXTENSION_FIELD_HPP
#define EXTENSION_FIELD_HPP

#include <memory>
#include <string>

#include <neon/reflection/type-info.hpp>
#include <neon/world-system/ecs/entity.hpp>

namespace neon
{
  /// A field of a kind of component that an extension found by its name,
  /// and reads and changes through its description.
  struct ExtensionField
  {
    ComponentId component = No_Component;

    /// `Transform.position`, for messages.
    std::string name;

    /// Keeps the description the field belongs to.
    std::shared_ptr<const TypeInfo> type;

    const FieldInfo *field = nullptr;
  };
} // neon

#endif //EXTENSION_FIELD_HPP
