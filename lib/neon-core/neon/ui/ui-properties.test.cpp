#include "ui-properties.hpp"

#include <set>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/data-reader.hpp>

namespace
{
  using neon::Color;
  using neon::DataReader;
  using neon::DataValue;
  using neon::LayoutLength;
  using neon::UiProperties;
  using neon::UiProperty;
  using neon::UiPropertyValue;
  using neon::UiStyle;
  using neon::UiValueKind;
  using ::testing::Contains;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;
  using ::testing::UnorderedElementsAre;

  const UiProperty &Property(const std::string &name)
  {
    const UiProperty *property = UiProperties::Get().Find(name);
    if (property == nullptr) { throw std::runtime_error("There is no property '" + name + "'"); }
    return *property;
  }

  /// A style with one property read into it, as a file writes it.
  UiStyle Written(const std::string &yaml_name, const DataValue &value, UiStyle style = {})
  {
    DataValue map = DataValue::Map();
    map.Set(yaml_name, value);

    std::vector<std::string> errors;
    const DataReader reader(map, "test", "a style", errors);
    neon::ReadUiStyle(reader, style);
    reader.Finish();

    EXPECT_THAT(errors, IsEmpty()) << yaml_name;
    return style;
  }

  UiStyle Written(const std::string &yaml_name, const std::string &text, const UiStyle &style = {})
  {
    return Written(yaml_name, DataValue::Text(text), style);
  }

  std::string Formatted(const std::string &name, const UiStyle &style)
  {
    const UiProperty &property = Property(name);
    return property.Format(property.get(style));
  }

  UiPropertyValue Number(const float number)
  {
    UiPropertyValue value;
    value.kind = UiValueKind::Number;
    value.number = number;
    return value;
  }

  UiPropertyValue Length(const LayoutLength &length)
  {
    UiPropertyValue value;
    value.kind = UiValueKind::Length;
    value.length = length;
    return value;
  }

  UiPropertyValue ColorValue(const float r, const float g, const float b, const float a)
  {
    UiPropertyValue value;
    value.kind = UiValueKind::Color;
    value.color = {r, g, b, a};
    value.flag = true;
    return value;
  }

  UiPropertyValue Keyword(const int keyword)
  {
    UiPropertyValue value;
    value.kind = UiValueKind::Keyword;
    value.keyword = keyword;
    return value;
  }

  // names

  TEST(UiPropertiesTest, FindsAPropertyByEitherWayOfWritingItsName)
  {
    const UiProperties &properties = UiProperties::Get();

    ASSERT_NE(properties.Find("background-color"), nullptr);
    EXPECT_EQ(properties.Find("background-color"), properties.Find("background_color"));
    EXPECT_EQ(properties.Find("background-color")->name, "background-color");
    EXPECT_EQ(properties.Find("background-color")->yaml_name, "background_color");

    EXPECT_EQ(properties.Find("colour"), nullptr);
    EXPECT_EQ(properties.Find(""), nullptr);
  }

  TEST(UiPropertiesTest, WritesANameEitherWay)
  {
    EXPECT_EQ(UiProperties::ToCssName("background_color"), "background-color");
    EXPECT_EQ(UiProperties::ToCssName("background-color"), "background-color");
    EXPECT_EQ(UiProperties::ToYamlName("background-color"), "background_color");
    EXPECT_EQ(UiProperties::ToYamlName("opacity"), "opacity");

    // the name of a custom property is what it is
    EXPECT_EQ(UiProperties::ToCssName("--my_gap"), "--my_gap");
    EXPECT_EQ(UiProperties::ToYamlName("--my-gap"), "--my-gap");
  }

  TEST(UiPropertiesTest, KnowsEveryNameOnce)
  {
    std::set<std::string> names;
    for (const auto &property : UiProperties::Get().GetAll())
    {
      EXPECT_TRUE(names.insert(property.name).second) << property.name << " is there twice";
      EXPECT_FALSE(property.description.empty()) << property.name << " says nothing about itself";
    }
  }

