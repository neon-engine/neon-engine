#include "field-value.hpp"

#include <cmath>
#include <limits>

namespace neon
{
  // Helpers of the field values, for this file alone.
  namespace
  {
    /// The one way two values of the same alternative are compared.
    struct SameVisitor
    {
      const FieldValue &right;

      bool operator()(const std::monostate &) const { return true; }

      bool operator()(const Color &color) const
      {
        const auto &other = std::get<Color>(right);
        return color.r == other.r && color.g == other.g && color.b == other.b && color.a == other.a;
      }

      template<typename T>
      bool operator()(const T &value) const
      {
        return value == std::get<T>(right);
      }
    };
  }

  std::string Describe(const FieldKind kind)
  {
    switch (kind)
    {
      case FieldKind::Boolean: return "true or false";
      case FieldKind::Integer: return "a whole number";
      case FieldKind::Float: return "a number";
      case FieldKind::String: return "text";
      case FieldKind::Vector3: return "three numbers";
      case FieldKind::Color: return "a color";
      case FieldKind::StringList: return "a list of texts";
      case FieldKind::Choice: return "one of a few words";
      case FieldKind::Group: return "a group";
      case FieldKind::Length: return "a length";
      case FieldKind::FloatList: return "a list of numbers";
      case FieldKind::Layers: return "a list of layers";
      case FieldKind::Double: return "a number";
      case FieldKind::Byte: return "a whole number from 0 to 255";
      case FieldKind::Char: return "one character";
      case FieldKind::Short: return "a whole number from -32768 to 32767";
      case FieldKind::UnsignedShort: return "a whole number from 0 to 65535";
      case FieldKind::UnsignedInteger: return "a whole number from 0";
      case FieldKind::Long: return "a whole number";
      case FieldKind::UnsignedLong: return "a whole number from 0";
      case FieldKind::Vector2: return "two numbers";
      case FieldKind::Vector4: return "four numbers";
      case FieldKind::IntegerVector2: return "two whole numbers";
      case FieldKind::IntegerVector3: return "three whole numbers";
      case FieldKind::IntegerList: return "a list of whole numbers";
      case FieldKind::Vector3List: return "a list of vectors of three numbers";
      case FieldKind::Quaternion: return "a quaternion of four numbers";
      case FieldKind::Matrix3: return "three rows of three numbers";
      case FieldKind::Matrix4: return "four rows of four numbers";
    }
    return "unknown";
  }

  bool Holds(const FieldValue &value, const FieldKind kind)
  {
    switch (kind)
    {
      case FieldKind::Boolean: return std::holds_alternative<bool>(value);
      case FieldKind::Integer: return std::holds_alternative<int>(value);
      case FieldKind::Float: return std::holds_alternative<float>(value);
      case FieldKind::String: return std::holds_alternative<std::string>(value);
      case FieldKind::Vector3: return std::holds_alternative<glm::vec3>(value);
      case FieldKind::Color: return std::holds_alternative<Color>(value);
      case FieldKind::StringList: return std::holds_alternative<std::vector<std::string>>(value);
      case FieldKind::Choice: return std::holds_alternative<std::string>(value);
      case FieldKind::Group: return false;
      case FieldKind::Length: return std::holds_alternative<FieldLength>(value);
      case FieldKind::FloatList: return std::holds_alternative<std::vector<float>>(value);
      case FieldKind::Layers: return std::holds_alternative<std::vector<float>>(value);
      case FieldKind::Double: return std::holds_alternative<double>(value);
      case FieldKind::Byte: return std::holds_alternative<std::uint8_t>(value);
      case FieldKind::Char: return std::holds_alternative<char>(value);
      case FieldKind::Short: return std::holds_alternative<std::int16_t>(value);
      case FieldKind::UnsignedShort: return std::holds_alternative<std::uint16_t>(value);
      case FieldKind::UnsignedInteger: return std::holds_alternative<std::uint32_t>(value);
      case FieldKind::Long: return std::holds_alternative<std::int64_t>(value);
      case FieldKind::UnsignedLong: return std::holds_alternative<std::uint64_t>(value);
      case FieldKind::Vector2: return std::holds_alternative<glm::vec2>(value);
      case FieldKind::Vector4: return std::holds_alternative<glm::vec4>(value);
      case FieldKind::IntegerVector2: return std::holds_alternative<glm::ivec2>(value);
      case FieldKind::IntegerVector3: return std::holds_alternative<glm::ivec3>(value);
      case FieldKind::IntegerList: return std::holds_alternative<std::vector<int>>(value);
      case FieldKind::Vector3List: return std::holds_alternative<std::vector<glm::vec3>>(value);
      case FieldKind::Quaternion: return std::holds_alternative<glm::quat>(value);
      case FieldKind::Matrix3: return std::holds_alternative<glm::mat3>(value);
      case FieldKind::Matrix4: return std::holds_alternative<glm::mat4>(value);
    }
    return false;
  }

