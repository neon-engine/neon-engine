#include "vk-shader-values.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "vk-renderer-2d.hpp"

// The values of a shader are read from SPIR-V, which is put together by
// hand here: the words a compiler writes for a block of values, and nothing
// of the rest of a shader.

namespace
{
  using neon::MaterialValue2D;
  using neon::VK_Renderer2D;
  using neon::VK_ShaderValue;
  using neon::VK_ShaderValues;

  using Words = std::vector<std::uint32_t>;

  // the numbers of the instructions in the specification of SPIR-V
  constexpr std::uint32_t op_name = 5;
  constexpr std::uint32_t op_member_name = 6;
  constexpr std::uint32_t op_type_int = 21;
  constexpr std::uint32_t op_type_float = 22;
  constexpr std::uint32_t op_type_vector = 23;
  constexpr std::uint32_t op_type_matrix = 24;
  constexpr std::uint32_t op_type_struct = 30;
  constexpr std::uint32_t op_member_decorate = 72;
  constexpr std::uint32_t decoration_offset = 35;

  Words Header()
  {
    return {0x07230203, 0x00010000, 0, 100, 0};
  }

  void Add(Words &words, const std::uint32_t operation, const Words &operands)
  {
    words.push_back((static_cast<std::uint32_t>(operands.size() + 1) << 16) | operation);
    words.insert(words.end(), operands.begin(), operands.end());
  }

  /// Text as SPIR-V holds it: four bytes in a word, and a byte of 0 at its
  /// end.
  Words Text(const std::string &text)
  {
    Words words((text.size() + 4) / 4, 0);
    std::memcpy(words.data(), text.data(), text.size());
    return words;
  }

  void AddName(Words &words, const std::uint32_t id, const std::string &name)
  {
    Words operands = {id};
    const Words text = Text(name);
    operands.insert(operands.end(), text.begin(), text.end());
    Add(words, op_name, operands);
  }

  void AddMember(
    Words &words,
    const std::uint32_t block,
    const std::uint32_t member,
    const std::string &name,
    const std::uint32_t offset)
  {
    Words operands = {block, member};
    const Words text = Text(name);
    operands.insert(operands.end(), text.begin(), text.end());
    Add(words, op_member_name, operands);

    Add(words, op_member_decorate, {block, member, decoration_offset, offset});
  }

  /// What a compiler writes for
  ///
  ///     uniform Values { float intensity; vec4 tint; vec2 offset; int steps; mat4 turn; } values;
  Words AShader(const std::string &block_name = "Values")
  {
    Words words = Header();

    constexpr std::uint32_t float_type = 1;
    constexpr std::uint32_t vec4_type = 2;
    constexpr std::uint32_t vec2_type = 3;
    constexpr std::uint32_t int_type = 4;
    constexpr std::uint32_t mat4_type = 5;
    constexpr std::uint32_t block = 6;
    constexpr std::uint32_t other = 7;

    // names come in front of the types they name
    AddName(words, block, block_name);
    AddMember(words, block, 0, "intensity", 0);
    AddMember(words, block, 1, "tint", 16);
    AddMember(words, block, 2, "offset", 32);
    AddMember(words, block, 3, "steps", 40);
    AddMember(words, block, 4, "turn", 48);

    AddName(words, other, "Light");
    AddMember(words, other, 0, "color", 0);

    Add(words, op_type_float, {float_type, 32});
    Add(words, op_type_vector, {vec4_type, float_type, 4});
    Add(words, op_type_vector, {vec2_type, float_type, 2});
    Add(words, op_type_int, {int_type, 32, 1});
    Add(words, op_type_matrix, {mat4_type, vec4_type, 4});
    Add(words, op_type_struct, {block, float_type, vec4_type, vec2_type, int_type, mat4_type});
    Add(words, op_type_struct, {other, vec4_type});

    return words;
  }