  TEST(UiPropertiesTest, AsksWhatIsReadForTheNamesThatAreKnown)
  {
    const auto &names = UiProperties::GetStyleNames();

    EXPECT_THAT(names, Contains("width"));
    EXPECT_THAT(names, Contains("background_color"));
    EXPECT_THAT(names, Contains("border"));
    EXPECT_THAT(names, Contains("overflow_x"));
    EXPECT_THAT(names, Contains("animation_play_state"));

    EXPECT_TRUE(UiProperties::IsStyleName("flex_direction"));
    EXPECT_FALSE(UiProperties::IsStyleName("flex-direction"));
    EXPECT_FALSE(UiProperties::IsStyleName("colour"));
    EXPECT_FALSE(UiProperties::IsStyleName("text"));
    EXPECT_FALSE(UiProperties::IsStyleName("type"));
  }

  // The table and what reads a style are kept in line by these two. A
  // property that is read and is missing in the table is written in files
  // and in style sheets, and is neither inherited nor moved over time.
  TEST(UiPropertiesTest, KnowsEveryPropertyThatIsRead)
  {
    for (const auto &name : UiProperties::GetStyleNames())
    {
      EXPECT_NE(UiProperties::Get().Find(name), nullptr)
        << "'" << name << "' is read by ReadUiStyle() and is missing in the table of ui-properties.cpp";
    }
  }

  TEST(UiPropertiesTest, KnowsNoPropertyThatIsNotRead)
  {
    for (const auto &property : UiProperties::Get().GetAll())
    {
      EXPECT_TRUE(UiProperties::IsStyleName(property.yaml_name))
        << "'" << property.yaml_name << "' is in the table and is not read by ReadUiStyle()";
    }
  }

  TEST(UiPropertiesTest, NamesWhatIsNearToANameThatIsNotKnown)
  {
    EXPECT_THAT(UiProperties::GetSimilarNames("colour"), Contains("color"));
    EXPECT_THAT(UiProperties::GetSimilarNames("background-colour"), ElementsAre("background-color"));
    EXPECT_THAT(UiProperties::GetSimilarNames("widht"), Contains("width"));
    EXPECT_THAT(UiProperties::GetSimilarNames("font_sise"), Contains("font-size"));
    EXPECT_THAT(UiProperties::GetSimilarNames("something-else-entirely"), IsEmpty());
  }

  // what is inherited

  TEST(UiPropertiesTest, InheritsWhatCssInherits)
  {
    std::vector<std::string> inherited;
    for (const auto &property : UiProperties::Get().GetAll())
    {
      if (property.is_inherited) { inherited.push_back(property.name); }
    }

    EXPECT_THAT(
      inherited,
      UnorderedElementsAre(
        "color", "font-family", "font-size", "font-weight", "line-height", "text-align", "visibility",
        "cursor", "caret-color", "scrollbar-color", "letter-spacing", "word-spacing", "text-transform",
        "text-shadow", "white-space", "font-style", "text-stroke-width", "text-stroke-color", "direction",
        "image-rendering"));
  }

  TEST(UiPropertiesTest, TakesEveryInheritedPropertyFromOneStyleToAnother)
  {
    UiStyle parent;
    parent.color = {1.0f, 0.0f, 0.0f, 1.0f};
    parent.font_family = "Title";
    parent.font_size = 32.0f;
    parent.font_weight = 700;
    parent.line_height = 1.5f;
    parent.line_height_is_multiple = true;
    parent.text_align = neon::TextAlign::Center;
    parent.visibility = neon::UiVisibility::Hidden;
    parent.cursor = neon::UiCursor::Pointer;
    parent.caret_color = Color{0.0f, 1.0f, 0.0f, 1.0f};

    // and what is not inherited
    parent.background_color = {0.0f, 0.0f, 1.0f, 1.0f};
    parent.opacity = 0.5f;
    parent.layout.width = LayoutLength::Pixels(100.0f);

    UiStyle child;
    for (const auto &property : UiProperties::Get().GetAll())
    {
      if (property.is_inherited) { property.Copy(parent, child); }
    }

    EXPECT_FLOAT_EQ(child.color.r, 1.0f);
    EXPECT_FLOAT_EQ(child.color.g, 0.0f);
    EXPECT_EQ(child.font_family, "Title");
    EXPECT_FLOAT_EQ(child.font_size, 32.0f);
    EXPECT_EQ(child.font_weight, 700);
    EXPECT_FLOAT_EQ(child.line_height, 1.5f);
    EXPECT_TRUE(child.line_height_is_multiple);
    EXPECT_EQ(child.text_align, neon::TextAlign::Center);
    EXPECT_EQ(child.visibility, neon::UiVisibility::Hidden);
    EXPECT_EQ(child.cursor, neon::UiCursor::Pointer);
    ASSERT_TRUE(child.caret_color.has_value());
    EXPECT_FLOAT_EQ(child.caret_color->g, 1.0f);

    EXPECT_FLOAT_EQ(child.background_color.a, 0.0f);
    EXPECT_FLOAT_EQ(child.opacity, 1.0f);
    EXPECT_TRUE(child.layout.width.IsAuto());
  }

