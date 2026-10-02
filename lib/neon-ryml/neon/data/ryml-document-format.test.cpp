#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ryml-document-format.hpp"

namespace
{
  using neon::DataValue;
  using neon::DocumentFormat;
  using neon::RYML_DocumentFormat;
  using ::testing::HasSubstr;
  using ::testing::SizeIs;
  using ::testing::StartsWith;

  class RymlDocumentFormatTest : public ::testing::Test
  {
  protected:
    RYML_DocumentFormat _yaml;

    // the tests see what the engine sees
    DocumentFormat &_format = _yaml;

    DataValue Read(const std::string &text)
    {
      DataValue document;
      std::string error;
      EXPECT_TRUE(_format.Read("file.yml", text, document, error)) << error;
      return document;
    }

    std::string ErrorOf(const std::string &text)
    {
      DataValue document;
      std::string error;
      EXPECT_FALSE(_format.Read("file.yml", text, document, error));
      return error;
    }

    static std::string TextOf(const DataValue &value)
    {
      std::string text;
      EXPECT_TRUE(value.GetText(text));
      return text;
    }

    static double NumberOf(const DataValue &value)
    {
      double number = 0.0;
      EXPECT_TRUE(value.GetNumber(number));
      return number;
    }
  };

  bool Same(const DataValue &left, const DataValue &right)
  {
    if (left.GetKind() != right.GetKind()) { return false; }

    switch (left.GetKind())
    {
      case DataValue::Kind::Empty: return true;
      case DataValue::Kind::Bool:
      {
        bool a = false, b = false;
        (void) left.GetBool(a);
        (void) right.GetBool(b);
        return a == b;
      }
      case DataValue::Kind::Number:
      {
        double a = 0.0, b = 0.0;
        (void) left.GetNumber(a);
        (void) right.GetNumber(b);
        if (left.IsSinglePrecision() || right.IsSinglePrecision())
        {
          return static_cast<float>(a) == static_cast<float>(b);
        }
        return a == b;
      }
      case DataValue::Kind::Text:
      {
        std::string a, b;
        (void) left.GetText(a);
        (void) right.GetText(b);
        return a == b;
      }
      case DataValue::Kind::List:
      {
        if (left.GetItems().size() != right.GetItems().size()) { return false; }
        for (std::size_t i = 0; i < left.GetItems().size(); i++)
        {
          if (!Same(left.GetItems()[i], right.GetItems()[i])) { return false; }
        }
        return true;
      }
      case DataValue::Kind::Map:
      {
        if (left.GetEntries().size() != right.GetEntries().size()) { return false; }
        for (std::size_t i = 0; i < left.GetEntries().size(); i++)
        {
          if (left.GetEntries()[i].first != right.GetEntries()[i].first) { return false; }
          if (!Same(left.GetEntries()[i].second, right.GetEntries()[i].second)) { return false; }
        }
        return true;
      }
    }
    return false;
  }

  // reading: what a value is

  TEST_F(RymlDocumentFormatTest, PlainTextIsText)
  {
    EXPECT_EQ(TextOf(*Read("name: demo\n").Find("name")), "demo");
  }

  TEST_F(RymlDocumentFormatTest, AWholeNumberIsANumber)
  {
    EXPECT_EQ(NumberOf(*Read("version: 1\n").Find("version")), 1.0);
  }

  TEST_F(RymlDocumentFormatTest, ADecimalIsANumber)
  {
    EXPECT_EQ(NumberOf(*Read("ratio: 0.5\n").Find("ratio")), 0.5);
  }

  TEST_F(RymlDocumentFormatTest, ANegativeNumberWithAnExponentIsANumber)
  {
    EXPECT_EQ(NumberOf(*Read("value: -3e2\n").Find("value")), -300.0);
  }

  TEST_F(RymlDocumentFormatTest, ANumberWithFIsASingleNumber)
  {
    const DataValue document = Read("scale: 0.5f\nsize: [1.0f, 2.5f]\n");

    const DataValue &scale = *document.Find("scale");
    EXPECT_EQ(NumberOf(scale), 0.5);
    EXPECT_TRUE(scale.IsSinglePrecision());
    EXPECT_FALSE(scale.IsPrecise());
    EXPECT_EQ(NumberOf(document.Find("size")->GetItems()[1]), 2.5);
  }

  TEST_F(RymlDocumentFormatTest, ANumberWithDIsAPreciseNumber)
  {
    const DataValue document = Read("fade: 2.0d\nodometer: -123456.789012345d\n");

    const DataValue &fade = *document.Find("fade");
    EXPECT_EQ(NumberOf(fade), 2.0);
    EXPECT_TRUE(fade.IsPrecise());
    EXPECT_FALSE(fade.IsSinglePrecision());
    EXPECT_EQ(NumberOf(*document.Find("odometer")), -123456.789012345);
  }