  TEST(VkShaderValuesTest, ReadsTheValuesOfTheBlock)
  {
    VK_ShaderValues values;
    ASSERT_TRUE(VK_ShaderValues::Read(AShader(), values));

    // the matrix is left out, since a file writes no matrix
    ASSERT_EQ(values.values.size(), 4u);

    const VK_ShaderValue *intensity = values.Find("intensity");
    ASSERT_NE(intensity, nullptr);
    EXPECT_EQ(intensity->offset, 0u);
    EXPECT_EQ(intensity->count, 1u);
    EXPECT_FALSE(intensity->is_integer);

    const VK_ShaderValue *tint = values.Find("tint");
    ASSERT_NE(tint, nullptr);
    EXPECT_EQ(tint->offset, 16u);
    EXPECT_EQ(tint->count, 4u);

    const VK_ShaderValue *offset = values.Find("offset");
    ASSERT_NE(offset, nullptr);
    EXPECT_EQ(offset->offset, 32u);
    EXPECT_EQ(offset->count, 2u);

    const VK_ShaderValue *steps = values.Find("steps");
    ASSERT_NE(steps, nullptr);
    EXPECT_EQ(steps->offset, 40u);
    EXPECT_TRUE(steps->is_integer);

    EXPECT_EQ(values.Find("turn"), nullptr);
    EXPECT_EQ(values.Find("color"), nullptr) << "of another block";
    EXPECT_EQ(values.Find("missing"), nullptr);
  }

  TEST(VkShaderValuesTest, TheBlockIsAsLargeAsItsLastValueRoundedUp)
  {
    VK_ShaderValues values;
    ASSERT_TRUE(VK_ShaderValues::Read(AShader(), values));

    // up to 44, which is rounded up to a multiple of 16
    EXPECT_EQ(values.size, 48u);
  }

  TEST(VkShaderValuesTest, AShaderWithoutTheBlockHasNoValues)
  {
    VK_ShaderValues values;
    ASSERT_TRUE(VK_ShaderValues::Read(AShader("Settings"), values));

    EXPECT_TRUE(values.values.empty());
    EXPECT_EQ(values.size, 0u);

    ASSERT_TRUE(VK_ShaderValues::Read(Header(), values));
    EXPECT_TRUE(values.values.empty());
  }

  TEST(VkShaderValuesTest, RefusesWhatIsNoSpirV)
  {
    VK_ShaderValues values;

    EXPECT_FALSE(VK_ShaderValues::Read({}, values));
    EXPECT_FALSE(VK_ShaderValues::Read({0x07230203, 0, 0}, values));
    EXPECT_FALSE(VK_ShaderValues::Read({1, 2, 3, 4, 5, 6}, values));
  }

  TEST(VkShaderValuesTest, RefusesAnInstructionThatIsCutOff)
  {
    Words words = AShader();

    // one that says it is longer than what is left
    words.push_back((std::uint32_t{9} << 16) | op_name);
    words.push_back(1);

    VK_ShaderValues values;
    EXPECT_FALSE(VK_ShaderValues::Read(words, values));

    // and one without a length, which would never end
    Words endless = Header();
    endless.push_back(op_name);

    EXPECT_FALSE(VK_ShaderValues::Read(endless, values));
  }

  class FillValuesTest : public ::testing::Test
  {
  protected:
    VK_ShaderValues _declared;
    std::vector<unsigned char> _bytes;
    std::vector<std::string> _unknown;

    void SetUp() override
    {
      ASSERT_TRUE(VK_ShaderValues::Read(AShader(), _declared));
    }

    static MaterialValue2D Number(const std::string &name, const float number)
    {
      MaterialValue2D value;
      value.name = name;
      value.numbers[0] = number;
      value.count = 1;
      return value;
    }