  TEST(UiPropertiesTest, KeepsAColorThatFollowsTheTextFollowingIt)
  {
    UiStyle from;
    from.color = {1.0f, 0.0f, 0.0f, 1.0f};

    UiStyle to;
    to.border_color = Color{0.0f, 0.0f, 1.0f, 1.0f};
    to.color = {0.0f, 1.0f, 0.0f, 1.0f};

    Property("border-color").Copy(from, to);

    EXPECT_FALSE(to.border_color.has_value());
    EXPECT_FLOAT_EQ(to.BorderColor().g, 1.0f);
  }

  TEST(UiPropertiesTest, TakesAListFromOneStyleToAnother)
  {
    UiStyle from;
    from.transitions.durations = {0.2f, 1.0f};
    from.transitions.properties = {"opacity", "color"};
    from.animations.names = {"fade"};
    from.scrollbar_thumb_color = Color{1.0f, 0.0f, 0.0f, 1.0f};
    from.scrollbar_track_color = Color{0.0f, 0.0f, 0.0f, 0.5f};

    UiStyle to;
    Property("transition-duration").Copy(from, to);
    Property("animation-name").Copy(from, to);
    Property("scrollbar-color").Copy(from, to);

    EXPECT_THAT(to.transitions.durations, ElementsAre(0.2f, 1.0f));
    EXPECT_THAT(to.transitions.properties, ElementsAre("all"));
    EXPECT_THAT(to.animations.names, ElementsAre("fade"));
    ASSERT_TRUE(to.scrollbar_thumb_color.has_value());
    EXPECT_FLOAT_EQ(to.scrollbar_thumb_color->r, 1.0f);
  }

  // what a shorthand stands for

  TEST(UiPropertiesTest, KnowsWhatAShorthandStandsFor)
  {
    const UiProperties &properties = UiProperties::Get();

    const auto names = [&properties](const std::string &shorthand)
    {
      std::vector<std::string> longhands;
      for (const UiProperty *each : properties.GetLonghands(Property(shorthand)))
      {
        longhands.push_back(each->name);
      }
      return longhands;
    };

    EXPECT_THAT(names("margin"), ElementsAre("margin-top", "margin-right", "margin-bottom", "margin-left"));
    EXPECT_THAT(names("padding"), ElementsAre("padding-top", "padding-right", "padding-bottom", "padding-left"));
    EXPECT_THAT(names("border"), ElementsAre("border-width", "border-color"));
    EXPECT_THAT(names("flex"), ElementsAre("flex-grow", "flex-shrink", "flex-basis"));
    EXPECT_THAT(names("gap"), ElementsAre("row-gap", "column-gap"));
    EXPECT_THAT(names("overflow"), ElementsAre("overflow-x", "overflow-y"));
    EXPECT_THAT(
      names("transition"),
      ElementsAre(
        "transition-property", "transition-duration", "transition-timing-function", "transition-delay"));
    EXPECT_EQ(names("animation").size(), 8u);

    // what is no shorthand stands for itself
    EXPECT_THAT(names("opacity"), ElementsAre("opacity"));
    EXPECT_TRUE(Property("margin").IsShorthand());
    EXPECT_FALSE(Property("margin-top").IsShorthand());
  }

  // what is read is what the table reaches

  TEST(UiPropertiesTest, ReachesEveryWordOfEveryKeywordAsItIsRead)
  {
    for (const auto &property : UiProperties::Get().GetAll())
    {
      if (property.kind != UiValueKind::Keyword) { continue; }

      ASSERT_FALSE(property.keywords.empty()) << property.name;

      for (std::size_t i = 0; i < property.keywords.size(); i++)
      {
        const UiStyle style = Written(property.yaml_name, property.keywords[i]);

        EXPECT_EQ(property.get(style).keyword, static_cast<int>(i))
          << property.name << ": " << property.keywords[i];
        EXPECT_EQ(property.Format(property.get(style)), property.keywords[i]) << property.name;
      }
    }
  }