  TEST_F(RymlDocumentFormatTest, ASuffixOnWhatIsNoNumberLeavesText)
  {
    const DataValue document = Read("a: f\nb: d\nc: 1.2.3f\ne: 1e5d\nw: 2d\nv: 2f\n");

    EXPECT_EQ(TextOf(*document.Find("a")), "f");
    EXPECT_EQ(TextOf(*document.Find("b")), "d");
    EXPECT_EQ(TextOf(*document.Find("c")), "1.2.3f");

    // an exponent is a number with a fraction, so the suffix counts
    EXPECT_EQ(NumberOf(*document.Find("e")), 100000.0);
    EXPECT_TRUE(document.Find("e")->IsPrecise());

    // a whole number takes no suffix: the precision is for fractions
    EXPECT_EQ(TextOf(*document.Find("w")), "2d");
    EXPECT_EQ(TextOf(*document.Find("v")), "2f");
  }

  TEST_F(RymlDocumentFormatTest, HexadecimalAndOctalAreText)
  {
    // numbers are written as people write them; the computer's forms are
    // text, for a field that wants them as such
    const DataValue document = Read("a: 0x1d\nb: 0XFF\nc: -0x10\nd: 0o17\ne: 1_000\n");

    EXPECT_EQ(TextOf(*document.Find("a")), "0x1d");
    EXPECT_EQ(TextOf(*document.Find("b")), "0XFF");
    EXPECT_EQ(TextOf(*document.Find("c")), "-0x10");
    EXPECT_EQ(TextOf(*document.Find("d")), "0o17");
    EXPECT_EQ(TextOf(*document.Find("e")), "1_000");
  }

  TEST_F(RymlDocumentFormatTest, WritesNumbersWithoutASuffix)
  {
    auto map = DataValue::Map();
    map.Set("fade", DataValue::PreciseNumber(2.0));
    map.Set("scale", DataValue::Number(0.5f));

    EXPECT_EQ(_format.Write(map), "fade: 2\nscale: 0.5\n");
  }

  TEST_F(RymlDocumentFormatTest, TrueAndFalseAreBools)
  {
    const auto document = Read("on: true\noff: false\n");

    bool on = false, off = true;
    EXPECT_TRUE(document.Find("on")->GetBool(on));
    EXPECT_TRUE(document.Find("off")->GetBool(off));
    EXPECT_TRUE(on);
    EXPECT_FALSE(off);
  }

  TEST_F(RymlDocumentFormatTest, NothingAndATildeAndNullAreEmpty)
  {
    const auto document = Read("nothing:\ntilde: ~\nnull_word: null\n");

    EXPECT_TRUE(document.Find("nothing")->IsEmpty());
    EXPECT_TRUE(document.Find("tilde")->IsEmpty());
    EXPECT_TRUE(document.Find("null_word")->IsEmpty());
  }

  TEST_F(RymlDocumentFormatTest, ANumberInQuotesIsText)
  {
    EXPECT_EQ(TextOf(*Read("value: \"12\"\n").Find("value")), "12");
  }

  TEST_F(RymlDocumentFormatTest, TrueInQuotesIsText)
  {
    EXPECT_EQ(TextOf(*Read("value: 'true'\n").Find("value")), "true");
  }

  TEST_F(RymlDocumentFormatTest, YesAndNoAreTextAndNotBools)
  {
    const auto document = Read("a: yes\nb: no\n");

    EXPECT_EQ(TextOf(*document.Find("a")), "yes");
    EXPECT_EQ(TextOf(*document.Find("b")), "no");
  }

  TEST_F(RymlDocumentFormatTest, AVirtualPathNeedsNoQuotes)
  {
    EXPECT_EQ(TextOf(*Read("path: assets://models/bear.obj\n").Find("path")), "assets://models/bear.obj");
  }

  TEST_F(RymlDocumentFormatTest, AListOnOneLineIsRead)
  {
    const auto document = Read("list: [1, 2.5, 3]\n");

    ASSERT_THAT(document.Find("list")->GetItems(), SizeIs(3));
    EXPECT_EQ(NumberOf(document.Find("list")->GetItems()[1]), 2.5);
  }

  TEST_F(RymlDocumentFormatTest, AListOfMapsIsRead)
  {
    const auto document = Read("items:\n  - name: a\n    size: 2\n  - name: b\n");

    ASSERT_THAT(document.Find("items")->GetItems(), SizeIs(2));
    EXPECT_EQ(TextOf(*document.Find("items")->GetItems()[1].Find("name")), "b");
  }

  TEST_F(RymlDocumentFormatTest, EmptyContainersKeepTheirKind)
  {
    const auto document = Read("map: {}\nlist: []\n");

    EXPECT_TRUE(document.Find("map")->IsMap());
    EXPECT_TRUE(document.Find("list")->IsList());
  }