  bool Same(const FieldValue &left, const FieldValue &right)
  {
    if (left.index() != right.index()) { return false; }
    return std::visit(SameVisitor{right}, left);
  }

  bool IsWholeKind(const FieldKind kind)
  {
    switch (kind)
    {
      case FieldKind::Integer:
      case FieldKind::Byte:
      case FieldKind::Short:
      case FieldKind::UnsignedShort:
      case FieldKind::UnsignedInteger:
      case FieldKind::Long:
      case FieldKind::UnsignedLong: return true;
      default: return false;
    }
  }

  void RangeOfWholeKind(const FieldKind kind, double &least, double &most)
  {
    // 2^53, the last whole number a double counts one by one
    constexpr double exact = 9007199254740992.0;
    switch (kind)
    {
      case FieldKind::Integer:
        least = std::numeric_limits<int>::min();
        most = std::numeric_limits<int>::max();
        break;
      case FieldKind::Byte:
        least = 0;
        most = 255;
        break;
      case FieldKind::Short:
        least = -32768;
        most = 32767;
        break;
      case FieldKind::UnsignedShort:
        least = 0;
        most = 65535;
        break;
      case FieldKind::UnsignedInteger:
        least = 0;
        most = std::numeric_limits<std::uint32_t>::max();
        break;
      case FieldKind::Long:
        least = -exact;
        most = exact;
        break;
      case FieldKind::UnsignedLong:
        least = 0;
        most = exact;
        break;
      default:
        least = 0;
        most = 0;
        break;
    }
  }

  FieldValue WholeValue(const FieldKind kind, const double number)
  {
    switch (kind)
    {
      case FieldKind::Integer: return static_cast<int>(number);
      case FieldKind::Byte: return static_cast<std::uint8_t>(number);
      case FieldKind::Short: return static_cast<std::int16_t>(number);
      case FieldKind::UnsignedShort: return static_cast<std::uint16_t>(number);
      case FieldKind::UnsignedInteger: return static_cast<std::uint32_t>(number);
      case FieldKind::Long: return static_cast<std::int64_t>(number);
      case FieldKind::UnsignedLong: return static_cast<std::uint64_t>(number);
      default: return {};
    }
  }

  double WholeNumber(const FieldValue &value)
  {
    if (const auto *held = std::get_if<int>(&value)) { return *held; }
    if (const auto *held = std::get_if<std::uint8_t>(&value)) { return *held; }
    if (const auto *held = std::get_if<std::int16_t>(&value)) { return *held; }
    if (const auto *held = std::get_if<std::uint16_t>(&value)) { return *held; }
    if (const auto *held = std::get_if<std::uint32_t>(&value)) { return *held; }
    if (const auto *held = std::get_if<std::int64_t>(&value)) { return static_cast<double>(*held); }
    if (const auto *held = std::get_if<std::uint64_t>(&value)) { return static_cast<double>(*held); }
    return 0.0;
  }
} // neon
