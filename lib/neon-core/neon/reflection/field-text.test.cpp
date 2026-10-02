#include "field-text.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
  using neon::Color;
  using neon::FieldInfo;
  using neon::FieldKind;
  using neon::FieldLength;
  using neon::FieldValue;
  using ::testing::ElementsAre;

  FieldInfo Field(const FieldKind kind)
  {
    FieldInfo field;
    field.name = "field";
    field.kind = kind;
    return field;
  }

  FieldValue Read(const std::string &text, const FieldKind kind)
  {
    FieldValue value;
    std::string error;
    EXPECT_TRUE(neon::ParseField(text, Field(kind), "'field' of Thing", value, error)) << text << ": " << error;
    EXPECT_TRUE(neon::Holds(value, kind)) << text;
    return value;
  }

  std::string ProblemOf(const std::string &text, const FieldKind kind)
  {
    FieldValue value = 7;
    std::string error;
    EXPECT_FALSE(neon::ParseField(text, Field(kind), "'field' of Thing", value, error)) << text;

    // what cannot be read leaves the value alone
    EXPECT_EQ(std::get<int>(value), 7);
    return error;
  }

  // lengths

  TEST(FieldLengthTest, StartsAsAuto)
  {
    EXPECT_TRUE(FieldLength{}.is_auto);
    EXPECT_EQ(FieldLength{}, FieldLength::Auto());
  }

  TEST(FieldLengthTest, IsWrittenAsAStyleSheetWritesIt)
  {
    EXPECT_EQ(neon::FormatFieldLength(FieldLength::Auto()), "auto");
    EXPECT_EQ(neon::FormatFieldLength(FieldLength::Pixels(12.0f)), "12px");
    EXPECT_EQ(neon::FormatFieldLength(FieldLength::Pixels(-0.5f)), "-0.5px");
    EXPECT_EQ(neon::FormatFieldLength(FieldLength::Pixels(0.0f)), "0px");
    EXPECT_EQ(neon::FormatFieldLength(FieldLength::Percent(50.0f)), "50%");
    EXPECT_EQ(neon::FormatFieldLength(FieldLength::Percent(33.3333f)), "33.3333%");
    EXPECT_EQ(neon::FormatFieldLength(FieldLength::Sum(-20.0f, 100.0f)), "calc(100% + -20px)");
  }

  TEST(FieldLengthTest, IsReadAsAStyleSheetWritesIt)
  {
    FieldLength length;

    ASSERT_TRUE(neon::ParseFieldLength("12px", length));
    EXPECT_EQ(length, FieldLength::Pixels(12.0f));

    ASSERT_TRUE(neon::ParseFieldLength(" 12.5PX ", length));
    EXPECT_EQ(length, FieldLength::Pixels(12.5f));

    ASSERT_TRUE(neon::ParseFieldLength("-4", length));
    EXPECT_EQ(length, FieldLength::Pixels(-4.0f));

    ASSERT_TRUE(neon::ParseFieldLength("50%", length));
    EXPECT_EQ(length, FieldLength::Percent(50.0f));

    ASSERT_TRUE(neon::ParseFieldLength("Auto", length));
    EXPECT_TRUE(length.is_auto);

    ASSERT_TRUE(neon::ParseFieldLength("calc(100% + -20px)", length));
    EXPECT_EQ(length, FieldLength::Sum(-20.0f, 100.0f));
  }

  TEST(FieldLengthTest, ComesBackAsItWasWritten)
  {
    for (const FieldLength &length : {
           FieldLength::Auto(), FieldLength::Pixels(12.0f), FieldLength::Pixels(0.25f), FieldLength::Percent(75.0f),
           FieldLength::Sum(8.0f, 50.0f)
         })
    {
      FieldLength read = FieldLength::Pixels(999.0f);
      ASSERT_TRUE(neon::ParseFieldLength(neon::FormatFieldLength(length), read));
      EXPECT_EQ(read, length);
    }
  }

  TEST(FieldLengthTest, RefusesWhatIsNoLength)
  {
    FieldLength length = FieldLength::Pixels(7.0f);

    for (const std::string text : {"", "wide", "12em", "12 px", "px", "%", "1.2.3", "calc(1px)", "calc(50% - 2px)"})
    {
      EXPECT_FALSE(neon::ParseFieldLength(text, length)) << text;
    }

    EXPECT_EQ(length, FieldLength::Pixels(7.0f));
  }

  // colours

  TEST(FieldColorTest, IsWrittenInTheNotationOfCss)
  {
    EXPECT_EQ(neon::FormatFieldColor({1.0f, 0.5f, 0.0f, 1.0f}), "#ff8000");
    EXPECT_EQ(neon::FormatFieldColor({0.0f, 0.0f, 0.0f, 1.0f}), "#000000");
    EXPECT_EQ(neon::FormatFieldColor({1.0f, 1.0f, 1.0f, 0.5f}), "#ffffff80");
    EXPECT_EQ(neon::FormatFieldColor({0.0f, 0.0f, 0.0f, 0.0f}), "#00000000");

    // held to what a colour can be
    EXPECT_EQ(neon::FormatFieldColor({2.0f, -1.0f, 0.2f, 1.0f}), "#ff0033");
  }

  TEST(FieldColorTest, IsReadInEveryNotationWithAHash)
  {
    Color color;

    ASSERT_TRUE(neon::ParseFieldColor("#ff8000", color));
    EXPECT_FLOAT_EQ(color.r, 1.0f);
    EXPECT_NEAR(color.g, 0.502f, 0.001f);
    EXPECT_FLOAT_EQ(color.b, 0.0f);
    EXPECT_FLOAT_EQ(color.a, 1.0f);

    ASSERT_TRUE(neon::ParseFieldColor("#FF800080", color));
    EXPECT_NEAR(color.a, 0.502f, 0.001f);

    ASSERT_TRUE(neon::ParseFieldColor("#f80", color));
    EXPECT_FLOAT_EQ(color.r, 1.0f);
    EXPECT_NEAR(color.g, 0.533f, 0.001f);

    ASSERT_TRUE(neon::ParseFieldColor(" #f808 ", color));
    EXPECT_NEAR(color.a, 0.533f, 0.001f);
  }

  TEST(FieldColorTest, IsReadAsAFunction)
  {
    Color color;

    ASSERT_TRUE(neon::ParseFieldColor("rgb(255, 128, 0)", color));
    EXPECT_FLOAT_EQ(color.r, 1.0f);
    EXPECT_NEAR(color.g, 0.502f, 0.001f);
    EXPECT_FLOAT_EQ(color.a, 1.0f);

    ASSERT_TRUE(neon::ParseFieldColor("rgba(255, 128, 0, 0.5)", color));
    EXPECT_FLOAT_EQ(color.a, 0.5f);

    ASSERT_TRUE(neon::ParseFieldColor("rgb(255 128 0 / 25%)", color));
    EXPECT_FLOAT_EQ(color.a, 0.25f);

    ASSERT_TRUE(neon::ParseFieldColor("rgb(100%, 50%, 0%)", color));
    EXPECT_FLOAT_EQ(color.g, 0.5f);
  }

  TEST(FieldColorTest, ComesBackAsItWasWritten)
  {
    for (const std::string text : {"#ff8000", "#000000", "#12345678", "#ffffff00"})
    {
      Color color;
      ASSERT_TRUE(neon::ParseFieldColor(text, color)) << text;
      EXPECT_EQ(neon::FormatFieldColor(color), text);
    }
  }

  TEST(FieldColorTest, RefusesWhatIsNoColour)
  {
    Color color{0.1f, 0.2f, 0.3f, 0.4f};

    for (const std::string text : {"", "red", "#", "#ff", "#ff800", "#gg8000", "rgb(1, 2)", "rgb(a, b, c)",
                                   "rgb(1, 2, 3", "hsl(1, 2%, 3%)"})
    {
      EXPECT_FALSE(neon::ParseFieldColor(text, color)) << text;
    }

    EXPECT_FLOAT_EQ(color.r, 0.1f);
    EXPECT_FLOAT_EQ(color.a, 0.4f);
  }

  // every kind as text

  TEST(FieldTextTest, WritesAValueOfEveryKind)
  {
    EXPECT_EQ(neon::FormatField(FieldValue{true}), "true");
    EXPECT_EQ(neon::FormatField(FieldValue{false}), "false");
    EXPECT_EQ(neon::FormatField(FieldValue{42}), "42");
    EXPECT_EQ(neon::FormatField(FieldValue{-7}), "-7");
    EXPECT_EQ(neon::FormatField(FieldValue{0.5f}), "0.5");
    EXPECT_EQ(neon::FormatField(FieldValue{75.0f}), "75");
    EXPECT_EQ(neon::FormatField(FieldValue{1234.56789012345}), "1234.56789012345");
    EXPECT_EQ(neon::FormatField(FieldValue{std::string("Hello, world")}), "Hello, world");
    EXPECT_EQ(neon::FormatField(FieldValue{glm::vec3{1.0f, 2.5f, -3.0f}}), "1 2.5 -3");
    EXPECT_EQ(neon::FormatField(FieldValue{Color{1.0f, 0.5f, 0.0f, 1.0f}}), "#ff8000");
    EXPECT_EQ(neon::FormatField(FieldValue{std::vector<std::string>{"a", "b c", "d"}}), "a, b c, d");
    EXPECT_EQ(neon::FormatField(FieldValue{FieldLength::Percent(50.0f)}), "50%");
    EXPECT_EQ(neon::FormatField(FieldValue{std::vector<float>{1.0f, 2.5f, 3.0f, 4.0f}}), "1 2.5 3 4");
    EXPECT_EQ(neon::FormatField(FieldValue{}), "");
  }

  TEST(FieldTextTest, ReadsAValueOfEveryKind)
  {
    EXPECT_EQ(std::get<bool>(Read("true", FieldKind::Bool)), true);
    EXPECT_EQ(std::get<bool>(Read(" False ", FieldKind::Bool)), false);

    EXPECT_EQ(std::get<int>(Read("42", FieldKind::Whole)), 42);
    EXPECT_EQ(std::get<int>(Read("-7", FieldKind::Whole)), -7);
    EXPECT_EQ(std::get<int>(Read("3.0", FieldKind::Whole)), 3);

    EXPECT_FLOAT_EQ(std::get<float>(Read("0.5", FieldKind::Number)), 0.5f);
    EXPECT_FLOAT_EQ(std::get<float>(Read(" -12.25 ", FieldKind::Number)), -12.25f);
    EXPECT_FLOAT_EQ(std::get<float>(Read(".5", FieldKind::Number)), 0.5f);

    // a precise number keeps the digits a float would lose
    EXPECT_EQ(std::get<double>(Read("1234.56789012345", FieldKind::Precise)), 1234.56789012345);
    EXPECT_EQ(std::get<double>(Read(" -0.25 ", FieldKind::Precise)), -0.25);

    // a text is what it is, spaces included
    EXPECT_EQ(std::get<std::string>(Read("  Hello, world ", FieldKind::Text)), "  Hello, world ");
    EXPECT_EQ(std::get<std::string>(Read("", FieldKind::Text)), "");
    EXPECT_EQ(std::get<std::string>(Read(" window ", FieldKind::Choice)), "window");

    EXPECT_EQ(std::get<glm::vec3>(Read("1 2.5 -3", FieldKind::Vector)), (glm::vec3{1.0f, 2.5f, -3.0f}));
    EXPECT_EQ(std::get<glm::vec3>(Read("1, 2, 3", FieldKind::Vector)), (glm::vec3{1.0f, 2.0f, 3.0f}));

    EXPECT_FLOAT_EQ(std::get<Color>(Read("#ff8000", FieldKind::Color)).r, 1.0f);
    EXPECT_FLOAT_EQ(std::get<Color>(Read("rgba(0, 0, 255, 0.5)", FieldKind::Color)).a, 0.5f);

    EXPECT_THAT(std::get<std::vector<std::string>>(Read("a, b c ,d", FieldKind::TextList)), ElementsAre("a", "b c", "d"));
    EXPECT_TRUE(std::get<std::vector<std::string>>(Read("", FieldKind::TextList)).empty());

    EXPECT_EQ(std::get<FieldLength>(Read("50%", FieldKind::Length)), FieldLength::Percent(50.0f));
    EXPECT_EQ(std::get<FieldLength>(Read("auto", FieldKind::Length)), FieldLength::Auto());

    EXPECT_THAT(std::get<std::vector<float>>(Read("1 2.5, 3", FieldKind::NumberList)), ElementsAre(1.0f, 2.5f, 3.0f));
    EXPECT_TRUE(std::get<std::vector<float>>(Read("", FieldKind::NumberList)).empty());
  }

  TEST(FieldTextTest, ReadsOneNumberForAVectorWhereTheFieldAllowsIt)
  {
    FieldInfo field = Field(FieldKind::Vector);
    field.one_number_for_all = true;

    FieldValue value;
    std::string error;
    ASSERT_TRUE(neon::ParseField("2", field, "'scale' of Transform", value, error));
    EXPECT_EQ(std::get<glm::vec3>(value), (glm::vec3{2.0f, 2.0f, 2.0f}));

    EXPECT_EQ(
      ProblemOf("2", FieldKind::Vector),
      "'field' of Thing is '2', where three numbers, such as 1 2 3 was expected");
  }

  TEST(FieldTextTest, ComesBackAsItWasWrittenForEveryKind)
  {
    const std::pair<FieldKind, FieldValue> values[] = {
      {FieldKind::Bool, FieldValue{true}},
      {FieldKind::Whole, FieldValue{-12}},
      {FieldKind::Number, FieldValue{0.125f}},
      {FieldKind::Precise, FieldValue{1234.56789012345}},
      {FieldKind::Text, FieldValue{std::string("some text")}},
      {FieldKind::Vector, FieldValue{glm::vec3{1.0f, -2.0f, 0.5f}}},
      {FieldKind::TextList, FieldValue{std::vector<std::string>{"one", "two"}}},
      {FieldKind::Length, FieldValue{FieldLength::Sum(4.0f, 25.0f)}},
      {FieldKind::NumberList, FieldValue{std::vector<float>{8.0f, 16.0f}}}
    };

    for (const auto &[kind, value] : values)
    {
      const FieldValue read = Read(neon::FormatField(value), kind);
      EXPECT_TRUE(neon::Same(read, value)) << neon::FormatField(value);
    }
  }

  TEST(FieldTextTest, SaysWhatWasExpected)
  {
    EXPECT_EQ(ProblemOf("yes", FieldKind::Bool), "'field' of Thing is 'yes', where true or false was expected");
    EXPECT_EQ(ProblemOf("1.5", FieldKind::Whole), "'field' of Thing is '1.5', where a whole number was expected");
    EXPECT_EQ(ProblemOf("many", FieldKind::Whole), "'field' of Thing is 'many', where a whole number was expected");
    EXPECT_EQ(ProblemOf("fast", FieldKind::Number), "'field' of Thing is 'fast', where a number was expected");
    EXPECT_EQ(ProblemOf("1,5", FieldKind::Number), "'field' of Thing is '1,5', where a number was expected");
    EXPECT_EQ(ProblemOf("", FieldKind::Number), "'field' of Thing is '', where a number was expected");
    EXPECT_EQ(ProblemOf("slow", FieldKind::Precise), "'field' of Thing is 'slow', where a number was expected");
    EXPECT_EQ(
      ProblemOf("1 2", FieldKind::Vector),
      "'field' of Thing is '1 2', where three numbers, such as 1 2 3 was expected");
    EXPECT_EQ(
      ProblemOf("red", FieldKind::Color),
      "'field' of Thing is 'red', where a color such as #ff8000 or rgb(255, 128, 0) was expected");
    EXPECT_EQ(
      ProblemOf("wide", FieldKind::Length),
      "'field' of Thing is 'wide', where a length such as 12px, 50%, or auto was expected");
    EXPECT_EQ(
      ProblemOf("1 two 3", FieldKind::NumberList),
      "'field' of Thing is '1 two 3', where numbers, such as 1 2 3 4 was expected");
    EXPECT_EQ(
      ProblemOf("1 ground", FieldKind::Layers),
      "'field' of Thing is '1 ground', where the numbers of layers, such as 1 3 was expected");
    EXPECT_EQ(ProblemOf("anything", FieldKind::Group), "'field' of Thing is a group and holds no value of its own");
  }

  TEST(FieldTextTest, WritesAndReadsLayersAsTheirNumbers)
  {
    const FieldValue layers = std::vector{1.0f, 3.0f, 32.0f};

    EXPECT_EQ(neon::FormatField(layers), "1 3 32");
    EXPECT_TRUE(neon::Same(Read("1, 3 32", FieldKind::Layers), layers));
    EXPECT_TRUE(neon::Same(Read("", FieldKind::Layers), FieldValue{std::vector<float>{}}));
  }
} // namespace