  TEST_F(RymlDocumentFormatTest, KeepsTheOrderNamesWereWrittenIn)
  {
    const auto document = Read("zebra: 1\napple: 2\nmango: 3\n");

    ASSERT_THAT(document.GetEntries(), SizeIs(3));
    EXPECT_EQ(document.GetEntries()[0].first, "zebra");
    EXPECT_EQ(document.GetEntries()[2].first, "mango");
  }

  TEST_F(RymlDocumentFormatTest, CommentsAreLeftOut)
  {
    const auto document = Read("# at the top\nname: demo # at the end\n");

    ASSERT_THAT(document.GetEntries(), SizeIs(1));
    EXPECT_EQ(TextOf(*document.Find("name")), "demo");
  }

  TEST_F(RymlDocumentFormatTest, AValueKnowsTheLineItWasReadFrom)
  {
    const auto document = Read("# a comment\nname: demo\nitems:\n  - name: a\n  - name: b\n");

    EXPECT_EQ(document.Find("name")->GetLine(), 2u);
    EXPECT_EQ(document.Find("items")->GetItems()[1].Find("name")->GetLine(), 5u);
  }

  TEST_F(RymlDocumentFormatTest, AnEmptyTextIsAnEmptyDocument)
  {
    EXPECT_TRUE(Read("").IsEmpty());
  }

  // reading: what is refused

  TEST_F(RymlDocumentFormatTest, SaysWhichFileAndLineWhenABracketIsNotClosed)
  {
    EXPECT_THAT(ErrorOf("a: [1, 2\nb: 3\n"), StartsWith("file.yml:"));
  }

  TEST_F(RymlDocumentFormatTest, SaysTheReasonOnOneLine)
  {
    EXPECT_THAT(ErrorOf("a:\n  b: 1\n c: 2\n"), ::testing::Not(HasSubstr("\n")));
  }

  TEST_F(RymlDocumentFormatTest, RefusesANameThatIsWrittenTwice)
  {
    EXPECT_EQ(ErrorOf("a: 1\na: 2\n"), "file.yml:2: 'a' is written twice");
  }

  TEST_F(RymlDocumentFormatTest, RefusesAnchorsAndAliases)
  {
    EXPECT_EQ(ErrorOf("a: &x 1\nb: *x\n"), "file.yml:1: anchors and aliases are not supported");
  }

  TEST_F(RymlDocumentFormatTest, RefusesTags)
  {
    EXPECT_EQ(ErrorOf("a: !!str 1\n"), "file.yml:1: tags are not supported");
  }

  TEST_F(RymlDocumentFormatTest, RefusesSeveralDocumentsInOneText)
  {
    EXPECT_EQ(ErrorOf("a: 1\n---\nb: 2\n"), "file.yml:1: several documents in one text are not supported");
  }

  TEST_F(RymlDocumentFormatTest, ReadsAgainAfterATextWasRefused)
  {
    (void) ErrorOf("a: [1, 2\n");

    EXPECT_EQ(NumberOf(*Read("a: 1\n").Find("a")), 1.0);
  }

  // writing

  TEST_F(RymlDocumentFormatTest, WritesAListOfNumbersOnOneLine)
  {
    auto document = DataValue::Map();
    auto &position = document.Set("position", DataValue::List());
    position.Add(DataValue::Number(1.2f));
    position.Add(DataValue::Number(-0.55f));
    position.Add(DataValue::Number(100.0f));

    EXPECT_EQ(_format.Write(document), "position: [1.2, -0.55, 100]\n");
  }

  TEST_F(RymlDocumentFormatTest, WritesANumberThatCameFromAFloatWithTheDigitsItNeeds)
  {
    auto document = DataValue::Map();
    document.Set("single", DataValue::Number(0.31f));
    document.Set("double", DataValue::Number(0.1));

    EXPECT_EQ(_format.Write(document), "single: 0.31\ndouble: 0.1\n");
  }

  TEST_F(RymlDocumentFormatTest, WritesAListOfTextsAsABlock)
  {
    auto document = DataValue::Map();
    auto &textures = document.Set("textures", DataValue::List());
    textures.Add(DataValue::Text("assets://a.png"));
    textures.Add(DataValue::Text("assets://b.png"));

    EXPECT_EQ(_format.Write(document), "textures:\n  - assets://a.png\n  - assets://b.png\n");
  }

  TEST_F(RymlDocumentFormatTest, WritesEmptyContainersAndNothing)
  {
    auto document = DataValue::Map();
    document.Set("map", DataValue::Map());
    document.Set("list", DataValue::List());
    document.Set("nothing", DataValue{});

    EXPECT_EQ(_format.Write(document), "map: {}\nlist: []\nnothing: null\n");
  }