  TEST(UiPropertiesTest, ReachesEveryLengthAsItIsRead)
  {
    for (const auto &property : UiProperties::Get().GetAll())
    {
      if (property.kind != UiValueKind::Length) { continue; }

      const UiStyle style = Written(property.yaml_name, "25%");
      EXPECT_EQ(property.get(style).length, LayoutLength::Percent(25.0f)) << property.name;

      UiStyle changed;
      property.set(changed, Length(LayoutLength::Pixels(7.0f)));
      EXPECT_EQ(property.get(changed).length, LayoutLength::Pixels(7.0f)) << property.name;
    }
  }

  TEST(UiPropertiesTest, ReachesEveryColorAsItIsRead)
  {
    for (const auto &property : UiProperties::Get().GetAll())
    {
      if (property.kind != UiValueKind::Color) { continue; }

      const UiStyle style = Written(property.yaml_name, "#ff8000");
      const UiPropertyValue value = property.get(style);

      EXPECT_FLOAT_EQ(value.color.r, 1.0f) << property.name;
      EXPECT_NEAR(value.color.g, 0.502f, 0.001f) << property.name;
      EXPECT_FLOAT_EQ(value.color.b, 0.0f) << property.name;
      EXPECT_TRUE(value.flag) << property.name;

      UiStyle changed;
      property.set(changed, ColorValue(0.0f, 1.0f, 0.0f, 0.5f));
      EXPECT_FLOAT_EQ(property.get(changed).color.g, 1.0f) << property.name;
      EXPECT_FLOAT_EQ(property.get(changed).color.a, 0.5f) << property.name;
    }
  }

  TEST(UiPropertiesTest, ReachesEveryNumberAsItIsRead)
  {
    for (const auto &property : UiProperties::Get().GetAll())
    {
      if (property.kind != UiValueKind::Number || property.name == "line-height") { continue; }

      const float written = property.name == "opacity" ? 0.5f : 3.0f;
      const UiStyle style = Written(property.yaml_name, DataValue::Number(written));
      EXPECT_FLOAT_EQ(property.get(style).number, written) << property.name;

      UiStyle changed;
      property.set(changed, Number(0.25f));
      EXPECT_FLOAT_EQ(property.get(changed).number, 0.25f) << property.name;
    }
  }

  TEST(UiPropertiesTest, ReachesTheHeightOfALineAsAMultipleAndAsPixels)
  {
    const UiProperty &line_height = Property("line-height");

    const UiStyle multiple = Written("line_height", DataValue::Number(1.5f));
    EXPECT_FLOAT_EQ(line_height.get(multiple).number, 1.5f);
    EXPECT_TRUE(line_height.get(multiple).flag);
    EXPECT_EQ(line_height.Format(line_height.get(multiple)), "1.5");

    const UiStyle pixels = Written("line_height", "24px");
    EXPECT_FLOAT_EQ(line_height.get(pixels).number, 24.0f);
    EXPECT_FALSE(line_height.get(pixels).flag);
    EXPECT_EQ(line_height.Format(line_height.get(pixels)), "24px");

    EXPECT_EQ(line_height.Format(line_height.get(UiStyle{})), "normal");
  }

  // what is written

