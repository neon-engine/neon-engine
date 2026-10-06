#include "ui-fixture.hpp"

#include <neon/ui/elements/ui-bar.hpp>
#include <neon/ui/elements/ui-button.hpp>
#include <neon/ui/elements/ui-image.hpp>
#include <neon/ui/elements/ui-label.hpp>
#include <neon/ui/elements/ui-panel.hpp>

// Reading a user interface from a file: what every element holds, what is
// left out, and what is said about a file that is wrong.

namespace
{
  using neon::Color;
  using neon::LayoutLength;
  using neon::UiStates;
  using neon::UiStyle;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;
  using ::testing::ElementsAre;
  using ::testing::HasSubstr;
  using ::testing::StartsWith;

  class UiFileTest : public UiTest
  {
  protected:
    /// What is said about a file that is refused, without the line that
    /// counts the problems.
    std::vector<std::string> ProblemsOf(const std::string &yaml)
    {
      _logger->Clear();
      EXPECT_EQ(Show(yaml), -1);

      auto errors = Errors();
      if (!errors.empty()) { errors.pop_back(); }
      return errors;
    }

    std::vector<std::string> ProblemsUnderRoot(const std::string &children)
    {
      return ProblemsOf(
        "root:\n"
        "  type: panel\n"
        "  children:\n" + Indented(children, "    "));
    }

    /// The style an element has in a state.
    UiStyle StyleIn(const std::string &name, const UiStates &states)
    {
      auto &element = const_cast<neon::UiElement &>(Element(name));
      const UiStates before = element.GetStates();

      element.SetStates(states);
      UiStyle style = element.GetStyle();
      element.SetStates(before);
      return style;
    }

    static void ExpectColor(const Color &color, const float r, const float g, const float b, const float a)
    {
      EXPECT_NEAR(color.r, r, 0.002f);
      EXPECT_NEAR(color.g, g, 0.002f);
      EXPECT_NEAR(color.b, b, 0.002f);
      EXPECT_NEAR(color.a, a, 0.002f);
    }
  };

  // what a file holds

  TEST_F(UiFileTest, ReadsTheSmallestFile)
  {
    EXPECT_GE(Show("root:\n  type: panel\n"), 0);
    EXPECT_TRUE(Errors().empty());
  }

  TEST_F(UiFileTest, ReadsEveryKindOfElement)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: a-panel\n"
      "- type: label\n"
      "  name: a-label\n"
      "  text: Paused\n"
      "- type: image\n"
      "  name: an-image\n"
      "  src: assets://ui/heart.png\n"
      "- type: button\n"
      "  name: a-button\n"
      "  text: Start\n"
      "- type: bar\n"
      "  name: a-bar\n"
      "  value: 3\n"
      "  max: 4\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();

    EXPECT_NE(dynamic_cast<const neon::UiPanel *>(&Element("a-panel")), nullptr);

