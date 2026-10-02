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
    Group,

    /// A length with a unit, as a style sheet writes one: `12px`, `50%`,
    /// `auto`, and both added up.
    Length,

    /// A list of numbers.
    NumberList,

    /// Some of the 32 layers, such as those a body of the physics is in.
    /// Held as the numbers of the layers, from 1 to 32, and kept as one bit
    /// for each.
    Layers,

    /// A number kept with the precision of a `double`, for what adds up
    /// over time, such as seconds. It is written and read as a number.
    Precise
  };

  /// A length as a style sheet writes one: pixels, a percentage of
  /// something, both added up, or `auto`, which leaves the length to what
  /// works it out.
  struct FieldLength
  {
    bool is_auto = true;
    float pixels = 0.0f;
    float percent = 0.0f;

    static FieldLength Auto()
    {
      return {};
    }

    static FieldLength Pixels(const float pixels)
    {
      return {false, pixels, 0.0f};
    }

    static FieldLength Percent(const float percent)
    {
      return {false, 0.0f, percent};
    }

    /// Pixels and a percentage added up, as `calc(100% - 20px)`.
    static FieldLength Sum(const float pixels, const float percent)
    {
      return {false, pixels, percent};
    }

    bool operator==(const FieldLength &other) const = default;
  };

  /// The value of a field, whatever the type of the field is in C++.
  ///
  /// It is what lets code that knows nothing of a component read and change
  /// it: what reads a scene, the editor, and a script. A choice is held as
  /// its word. A group has no value. Types are added at the end, so that
  /// what a value holds keeps the place it has.
  using FieldValue = std::variant<
    std::monostate,
    bool,
    int,
    float,
    std::string,
    glm::vec3,
    Color,
    std::vector<std::string>,
    FieldLength,
    std::vector<float>,
    double>;

  /// What a kind is called in a message, such as `a number`.
  [[nodiscard]] std::string Describe(FieldKind kind);

  /// Whether a value is of the type that a kind holds.
  [[nodiscard]] bool Holds(const FieldValue &value, FieldKind kind);

  /// Whether two values are the same. Two colors and two vectors are the
  /// same when all of their numbers are.
  [[nodiscard]] bool Same(const FieldValue &left, const FieldValue &right);
} // neon

#endif //FIELD_VALUE_HPP