  TEST(UiPropertiesTest, WritesAValueAsCssWritesIt)
  {
    UiStyle style;
    style.layout.width = LayoutLength::Pixels(120.5f);
    style.layout.height = LayoutLength::Percent(50.0f);
    style.layout.min_width = LayoutLength::Sum(-20.0f, 100.0f);
    style.layout.border = {1.0f, 2.0f, 3.0f, 4.0f};
    style.layout.row_gap = 8.0f;
    style.layout.flex_grow = 2.0f;
    style.background_color = {1.0f, 0.5f, 0.0f, 1.0f};
    style.color = {0.0f, 0.0f, 0.0f, 0.5f};
    style.opacity = 0.25f;
    style.z_index = -3;
    style.font_family = "Inter";
    style.font_size = 18.0f;
    style.font_weight = 700;
    style.background_image = "assets://ui/panel.png";
    style.border_image_slice = {16.0f, 16.0f, 16.0f, 16.0f};

    EXPECT_EQ(Formatted("width", style), "120.5px");
    EXPECT_EQ(Formatted("height", style), "50%");
    EXPECT_EQ(Formatted("min-width", style), "calc(100% + -20px)");
    EXPECT_EQ(Formatted("flex-basis", style), "auto");
    EXPECT_EQ(Formatted("max-width", style), "none");
    EXPECT_EQ(Formatted("margin-top", style), "0px");
    EXPECT_EQ(Formatted("border-width", style), "1px 2px 3px 4px");
    EXPECT_EQ(Formatted("row-gap", style), "8px");
    EXPECT_EQ(Formatted("flex-grow", style), "2");
    EXPECT_EQ(Formatted("background-color", style), "rgb(255, 128, 0)");
    EXPECT_EQ(Formatted("color", style), "rgba(0, 0, 0, 0.5)");
    EXPECT_EQ(Formatted("opacity", style), "0.25");
    EXPECT_EQ(Formatted("z-index", style), "-3");
    EXPECT_EQ(Formatted("font-family", style), "Inter");
    EXPECT_EQ(Formatted("font-size", style), "18px");
    EXPECT_EQ(Formatted("font-weight", style), "700");
    EXPECT_EQ(Formatted("background-image", style), "assets://ui/panel.png");
    EXPECT_EQ(Formatted("border-image-source", style), "none");
    EXPECT_EQ(Formatted("border-image-slice", style), "16 16 16 16");
    EXPECT_EQ(Formatted("display", style), "flex");
    EXPECT_EQ(Formatted("justify-content", style), "flex-start");
  }

  TEST(UiPropertiesTest, WritesAColorThatFollowsTheTextAsTheColorOfTheText)
  {
    UiStyle style;
    style.color = {1.0f, 0.0f, 0.0f, 1.0f};

    EXPECT_EQ(Formatted("border-color", style), "rgb(255, 0, 0)");
    EXPECT_FALSE(Property("border-color").get(style).flag);

    style.border_color = Color{0.0f, 0.0f, 1.0f, 1.0f};
    EXPECT_EQ(Formatted("border-color", style), "rgb(0, 0, 255)");
  }

  TEST(UiPropertiesTest, WritesTheListsOfTransitionsAndAnimations)
  {
    const UiStyle initial;
    EXPECT_EQ(Formatted("transition-property", initial), "all");
    EXPECT_EQ(Formatted("transition-duration", initial), "0s");
    EXPECT_EQ(Formatted("transition-timing-function", initial), "ease");
    EXPECT_EQ(Formatted("animation-name", initial), "none");
    EXPECT_EQ(Formatted("animation-iteration-count", initial), "1");
    EXPECT_EQ(Formatted("animation-direction", initial), "normal");
    EXPECT_EQ(Formatted("animation-fill-mode", initial), "none");
    EXPECT_EQ(Formatted("animation-play-state", initial), "running");
    EXPECT_EQ(Formatted("scrollbar-color", initial), "auto");

    UiStyle style = Written("transition", "opacity 0.2s ease-in 100ms, color 1s steps(2, jump-start)");
    style = Written("animation", "fade 0.5s infinite alternate both paused", style);
    style = Written("scrollbar_color", "#ff0000 #00000080", style);

    EXPECT_EQ(Formatted("transition-property", style), "opacity, color");
    EXPECT_EQ(Formatted("transition-duration", style), "0.2s, 1s");
    EXPECT_EQ(Formatted("transition-delay", style), "0.1s, 0s");
    EXPECT_EQ(Formatted("transition-timing-function", style), "ease-in, steps(2, jump-start)");
    EXPECT_EQ(Formatted("animation-name", style), "fade");
    EXPECT_EQ(Formatted("animation-duration", style), "0.5s");
    EXPECT_EQ(Formatted("animation-iteration-count", style), "infinite");
    EXPECT_EQ(Formatted("animation-direction", style), "alternate");
    EXPECT_EQ(Formatted("animation-fill-mode", style), "both");
    EXPECT_EQ(Formatted("animation-play-state", style), "paused");
    EXPECT_EQ(Formatted("scrollbar-color", style), "rgb(255, 0, 0) rgba(0, 0, 0, 0.502)");
  }

