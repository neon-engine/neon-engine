#ifndef FIELD_VALUE_HPP
#define FIELD_VALUE_HPP

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <neon/common/color.hpp>

namespace neon
{
  /// What a field holds.
  enum class FieldKind
  {
    /// `bool`.
    Boolean = 0,

    /// `int`, a number without a fraction.
    Integer,

    /// `float`.
    Float,

    /// `std::string`.
    String,

    /// `glm::vec3`, three numbers.
    Vector3,

    /// `Color`, red, green, blue, and alpha.
    Color,

    /// `std::vector<std::string>`.
    StringList,

    /// One of a few words, which is how an enum is seen from outside.
    Choice,

    /// Fields that belong together under a name of their own, such as the
    /// material of what is drawn. A group holds no value itself.
    Group,

    /// A length with a unit, as a style sheet writes one: `12px`, `50%`,
    /// `auto`, and both added up.
    Length,

    /// `std::vector<float>`.
    FloatList,

    /// Some of the 32 layers, such as those a body of the physics is in.
    /// Held as the numbers of the layers, from 1 to 32, and kept as one bit
    /// for each.
    Layers,

    /// `double`, a number kept with every digit, for what adds up over
    /// time, such as seconds. It is written and read as a number.
    Double,

    /// `std::uint8_t`, a whole number from 0 to 255.
    Byte,

    /// `char`, one character.
    Char,

    /// `std::int16_t`.
    Short,

    /// `std::uint16_t`.
    UnsignedShort,

    /// `std::uint32_t`.
    UnsignedInteger,

    /// `std::int64_t`. A recipe keeps the digits a `double` holds, up to
    /// 2^53 either way.
    Long,

    /// `std::uint64_t`, up to 2^53 in a recipe.
    UnsignedLong,

    /// `glm::vec2`, two numbers.
    Vector2,

    /// `glm::vec4`, four numbers.
    Vector4,

    /// `glm::ivec2`, two whole numbers.
    IntegerVector2,

    /// `glm::ivec3`, three whole numbers.
    IntegerVector3,

    /// `std::vector<int>`.
    IntegerList,

    /// `std::vector<glm::vec3>`, a list of lists of three numbers.
    Vector3List,

    /// `glm::quat`, a turn as four numbers x, y, z, w. A Transform keeps
    /// its rotation as degrees in a recipe; this is for a component that
    /// holds a quaternion of its own.
    Quaternion,

    /// `glm::mat3`, three rows of three numbers.
    Matrix3,

    /// `glm::mat4`, four rows of four numbers.
    Matrix4
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
    double,
    std::uint8_t,
    char,
    std::int16_t,
    std::uint16_t,
    std::uint32_t,
    std::int64_t,
    std::uint64_t,
    glm::vec2,
    glm::vec4,
    glm::ivec2,
    glm::ivec3,
    std::vector<int>,
    std::vector<glm::vec3>,
    glm::quat,
    glm::mat3,
    glm::mat4>;

  /// What a kind is called in a message, such as `a number`.
  [[nodiscard]] std::string Describe(FieldKind kind);

  /// Whether a value is of the type that a kind holds.
  [[nodiscard]] bool Holds(const FieldValue &value, FieldKind kind);

  /// Whether two values are the same. Two colors and two vectors are the
  /// same when all of their numbers are.
  [[nodiscard]] bool Same(const FieldValue &left, const FieldValue &right);

  /// Whether a kind holds one whole number: Integer, Byte, Short, Long,
  /// and the unsigned ones. Char is not a number.
  [[nodiscard]] bool IsWholeKind(FieldKind kind);

  /// The least and the most a whole kind holds, as doubles, which is how a
  /// recipe reads a number. A Long and an UnsignedLong are cut to 2^53,
  /// the digits a double keeps.
  void RangeOfWholeKind(FieldKind kind, double &least, double &most);

  /// A value of a whole kind from a number that is whole and in its range,
  /// which the caller has checked.
  [[nodiscard]] FieldValue WholeValue(FieldKind kind, double number);

  /// The number a value of a whole kind holds, as a double. 0 for any
  /// other value.
  [[nodiscard]] double WholeNumber(const FieldValue &value);
} // neon

#endif //FIELD_VALUE_HPP
