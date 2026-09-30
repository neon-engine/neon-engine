#include "ui-style.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// The properties of behaviour, read from values without a file. What is
// expected is what the specifications of CSS say a value means.

namespace
{
  using neon::DataReader;
  using neon::DataValue;
  using neon::ReadUiStyle;
  using neon::UiAnimationDirection;
  using neon::UiAnimationFillMode;
  using neon::UiAnimationPlayState;
  using neon::UiCursor;
  using neon::UiOverflow;
  using neon::UiScrollBehavior;
  using neon::UiScrollbarWidth;
  using neon::UiScrollDrag;
  using neon::UiStyle;
  using neon::UiTimingFunction;
  using neon::UiVisibility;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  class UiBehaviourStyleTest : public ::testing::Test
  {
  protected:
    DataValue _map = DataValue::Map();
    std::vector<std::string> _errors;

    void Write(const std::string &name, const std::string &text, const std::size_t line = 0)
    {
      auto value = DataValue::Text(text);
      value.SetLine(line);
      _map.Set(name, value);
    }

    void WriteNumber(const std::string &name, const double number)
    {
      _map.Set(name, DataValue::Number(number));
    }

    void WriteList(const std::string &name, const std::vector<std::string> &items)
    {
      auto list = DataValue::List();
      for (const auto &item : items) { list.Add(DataValue::Text(item)); }
      _map.Set(name, list);
    }

    UiStyle Read(UiStyle style = {})
    {
      const DataReader reader(_map, "test.ui.yml", "panel 'box'", _errors);
      ReadUiStyle(reader, style);
      reader.Finish();
      return style;
    }

    /// What is wrong with one property that holds a text.
    std::vector<std::string> ProblemsOf(const std::string &name, const std::string &text)
    {
      _map = DataValue::Map();
      _errors.clear();
      Write(name, text, 7);
      Read();
      return _errors;
    }
  };

  TEST_F(UiBehaviourStyleTest, StartsWithTheInitialValuesOfCss)
  {
    const UiStyle style;

    EXPECT_EQ(style.visibility, UiVisibility::Visible);
    EXPECT_EQ(style.cursor, UiCursor::Auto);
    EXPECT_EQ(style.overflow_x, UiOverflow::Visible);
    EXPECT_EQ(style.overflow_y, UiOverflow::Visible);
    EXPECT_EQ(style.scrollbar_width, UiScrollbarWidth::Auto);
    EXPECT_FALSE(style.scrollbar_thumb_color.has_value());
    EXPECT_EQ(style.scroll_behavior, UiScrollBehavior::Auto);
    EXPECT_EQ(style.scroll_drag, UiScrollDrag::None);
    EXPECT_FALSE(style.caret_color.has_value());

    // transition: all 0s ease 0s
    EXPECT_THAT(style.transitions.properties, ElementsAre("all"));
    EXPECT_THAT(style.transitions.durations, ElementsAre(0.0f));
    EXPECT_THAT(style.transitions.delays, ElementsAre(0.0f));
    EXPECT_THAT(style.transitions.timing_functions, ElementsAre(UiTimingFunction::Ease()));
    EXPECT_FALSE(style.transitions.IsUsed());

    // animation: none 0s ease 0s 1 normal none running
    EXPECT_THAT(style.animations.names, IsEmpty());
    EXPECT_THAT(style.animations.durations, ElementsAre(0.0f));
    EXPECT_THAT(style.animations.iteration_counts, ElementsAre(1.0f));
    EXPECT_THAT(style.animations.directions, ElementsAre(UiAnimationDirection::Normal));
    EXPECT_THAT(style.animations.fill_modes, ElementsAre(UiAnimationFillMode::None));
    EXPECT_THAT(style.animations.play_states, ElementsAre(UiAnimationPlayState::Running));
  }