  TEST(UiPropertiesTest, WritesColorsAndLengths)
  {
    EXPECT_EQ(neon::FormatCssColor({1.0f, 1.0f, 1.0f, 1.0f}), "rgb(255, 255, 255)");
    EXPECT_EQ(neon::FormatCssColor({0.0f, 0.0f, 0.0f, 0.0f}), "rgba(0, 0, 0, 0)");
    EXPECT_EQ(neon::FormatCssColor({2.0f, -1.0f, 0.5f, 0.25f}), "rgba(255, 0, 128, 0.25)");

    EXPECT_EQ(neon::FormatCssLength(LayoutLength::Auto()), "auto");
    EXPECT_EQ(neon::FormatCssLength(LayoutLength::Pixels(-4.0f)), "-4px");
    EXPECT_EQ(neon::FormatCssLength(LayoutLength::Percent(33.5f)), "33.5%");
    EXPECT_EQ(neon::FormatCssLength(LayoutLength::Sum(8.0f, 50.0f)), "calc(50% + 8px)");
  }

  // what affects layout

  TEST(UiPropertiesTest, KnowsWhatMovesAndResizes)
  {
    for (const std::string name : {
           "width", "height", "min-width", "max-height", "margin-top", "padding-left", "border-width", "top",
           "display", "position", "box-sizing", "flex-direction", "flex-wrap", "justify-content", "align-items",
           "align-self", "align-content", "flex-grow", "flex-shrink", "flex-basis", "row-gap", "column-gap",
           "font-family", "font-size", "font-weight", "line-height", "overflow-x", "overflow-y",
           "scrollbar-width"
         })
    {
      EXPECT_TRUE(Property(name).affects_layout) << name;
    }

    for (const std::string name : {
           "color", "background-color", "background-image", "border-color", "outline-width", "outline-color",
           "opacity", "z-index", "pointer-events", "accent-color", "visibility", "cursor", "caret-color",
           "text-align", "border-image-source", "scroll-behavior"
         })
    {
      EXPECT_FALSE(Property(name).affects_layout) << name;
    }
  }

  // from one value to another

  TEST(UiInterpolationTest, MovesANumberStraight)
  {
    EXPECT_FLOAT_EQ(neon::InterpolateUiValue(Number(0.0f), Number(1.0f), 0.0f).number, 0.0f);
    EXPECT_FLOAT_EQ(neon::InterpolateUiValue(Number(0.0f), Number(1.0f), 0.25f).number, 0.25f);
    EXPECT_FLOAT_EQ(neon::InterpolateUiValue(Number(10.0f), Number(20.0f), 0.5f).number, 15.0f);
    EXPECT_FLOAT_EQ(neon::InterpolateUiValue(Number(10.0f), Number(-10.0f), 0.75f).number, -5.0f);
    EXPECT_FLOAT_EQ(neon::InterpolateUiValue(Number(0.0f), Number(1.0f), 1.0f).number, 1.0f);
  }

  TEST(UiInterpolationTest, GoesOnPastTheEndsForATimingFunctionThatSwingsOut)
  {
    EXPECT_FLOAT_EQ(neon::InterpolateUiValue(Number(0.0f), Number(100.0f), 1.1f).number, 110.0f);
    EXPECT_FLOAT_EQ(neon::InterpolateUiValue(Number(0.0f), Number(100.0f), -0.1f).number, -10.0f);
  }

  TEST(UiInterpolationTest, MovesAWholeNumberToTheNearestWholeNumber)
  {
    UiPropertyValue from = Number(0.0f);
    from.kind = UiValueKind::Whole;

    UiPropertyValue to = Number(10.0f);
    to.kind = UiValueKind::Whole;

    EXPECT_FLOAT_EQ(neon::InterpolateUiValue(from, to, 0.24f).number, 2.0f);
    EXPECT_FLOAT_EQ(neon::InterpolateUiValue(from, to, 0.26f).number, 3.0f);
    EXPECT_FLOAT_EQ(neon::InterpolateUiValue(from, to, 0.5f).number, 5.0f);
  }

