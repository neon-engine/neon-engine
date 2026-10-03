#include "field-numbers.hpp"

#include <cstdint>
#include <type_traits>

namespace neon
{
  // Helpers of ToNumbers, for this file alone.
  namespace
  {
    /// Writes what a value holds, by its type. What is not made of numbers
    /// writes none.
    struct Writer
    {
      double *numbers;

      std::size_t operator()(const bool value) const
      {
        numbers[0] = value ? 1.0 : 0.0;
        return 1;
      }

      std::size_t operator()(const glm::vec2 &value) const
      {
        numbers[0] = value.x;
        numbers[1] = value.y;
        return 2;
      }

      std::size_t operator()(const glm::vec3 &value) const
      {
        numbers[0] = value.x;
        numbers[1] = value.y;
        numbers[2] = value.z;
        return 3;
      }

      std::size_t operator()(const glm::vec4 &value) const
      {
        numbers[0] = value.x;
        numbers[1] = value.y;
        numbers[2] = value.z;
        numbers[3] = value.w;
        return 4;
      }

      std::size_t operator()(const glm::ivec2 &value) const
      {
        numbers[0] = value.x;
        numbers[1] = value.y;
        return 2;
      }

      std::size_t operator()(const glm::ivec3 &value) const
      {
        numbers[0] = value.x;
        numbers[1] = value.y;
        numbers[2] = value.z;
        return 3;
      }

      std::size_t operator()(const Color &value) const
      {
        numbers[0] = value.r;
        numbers[1] = value.g;
        numbers[2] = value.b;
        numbers[3] = value.a;
        return 4;
      }

      std::size_t operator()(const glm::quat &value) const
      {
        numbers[0] = value.x;
        numbers[1] = value.y;
        numbers[2] = value.z;
        numbers[3] = value.w;
        return 4;
      }

      /// A number is one number; everything else that has no function of
      /// its own above is not made of numbers.
      template<typename T>
      std::size_t operator()(const T &value) const
      {
        if constexpr (std::is_arithmetic_v<T>)
        {
          numbers[0] = static_cast<double>(value);
          return 1;
        } else
        {
          return 0;
        }
      }
    };
  }

  std::size_t ToNumbers(const FieldValue &value, double *numbers)
  {
    return std::visit(Writer{numbers}, value);
  }

  bool FromNumbers(const FieldKind kind, const double *numbers, FieldValue &value)
  {
    const double *n = numbers;
    switch (kind)
    {
      case FieldKind::Boolean: value = n[0] != 0.0;
        return true;
      case FieldKind::Integer: value = static_cast<int>(n[0]);
        return true;
      case FieldKind::Float: value = static_cast<float>(n[0]);
        return true;
      case FieldKind::Double: value = n[0];
        return true;
      case FieldKind::Vector2: value = glm::vec2(n[0], n[1]);
        return true;
      case FieldKind::Vector3: value = glm::vec3(n[0], n[1], n[2]);
        return true;
      case FieldKind::Vector4: value = glm::vec4(n[0], n[1], n[2], n[3]);
        return true;
      case FieldKind::Color:
        value = Color{
          static_cast<float>(n[0]), static_cast<float>(n[1]), static_cast<float>(n[2]), static_cast<float>(n[3])
        };
        return true;
      case FieldKind::Quaternion:
        // glm takes w first, and keeps x, y, z, w in memory
        value = glm::quat(
          static_cast<float>(n[3]), static_cast<float>(n[0]), static_cast<float>(n[1]), static_cast<float>(n[2]));
        return true;
      case FieldKind::Char: value = static_cast<char>(n[0]);
        return true;
      case FieldKind::Byte: value = static_cast<std::uint8_t>(n[0]);
        return true;
      case FieldKind::Short: value = static_cast<std::int16_t>(n[0]);
        return true;
      case FieldKind::UnsignedShort: value = static_cast<std::uint16_t>(n[0]);
        return true;
      case FieldKind::UnsignedInteger: value = static_cast<std::uint32_t>(n[0]);
        return true;
      case FieldKind::Long: value = static_cast<std::int64_t>(n[0]);
        return true;
      case FieldKind::UnsignedLong: value = static_cast<std::uint64_t>(n[0]);
        return true;
      case FieldKind::IntegerVector2: value = glm::ivec2(n[0], n[1]);
        return true;
      case FieldKind::IntegerVector3: value = glm::ivec3(n[0], n[1], n[2]);
        return true;
      default: return false;
    }
  }
} // neon