  TEST_F(UiBehaviourStyleTest, LeavesEverythingAsItIsWhenNothingIsWritten)
  {
    UiStyle before;
    before.visibility = UiVisibility::Hidden;
    before.cursor = UiCursor::Pointer;
    before.overflow_y = UiOverflow::Scroll;
    before.transitions.durations = {0.5f};
    before.animations.names = {"fade"};

    const UiStyle after = Read(before);

    EXPECT_EQ(after.visibility, UiVisibility::Hidden);
    EXPECT_EQ(after.cursor, UiCursor::Pointer);
    EXPECT_EQ(after.overflow_y, UiOverflow::Scroll);
    EXPECT_EQ(after.transitions, before.transitions);
    EXPECT_EQ(after.animations, before.animations);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(UiBehaviourStyleTest, ReadsVisibility)
  {
    Write("visibility", "hidden");
    EXPECT_EQ(Read().visibility, UiVisibility::Hidden);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(UiBehaviourStyleTest, ReadsEveryShapeOfTheCursor)
  {
    const std::pair<const char *, UiCursor> shapes[] = {
      {"auto", UiCursor::Auto},
      {"default", UiCursor::Default},
      {"pointer", UiCursor::Pointer},
      {"text", UiCursor::Text},
      {"wait", UiCursor::Wait},
      {"progress", UiCursor::Progress},
      {"crosshair", UiCursor::Crosshair},
      {"move", UiCursor::Move},
      {"not-allowed", UiCursor::NotAllowed},
      {"ew-resize", UiCursor::EwResize},
      {"ns-resize", UiCursor::NsResize},
      {"nesw-resize", UiCursor::NeswResize},
      {"nwse-resize", UiCursor::NwseResize},
      {"grab", UiCursor::Grab},
      {"grabbing", UiCursor::Grabbing},
      {"none", UiCursor::None}
    };

    for (const auto &[word, shape] : shapes)
    {
      Write("cursor", word);
      EXPECT_EQ(Read().cursor, shape) << word;
    }
    EXPECT_THAT(_errors, IsEmpty());
  }

  // overflow

  TEST_F(UiBehaviourStyleTest, TakesOverflowForBothSides)
  {
    Write("overflow", "scroll");

    const UiStyle style = Read();
    EXPECT_EQ(style.overflow, UiOverflow::Scroll);
    EXPECT_EQ(style.overflow_x, UiOverflow::Scroll);
    EXPECT_EQ(style.overflow_y, UiOverflow::Scroll);
    EXPECT_TRUE(style.ScrollsX());
    EXPECT_TRUE(style.ScrollsY());
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(UiBehaviourStyleTest, ReadsEachSideOfOverflowByItself)
  {
    Write("overflow_x", "hidden");
    Write("overflow_y", "auto");

    const UiStyle style = Read();
    EXPECT_EQ(style.overflow_x, UiOverflow::Hidden);
    EXPECT_EQ(style.overflow_y, UiOverflow::Auto);

    EXPECT_TRUE(style.ClipsX());
    EXPECT_FALSE(style.ScrollsX());
    EXPECT_TRUE(style.ClipsY());
    EXPECT_TRUE(style.ScrollsY());
  }

  TEST_F(UiBehaviourStyleTest, TakesASideThatIsVisibleNextToOneThatIsNotForAuto)
  {
    // https://www.w3.org/TR/css-overflow-3/#overflow-properties
    Write("overflow_y", "scroll");

    const UiStyle scrolled = Read();
    EXPECT_EQ(scrolled.overflow_x, UiOverflow::Visible);
    EXPECT_EQ(scrolled.OverflowX(), UiOverflow::Auto);
    EXPECT_EQ(scrolled.OverflowY(), UiOverflow::Scroll);
    EXPECT_TRUE(scrolled.ClipsX());
    EXPECT_TRUE(scrolled.ScrollsX());

    Write("overflow_y", "visible");
    Write("overflow_x", "hidden");

    const UiStyle hidden = Read();
    EXPECT_EQ(hidden.OverflowX(), UiOverflow::Hidden);
    EXPECT_EQ(hidden.OverflowY(), UiOverflow::Auto);
    EXPECT_TRUE(hidden.ClipsY());
  }

  TEST_F(UiBehaviourStyleTest, ReadsTheShorthandInFrontOfWhatItStandsFor)
  {
    // wherever the two are written
    Write("overflow_y", "scroll");
    Write("overflow", "hidden");

    const UiStyle style = Read();
    EXPECT_EQ(style.overflow_x, UiOverflow::Hidden);
    EXPECT_EQ(style.overflow_y, UiOverflow::Scroll);
  }

  TEST_F(UiBehaviourStyleTest, CutsNothingOffThatIsVisible)
  {
    const UiStyle style;
    EXPECT_FALSE(style.ClipsX());
    EXPECT_FALSE(style.ClipsY());
    EXPECT_FALSE(style.ScrollsX());
    EXPECT_FALSE(style.ScrollsY());
  }

  // scrollbars

  TEST_F(UiBehaviourStyleTest, ReadsTheScrollbar)
  {
    Write("scrollbar_width", "thin");
    Write("scrollbar_color", "#ff0000 #00ff0080");
    Write("scroll_behavior", "smooth");
    Write("scroll_drag", "inertia");

    const UiStyle style = Read();
    EXPECT_THAT(_errors, IsEmpty());

    EXPECT_EQ(style.scrollbar_width, UiScrollbarWidth::Thin);
    EXPECT_EQ(style.scroll_behavior, UiScrollBehavior::Smooth);
    EXPECT_EQ(style.scroll_drag, UiScrollDrag::Inertia);

    ASSERT_TRUE(style.scrollbar_thumb_color.has_value());
    EXPECT_FLOAT_EQ(style.scrollbar_thumb_color->r, 1.0f);
    EXPECT_FLOAT_EQ(style.scrollbar_thumb_color->g, 0.0f);

    ASSERT_TRUE(style.scrollbar_track_color.has_value());
    EXPECT_FLOAT_EQ(style.scrollbar_track_color->g, 1.0f);
    EXPECT_NEAR(style.scrollbar_track_color->a, 0.5f, 0.01f);
  }

  TEST_F(UiBehaviourStyleTest, ReadsTheColoursOfTheScrollbarFromAListAndFromFunctions)
  {
    WriteList("scrollbar_color", {"#ffffff", "#000000"});
    EXPECT_FLOAT_EQ(Read().scrollbar_thumb_color->r, 1.0f);

    Write("scrollbar_color", "rgb(255, 0, 0) rgba(0, 0, 255, 0.5)");
    const UiStyle style = Read();
    EXPECT_FLOAT_EQ(style.scrollbar_thumb_color->r, 1.0f);
    EXPECT_FLOAT_EQ(style.scrollbar_track_color->b, 1.0f);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(UiBehaviourStyleTest, TakesTheColoursOfTheScrollbarAwayWithAuto)
  {
    UiStyle before;
    before.scrollbar_thumb_color = neon::Color{1.0f, 0.0f, 0.0f, 1.0f};
    before.scrollbar_track_color = neon::Color{0.0f, 1.0f, 0.0f, 1.0f};

    Write("scrollbar_color", "auto");

    const UiStyle style = Read(before);
    EXPECT_FALSE(style.scrollbar_thumb_color.has_value());
    EXPECT_FALSE(style.scrollbar_track_color.has_value());
  }

  TEST_F(UiBehaviourStyleTest, ReadsTheColourOfTheCaret)
  {
    Write("caret_color", "#00ff00");
    const UiStyle green = Read();
    ASSERT_TRUE(green.caret_color.has_value());
    EXPECT_FLOAT_EQ(green.CaretColor().g, 1.0f);
    EXPECT_FLOAT_EQ(green.CaretColor().r, 0.0f);

    // without one it is that of the text
    UiStyle before = green;
    before.color = {1.0f, 0.0f, 0.0f, 1.0f};
    Write("caret_color", "auto");

    const UiStyle automatic = Read(before);
    EXPECT_FALSE(automatic.caret_color.has_value());
    EXPECT_FLOAT_EQ(automatic.CaretColor().r, 1.0f);
  }

  // transitions

  TEST_F(UiBehaviourStyleTest, ReadsTheLonghandsOfATransition)
  {
    Write("transition_property", "opacity, background_color");
    Write("transition_duration", "0.2s, 1s");
    Write("transition_delay", "100ms");
    Write("transition_timing_function", "ease-in, cubic-bezier(0.1, 0.7, 1, 0.1)");

    const UiStyle style = Read();
    EXPECT_THAT(_errors, IsEmpty());

    // a name is kept as CSS writes it
    EXPECT_THAT(style.transitions.properties, ElementsAre("opacity", "background-color"));
    EXPECT_THAT(style.transitions.durations, ElementsAre(0.2f, 1.0f));
    EXPECT_THAT(style.transitions.delays, ElementsAre(0.1f));
    EXPECT_THAT(
      style.transitions.timing_functions,
      ElementsAre(UiTimingFunction::EaseIn(), UiTimingFunction::CubicBezier(0.1f, 0.7f, 1.0f, 0.1f)));
    EXPECT_TRUE(style.transitions.IsUsed());
  }

  TEST_F(UiBehaviourStyleTest, ReadsAListOfAFileForTheLonghandsOfATransition)
  {
    WriteList("transition_property", {"opacity", "width"});
    WriteList("transition_duration", {"0.2s", "300ms"});

    const UiStyle style = Read();
    EXPECT_THAT(style.transitions.properties, ElementsAre("opacity", "width"));
    EXPECT_THAT(style.transitions.durations, ElementsAre(0.2f, 0.3f));
  }

  TEST_F(UiBehaviourStyleTest, TakesANumberWithoutAUnitForSeconds)
  {
    WriteNumber("transition_duration", 0.25);
    WriteNumber("animation_duration", 2);

    const UiStyle style = Read();
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_THAT(style.transitions.durations, ElementsAre(0.25f));
    EXPECT_THAT(style.animations.durations, ElementsAre(2.0f));
  }

  TEST_F(UiBehaviourStyleTest, ReadsTheShorthandOfATransition)
  {
    Write("transition", "opacity 0.2s ease-in 100ms");

    const UiStyle style = Read();
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_THAT(style.transitions.properties, ElementsAre("opacity"));
    EXPECT_THAT(style.transitions.durations, ElementsAre(0.2f));
    EXPECT_THAT(style.transitions.delays, ElementsAre(0.1f));
    EXPECT_THAT(style.transitions.timing_functions, ElementsAre(UiTimingFunction::EaseIn()));
  }

  TEST_F(UiBehaviourStyleTest, ReadsSeveralTransitionsFromTheShorthand)
  {
    Write("transition", "opacity 0.2s, background-color 1s linear, width 300ms steps(3, jump-start) 1s");

    const UiStyle style = Read();
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_THAT(style.transitions.properties, ElementsAre("opacity", "background-color", "width"));
    EXPECT_THAT(style.transitions.durations, ElementsAre(0.2f, 1.0f, 0.3f));
    EXPECT_THAT(style.transitions.delays, ElementsAre(0.0f, 0.0f, 1.0f));
    EXPECT_THAT(
      style.transitions.timing_functions,
      ElementsAre(
        UiTimingFunction::Ease(),
        UiTimingFunction::Linear(),
        UiTimingFunction::Steps(3, UiTimingFunction::StepPosition::JumpStart)));
  }

  TEST_F(UiBehaviourStyleTest, TakesWhatTheShorthandLeavesOutForItsInitialValue)
  {
    Write("transition", "0.5s");

    const UiStyle style = Read();
    EXPECT_THAT(style.transitions.properties, ElementsAre("all"));
    EXPECT_THAT(style.transitions.durations, ElementsAre(0.5f));
    EXPECT_THAT(style.transitions.delays, ElementsAre(0.0f));
    EXPECT_THAT(style.transitions.timing_functions, ElementsAre(UiTimingFunction::Ease()));
  }

  TEST_F(UiBehaviourStyleTest, TakesTheFirstTimeOfTheShorthandForTheDurationInAnyOrder)
  {
    Write("transition", "ease-out 1s width 2s");

    const UiStyle style = Read();
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_THAT(style.transitions.properties, ElementsAre("width"));
    EXPECT_THAT(style.transitions.durations, ElementsAre(1.0f));
    EXPECT_THAT(style.transitions.delays, ElementsAre(2.0f));
  }

  TEST_F(UiBehaviourStyleTest, ReadsTheLonghandsOfATransitionBehindItsShorthand)
  {
    Write("transition_duration", "2s");
    Write("transition", "opacity 0.2s");

    const UiStyle style = Read();
    EXPECT_THAT(style.transitions.properties, ElementsAre("opacity"));
    EXPECT_THAT(style.transitions.durations, ElementsAre(2.0f));
  }

  TEST_F(UiBehaviourStyleTest, TakesNoneForNoTransition)
  {
    Write("transition", "none");

    const UiStyle style = Read();
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_THAT(style.transitions.properties, ElementsAre("none"));
    EXPECT_FALSE(style.transitions.IsUsed());
  }

  TEST_F(UiBehaviourStyleTest, TakesATransitionThatOnlyWaitsForOneThatIsUsed)
  {
    Write("transition_delay", "1s");
    EXPECT_TRUE(Read().transitions.IsUsed());
  }

  TEST_F(UiBehaviourStyleTest, AllowsADelayBelowZeroAndNoDurationBelowZero)
  {
    Write("transition_delay", "-1s");
    EXPECT_THAT(Read().transitions.delays, ElementsAre(-1.0f));
    EXPECT_THAT(_errors, IsEmpty());

    EXPECT_THAT(
      ProblemsOf("transition_duration", "-1s"),
      ElementsAre(
        "test.ui.yml:7: 'transition_duration' of panel 'box' is '-1s', where a time such as 0.2s or 150ms "
        "that is not below 0, or a list of them was expected"));
  }

  // animations

  TEST_F(UiBehaviourStyleTest, ReadsTheLonghandsOfAnAnimation)
  {
    Write("animation_name", "fade-in, slide");
    Write("animation_duration", "0.3s, 2s");
    Write("animation_delay", "0s, 500ms");
    Write("animation_iteration_count", "2, infinite");
    Write("animation_direction", "alternate, reverse");
    Write("animation_fill_mode", "both, forwards");
    Write("animation_play_state", "running, paused");
    Write("animation_timing_function", "linear, ease-out");

    const UiStyle style = Read();
    EXPECT_THAT(_errors, IsEmpty());

    EXPECT_THAT(style.animations.names, ElementsAre("fade-in", "slide"));
    EXPECT_THAT(style.animations.durations, ElementsAre(0.3f, 2.0f));
    EXPECT_THAT(style.animations.delays, ElementsAre(0.0f, 0.5f));
    EXPECT_THAT(style.animations.iteration_counts, ElementsAre(2.0f, -1.0f));
    EXPECT_THAT(
      style.animations.directions,
      ElementsAre(UiAnimationDirection::Alternate, UiAnimationDirection::Reverse));
    EXPECT_THAT(
      style.animations.fill_modes,
      ElementsAre(UiAnimationFillMode::Both, UiAnimationFillMode::Forwards));
    EXPECT_THAT(
      style.animations.play_states,
      ElementsAre(UiAnimationPlayState::Running, UiAnimationPlayState::Paused));
    EXPECT_THAT(
      style.animations.timing_functions,
      ElementsAre(UiTimingFunction::Linear(), UiTimingFunction::EaseOut()));
  }

  TEST_F(UiBehaviourStyleTest, KeepsTheNameOfAnAnimationAsItIsWritten)
  {
    Write("animation_name", "Fade_In");
    EXPECT_THAT(Read().animations.names, ElementsAre("Fade_In"));
  }

  TEST_F(UiBehaviourStyleTest, ReadsEveryDirectionAndFillMode)
  {
    Write("animation_direction", "normal, reverse, alternate, alternate-reverse");
    Write("animation_fill_mode", "none, forwards, backwards, both");

    const UiStyle style = Read();
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_THAT(
      style.animations.directions,
      ElementsAre(
        UiAnimationDirection::Normal, UiAnimationDirection::Reverse, UiAnimationDirection::Alternate,
        UiAnimationDirection::AlternateReverse));
    EXPECT_THAT(
      style.animations.fill_modes,
      ElementsAre(
        UiAnimationFillMode::None, UiAnimationFillMode::Forwards, UiAnimationFillMode::Backwards,
        UiAnimationFillMode::Both));
  }

  TEST_F(UiBehaviourStyleTest, ReadsTheShorthandOfAnAnimation)
  {
    Write("animation", "fade-in 0.3s ease-out 100ms 2 alternate both paused");

    const UiStyle style = Read();
    EXPECT_THAT(_errors, IsEmpty());

    EXPECT_THAT(style.animations.names, ElementsAre("fade-in"));
    EXPECT_THAT(style.animations.durations, ElementsAre(0.3f));
    EXPECT_THAT(style.animations.delays, ElementsAre(0.1f));
    EXPECT_THAT(style.animations.iteration_counts, ElementsAre(2.0f));
    EXPECT_THAT(style.animations.directions, ElementsAre(UiAnimationDirection::Alternate));
    EXPECT_THAT(style.animations.fill_modes, ElementsAre(UiAnimationFillMode::Both));
    EXPECT_THAT(style.animations.play_states, ElementsAre(UiAnimationPlayState::Paused));
    EXPECT_THAT(style.animations.timing_functions, ElementsAre(UiTimingFunction::EaseOut()));
  }

  TEST_F(UiBehaviourStyleTest, ReadsTheShorthandOfAnAnimationInAnyOrder)
  {
    Write("animation", "infinite 2s pulse linear");

    const UiStyle style = Read();
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_THAT(style.animations.names, ElementsAre("pulse"));
    EXPECT_THAT(style.animations.durations, ElementsAre(2.0f));
    EXPECT_THAT(style.animations.iteration_counts, ElementsAre(-1.0f));
    EXPECT_THAT(style.animations.timing_functions, ElementsAre(UiTimingFunction::Linear()));
  }

  TEST_F(UiBehaviourStyleTest, ReadsSeveralAnimationsFromTheShorthand)
  {
    Write("animation", "fade-in 0.3s, slide 1s ease-in-out infinite alternate");

    const UiStyle style = Read();
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_THAT(style.animations.names, ElementsAre("fade-in", "slide"));
    EXPECT_THAT(style.animations.durations, ElementsAre(0.3f, 1.0f));
    EXPECT_THAT(style.animations.iteration_counts, ElementsAre(1.0f, -1.0f));
    EXPECT_THAT(
      style.animations.directions,
      ElementsAre(UiAnimationDirection::Normal, UiAnimationDirection::Alternate));
  }

  TEST_F(UiBehaviourStyleTest, TakesNoneForNoAnimation)
  {
    UiStyle before;
    before.animations.names = {"fade"};

    Write("animation", "none");

    const UiStyle style = Read(before);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_THAT(style.animations.names, ElementsAre("none"));
    EXPECT_THAT(style.animations.fill_modes, ElementsAre(UiAnimationFillMode::None));
  }

  TEST_F(UiBehaviourStyleTest, ReadsAPartOfARunAsHowOftenAnAnimationRuns)
  {
    Write("animation_iteration_count", "0.5");
    EXPECT_THAT(Read().animations.iteration_counts, ElementsAre(0.5f));

    WriteNumber("animation_iteration_count", 3);
    EXPECT_THAT(Read().animations.iteration_counts, ElementsAre(3.0f));
    EXPECT_THAT(_errors, IsEmpty());
  }

  // what is wrong

  TEST_F(UiBehaviourStyleTest, SaysWhatIsWrongWithAWord)
  {
    EXPECT_THAT(
      ProblemsOf("visibility", "collapse"),
      ElementsAre(
        "test.ui.yml:7: 'visibility' of panel 'box' is 'collapse', where one of these was expected: "
        "visible, hidden"));

    EXPECT_THAT(
      ProblemsOf("cursor", "hand"),
      ElementsAre(
        "test.ui.yml:7: 'cursor' of panel 'box' is 'hand', where one of these was expected: auto, default, "
        "pointer, text, wait, progress, crosshair, move, not-allowed, ew-resize, ns-resize, nesw-resize, "
        "nwse-resize, grab, grabbing, none"));

    EXPECT_THAT(
      ProblemsOf("overflow_x", "clip"),
      ElementsAre(
        "test.ui.yml:7: 'overflow_x' of panel 'box' is 'clip', where one of these was expected: visible, "
        "hidden, scroll, auto"));

    EXPECT_THAT(
      ProblemsOf("overflow", "clip"),
      ElementsAre(
        "test.ui.yml:7: 'overflow' of panel 'box' is 'clip', where one of these was expected: visible, "
        "hidden, scroll, auto"));

    EXPECT_THAT(
      ProblemsOf("scrollbar_width", "wide"),
      ElementsAre(
        "test.ui.yml:7: 'scrollbar_width' of panel 'box' is 'wide', where one of these was expected: auto, "
        "thin, none"));

    EXPECT_THAT(
      ProblemsOf("scroll_behavior", "fast"),
      ElementsAre(
        "test.ui.yml:7: 'scroll_behavior' of panel 'box' is 'fast', where one of these was expected: auto, "
        "smooth"));

    EXPECT_THAT(
      ProblemsOf("scroll_drag", "yes"),
      ElementsAre(
        "test.ui.yml:7: 'scroll_drag' of panel 'box' is 'yes', where one of these was expected: none, "
        "inertia"));
  }

  TEST_F(UiBehaviourStyleTest, SaysWhatIsWrongWithAColour)
  {
    EXPECT_THAT(
      ProblemsOf("scrollbar_color", "#ffffff"),
      ElementsAre(
        "test.ui.yml:7: 'scrollbar_color' of panel 'box' is '#ffffff', where auto, or two colours: that of "
        "what is dragged and that of what it is dragged along, such as \"#ffffff80 #00000040\" was "
        "expected"));

    EXPECT_THAT(
      ProblemsOf("scrollbar_color", "#ffffff bright"),
      ElementsAre(::testing::StartsWith("test.ui.yml:7: 'scrollbar_color' of panel 'box' is '#ffffff bright'")));

    EXPECT_THAT(
      ProblemsOf("caret_color", "bright"),
      ElementsAre(
        "test.ui.yml:7: 'caret_color' of panel 'box' is 'bright', where auto, or a colour such as "
        "\"#ff8000\" or rgb(255, 128, 0) was expected"));
  }

  TEST_F(UiBehaviourStyleTest, SaysWhatIsWrongWithATransition)
  {
    EXPECT_THAT(
      ProblemsOf("transition_duration", "fast"),
      ElementsAre(
        "test.ui.yml:7: 'transition_duration' of panel 'box' is 'fast', where a time such as 0.2s or 150ms "
        "that is not below 0, or a list of them was expected"));

    EXPECT_THAT(
      ProblemsOf("transition_delay", "0.2s, soon"),
      ElementsAre(
        "test.ui.yml:7: 'transition_delay' of panel 'box' is '0.2s, soon', where a time such as 0.2s or "
        "150ms, or a list of them was expected"));

    EXPECT_THAT(
      ProblemsOf("transition_timing_function", "bouncy"),
      ElementsAre(
        "test.ui.yml:7: 'transition_timing_function' of panel 'box' is 'bouncy', where linear, ease, "
        "ease-in, ease-out, ease-in-out, step-start, step-end, cubic-bezier(), or steps(), or a list of "
        "them was expected"));

    EXPECT_THAT(
      ProblemsOf("transition_property", "opacity, 2s"),
      ElementsAre(
        "test.ui.yml:7: 'transition_property' of panel 'box' is 'opacity, 2s', where all, none, or the "
        "names of properties such as opacity, background-color was expected"));

    const std::string expected =
      ", where none, or for each transition a property, a duration, a timing function, and a delay, such "
      "as \"opacity 0.2s ease-in, color 1s\" was expected";

    EXPECT_THAT(
      ProblemsOf("transition", "opacity 1s 2s 3s"),
      ElementsAre("test.ui.yml:7: 'transition' of panel 'box' is 'opacity 1s 2s 3s'" + expected));

    EXPECT_THAT(
      ProblemsOf("transition", "opacity width 1s"),
      ElementsAre("test.ui.yml:7: 'transition' of panel 'box' is 'opacity width 1s'" + expected));

    EXPECT_THAT(
      ProblemsOf("transition", "opacity 1s,"),
      ElementsAre("test.ui.yml:7: 'transition' of panel 'box' is 'opacity 1s,'" + expected));

    EXPECT_THAT(
      ProblemsOf("transition", "none, opacity 1s"),
      ElementsAre("test.ui.yml:7: 'transition' of panel 'box' is 'none, opacity 1s'" + expected));
  }

  TEST_F(UiBehaviourStyleTest, SaysWhatIsWrongWithAnAnimation)
  {
    EXPECT_THAT(
      ProblemsOf("animation_iteration_count", "-1"),
      ElementsAre(
        "test.ui.yml:7: 'animation_iteration_count' of panel 'box' is '-1', where infinite, or a number "
        "that is not below 0, or a list of them was expected"));

    EXPECT_THAT(
      ProblemsOf("animation_direction", "backwards"),
      ElementsAre(
        "test.ui.yml:7: 'animation_direction' of panel 'box' is 'backwards', where one of these, or a list "
        "of them: normal, reverse, alternate, alternate-reverse was expected"));

    EXPECT_THAT(
      ProblemsOf("animation_fill_mode", "reverse"),
      ElementsAre(
        "test.ui.yml:7: 'animation_fill_mode' of panel 'box' is 'reverse', where one of these, or a list of "
        "them: none, forwards, backwards, both was expected"));

    EXPECT_THAT(
      ProblemsOf("animation_play_state", "stopped"),
      ElementsAre(
        "test.ui.yml:7: 'animation_play_state' of panel 'box' is 'stopped', where one of these, or a list "
        "of them: running, paused was expected"));

    EXPECT_THAT(
      ProblemsOf("animation_name", "2fast"),
      ElementsAre(
        "test.ui.yml:7: 'animation_name' of panel 'box' is '2fast', where none, or the names of animations "
        "was expected"));

    EXPECT_THAT(
      ProblemsOf("animation", "fade 1s 2s 3s"),
      ElementsAre(::testing::StartsWith("test.ui.yml:7: 'animation' of panel 'box' is 'fade 1s 2s 3s', where ")));
  }

  TEST_F(UiBehaviourStyleTest, LeavesAPropertyAsItWasWhenWhatIsWrittenCannotBeRead)
  {
    UiStyle before;
    before.cursor = UiCursor::Pointer;
    before.transitions.durations = {0.5f};
    before.animations.names = {"fade"};

    Write("cursor", "hand");
    Write("transition_duration", "fast");
    Write("animation", "fade 1s 2s 3s");

    const UiStyle after = Read(before);
    EXPECT_EQ(_errors.size(), 3u);
    EXPECT_EQ(after.cursor, UiCursor::Pointer);
    EXPECT_THAT(after.transitions.durations, ElementsAre(0.5f));
    EXPECT_THAT(after.animations.names, ElementsAre("fade"));
  }

  TEST_F(UiBehaviourStyleTest, IsListedBehindThePropertiesThatWereThereBeforeIt)
  {
    Write("colour", "red", 3);
    Read();

    ASSERT_EQ(_errors.size(), 1u);
    EXPECT_THAT(_errors[0], ::testing::StartsWith(
                  "test.ui.yml:3: 'colour' is not known to panel 'box'. Known are: display, position, "
                  "box_sizing, width, height,"));
    EXPECT_THAT(_errors[0], ::testing::HasSubstr(", visibility, cursor, overflow_x, overflow_y, "));
    EXPECT_THAT(_errors[0], ::testing::HasSubstr(", transition, transition_property, "));
    EXPECT_THAT(_errors[0], ::testing::HasSubstr(", animation, animation_name, "));
  }
} // namespace
