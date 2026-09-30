#ifndef TYPE_INFO_HPP
#define TYPE_INFO_HPP

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "field-value.hpp"

namespace neon
{
  template<typename T>
  class TypeBuilder;

  /// Describes one field of a type: what it is called, what it holds, and
  /// how it is read and changed.
  struct FieldInfo
  {
    /// What the field is called from outside, such as in a scene file. It
    /// need not be the name of the member in C++.
    std::string name;

    FieldKind kind = FieldKind::Number;

    /// What the field is for, in a sentence. For the editor.
    std::string description;

    /// The words of a choice, in the order of the enum.
    std::vector<std::string> choices;

    /// The fields of a group.
    std::vector<FieldInfo> fields;

    /// Whether a value has to be given. Only for text, where it means that
    /// the text is not empty.
    bool required = false;

    /// Whether the field is written when it holds its default. For a field
    /// that says what something is, such as the type of a light.
    bool always_written = false;

    /// Whether one number may be written for a vector, which then stands
    /// for all three. For a scale.
    bool one_number_for_all = false;

    /// What a number has to be above, if anything.
    std::optional<float> above;

    /// The least and the most a number may be, if anything.
    std::optional<float> at_least;
    std::optional<float> at_most;

    /// Reads the field of an object. `object` points to an object of the
    /// type the field belongs to.
    std::function<FieldValue(const void *object)> get;

    /// Changes the field of an object. The value has to be of the kind of
    /// the field, which Check() tells.
    std::function<void(void *object, const FieldValue &value)> set;

    /// Says what is wrong with a value for this field, or nothing when it
    /// can be set. `what` is how the field is called in the message, such as
    /// `'fov' of Camera`.
    [[nodiscard]] std::string Check(const FieldValue &value, const std::string &what) const;
  };

  /// Describes a type: what it is called and the fields it has.
  ///
  /// A description is written once, next to the type. Everything that works
  /// with the type from outside follows from it: how it is written in a
  /// scene file, what the editor shows, and what a script can reach.
  struct TypeInfo
  {
    /// What the type is called from outside, such as `Transform`.
    std::string name;

    std::string description;

    std::vector<FieldInfo> fields;

    /// The field of a name, or nullptr. A field of a group is named with
    /// the group in front and a dot between, such as `material.color`.
    [[nodiscard]] const FieldInfo *Find(const std::string &path) const;

    /// Every field that holds a value, with the name it is found by. Groups
    /// are left out, their fields are not.
    [[nodiscard]] std::vector<std::string> GetPaths() const;

    /// The description of a C++ type. There has to be a function
    /// `void Describe(TypeBuilder<T> &type)` next to the type.
    template<typename T>
    [[nodiscard]] static TypeInfo Of();
  };
} // neon

#include "type-builder.hpp"

namespace neon
{
  template<typename T>
  TypeInfo TypeInfo::Of()
  {
    TypeBuilder<T> builder;

    // found next to T, which is what the call without a namespace is for
    Describe(builder);

    return builder.Build();
  }
} // neon

#endif //TYPE_INFO_HPP