    static MaterialValue2D ColorValue(const std::string &name, const float r, const float g, const float b, const float a)
    {
      MaterialValue2D value;
      value.name = name;
      value.numbers[0] = r;
      value.numbers[1] = g;
      value.numbers[2] = b;
      value.numbers[3] = a;
      value.count = 4;
      return value;
    }

    [[nodiscard]] float FloatAt(const std::size_t offset) const
    {
      float number = 0.0f;
      std::memcpy(&number, _bytes.data() + offset, sizeof(number));
      return number;
    }

    [[nodiscard]] std::int32_t IntAt(const std::size_t offset) const
    {
      std::int32_t number = 0;
      std::memcpy(&number, _bytes.data() + offset, sizeof(number));
      return number;
    }
  };

  TEST_F(FillValuesTest, PutsEveryValueAtThePlaceTheShaderDeclaresItAt)
  {
    VK_Renderer2D::FillValues(
      _declared,
      {ColorValue("tint", 1.0f, 0.5f, 0.25f, 0.75f), Number("intensity", 2.5f), Number("steps", 7.0f)},
      _bytes,
      _unknown);

    ASSERT_EQ(_bytes.size(), 48u);
    EXPECT_TRUE(_unknown.empty());

    EXPECT_FLOAT_EQ(FloatAt(0), 2.5f);
    EXPECT_FLOAT_EQ(FloatAt(16), 1.0f);
    EXPECT_FLOAT_EQ(FloatAt(20), 0.5f);
    EXPECT_FLOAT_EQ(FloatAt(24), 0.25f);
    EXPECT_FLOAT_EQ(FloatAt(28), 0.75f);
    EXPECT_EQ(IntAt(40), 7) << "a whole number is written as one";
  }

  TEST_F(FillValuesTest, AValueThatIsNotSetIsZero)
  {
    VK_Renderer2D::FillValues(_declared, {Number("intensity", 1.0f)}, _bytes, _unknown);

    EXPECT_FLOAT_EQ(FloatAt(16), 0.0f);
    EXPECT_FLOAT_EQ(FloatAt(32), 0.0f);
    EXPECT_EQ(IntAt(40), 0);
  }

  TEST_F(FillValuesTest, ANumberForAVectorIsTheNumberInEveryPart)
  {
    VK_Renderer2D::FillValues(_declared, {Number("tint", 0.5f)}, _bytes, _unknown);

    for (const std::size_t offset : {16u, 20u, 24u, 28u}) { EXPECT_FLOAT_EQ(FloatAt(offset), 0.5f); }
  }

  TEST_F(FillValuesTest, AValueThatIsLongerThanItsPlaceIsCutOff)
  {
    // a color for two numbers writes two, and leaves what follows alone
    VK_Renderer2D::FillValues(
      _declared, {Number("steps", 3.0f), ColorValue("offset", 1.0f, 2.0f, 3.0f, 4.0f)}, _bytes, _unknown);

    EXPECT_FLOAT_EQ(FloatAt(32), 1.0f);
    EXPECT_FLOAT_EQ(FloatAt(36), 2.0f);
    EXPECT_EQ(IntAt(40), 3);
  }

  TEST_F(FillValuesTest, SaysWhichNamesTheShaderDoesNotDeclare)
  {
    VK_Renderer2D::FillValues(
      _declared, {Number("intensity", 1.0f), Number("strength", 2.0f), Number("turn", 1.0f)}, _bytes, _unknown);

    EXPECT_EQ(_unknown, (std::vector<std::string>{"strength", "turn"}));
    EXPECT_FLOAT_EQ(FloatAt(0), 1.0f);
  }

  TEST_F(FillValuesTest, AShaderWithoutValuesTakesNone)
  {
    VK_Renderer2D::FillValues(VK_ShaderValues{}, {Number("intensity", 1.0f)}, _bytes, _unknown);

    EXPECT_TRUE(_bytes.empty());
    EXPECT_EQ(_unknown, (std::vector<std::string>{"intensity"}));
  }
}