  TEST(UiInterpolationTest, MovesALengthOfOneUnitStraight)
  {
    const auto pixels = neon::InterpolateUiValue(
      Length(LayoutLength::Pixels(100.0f)), Length(LayoutLength::Pixels(200.0f)), 0.25f);
    EXPECT_EQ(pixels.length, LayoutLength::Pixels(125.0f));

    const auto percent = neon::InterpolateUiValue(
      Length(LayoutLength::Percent(0.0f)), Length(LayoutLength::Percent(50.0f)), 0.5f);
    EXPECT_EQ(percent.length, LayoutLength::Percent(25.0f));
  }

  TEST(UiInterpolationTest, MovesBetweenPixelsAndAPercentageAsASumOfBoth)
  {
    // as calc() of CSS does: half of 100px and half of 50%
    const auto half = neon::InterpolateUiValue(
      Length(LayoutLength::Pixels(100.0f)), Length(LayoutLength::Percent(50.0f)), 0.5f);
    EXPECT_EQ(half.length, LayoutLength::Sum(50.0f, 25.0f));

    const auto start = neon::InterpolateUiValue(
      Length(LayoutLength::Pixels(100.0f)), Length(LayoutLength::Percent(50.0f)), 0.0f);
    EXPECT_EQ(start.length, LayoutLength::Sum(100.0f, 0.0f));

    const auto sums = neon::InterpolateUiValue(
      Length(LayoutLength::Sum(-20.0f, 100.0f)), Length(LayoutLength::Sum(20.0f, 0.0f)), 0.5f);
    EXPECT_EQ(sums.length, LayoutLength::Sum(0.0f, 50.0f));
  }

  TEST(UiInterpolationTest, SwitchesALengthHalfwayWhenOneOfTheTwoIsAuto)
  {
    const auto from = Length(LayoutLength::Auto());
    const auto to = Length(LayoutLength::Pixels(100.0f));

    EXPECT_FALSE(neon::CanInterpolateUiValue(from, to));
    EXPECT_TRUE(neon::InterpolateUiValue(from, to, 0.49f).length.IsAuto());
    EXPECT_EQ(neon::InterpolateUiValue(from, to, 0.5f).length, LayoutLength::Pixels(100.0f));
  }

  TEST(UiInterpolationTest, MovesAColorWithoutAlphaStraight)
  {
    const auto value = neon::InterpolateUiValue(ColorValue(1.0f, 0.0f, 0.0f, 1.0f), ColorValue(0.0f, 0.0f, 1.0f, 1.0f), 0.25f);

    EXPECT_FLOAT_EQ(value.color.r, 0.75f);
    EXPECT_FLOAT_EQ(value.color.g, 0.0f);
    EXPECT_FLOAT_EQ(value.color.b, 0.25f);
    EXPECT_FLOAT_EQ(value.color.a, 1.0f);
  }

  TEST(UiInterpolationTest, MovesAColorWithItsAlphaMultipliedIn)
  {
    // from a red that cannot be seen to a blue that can: the red adds
    // nothing on the way, where mixing the colors alone would give purple
    const auto half = neon::InterpolateUiValue(ColorValue(1.0f, 0.0f, 0.0f, 0.0f), ColorValue(0.0f, 0.0f, 1.0f, 1.0f), 0.5f);

    EXPECT_FLOAT_EQ(half.color.r, 0.0f);
    EXPECT_FLOAT_EQ(half.color.g, 0.0f);
    EXPECT_FLOAT_EQ(half.color.b, 1.0f);
    EXPECT_FLOAT_EQ(half.color.a, 0.5f);

    // rgba(255, 0, 0, 0.2) to rgba(0, 0, 255, 0.6) at half:
    // alpha 0.4, red 0.5 * 0.2 / 0.4, blue 0.5 * 0.6 / 0.4
    const auto mixed = neon::InterpolateUiValue(ColorValue(1.0f, 0.0f, 0.0f, 0.2f), ColorValue(0.0f, 0.0f, 1.0f, 0.6f), 0.5f);

    EXPECT_NEAR(mixed.color.a, 0.4f, 1e-6f);
    EXPECT_NEAR(mixed.color.r, 0.25f, 1e-6f);
    EXPECT_NEAR(mixed.color.b, 0.75f, 1e-6f);
  }

