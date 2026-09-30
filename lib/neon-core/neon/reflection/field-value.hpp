#ifndef FIELD_VALUE_HPP
#define FIELD_VALUE_HPP

#include <string>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

#include <neon/common/color.hpp>

namespace neon
{
  /// What a field holds.
  enum class FieldKind
  {
    Bool = 0,

    /// A number without a fraction.
    Whole,

    Number,
    Text,

    /// Three numbers.
    Vector,

    Color,

    /// A list of texts.
    TextList,

    /// One of a few words, which is how an enum is seen from outside.
    Choice,

    /// Fields that belong together under a name of their own, such as the
    /// material of what is drawn. A group holds no value itself.
    Group
  };

  /// The value of a field, whatever the type of the field is in C++.
  ///
  /// It is what lets code that knows nothing of a component read and change
  /// it: what reads a scene, the editor, and a script. A choice is held as
  /// its word. A group has no value.
  using FieldValue = std::variant<
    std::monostate,
    bool,
    int,
    float,
    std::string,
    glm::vec3,
    Color,
    std::vector<std::string>>;

  /// What a kind is called in a message, such as `a number`.
  [[nodiscard]] std::string Describe(FieldKind kind);

  /// Whether a value is of the type that a kind holds.
  [[nodiscard]] bool Holds(const FieldValue &value, FieldKind kind);

  /// Whether two values are the same. Two colors and two vectors are the
  /// same when all of their numbers are.
  [[nodiscard]] bool Same(const FieldValue &left, const FieldValue &right);
} // neon

#endif //FIELD_VALUE_HPP
