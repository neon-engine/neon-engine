#include "ui-style.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Reading properties from values, without a file. What a file of YAML turns
// into is covered by the functional tests of the user interface, with every
// message for a property that is wrong.

namespace
{
  using neon::DataReader;
  using neon::DataValue;
  using neon::LayoutLength;
  using neon::ReadUiStyle;
  using neon::UiStyle;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  class UiStyleTest : public ::testing::Test
  {
  protected:
    DataValue _map = DataValue::Map();
    std::vector<std::string> _errors;

    void Write(const std::string &name, DataValue value, const std::size_t line = 0)
    {
      value.SetLine(line);
      _map.Set(name, value);
    }

    UiStyle Read(UiStyle style = {})
    {
      const DataReader reader(_map, "test.ui.yml", "panel 'box'", _errors);
      ReadUiStyle(reader, style);
      reader.Finish();
      return style;
    }

    static DataValue Numbers(const std::initializer_list<double> numbers)
    {
      auto list = DataValue::List();
      for (const double number : numbers) { list.Add(DataValue::Number(number)); }
      return list;
    }
  };

  TEST_F(UiStyleTest, LeavesEverythingAsItIsWhenNothingIsWritten)
  {
    UiStyle before;
    before.font_size = 24;
    before.opacity = 0.5f;
    before.layout.width = LayoutLength::Pixels(100);
    before.layout.flex_grow = 2;
    before.background_color = {0.1f, 0.2f, 0.3f, 0.4f};

    const UiStyle after = Read(before);

    EXPECT_FLOAT_EQ(after.font_size, 24);
    EXPECT_FLOAT_EQ(after.opacity, 0.5f);
    EXPECT_EQ(after.layout, before.layout);
    EXPECT_FLOAT_EQ(after.background_color.g, 0.2f);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(UiStyleTest, ChangesWhatIsWrittenAndNothingElse)
  {
    UiStyle before;
    before.font_size = 24;
    before.layout.height = LayoutLength::Pixels(50);

    Write("width", DataValue::Number(200));

    const UiStyle after = Read(before);

    EXPECT_EQ(after.layout.width, LayoutLength::Pixels(200));
    EXPECT_EQ(after.layout.height, LayoutLength::Pixels(50));
    EXPECT_FLOAT_EQ(after.font_size, 24);
  }

  TEST_F(UiStyleTest, LeavesAPropertyAsItIsWhenItCannotBeRead)
  {
    UiStyle before;
    before.layout.width = LayoutLength::Pixels(100);
    before.color = {0.1f, 0.2f, 0.3f, 1.0f};

    Write("width", DataValue::Text("wide"), 4);
    Write("color", DataValue::Text("blue"), 5);

    const UiStyle after = Read(before);

    EXPECT_EQ(after.layout.width, LayoutLength::Pixels(100));
    EXPECT_FLOAT_EQ(after.color.g, 0.2f);
    EXPECT_EQ(_errors.size(), 2u);
  }

  TEST_F(UiStyleTest, ReadsALengthAsANumberAndAsText)
  {
    Write("width", DataValue::Number(200));
    Write("height", DataValue::Text("50%"));
    Write("min_width", DataValue::Text("12.5px"));
    Write("flex_basis", DataValue::Text("auto"));

    const UiStyle style = Read();

    EXPECT_EQ(style.layout.width, LayoutLength::Pixels(200));
    EXPECT_EQ(style.layout.height, LayoutLength::Percent(50));
    EXPECT_EQ(style.layout.min_width, LayoutLength::Pixels(12.5f));
    EXPECT_EQ(style.layout.flex_basis, LayoutLength::Auto());
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(UiStyleTest, ReadsSeveralValuesAsAListAndAsText)
  {
    Write("margin", Numbers({1, 2, 3, 4}));
    Write("padding", DataValue::Text("5px 10%"));

    const UiStyle style = Read();

    EXPECT_EQ(style.layout.margin.top, LayoutLength::Pixels(1));
    EXPECT_EQ(style.layout.margin.right, LayoutLength::Pixels(2));
    EXPECT_EQ(style.layout.margin.bottom, LayoutLength::Pixels(3));
    EXPECT_EQ(style.layout.margin.left, LayoutLength::Pixels(4));

    EXPECT_EQ(style.layout.padding.top, LayoutLength::Pixels(5));
    EXPECT_EQ(style.layout.padding.right, LayoutLength::Percent(10));
    EXPECT_EQ(style.layout.padding.bottom, LayoutLength::Pixels(5));
    EXPECT_EQ(style.layout.padding.left, LayoutLength::Percent(10));
  }

  TEST_F(UiStyleTest, AMarginMayBeBelowZeroAndPaddingMayNot)
  {
    Write("margin", DataValue::Number(-8), 2);
    Write("padding", DataValue::Number(-8), 3);

    const UiStyle style = Read();

    EXPECT_EQ(style.layout.margin.left, LayoutLength::Pixels(-8));
    EXPECT_EQ(style.layout.padding.left, LayoutLength::Pixels(0));
    EXPECT_EQ(_errors.size(), 1u);
  }

  TEST_F(UiStyleTest, NamesTheFileTheLineAndWhatWasExpected)
  {
    Write("width", DataValue::Text("wide"), 12);

    (void) Read();

    EXPECT_THAT(_errors, ElementsAre(
                  "test.ui.yml:12: 'width' of panel 'box' is 'wide', where a number of pixels, a percentage "
                  "such as 50%, or auto was expected"));
  }

  TEST_F(UiStyleTest, ReportsANameThatIsNoProperty)
  {
    Write("colour", DataValue::Text("#ffffff"), 3);

    (void) Read();

    ASSERT_EQ(_errors.size(), 1u);
    EXPECT_TRUE(_errors[0].starts_with("test.ui.yml:3: 'colour' is not known to panel 'box'. Known are: display,"))
      << _errors[0];
  }

  TEST_F(UiStyleTest, ReportsEveryPropertyThatIsWrong)
  {
    Write("width", DataValue::Text("wide"), 1);
    Write("opacity", DataValue::Number(2), 2);
    Write("display", DataValue::Text("block"), 3);
    Write("border", DataValue::Text("thick"), 4);

    (void) Read();

    EXPECT_EQ(_errors.size(), 4u);
  }

  TEST_F(UiStyleTest, AColourCarriesItsAlpha)
  {
    Write("background_color", DataValue::Text("#10141880"));

    const UiStyle style = Read();

    EXPECT_NEAR(style.background_color.r, 16.0f / 255.0f, 0.0001f);
    EXPECT_NEAR(style.background_color.a, 128.0f / 255.0f, 0.0001f);
  }

  TEST_F(UiStyleTest, ABorderAndAnOutlineHaveTheColourOfTheTextUntilTheyAreGivenOne)
  {
    Write("color", DataValue::Text("#ff0000"));

    UiStyle style = Read();

    EXPECT_FALSE(style.border_color.has_value());
    EXPECT_FLOAT_EQ(style.BorderColor().r, 1);
    EXPECT_FLOAT_EQ(style.BorderColor().g, 0);
    EXPECT_FLOAT_EQ(style.OutlineColor().r, 1);

    Write("border_color", DataValue::Text("#00ff00"));
    Write("outline_color", DataValue::Text("#0000ff"));
    style = Read();

    EXPECT_FLOAT_EQ(style.BorderColor().g, 1);
    EXPECT_FLOAT_EQ(style.OutlineColor().b, 1);
  }

  TEST_F(UiStyleTest, TheHeightOfALineIsInTheUnitsOfTheFont)
  {
    UiStyle style;
    style.font_size = 20;
    EXPECT_FLOAT_EQ(style.LineHeight(), 0);

    Write("line_height", DataValue::Number(1.5));
    EXPECT_FLOAT_EQ(Read(style).LineHeight(), 30);

    Write("line_height", DataValue::Text("28px"));
    EXPECT_FLOAT_EQ(Read(style).LineHeight(), 28);

    Write("line_height", DataValue::Text("normal"));
    EXPECT_FLOAT_EQ(Read(style).LineHeight(), 0);
  }

  TEST_F(UiStyleTest, ANameWithNothingBehindItCountsAsNotWritten)
  {
    UiStyle before;
    before.layout.width = LayoutLength::Pixels(100);

    Write("width", DataValue{});

    EXPECT_EQ(Read(before).layout.width, LayoutLength::Pixels(100));
    EXPECT_THAT(_errors, IsEmpty());
  }
}
