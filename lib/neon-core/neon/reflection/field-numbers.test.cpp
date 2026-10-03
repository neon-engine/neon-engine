#include "field-numbers.hpp"

#include <array>
#include <string>

#include <gtest/gtest.h>

namespace
{
  using neon::FieldKind;
  using neon::FieldValue;
  using neon::FromNumbers;
  using neon::ToNumbers;

  /// Writes a value as numbers, reads it back as its kind, and expects the
  /// same value and as many numbers as were said.
  template<typename T>
  void ExpectRoundTrip(const FieldKind kind, const T &original, const std::size_t count)
  {
    std::array<double, neon::Max_Field_Numbers> numbers{};
    EXPECT_EQ(ToNumbers(FieldValue(original), numbers.data()), count);

    FieldValue back;
    ASSERT_TRUE(FromNumbers(kind, numbers.data(), back));
    ASSERT_TRUE(std::holds_alternative<T>(back));
    EXPECT_TRUE(std::get<T>(back) == original);
  }

  TEST(FieldNumbersTest, WritesANumberAsOneNumberAndReadsItBack)
  {
    ExpectRoundTrip(FieldKind::Float, 1.5f, 1);
    ExpectRoundTrip(FieldKind::Double, 0.1, 1);
    ExpectRoundTrip(FieldKind::Integer, -7, 1);
    ExpectRoundTrip(FieldKind::Byte, std::uint8_t{200}, 1);
    ExpectRoundTrip(FieldKind::Short, std::int16_t{-300}, 1);
    ExpectRoundTrip(FieldKind::UnsignedShort, std::uint16_t{60000}, 1);
    ExpectRoundTrip(FieldKind::UnsignedInteger, std::uint32_t{4000000000u}, 1);
    ExpectRoundTrip(FieldKind::Long, std::int64_t{-5000000000}, 1);
    ExpectRoundTrip(FieldKind::UnsignedLong, std::uint64_t{5000000000u}, 1);
  }

  TEST(FieldNumbersTest, WritesABooleanAsZeroOrOne)
  {
    std::array<double, neon::Max_Field_Numbers> numbers{};
    EXPECT_EQ(ToNumbers(FieldValue(true), numbers.data()), 1u);
    EXPECT_EQ(numbers[0], 1.0);

    ExpectRoundTrip(FieldKind::Boolean, false, 1);
    ExpectRoundTrip(FieldKind::Boolean, true, 1);
  }

  TEST(FieldNumbersTest, WritesVectorsInTheOrderOfTheirMembers)
  {
    ExpectRoundTrip(FieldKind::Vector2, glm::vec2(1.0f, 2.0f), 2);
    ExpectRoundTrip(FieldKind::Vector3, glm::vec3(1.0f, 2.0f, 3.0f), 3);
    ExpectRoundTrip(FieldKind::Vector4, glm::vec4(1.0f, 2.0f, 3.0f, 4.0f), 4);
    ExpectRoundTrip(FieldKind::IntegerVector2, glm::ivec2(1, -2), 2);
    ExpectRoundTrip(FieldKind::IntegerVector3, glm::ivec3(1, -2, 3), 3);
  }

  TEST(FieldNumbersTest, WritesAQuaternionAsXYZW)
  {
    std::array<double, neon::Max_Field_Numbers> numbers{};
    // glm takes w first
    EXPECT_EQ(ToNumbers(FieldValue(glm::quat(4.0f, 1.0f, 2.0f, 3.0f)), numbers.data()), 4u);
    EXPECT_EQ(numbers[0], 1.0);
    EXPECT_EQ(numbers[3], 4.0);

    ExpectRoundTrip(FieldKind::Quaternion, glm::quat(4.0f, 1.0f, 2.0f, 3.0f), 4);
  }

  TEST(FieldNumbersTest, WritesAColourAsRedGreenBlueAlpha)
  {
    std::array<double, neon::Max_Field_Numbers> numbers{};
    EXPECT_EQ(ToNumbers(FieldValue(neon::Color{0.1f, 0.2f, 0.3f, 0.4f}), numbers.data()), 4u);
    EXPECT_FLOAT_EQ(static_cast<float>(numbers[2]), 0.3f);

    FieldValue back;
    ASSERT_TRUE(FromNumbers(FieldKind::Color, numbers.data(), back));
    EXPECT_FLOAT_EQ(std::get<neon::Color>(back).a, 0.4f);
  }

  TEST(FieldNumbersTest, WritesNoNumbersForWhatIsNotMadeOfThem)
  {
    std::array<double, neon::Max_Field_Numbers> numbers{};
    EXPECT_EQ(ToNumbers(FieldValue(std::string("text")), numbers.data()), 0u);
    EXPECT_EQ(ToNumbers(FieldValue(std::vector<float>{1.0f}), numbers.data()), 0u);
    EXPECT_EQ(ToNumbers(FieldValue(glm::mat4(1.0f)), numbers.data()), 0u);
    EXPECT_EQ(ToNumbers(FieldValue(), numbers.data()), 0u);

    FieldValue value;
    EXPECT_FALSE(FromNumbers(FieldKind::String, numbers.data(), value));
    EXPECT_FALSE(FromNumbers(FieldKind::Matrix4, numbers.data(), value));
    EXPECT_FALSE(FromNumbers(FieldKind::Choice, numbers.data(), value));
  }
}