  TEST(UiInterpolationTest, MovesBetweenTwoColorsThatCannotBeSeenWithoutDividingByZero)
  {
    const auto value = neon::InterpolateUiValue(ColorValue(1.0f, 0.0f, 0.0f, 0.0f), ColorValue(0.0f, 1.0f, 0.0f, 0.0f), 0.5f);

    EXPECT_FLOAT_EQ(value.color.a, 0.0f);
    EXPECT_FLOAT_EQ(value.color.r, 0.0f);
    EXPECT_FLOAT_EQ(value.color.g, 0.0f);
  }

  TEST(UiInterpolationTest, SetsAColorThatIsOnItsWay)
  {
    UiPropertyValue from = ColorValue(1.0f, 0.0f, 0.0f, 1.0f);
    from.flag = false;

    const auto value = neon::InterpolateUiValue(from, ColorValue(0.0f, 0.0f, 1.0f, 1.0f), 0.5f);
    EXPECT_TRUE(value.flag);
  }

  TEST(UiInterpolationTest, MovesEverySideOfEdges)
  {
    UiPropertyValue from;
    from.kind = UiValueKind::Edges;
    from.edges = {0.0f, 10.0f, 20.0f, 30.0f};

    UiPropertyValue to;
    to.kind = UiValueKind::Edges;
    to.edges = {10.0f, 10.0f, 0.0f, 50.0f};

    const auto value = neon::InterpolateUiValue(from, to, 0.5f);
    EXPECT_THAT(value.edges, ElementsAre(5.0f, 10.0f, 10.0f, 40.0f));
  }

  TEST(UiInterpolationTest, SwitchesHalfwayWhatCannotBeMoved)
  {
    EXPECT_FALSE(neon::CanInterpolateUiValue(Keyword(0), Keyword(2)));

    EXPECT_EQ(neon::InterpolateUiValue(Keyword(0), Keyword(2), 0.0f).keyword, 0);
    EXPECT_EQ(neon::InterpolateUiValue(Keyword(0), Keyword(2), 0.49f).keyword, 0);
    EXPECT_EQ(neon::InterpolateUiValue(Keyword(0), Keyword(2), 0.5f).keyword, 2);
    EXPECT_EQ(neon::InterpolateUiValue(Keyword(0), Keyword(2), 1.0f).keyword, 2);

    UiPropertyValue from;
    from.kind = UiValueKind::Text;
    from.text = "assets://a.png";

    UiPropertyValue to;
    to.kind = UiValueKind::Text;
    to.text = "assets://b.png";

    EXPECT_EQ(neon::InterpolateUiValue(from, to, 0.4f).text, "assets://a.png");
    EXPECT_EQ(neon::InterpolateUiValue(from, to, 0.6f).text, "assets://b.png");
  }

  TEST(UiInterpolationTest, SwitchesHalfwayBetweenTheHeightOfALineAsAMultipleAndAsPixels)
  {
    UiPropertyValue multiple = Number(1.5f);
    multiple.flag = true;

    const UiPropertyValue pixels = Number(24.0f);

    EXPECT_FALSE(neon::CanInterpolateUiValue(multiple, pixels));
    EXPECT_FLOAT_EQ(neon::InterpolateUiValue(multiple, pixels, 0.25f).number, 1.5f);
    EXPECT_FLOAT_EQ(neon::InterpolateUiValue(multiple, pixels, 0.75f).number, 24.0f);
  }

  TEST(UiInterpolationTest, TellsTwoValuesApart)
  {
    EXPECT_EQ(Number(1.0f), Number(1.0f));
    EXPECT_NE(Number(1.0f), Number(2.0f));
    EXPECT_NE(Number(1.0f), Keyword(1));
    EXPECT_EQ(ColorValue(1.0f, 0.5f, 0.0f, 1.0f), ColorValue(1.0f, 0.5f, 0.0f, 1.0f));
    EXPECT_NE(ColorValue(1.0f, 0.5f, 0.0f, 1.0f), ColorValue(1.0f, 0.5f, 0.0f, 0.5f));
    EXPECT_EQ(Length(LayoutLength::Pixels(1.0f)), Length(LayoutLength::Pixels(1.0f)));
    EXPECT_NE(Length(LayoutLength::Pixels(1.0f)), Length(LayoutLength::Percent(1.0f)));
  }
} // namespace