  TEST_F(RymlDocumentFormatTest, WritesNestedMapsWithTwoSpacesForALevel)
  {
    auto document = DataValue::Map();
    auto &outer = document.Set("outer", DataValue::Map());
    outer.Set("inner", DataValue::Map()).Set("value", DataValue::Bool(true));

    EXPECT_EQ(_format.Write(document), "outer:\n  inner:\n    value: true\n");
  }

  TEST_F(RymlDocumentFormatTest, SetsTheMapsOfAListBelowTheTopApartWithBlankLines)
  {
    auto document = DataValue::Map();
    document.Set("scene", DataValue::Text("demo"));
    auto &entities = document.Set("entities", DataValue::List());
    entities.Add(DataValue::Map()).Set("name", DataValue::Text("a"));
    entities.Add(DataValue::Map()).Set("name", DataValue::Text("b"));

    EXPECT_EQ(_format.Write(document), "scene: demo\n\nentities:\n  - name: a\n\n  - name: b\n");
  }

  class RymlDocumentFormatTextTest
    : public RymlDocumentFormatTest,
      public ::testing::WithParamInterface<std::string>
  {
  };

  // every text has to come back as the text it was, whatever it looks like
  TEST_P(RymlDocumentFormatTextTest, ATextThatIsWrittenReadsBackAsTheSameText)
  {
    auto document = DataValue::Map();
    document.Set("text", DataValue::Text(GetParam()));
    document.Set("list", DataValue::List()).Add(DataValue::Text(GetParam()));

    const auto written = _format.Write(document);
    const auto back = Read(written);

    ASSERT_NE(back.Find("text"), nullptr) << written;
    EXPECT_EQ(TextOf(*back.Find("text")), GetParam()) << written;
    ASSERT_THAT(back.Find("list")->GetItems(), SizeIs(1)) << written;
    EXPECT_EQ(TextOf(back.Find("list")->GetItems()[0]), GetParam()) << written;
  }

  INSTANTIATE_TEST_SUITE_P(
    Texts,
    RymlDocumentFormatTextTest,
    ::testing::Values(
      "",
      "plain text",
      "true",
      "false",
      "null",
      "~",
      "12",
      "-3.5",
      "1e5",
      "a: b",
      "ends with a colon:",
      " leading space",
      "trailing space ",
      "has # a hash",
      "#starts with a hash",
      "-dash",
      "- dash and space",
      "quote\"inside",
      "single'inside",
      "'quoted'",
      "line\nbreak",
      "tab\there",
      "back\\slash",
      "[list]",
      "{map}",
      "a, b",
      "*star",
      "&and",
      "!bang",
      "|pipe",
      ">more",
      "%percent",
      "@at",
      "`tick",
      "?question",
      "assets://models/bear.obj",
      "C:\\folder\\file",
      "ünïcödé",
      "yes",
      "no"));

  TEST_F(RymlDocumentFormatTest, ANameThatNeedsQuotesReadsBackAsTheSameName)
  {
    auto document = DataValue::Map();
    document.Set("weird name: x", DataValue::Number(3));
    document.Set("", DataValue::Number(4));

    const auto back = Read(_format.Write(document));

    ASSERT_NE(back.Find("weird name: x"), nullptr);
    ASSERT_NE(back.Find(""), nullptr);
  }

  TEST_F(RymlDocumentFormatTest, ADocumentThatIsWrittenReadsBackAsTheSame)
  {
    auto document = DataValue::Map();
    document.Set("scene", DataValue::Text("demo"));
    document.Set("version", DataValue::Number(1));

    auto &entity = document.Set("entities", DataValue::List()).Add(DataValue::Map());
    entity.Set("name", DataValue::Text("bear"));
    auto &components = entity.Set("components", DataValue::Map());
    auto &position = components.Set("Transform", DataValue::Map()).Set("position", DataValue::List());
    position.Add(DataValue::Number(1.2f));
    position.Add(DataValue::Number(-0.55f));
    position.Add(DataValue::Number(100.0f));
    components.Set("Spectator", DataValue::Map());
    components.Set("flag", DataValue::Bool(false));
    components.Set("nothing", DataValue{});
    components.Set("nested", DataValue::List()).Add(DataValue::List()).Add(DataValue::Text("x"));
    entity.Set("children", DataValue::List()).Add(DataValue::Map()).Set("name", DataValue::Text("cub"));

    const auto written = _format.Write(document);

    EXPECT_TRUE(Same(document, Read(written))) << written;
  }

  TEST_F(RymlDocumentFormatTest, WhatIsReadAndWrittenAndReadAgainIsTheSame)
  {
    const auto first = Read(
      "scene: demo\nentities:\n  - name: a\n    components:\n      Transform:\n"
      "        position: [0, -0.55, 0]\n        scale: 0.2\n      Spectator: {}\n");

    EXPECT_TRUE(Same(first, Read(_format.Write(first))));
  }
}
