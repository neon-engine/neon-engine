#ifndef SCRIPT_FIELD_HPP
#define SCRIPT_FIELD_HPP

#include <string>

#include <neon/reflection/field-value.hpp>

namespace neon
{
  /// One field of a component a script declares: what it is called, what it
  /// holds, and the value it starts with, which is also what a recipe leaves
  /// out.
  struct ScriptField
  {
    std::string name;

    FieldKind kind = FieldKind::Number;

    /// The default. It is of the type the kind holds, which the layout
    /// checks.
    FieldValue standard;

    /// What the field is for, in a sentence. For the editor. May be empty.
    std::string description;
  };
} // neon

#endif //SCRIPT_FIELD_HPP