    const auto *label = dynamic_cast<const neon::UiLabel *>(&Element("a-label"));
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->GetText(), "Paused");

    const auto *image = dynamic_cast<const neon::UiImageElement *>(&Element("an-image"));
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->GetSource(), "assets://ui/heart.png");

    const auto *button = dynamic_cast<const neon::UiButton *>(&Element("a-button"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->GetText(), "Start");
    EXPECT_TRUE(button->IsEnabled());
    EXPECT_FALSE(button->WantsFocus());

    const auto *bar = dynamic_cast<const neon::UiBar *>(&Element("a-bar"));
    ASSERT_NE(bar, nullptr);
    EXPECT_FLOAT_EQ(bar->GetFilled(), 0.75f);

    EXPECT_EQ(Element("a-bar").GetType(), "bar");
    EXPECT_EQ(Element("a-bar").Describe(), "bar 'a-bar'");
  }

  TEST_F(UiFileTest, KeepsTheLineAnElementStartsAt)
  {
    ASSERT_GE(Show(
      "ui: test\n"
      "\n"
      "root:\n"
      "  type: panel\n"
      "  name: top\n"
      "  children:\n"
      "    # the health of the player\n"
      "    - type: label\n"
      "      name: health\n"), 0);

    // an element under a name starts at the name, one of a list at its
    // first line
    EXPECT_EQ(Element("top").GetLine(), 3u);
    EXPECT_EQ(Element("health").GetLine(), 8u);
  }

  TEST_F(UiFileTest, ElementsAreKeptInTheOrderOfTheFile)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: outer\n"
      "  children:\n"
      "    - type: label\n"
      "      name: first\n"
      "    - type: button\n"
      "      name: second\n"
      "      children:\n"
      "        - type: label\n"
      "          name: inside\n"
      "    - type: bar\n"
      "      name: third\n"), 0) << _logger->Messages(LogLevel::Error);

    const auto &children = Element("outer").GetChildren();
    ASSERT_EQ(children.size(), 3u);
    EXPECT_EQ(children[0]->GetName(), "first");
    EXPECT_EQ(children[1]->GetName(), "second");
    EXPECT_EQ(children[2]->GetName(), "third");

    EXPECT_EQ(Element("inside").GetParent(), &Element("second"));
    EXPECT_EQ(Element("second").GetParent(), &Element("outer"));
  }

  // defaults

  TEST_F(UiFileTest, WhatIsLeftOutKeepsTheInitialValueOfCss)
  {
    ASSERT_GE(ShowUnderRoot("- type: panel\n  name: plain\n"), 0);

    const UiStyle &style = Element("plain").GetStyle();
    const UiStyle initial;

    EXPECT_EQ(style.layout.display, neon::LayoutDisplay::Flex);
    EXPECT_EQ(style.layout.position, neon::LayoutPosition::Relative);
    EXPECT_EQ(style.layout.box_sizing, neon::LayoutBoxSizing::ContentBox);
    EXPECT_EQ(style.layout.width, LayoutLength::Auto());
    EXPECT_EQ(style.layout.height, LayoutLength::Auto());
    EXPECT_EQ(style.layout.margin.left, LayoutLength::Pixels(0));
    EXPECT_EQ(style.layout.padding.top, LayoutLength::Pixels(0));
    EXPECT_FLOAT_EQ(style.layout.border.top, 0);
    EXPECT_EQ(style.layout.inset.left, LayoutLength::Auto());
    EXPECT_EQ(style.layout.flex_direction, neon::FlexDirection::Row);
    EXPECT_EQ(style.layout.flex_wrap, neon::FlexWrap::NoWrap);
    EXPECT_EQ(style.layout.justify_content, neon::JustifyContent::FlexStart);
    EXPECT_EQ(style.layout.align_items, neon::AlignItems::Stretch);
    EXPECT_EQ(style.layout.align_self, neon::AlignSelf::Auto);
    EXPECT_FLOAT_EQ(style.layout.flex_grow, 0);
    EXPECT_FLOAT_EQ(style.layout.flex_shrink, 1);
    EXPECT_EQ(style.layout.flex_basis, LayoutLength::Auto());
    EXPECT_FLOAT_EQ(style.layout.row_gap, 0);

    ExpectColor(style.background_color, 0, 0, 0, 0);
    EXPECT_TRUE(style.background_image.empty());
    EXPECT_FALSE(style.border_color.has_value());
    EXPECT_FLOAT_EQ(style.opacity, 1);
    EXPECT_EQ(style.overflow, neon::UiOverflow::Visible);
    EXPECT_EQ(style.z_index, 0);
    EXPECT_FLOAT_EQ(style.font_size, 16);
    EXPECT_EQ(style.font_weight, 400);
    EXPECT_EQ(style.font_family, "sans-serif");
    EXPECT_EQ(style.text_align, neon::TextAlign::Left);
    EXPECT_FLOAT_EQ(style.LineHeight(), 0);
    EXPECT_EQ(style.object_fit, neon::UiObjectFit::Fill);
  }

  TEST_F(UiFileTest, TextIsWhiteUnlessTheFileSaysOtherwise)
  {
    ASSERT_GE(ShowUnderRoot("- type: label\n  name: plain\n  text: Paused\n"), 0);

    ExpectColor(Element("plain").GetStyle().color, 1, 1, 1, 1);
  }

  TEST_F(UiFileTest, ThePointerGoesThroughEverythingButAButton)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n  name: a-panel\n"
      "- type: label\n  name: a-label\n"
      "- type: image\n  name: an-image\n  src: assets://ui/heart.png\n"
      "- type: bar\n  name: a-bar\n"
      "- type: button\n  name: a-button\n"
      "- type: panel\n  name: a-window\n  pointer_events: auto\n"), 0);

    for (const std::string name : {"a-panel", "a-label", "an-image", "a-bar"})
    {
      EXPECT_EQ(Element(name).GetStyle().pointer_events, neon::UiPointerEvents::None) << name;
    }

    EXPECT_EQ(Element("a-button").GetStyle().pointer_events, neon::UiPointerEvents::Auto);
    EXPECT_EQ(Element("a-window").GetStyle().pointer_events, neon::UiPointerEvents::Auto);
  }

  TEST_F(UiFileTest, ABarStartsAsLargeAsTheOneOfABrowser)
  {
    ASSERT_GE(ShowUnderRoot("- type: bar\n  name: plain\n"), 0);

    const UiStyle &style = Element("plain").GetStyle();
    EXPECT_EQ(style.layout.width, LayoutLength::Pixels(160));
    EXPECT_EQ(style.layout.height, LayoutLength::Pixels(16));
    ExpectColor(style.background_color, 0, 0, 0, 0.5f);

    Frame();
    EXPECT_FLOAT_EQ(dynamic_cast<const neon::UiBar &>(Element("plain")).GetFilled(), 0);
  }

  TEST_F(UiFileTest, AButtonStartsWithPaddingAndABackground)
  {
    ASSERT_GE(ShowUnderRoot("- type: button\n  name: plain\n  text: Start\n"), 0);

    const UiStyle &style = Element("plain").GetStyle();
    EXPECT_EQ(style.layout.padding.top, LayoutLength::Pixels(8));
    EXPECT_EQ(style.layout.padding.right, LayoutLength::Pixels(16));
    EXPECT_EQ(style.layout.padding.bottom, LayoutLength::Pixels(8));
    EXPECT_EQ(style.layout.padding.left, LayoutLength::Pixels(16));
    EXPECT_EQ(style.text_align, neon::TextAlign::Center);
    ExpectColor(style.background_color, 0.231f, 0.259f, 0.322f, 1);
  }

  // properties

  TEST_F(UiFileTest, ReadsTheBoxModel)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: box\n"
      "  width: 200\n"
      "  height: 50%\n"
      "  min_width: 10px\n"
      "  max_width: 80%\n"
      "  min_height: auto\n"
      "  max_height: none\n"
      "  box_sizing: border-box\n"
      "  margin: 1 2 3 4\n"
      "  padding: [5, 6]\n"
      "  border_width: 7\n"), 0) << _logger->Messages(LogLevel::Error);

    const auto &layout = Element("box").GetStyle().layout;

    EXPECT_EQ(layout.width, LayoutLength::Pixels(200));
    EXPECT_EQ(layout.height, LayoutLength::Percent(50));
    EXPECT_EQ(layout.min_width, LayoutLength::Pixels(10));
    EXPECT_EQ(layout.max_width, LayoutLength::Percent(80));
    EXPECT_EQ(layout.min_height, LayoutLength::Auto());
    EXPECT_EQ(layout.max_height, LayoutLength::Auto());
    EXPECT_EQ(layout.box_sizing, neon::LayoutBoxSizing::BorderBox);

    // top, right, bottom, left
    EXPECT_EQ(layout.margin.top, LayoutLength::Pixels(1));
    EXPECT_EQ(layout.margin.right, LayoutLength::Pixels(2));
    EXPECT_EQ(layout.margin.bottom, LayoutLength::Pixels(3));
    EXPECT_EQ(layout.margin.left, LayoutLength::Pixels(4));

    // top and bottom, then left and right
    EXPECT_EQ(layout.padding.top, LayoutLength::Pixels(5));
    EXPECT_EQ(layout.padding.right, LayoutLength::Pixels(6));
    EXPECT_EQ(layout.padding.bottom, LayoutLength::Pixels(5));
    EXPECT_EQ(layout.padding.left, LayoutLength::Pixels(6));

    EXPECT_FLOAT_EQ(layout.border.top, 7);
    EXPECT_FLOAT_EQ(layout.border.left, 7);
  }

  TEST_F(UiFileTest, ReadsThreeValuesAsTopThenSidesThenBottom)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: box\n"
      "  margin: 1px auto 3px\n"), 0) << _logger->Messages(LogLevel::Error);

    const auto &margin = Element("box").GetStyle().layout.margin;
    EXPECT_EQ(margin.top, LayoutLength::Pixels(1));
    EXPECT_EQ(margin.right, LayoutLength::Auto());
    EXPECT_EQ(margin.bottom, LayoutLength::Pixels(3));
    EXPECT_EQ(margin.left, LayoutLength::Auto());
  }

  TEST_F(UiFileTest, ASideWinsOverTheShorthandNextToIt)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: box\n"
      "  padding_left: 40\n"
      "  padding: 8\n"
      "  margin: 8\n"
      "  margin_top: auto\n"), 0) << _logger->Messages(LogLevel::Error);

    const auto &layout = Element("box").GetStyle().layout;
    EXPECT_EQ(layout.padding.left, LayoutLength::Pixels(40));
    EXPECT_EQ(layout.padding.top, LayoutLength::Pixels(8));
    EXPECT_EQ(layout.margin.top, LayoutLength::Auto());
    EXPECT_EQ(layout.margin.left, LayoutLength::Pixels(8));
  }

  TEST_F(UiFileTest, ReadsPositionAndItsSides)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: box\n"
      "  position: absolute\n"
      "  top: 16\n"
      "  right: 10%\n"
      "  bottom: 4px\n"
      "  left: auto\n"
      "  z_index: 3\n"
      "  display: none\n"
      "  overflow: hidden\n"
      "  opacity: 0.5\n"), 0) << _logger->Messages(LogLevel::Error);

    const UiStyle &style = Element("box").GetStyle();
    EXPECT_EQ(style.layout.position, neon::LayoutPosition::Absolute);
    EXPECT_EQ(style.layout.inset.top, LayoutLength::Pixels(16));
    EXPECT_EQ(style.layout.inset.right, LayoutLength::Percent(10));
    EXPECT_EQ(style.layout.inset.bottom, LayoutLength::Pixels(4));
    EXPECT_EQ(style.layout.inset.left, LayoutLength::Auto());
    EXPECT_EQ(style.z_index, 3);
    EXPECT_EQ(style.layout.display, neon::LayoutDisplay::None);
    EXPECT_EQ(style.overflow, neon::UiOverflow::Hidden);
    EXPECT_FLOAT_EQ(style.opacity, 0.5f);
  }

  TEST_F(UiFileTest, ReadsFlexbox)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: box\n"
      "  flex_direction: column-reverse\n"
      "  flex_wrap: wrap-reverse\n"
      "  justify_content: space-between\n"
      "  align_items: flex-end\n"
      "  align_self: center\n"
      "  align_content: space-evenly\n"
      "  flex_grow: 2\n"
      "  flex_shrink: 0\n"
      "  flex_basis: 25%\n"
      "  gap: 4 8\n"), 0) << _logger->Messages(LogLevel::Error);

    const auto &layout = Element("box").GetStyle().layout;
    EXPECT_EQ(layout.flex_direction, neon::FlexDirection::ColumnReverse);
    EXPECT_EQ(layout.flex_wrap, neon::FlexWrap::WrapReverse);
    EXPECT_EQ(layout.justify_content, neon::JustifyContent::SpaceBetween);
    EXPECT_EQ(layout.align_items, neon::AlignItems::FlexEnd);
    EXPECT_EQ(layout.align_self, neon::AlignSelf::Center);
    EXPECT_EQ(layout.align_content, neon::AlignContent::SpaceEvenly);
    EXPECT_FLOAT_EQ(layout.flex_grow, 2);
    EXPECT_FLOAT_EQ(layout.flex_shrink, 0);
    EXPECT_EQ(layout.flex_basis, LayoutLength::Percent(25));

    // between rows, then between columns
    EXPECT_FLOAT_EQ(layout.row_gap, 4);
    EXPECT_FLOAT_EQ(layout.column_gap, 8);
  }

  TEST_F(UiFileTest, ReadsTheShorthandFlex)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n  name: one\n  flex: 1\n"
      "- type: panel\n  name: three\n  flex: 2 0 100px\n"
      "- type: panel\n  name: basis\n  flex: 1 30%\n"
      "- type: panel\n  name: factors\n  flex: 1 2\n"
      "- type: panel\n  name: none\n  flex: none\n"
      "- type: panel\n  name: auto\n  flex: auto\n"), 0) << _logger->Messages(LogLevel::Error);

    const auto layout_of = [this](const std::string &name) { return Element(name).GetStyle().layout; };

    // as in CSS, a number alone comes with a basis of 0
    EXPECT_FLOAT_EQ(layout_of("one").flex_grow, 1);
    EXPECT_FLOAT_EQ(layout_of("one").flex_shrink, 1);
    EXPECT_EQ(layout_of("one").flex_basis, LayoutLength::Pixels(0));

    EXPECT_FLOAT_EQ(layout_of("three").flex_grow, 2);
    EXPECT_FLOAT_EQ(layout_of("three").flex_shrink, 0);
    EXPECT_EQ(layout_of("three").flex_basis, LayoutLength::Pixels(100));

    EXPECT_FLOAT_EQ(layout_of("basis").flex_shrink, 1);
    EXPECT_EQ(layout_of("basis").flex_basis, LayoutLength::Percent(30));

    EXPECT_FLOAT_EQ(layout_of("factors").flex_shrink, 2);
    EXPECT_EQ(layout_of("factors").flex_basis, LayoutLength::Pixels(0));

    EXPECT_FLOAT_EQ(layout_of("none").flex_grow, 0);
    EXPECT_FLOAT_EQ(layout_of("none").flex_shrink, 0);
    EXPECT_EQ(layout_of("none").flex_basis, LayoutLength::Auto());

    EXPECT_FLOAT_EQ(layout_of("auto").flex_grow, 1);
    EXPECT_FLOAT_EQ(layout_of("auto").flex_shrink, 1);
    EXPECT_EQ(layout_of("auto").flex_basis, LayoutLength::Auto());
  }

  TEST_F(UiFileTest, ReadsColoursTheWayCssWritesThem)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: box\n"
      "  background_color: \"#ff8000\"\n"
      "  border_color: \"#ff800080\"\n"
      "  color: \"#f80\"\n"
      "  outline_color: rgb(255, 128, 0)\n"
      "  accent_color: rgba(255, 128, 0, 0.5)\n"), 0) << _logger->Messages(LogLevel::Error);

    const UiStyle &style = Element("box").GetStyle();
    ExpectColor(style.background_color, 1, 0.502f, 0, 1);
    ExpectColor(*style.border_color, 1, 0.502f, 0, 0.502f);
    ExpectColor(style.color, 1, 0.533f, 0, 1);
    ExpectColor(*style.outline_color, 1, 0.502f, 0, 1);
    ExpectColor(style.accent_color, 1, 0.502f, 0, 0.5f);
  }

  TEST_F(UiFileTest, ReadsColoursTheWaySceneFilesWriteThem)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: box\n"
      "  background_color: [1, 0.5, 0]\n"
      "  color: [1, 0.5, 0, 0.25]\n"), 0) << _logger->Messages(LogLevel::Error);

    ExpectColor(Element("box").GetStyle().background_color, 1, 0.5f, 0, 1);
    ExpectColor(Element("box").GetStyle().color, 1, 0.5f, 0, 0.25f);
  }

  TEST_F(UiFileTest, ReadsTheShorthandBorder)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n  name: full\n  border: \"2px solid #ff8000\"\n"
      "- type: panel\n  name: width\n  border: 4\n"
      "- type: panel\n  name: solid\n  border: solid\n"
      "- type: panel\n  name: none\n  border: none\n"
      "- type: panel\n  name: order\n  border: white solid 1px\n"
      "- type: panel\n  name: both\n  border: \"2px solid #ff8000\"\n  border_width: 1 2 3 4\n"), 0)
      << _logger->Messages(LogLevel::Error);

    EXPECT_FLOAT_EQ(Element("full").GetStyle().layout.border.left, 2);
    ExpectColor(*Element("full").GetStyle().border_color, 1, 0.502f, 0, 1);

    EXPECT_FLOAT_EQ(Element("width").GetStyle().layout.border.top, 4);
    EXPECT_FALSE(Element("width").GetStyle().border_color.has_value());

    // `medium` is 3 pixels
    EXPECT_FLOAT_EQ(Element("solid").GetStyle().layout.border.top, 3);
    EXPECT_FLOAT_EQ(Element("none").GetStyle().layout.border.top, 0);

    EXPECT_FLOAT_EQ(Element("order").GetStyle().layout.border.top, 1);
    ExpectColor(*Element("order").GetStyle().border_color, 1, 1, 1, 1);

    EXPECT_FLOAT_EQ(Element("both").GetStyle().layout.border.top, 1);
    EXPECT_FLOAT_EQ(Element("both").GetStyle().layout.border.left, 4);
    ExpectColor(*Element("both").GetStyle().border_color, 1, 0.502f, 0, 1);
  }

  TEST_F(UiFileTest, ReadsTheImagesOfABox)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: box\n"
      "  background_image: assets://ui/paper.png\n"
      "  border_image_source: assets://ui/frame.png\n"
      "  border_image_slice: 12 16\n"
      "  border_image_width: 24\n"
      "- type: panel\n"
      "  name: plain\n"
      "  border_image_source: none\n"), 0) << _logger->Messages(LogLevel::Error);

    const UiStyle &style = Element("box").GetStyle();
    EXPECT_EQ(style.background_image, "assets://ui/paper.png");
    EXPECT_EQ(style.border_image_source, "assets://ui/frame.png");
    EXPECT_FLOAT_EQ(style.border_image_slice.top, 12);
    EXPECT_FLOAT_EQ(style.border_image_slice.right, 16);
    EXPECT_FLOAT_EQ(style.border_image_slice.bottom, 12);
    EXPECT_FLOAT_EQ(style.border_image_slice.left, 16);
    EXPECT_FLOAT_EQ(style.border_image_width.left, 24);

    EXPECT_TRUE(Element("plain").GetStyle().border_image_source.empty());
  }

  TEST_F(UiFileTest, ReadsThePropertiesOfText)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n"
      "  name: title\n"
      "  text: Paused\n"
      "  font_family: title\n"
      "  font_size: 32px\n"
      "  font_weight: bold\n"
      "  text_align: center\n"
      "  line_height: 1.5\n"
      "- type: label\n  name: pixels\n  line_height: 24px\n  font_weight: 300\n"
      "- type: label\n  name: percent\n  line_height: 150%\n  font_size: 20\n"
      "- type: label\n  name: normal\n  line_height: normal\n  font_weight: normal\n"), 0)
      << _logger->Messages(LogLevel::Error);

    const UiStyle &style = Element("title").GetStyle();
    EXPECT_EQ(style.font_family, "title");
    EXPECT_FLOAT_EQ(style.font_size, 32);
    EXPECT_EQ(style.font_weight, 700);
    EXPECT_EQ(style.text_align, neon::TextAlign::Center);

    // a number alone is a multiple of the size of the font
    EXPECT_FLOAT_EQ(style.LineHeight(), 48);

    EXPECT_FLOAT_EQ(Element("pixels").GetStyle().LineHeight(), 24);
    EXPECT_EQ(Element("pixels").GetStyle().font_weight, 300);
    EXPECT_FLOAT_EQ(Element("percent").GetStyle().LineHeight(), 30);
    EXPECT_FLOAT_EQ(Element("normal").GetStyle().LineHeight(), 0);
    EXPECT_EQ(Element("normal").GetStyle().font_weight, 400);
  }

  TEST_F(UiFileTest, ReadsTheOutlineAndTheFitOfAnImage)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: image\n"
      "  name: icon\n"
      "  src: assets://ui/heart.png\n"
      "  object_fit: contain\n"
      "  outline_width: 2\n"
      "  outline_offset: -1\n"), 0) << _logger->Messages(LogLevel::Error);

    const UiStyle &style = Element("icon").GetStyle();
    EXPECT_EQ(style.object_fit, neon::UiObjectFit::Contain);
    EXPECT_FLOAT_EQ(style.outline_width, 2);
    EXPECT_FLOAT_EQ(style.outline_offset, -1);
  }

  TEST_F(UiFileTest, ANumberIsTextAsWell)
  {
    ASSERT_GE(ShowUnderRoot("- type: label\n  name: count\n  text: 75\n"), 0);
    Frame();

    EXPECT_EQ(dynamic_cast<const neon::UiLabel &>(Element("count")).GetText(), "75");
  }

  // states

  TEST_F(UiFileTest, AStateChangesWhatItWrites)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n"
      "  name: start\n"
      "  text: Start\n"
      "  background_color: \"#000000\"\n"
      "  color: \"#808080\"\n"
      "  hover:\n"
      "    background_color: \"#ff0000\"\n"
      "  active:\n"
      "    background_color: \"#00ff00\"\n"
      "  focus:\n"
      "    color: \"#0000ff\"\n"
      "  disabled:\n"
      "    color: \"#ffffff\"\n"), 0) << _logger->Messages(LogLevel::Error);

    ExpectColor(StyleIn("start", {}).background_color, 0, 0, 0, 1);
    ExpectColor(StyleIn("start", {}).color, 0.502f, 0.502f, 0.502f, 1);

    ExpectColor(StyleIn("start", {.hover = true}).background_color, 1, 0, 0, 1);
    ExpectColor(StyleIn("start", {.hover = true}).color, 0.502f, 0.502f, 0.502f, 1);

    ExpectColor(StyleIn("start", {.focus = true}).background_color, 0, 0, 0, 1);
    ExpectColor(StyleIn("start", {.focus = true}).color, 0, 0, 1, 1);

    ExpectColor(StyleIn("start", {.disabled = true}).color, 1, 1, 1, 1);
  }

  TEST_F(UiFileTest, SeveralStatesApplyTogetherAndActiveWinsOverHover)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n"
      "  name: start\n"
      "  hover:\n"
      "    background_color: \"#ff0000\"\n"
      "    opacity: 0.9\n"
      "  active:\n"
      "    background_color: \"#00ff00\"\n"
      "  focus:\n"
      "    color: \"#0000ff\"\n"), 0) << _logger->Messages(LogLevel::Error);

    const UiStyle style = StyleIn("start", {.focus = true, .hover = true, .active = true});

    ExpectColor(style.background_color, 0, 1, 0, 1);
    ExpectColor(style.color, 0, 0, 1, 1);
    EXPECT_FLOAT_EQ(style.opacity, 0.9f);
  }

  TEST_F(UiFileTest, WhatTheFileWritesWinsOverTheStatesOfTheEngine)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n  name: plain\n"
      "- type: button\n  name: coloured\n  background_color: \"#ff0000\"\n"), 0);

    // a button of the engine is lighter under the pointer
    ExpectColor(StyleIn("plain", {.hover = true}).background_color, 0.298f, 0.337f, 0.416f, 1);

    // as in CSS, where what an author writes wins over the browser, a
    // state of the browser included
    ExpectColor(StyleIn("coloured", {.hover = true}).background_color, 1, 0, 0, 1);

    // what the file did not write is still that of the engine
    EXPECT_FLOAT_EQ(StyleIn("coloured", {.focus = true}).outline_width, 2);
    EXPECT_FLOAT_EQ(StyleIn("coloured", {.disabled = true}).opacity, 0.5f);
  }

  TEST_F(UiFileTest, AStateCanChangeWhereAnElementGoes)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n"
      "  name: start\n"
      "  width: 100\n"
      "  hover:\n"
      "    width: 120\n"), 0) << _logger->Messages(LogLevel::Error);

    EXPECT_EQ(StyleIn("start", {.hover = true}).layout.width, LayoutLength::Pixels(120));
  }

  TEST_F(UiFileTest, EveryElementHasStates)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: row\n"
      "  pointer_events: auto\n"
      "  hover:\n"
      "    background_color: \"#ffffff20\"\n"), 0) << _logger->Messages(LogLevel::Error);

    ExpectColor(StyleIn("row", {.hover = true}).background_color, 1, 1, 1, 0.125f);
    ExpectColor(StyleIn("row", {}).background_color, 0, 0, 0, 0);
  }

  // the top of a file

  TEST_F(UiFileTest, ReadsWhatIsWrittenAtTheTop)
  {
    ASSERT_GE(Show(
      "ui: pause-menu\n"
      "version: 1\n"
      "modal: true\n"
      "scale: height\n"
      "reference_size: [1280, 720]\n"
      "fonts:\n"
      "  - family: title\n"
      "    weight: 700\n"
      "    src: assets://fonts/bold.ttf\n"
      "values:\n"
      "  health: 75\n"
      "  name: Ada\n"
      "  armed: true\n"
      "root:\n"
      "  type: panel\n"
      "  name: top\n"
      "  width: 100%\n"
      "  height: 100%\n"
      "  children:\n"
      "    - type: button\n"
      "      name: resume\n"
      "      text: \"{name} {health} {armed}\"\n"
      "      font_family: title\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();

    EXPECT_EQ(dynamic_cast<const neon::UiButton &>(Element("resume")).GetText(), "Ada 75 true");

    // the frame is 1080 high and the file was made for 720
    ExpectBox("top", 0, 0, 1280, 720);

    // a file that is modal gives the focus to something
    EXPECT_EQ(_ui->GetFocused(), "resume");

    EXPECT_TRUE(Errors().empty()) << _logger->Messages(LogLevel::Error);
  }

  // what is said about a file that is wrong

  TEST_F(UiFileTest, SaysThatAFileIsMissing)
  {
    EXPECT_EQ(_ui->Load("assets://ui/missing.ui.yml"), -1);

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "assets://ui/missing.ui.yml: the file cannot be read"))
      << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The user interface assets://ui/missing.ui.yml has 1 problem and is not shown"));
  }

  TEST_F(UiFileTest, SaysThatAFileIsNotYaml)
  {
    const auto problems = ProblemsOf("root:\n  type: [panel\n");

    ASSERT_EQ(problems.size(), 1u);
    EXPECT_THAT(problems[0], StartsWith("assets://ui/test.ui.yml:"));
  }

  TEST_F(UiFileTest, SaysThatThereIsNoRoot)
  {
    EXPECT_THAT(
      ProblemsOf("ui: test\nversion: 1\n"),
      ElementsAre("assets://ui/test.ui.yml:1: the user interface has no 'root', where an element was expected"));
  }

  TEST_F(UiFileTest, SaysThatTheFileIsNoMap)
  {
    EXPECT_THAT(
      ProblemsOf("- type: panel\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:1: the user interface is a list, where a map was expected",
        "assets://ui/test.ui.yml:1: the user interface has no 'root', where an element was expected"));
  }

  TEST_F(UiFileTest, SaysThatANameAtTheTopIsNotKnown)
  {
    EXPECT_THAT(
      ProblemsOf("ui: test\nroot:\n  type: panel\nrot: 3\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:4: 'rot' is not known to the user interface. Known are: "
        "ui, version, modal, scale, reference_size, fonts, values, root, styles, templates, cancel"));
  }

  TEST_F(UiFileTest, SaysThatTheVersionIsTooNew)
  {
    EXPECT_THAT(
      ProblemsOf("version: 2\nroot:\n  type: panel\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:1: the user interface has version 2, and this engine reads up to version 1"));
  }

  TEST_F(UiFileTest, SaysWhatIsWrongAtTheTop)
  {
    EXPECT_THAT(
      ProblemsOf(
        "ui: [a]\n"
        "modal: maybe\n"
        "scale: both\n"
        "reference_size: [1920]\n"
        "fonts: none\n"
        "values: 3\n"
        "root:\n"
        "  type: panel\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:1: 'ui' of the user interface is a list, where text was expected",
        "assets://ui/test.ui.yml:2: 'modal' of the user interface is text, where true or false was expected",
        "assets://ui/test.ui.yml:3: 'scale' of the user interface is 'both', where one of these was expected: "
        "fit, width, height, none",
        "assets://ui/test.ui.yml:4: 'reference_size' of the user interface is another list, where a list of "
        "2 numbers above 0 was expected, such as [1920, 1080]",
        "assets://ui/test.ui.yml:5: 'fonts' of the user interface is text, where a list was expected",
        "assets://ui/test.ui.yml:6: 'values' of the user interface is a number, where a map was expected"));
  }

  TEST_F(UiFileTest, SaysWhatIsWrongWithAFont)
  {
    EXPECT_THAT(
      ProblemsOf(
        "fonts:\n"
        "  - family: title\n"
        "  - src: assets://fonts/bold.ttf\n"
        "    weight: 7000\n"
        "  - family: body\n"
        "    src: assets://fonts/regular.ttf\n"
        "    stretch: wide\n"
        "root:\n"
        "  type: panel\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:2: font 1 has no 'src', where the virtual path of a font was expected",
        "assets://ui/test.ui.yml:3: font 2 has no 'family', where the name it is asked for by was expected",
        "assets://ui/test.ui.yml:4: 'weight' of font 2 is 7000, where a number from 1 to 1000 was expected",
        "assets://ui/test.ui.yml:7: 'stretch' is not known to font 3. Known are: family, src, weight, style, "
        "rendering"));
  }

  TEST_F(UiFileTest, SaysWhatIsWrongWithAValue)
  {
    EXPECT_THAT(
      ProblemsOf(
        "values:\n"
        "  health: 75\n"
        "  items: [a, b]\n"
        "root:\n"
        "  type: panel\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:3: value 'items' of the user interface is a list, where a number, text, "
        "true, or false was expected"));
  }

  TEST_F(UiFileTest, SaysThatAKindOfElementIsNotKnown)
  {
    EXPECT_THAT(
      ProblemsUnderRoot("- type: lable\n  name: health\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:4: type 'lable' of lable 'health' is not known. Known are: "
        "input, textarea, checkbox, radio, toggle, slider, select, panel, label, image, button, bar"));
  }

  TEST_F(UiFileTest, SaysThatAnElementHasNoKind)
  {
    EXPECT_THAT(
      ProblemsUnderRoot("- name: health\n  text: Paused\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:4: element 'health' has no 'type', where one of these was expected: "
        "input, textarea, checkbox, radio, toggle, slider, select, panel, label, image, button, bar"));
  }

  TEST_F(UiFileTest, CallsAnElementWithoutANameByItsPlace)
  {
    EXPECT_THAT(
      ProblemsUnderRoot(
        "- type: label\n"
        "- type: panel\n"
        "  children:\n"
        "    - type: label\n"
        "      width: wide\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:8: 'width' of child 1 of child 2 of the root is 'wide', where a number of "
        "pixels, a percentage such as 50%, or auto was expected"));
  }

  TEST_F(UiFileTest, SaysThatTheRootIsNoElement)
  {
    EXPECT_THAT(
      ProblemsOf("root: panel\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:1: the root is text, where a map was expected",
        "assets://ui/test.ui.yml:1: the root has no 'type', where one of these was expected: "
        "input, textarea, checkbox, radio, toggle, slider, select, panel, label, image, button, bar"));
  }

  TEST_F(UiFileTest, SaysThatAPropertyIsNotKnown)
  {
    const auto problems = ProblemsUnderRoot("- type: label\n  name: health\n  colour: \"#ffffff\"\n");

    ASSERT_EQ(problems.size(), 1u);
    EXPECT_THAT(problems[0], StartsWith(
                  "assets://ui/test.ui.yml:6: 'colour' is not known to label 'health'. Known are: "
                  "type, name, hidden, text, focus, hover, active, disabled, display, position, box_sizing, "
                  "width, height,"));
    EXPECT_THAT(problems[0], HasSubstr(", color, font_family, font_size, font_weight, text_align, "));
  }

  TEST_F(UiFileTest, SaysThatWhatBelongsToAnotherKindIsNotKnown)
  {
    EXPECT_THAT(
      ProblemsUnderRoot("- type: label\n  name: health\n  src: assets://ui/heart.png\n"),
      ElementsAre(StartsWith("assets://ui/test.ui.yml:6: 'src' is not known to label 'health'. Known are: ")));

    EXPECT_THAT(
      ProblemsUnderRoot("- type: label\n  name: health\n  children: []\n"),
      ElementsAre(StartsWith("assets://ui/test.ui.yml:6: 'children' is not known to label 'health'.")));

    EXPECT_THAT(
      ProblemsUnderRoot("- type: panel\n  name: box\n  text: Paused\n"),
      ElementsAre(StartsWith("assets://ui/test.ui.yml:6: 'text' is not known to panel 'box'.")));

    EXPECT_THAT(
      ProblemsUnderRoot("- type: label\n  name: health\n  enabled: false\n"),
      ElementsAre(StartsWith("assets://ui/test.ui.yml:6: 'enabled' is not known to label 'health'.")));
  }

  struct WrongProperty
  {
    const char *written;
    const char *problem;
  };

  void PrintTo(const WrongProperty &value, std::ostream *out)
  {
    *out << value.written;
  }

  class WrongPropertyTest : public UiFileTest, public ::testing::WithParamInterface<WrongProperty> {};

  TEST_P(WrongPropertyTest, IsReportedWithItsLineAndWhatWasExpected)
  {
    EXPECT_THAT(
      ProblemsUnderRoot(std::string("- type: button\n  name: start\n  ") + GetParam().written + "\n"),
      ElementsAre(std::string("assets://ui/test.ui.yml:6: ") + GetParam().problem));
  }

  INSTANTIATE_TEST_SUITE_P(EveryKindOfProperty, WrongPropertyTest, ::testing::Values(
    WrongProperty{
      "width: wide",
      "'width' of button 'start' is 'wide', where a number of pixels, a percentage such as 50%, or auto "
      "was expected"
    },
    WrongProperty{
      "height: [1, 2]",
      "'height' of button 'start' is a list, where a number of pixels, a percentage such as 50%, or auto "
      "was expected"
    },
    WrongProperty{
      "max_width: auto",
      "'max_width' of button 'start' is 'auto', where a number of pixels or a percentage such as 50% "
      "was expected"
    },
    WrongProperty{
      "padding_left: auto",
      "'padding_left' of button 'start' is 'auto', where a number of pixels or a percentage such as 50% "
      "was expected"
    },
    WrongProperty{
      // an `em` is a unit by now, and is read
      "width: 12ex",
      "'width' of button 'start' is '12ex', where a number of pixels, a percentage such as 50%, or auto "
      "was expected"
    },
    WrongProperty{
      "margin: 1 2 3 4 5",
      "'margin' of button 'start' is '1 2 3 4 5', where one to four values, each a number of pixels, "
      "a percentage such as 50%, or auto was expected"
    },
    WrongProperty{
      "padding: -4",
      "'padding' of button 'start' is a number, where one to four values, each a number of pixels or "
      "a percentage such as 50%, and none below 0 was expected"
    },
    WrongProperty{
      "border_width: 10%",
      "'border_width' of button 'start' is '10%', where one to four values, each a number of pixels, "
      "and none below 0 was expected"
    },
    WrongProperty{
      "border: 2px dotted red",
      "'border' of button 'start' is '2px dotted red', where a width, solid or none, and a colour, such as "
      "\"2px solid #ffffff\" was expected"
    },
    WrongProperty{
      "row_gap: -1",
      "'row_gap' of button 'start' is a number, where a number of pixels that is not below 0 was expected"
    },
    WrongProperty{
      "gap: 1 2 3",
      "'gap' of button 'start' is '1 2 3', where one or two numbers of pixels that are not below 0 "
      "was expected"
    },
    WrongProperty{
      "flex: grow",
      "'flex' of button 'start' is 'grow', where none, auto, or a number to grow by, which a number to "
      "shrink by and a basis may follow was expected"
    },
    WrongProperty{
      "flex_grow: -1",
      "'flex_grow' of button 'start' is a number, where a number that is not below 0 was expected"
    },
    WrongProperty{
      "opacity: 1.5",
      "'opacity' of button 'start' is a number, where a number from 0 to 1 was expected"
    },
    WrongProperty{
      "opacity: half",
      "'opacity' of button 'start' is 'half', where a number from 0 to 1 was expected"
    },
    WrongProperty{
      "display: block",
      "'display' of button 'start' is 'block', where one of these was expected: flex, none"
    },
    WrongProperty{
      "position: fixed",
      "'position' of button 'start' is 'fixed', where one of these was expected: relative, absolute"
    },
    WrongProperty{
      "justify_content: middle",
      "'justify_content' of button 'start' is 'middle', where one of these was expected: "
      "flex-start, flex-end, center, space-between, space-around, space-evenly"
    },
    WrongProperty{
      "align_items: baseline",
      "'align_items' of button 'start' is 'baseline', where one of these was expected: "
      "stretch, flex-start, flex-end, center"
    },
    WrongProperty{
      "flex_direction: 3",
      "'flex_direction' of button 'start' is a number, where one of these was expected: "
      "row, row-reverse, column, column-reverse"
    },
    WrongProperty{
      // `scroll` and `auto` are read by now
      "overflow: clip",
      "'overflow' of button 'start' is 'clip', where one of these was expected: visible, hidden, scroll, "
      "auto"
    },
    WrongProperty{
      "text_align: justify",
      "'text_align' of button 'start' is 'justify', where one of these was expected: left, center, right, "
      "start, end"
    },
    WrongProperty{
      "background_color: red",
      "'background_color' of button 'start' is 'red', where a colour such as \"#ff8000\", \"#ff800080\", "
      "or rgb(255, 128, 0), or a list of 3 to 4 numbers was expected"
    },
    WrongProperty{
      "color: [1, 0]",
      "'color' of button 'start' is a list, where a colour such as \"#ff8000\", \"#ff800080\", "
      "or rgb(255, 128, 0), or a list of 3 to 4 numbers was expected"
    },
    WrongProperty{
      "font_size: 0",
      "'font_size' of button 'start' is 0, where a number of pixels above 0 was expected"
    },
    WrongProperty{
      "font_size: large",
      "'font_size' of button 'start' is 'large', where a number of pixels that is not below 0 was expected"
    },
    WrongProperty{
      "font_weight: heavy",
      "'font_weight' of button 'start' is 'heavy', where a number from 1 to 1000, normal, or bold "
      "was expected"
    },
    WrongProperty{
      "line_height: tall",
      "'line_height' of button 'start' is 'tall', where normal, a multiple of the size of the font such "
      "as 1.5, or a number of pixels such as \"24px\" was expected"
    },
    WrongProperty{
      "font_family: [a]",
      "'font_family' of button 'start' is a list, where text was expected"
    },
    WrongProperty{
      "background_image: 3",
      "'background_image' of button 'start' is a number, where a virtual path such as "
      "assets://ui/panel.png, or none was expected"
    },
    WrongProperty{
      "z_index: front",
      "'z_index' of button 'start' is 'front', where a whole number was expected"
    },
    WrongProperty{
      "text: [a, b]",
      "'text' of button 'start' is a list, where text was expected"
    },
    WrongProperty{
      "text: \"Health: {health\"",
      "'text' of button 'start' cannot be read: a '{' is never closed. Write '{{' for the bracket itself"
    },
    WrongProperty{
      "on_click: \"unlock(\"",
      "'on_click' of button 'start' is 'unlock(', which is no call of a function: the ( is not closed with )"
    },
    WrongProperty{
      "action: jump",
      "'action' of button 'start' is 'jump', where one of these was expected: none, close, scene"
    },
    WrongProperty{
      "enabled: maybe",
      "'enabled' of button 'start' is 'maybe', where true, false, or a value such as \"{can_start}\" "
      "was expected"
    },
    WrongProperty{
      "hidden: 3",
      "'hidden' of button 'start' is a number, where true, false, or a value such as \"{paused}\" "
      "was expected"
    },
    WrongProperty{
      "autofocus: yes please",
      "'autofocus' of button 'start' is text, where true or false was expected"
    },
    WrongProperty{
      "children: none",
      "'children' of button 'start' is text, where a list was expected"
    },
    WrongProperty{
      "hover: red",
      "'hover' of button 'start' is text, where a map was expected"
    }));

  TEST_F(UiFileTest, SaysWhatIsWrongInAState)
  {
    EXPECT_THAT(
      ProblemsUnderRoot(
        "- type: button\n"
        "  name: start\n"
        "  hover:\n"
        "    background_color: red\n"
        "    text: Go\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:7: 'background_color' of 'hover' of button 'start' is 'red', where a colour "
        "such as \"#ff8000\", \"#ff800080\", or rgb(255, 128, 0), or a list of 3 to 4 numbers was expected",
        StartsWith(
          "assets://ui/test.ui.yml:8: 'text' is not known to 'hover' of button 'start'. Known are: "
          "display, position,")));
  }

  TEST_F(UiFileTest, SaysWhatIsWrongWithABar)
  {
    EXPECT_THAT(
      ProblemsUnderRoot("- type: bar\n  name: health\n  value: full\n  max: [100]\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:6: 'value' of bar 'health' is 'full', where a number or a value such as "
        "\"{health}\" was expected",
        "assets://ui/test.ui.yml:7: 'max' of bar 'health' is a list, where a number or a value such as "
        "\"{health}\" was expected"));
  }

  TEST_F(UiFileTest, SaysThatAnImageHasNoSource)
  {
    EXPECT_THAT(
      ProblemsUnderRoot("- type: image\n  name: icon\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:4: image 'icon' has no 'src', where the virtual path of an image "
        "was expected"));
  }

  TEST_F(UiFileTest, SaysThatAButtonHasBothTextAndChildren)
  {
    EXPECT_THAT(
      ProblemsUnderRoot(
        "- type: button\n"
        "  name: start\n"
        "  text: Start\n"
        "  children:\n"
        "    - type: label\n"
        "      text: Start\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:7: button 'start' has both 'text' and 'children', where one of them "
        "was expected"));
  }

  TEST_F(UiFileTest, SaysThatANameIsGivenTwice)
  {
    EXPECT_THAT(
      ProblemsUnderRoot(
        "- type: button\n"
        "  name: start\n"
        "- type: panel\n"
        "  children:\n"
        "    - type: label\n"
        "      name: start\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:8: label 'start' shares its name with the element at line 4. "
        "A name says which element an event is from, and is given once"));
  }

  TEST_F(UiFileTest, ReportsEveryProblemAndNotOnlyTheFirst)
  {
    const auto problems = ProblemsOf(
      "ui: test\n"
      "verison: 1\n"
      "root:\n"
      "  type: panel\n"
      "  widht: 100%\n"
      "  children:\n"
      "    - type: label\n"
      "      name: health\n"
      "      font_size: big\n"
      "      color: blue\n"
      "    - type: buton\n"
      "      children:\n"
      "        - type: bar\n"
      "          value: [1]\n");

    ASSERT_EQ(problems.size(), 6u) << _logger->Messages(LogLevel::Error);

    // from the top of the file to its end
    EXPECT_THAT(problems[0], StartsWith("assets://ui/test.ui.yml:2: 'verison' is not known to the user"));
    EXPECT_THAT(problems[1], StartsWith("assets://ui/test.ui.yml:5: 'widht' is not known to the root."));
    EXPECT_THAT(problems[2], StartsWith("assets://ui/test.ui.yml:9: 'font_size' of label 'health' is 'big'"));
    EXPECT_THAT(problems[3], StartsWith("assets://ui/test.ui.yml:10: 'color' of label 'health' is 'blue'"));
    EXPECT_THAT(problems[4], StartsWith("assets://ui/test.ui.yml:11: type 'buton' of child 2 of the root"));

    // what is below an element that could not be read is still read
    EXPECT_THAT(problems[5], StartsWith(
                  "assets://ui/test.ui.yml:14: 'value' of child 1 of child 2 of the root is a list"));

    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The user interface assets://ui/test.ui.yml has 6 problems and is not shown"));
  }

  TEST_F(UiFileTest, ReportsAProblemOnceThoughAnElementHasSeveralStates)
  {
    const auto problems = ProblemsUnderRoot(
      "- type: button\n"
      "  name: start\n"
      "  color: blue\n"
      "  hover:\n"
      "    color: red\n");

    EXPECT_EQ(problems.size(), 2u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(UiFileTest, ShowsNothingOfAFileThatIsWrong)
  {
    EXPECT_EQ(ShowUnderRoot("- type: label\n  name: health\n  colour: \"#ffffff\"\n"), -1);

    EXPECT_EQ(_ui->Find("health"), nullptr);

    Frame();
    EXPECT_TRUE(_renderer.batches.empty());
  }

  TEST_F(UiFileTest, AnchorsAndTagsAreRefusedAsInAScene)
  {
    const auto problems = ProblemsOf(
      "root:\n"
      "  type: panel\n"
      "  children:\n"
      "    - &shared\n"
      "      type: label\n"
      "    - *shared\n");

    ASSERT_EQ(problems.size(), 1u);
    EXPECT_THAT(problems[0], StartsWith("assets://ui/test.ui.yml"));
  }

  // kinds of elements of a game

  class Minimap final : public neon::UiElement
  {
  public:
    float zoom = 1.0f;

    void ApplyDefaults(UiStyle &style) const override
    {
      style.layout.width = LayoutLength::Pixels(128);
      style.layout.height = LayoutLength::Pixels(128);
    }

    void ReadAttributes(const neon::DataReader &reader) override
    {
      reader.Read("zoom", zoom);
    }
  };

  TEST_F(UiFileTest, AGameAddsAKindOfElementByAddingOneClass)
  {
    _ui->GetElementTypes().Add<Minimap>("minimap");

    ASSERT_GE(ShowUnderRoot(
      "- type: minimap\n"
      "  name: map\n"
      "  zoom: 2.5\n"
      "  position: absolute\n"
      "  top: 16\n"
      "  right: 16\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();

    const auto *map = dynamic_cast<const Minimap *>(&Element("map"));
    ASSERT_NE(map, nullptr);
    EXPECT_FLOAT_EQ(map->zoom, 2.5f);
    ExpectBox("map", 1920 - 16 - 128, 16, 128, 128);

    EXPECT_THAT(
      ProblemsUnderRoot("- type: compass\n"),
      ElementsAre(
        "assets://ui/test.ui.yml:4: type 'compass' of child 1 of the root is not known. Known are: "
        "input, textarea, checkbox, radio, toggle, slider, select, panel, label, image, button, bar, minimap"));
  }
}
