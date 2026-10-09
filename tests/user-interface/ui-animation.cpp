#include "ui-fixture.hpp"

// Transitions and animations. What is expected is what
// https://www.w3.org/TR/css-transitions-1/ and
// https://www.w3.org/TR/css-animations-1/ say. The values of the timing
// functions were worked out from their definition with another program.

namespace
{
  using neon::Color;
  using neon::LayoutLength;
  using neon::UiElementEvent;
  using neon::UiHandle;
  using neon::UiStyle;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  constexpr float near = 0.0005f;

  class UiAnimationTest : public UiTest
  {
  protected:
    /// Shows a panel `box` with the sheet, which holds the transitions
    /// and the keyframes.
    void ShowBox(const std::string &css, const std::string &of_box = "")
    {
      WriteAsset("ui/theme.css", css);

      ASSERT_GE(Show(
        "ui: test\n"
        "styles: [theme.css]\n"
        "root:\n"
        "  type: panel\n"
        "  name: root\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  children:\n"
        "    - type: panel\n"
        "      name: box\n" + Indented(of_box, "      ")), 0) << _logger->Messages(LogLevel::Error);

      Step(0.0);
    }

    /// A frame in which a time passes.
    void Step(const double seconds)
    {
      _ui->Advance(seconds);
      Frame();
    }

    [[nodiscard]] UiHandle Box() const
    {
      return _ui->FindByName("box");
    }

    [[nodiscard]] const UiStyle &Style(const std::string &name = "box") const
    {
      return Element(name).GetStyle();
    }

    [[nodiscard]] float Opacity() const
    {
      return Style().opacity;
    }

    [[nodiscard]] std::vector<std::string> Happened() const
    {
      std::vector<std::string> happened;
      for (const auto &event : _ui->GetElementEvents())
      {
        happened.push_back(event.name + " " + event.value);
      }
      return happened;
    }
  };

  // transitions

  TEST_F(UiAnimationTest, MovesAPropertyToItsNewValueOverTime)
  {
    ShowBox(
      "#box { opacity: 0; transition: opacity 1s linear; }\n"
      "#box.shown { opacity: 1; }\n");

    EXPECT_FLOAT_EQ(Opacity(), 0.0f);

    _ui->AddClass(Box(), "shown");
    Step(0.0);

    // what the cascade came to is where it is going, and what is drawn is
    // where it is
    EXPECT_FLOAT_EQ(Element("box").GetComputedStyle().opacity, 1.0f);
    EXPECT_FLOAT_EQ(Opacity(), 0.0f);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.75f, near);

    Step(0.25);
    EXPECT_FLOAT_EQ(Opacity(), 1.0f);

    Step(0.25);
    EXPECT_FLOAT_EQ(Opacity(), 1.0f);
  }

  TEST_F(UiAnimationTest, DoesNotMoveWhatAnElementStartsWith)
  {
    ShowBox("#box { opacity: 0.5; width: 100px; transition: all 1s linear; }");

    EXPECT_FLOAT_EQ(Opacity(), 0.5f);
    EXPECT_EQ(Style().layout.width, LayoutLength::Pixels(100.0f));
    EXPECT_EQ(_ui->GetMovingCount(), 0u);
  }

  TEST_F(UiAnimationTest, FollowsTheTimingFunction)
  {
    ShowBox(
      "#box { opacity: 0; transition: opacity 1s ease; }\n"
      "#box.shown { opacity: 1; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.408511f, near);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.802403f, near);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.960459f, near);
  }

  TEST_F(UiAnimationTest, StartsWithEaseWhenNothingElseIsSaid)
  {
    ShowBox(
      "#box { opacity: 0; transition-property: opacity; transition-duration: 2s; }\n"
      "#box.shown { opacity: 1; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    Step(1.0);

    EXPECT_NEAR(Opacity(), 0.802403f, near);
  }

  TEST_F(UiAnimationTest, JumpsInSteps)
  {
    ShowBox(
      "#box { opacity: 0; transition: opacity 1s steps(4, jump-end); }\n"
      "#box.shown { opacity: 1; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);

    Step(0.2);
    EXPECT_FLOAT_EQ(Opacity(), 0.0f);

    Step(0.1);
    EXPECT_FLOAT_EQ(Opacity(), 0.25f);

    Step(0.3);
    EXPECT_FLOAT_EQ(Opacity(), 0.5f);

    Step(0.3);
    EXPECT_FLOAT_EQ(Opacity(), 0.75f);

    Step(0.2);
    EXPECT_FLOAT_EQ(Opacity(), 1.0f);
  }

  TEST_F(UiAnimationTest, WaitsBeforeItStarts)
  {
    ShowBox(
      "#box { opacity: 0; transition: opacity 1s linear 0.5s; }\n"
      "#box.shown { opacity: 1; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);

    Step(0.25);
    EXPECT_FLOAT_EQ(Opacity(), 0.0f);

    Step(0.25);
    EXPECT_FLOAT_EQ(Opacity(), 0.0f);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    Step(0.5);
    EXPECT_FLOAT_EQ(Opacity(), 1.0f);
  }

  TEST_F(UiAnimationTest, StartsPartOfTheWayInWithADelayBelowZero)
  {
    ShowBox(
      "#box { opacity: 0; transition: opacity 1s linear -0.5s; }\n"
      "#box.shown { opacity: 1; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.75f, near);

    Step(0.25);
    EXPECT_FLOAT_EQ(Opacity(), 1.0f);
  }

  TEST_F(UiAnimationTest, MovesOnlyThePropertiesItNames)
  {
    ShowBox(
      "#box { opacity: 0; z-index: 0; outline-width: 0; transition: opacity 1s linear; }\n"
      "#box.shown { opacity: 1; z-index: 10; outline-width: 10px; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    Step(0.5);

    EXPECT_NEAR(Opacity(), 0.5f, near);
    EXPECT_EQ(Style().z_index, 10);
    EXPECT_FLOAT_EQ(Style().outline_width, 10.0f);
  }

  TEST_F(UiAnimationTest, MovesEveryPropertyWithAll)
  {
    ShowBox(
      "#box { opacity: 0; z-index: 0; outline-width: 0; transition: all 1s linear; }\n"
      "#box.shown { opacity: 1; z-index: 10; outline-width: 10px; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    Step(0.5);

    EXPECT_NEAR(Opacity(), 0.5f, near);
    EXPECT_EQ(Style().z_index, 5);
    EXPECT_NEAR(Style().outline_width, 5.0f, near);
  }

  TEST_F(UiAnimationTest, MovesNothingWithNone)
  {
    ShowBox(
      "#box { opacity: 0; transition: none 1s; }\n"
      "#box.shown { opacity: 1; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);

    EXPECT_FLOAT_EQ(Opacity(), 1.0f);
  }

  TEST_F(UiAnimationTest, GivesEveryPropertyItsOwnTime)
  {
    ShowBox(
      "#box { opacity: 0; outline-width: 0; transition: opacity 1s linear, outline-width 2s linear 1s; }\n"
      "#box.shown { opacity: 1; outline-width: 10px; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.5f, near);
    EXPECT_FLOAT_EQ(Style().outline_width, 0.0f);

    Step(1.5);
    EXPECT_FLOAT_EQ(Opacity(), 1.0f);
    EXPECT_NEAR(Style().outline_width, 5.0f, near);
  }

  TEST_F(UiAnimationTest, GoesThroughAListAgainThatIsShorterThanTheListOfProperties)
  {
    ShowBox(
      "#box {\n"
      "  opacity: 0; outline-width: 0; outline-offset: 0;\n"
      "  transition-property: opacity, outline-width, outline-offset;\n"
      "  transition-duration: 1s, 2s;\n"
      "  transition-timing-function: linear;\n"
      "}\n"
      "#box.shown { opacity: 1; outline-width: 10px; outline-offset: 10px; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    Step(0.5);

    // 1s, 2s, and 1s again
    EXPECT_NEAR(Opacity(), 0.5f, near);
    EXPECT_NEAR(Style().outline_width, 2.5f, near);
    EXPECT_NEAR(Style().outline_offset, 5.0f, near);
  }

  TEST_F(UiAnimationTest, TakesTheTransitionThatIsNamedLastForAProperty)
  {
    ShowBox(
      "#box { opacity: 0; transition: all 1s linear, opacity 2s linear; }\n"
      "#box.shown { opacity: 1; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    Step(1.0);

    EXPECT_NEAR(Opacity(), 0.5f, near);
  }

  TEST_F(UiAnimationTest, MovesWhatAShorthandStandsFor)
  {
    ShowBox(
      "#box { margin: 0; transition: margin 1s linear; }\n"
      "#box.shown { margin: 10px 20px; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    Step(0.5);

    EXPECT_EQ(Style().layout.margin.top, LayoutLength::Pixels(5.0f));
    EXPECT_EQ(Style().layout.margin.left, LayoutLength::Pixels(10.0f));
  }

  TEST_F(UiAnimationTest, MovesAColorWithItsAlphaMultipliedIn)
  {
    ShowBox(
      "#box { background-color: rgba(255, 0, 0, 0); transition: background-color 1s linear; }\n"
      "#box.shown { background-color: rgba(0, 0, 255, 1); }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    Step(0.5);

    // the red cannot be seen and adds nothing of its own
    const Color &color = Style().background_color;
    EXPECT_NEAR(color.r, 0.0f, near);
    EXPECT_NEAR(color.b, 1.0f, near);
    EXPECT_NEAR(color.a, 0.5f, near);
  }

  TEST_F(UiAnimationTest, MovesALengthAndPlacesWhatItResizes)
  {
    ShowBox(
      "#box { width: 100px; height: 50px; transition: width 1s linear; }\n"
      "#box.wide { width: 300px; }\n",
      "children:\n"
      "  - type: panel\n"
      "    name: inside\n"
      "    width: 50%\n"
      "    height: 10\n");

    ExpectBox("box", 0.0f, 0.0f, 100.0f, 50.0f);
    ExpectBox("inside", 0.0f, 0.0f, 50.0f, 10.0f);

    _ui->AddClass(Box(), "wide");
    Step(0.0);
    Step(0.5);

    ExpectBox("box", 0.0f, 0.0f, 200.0f, 50.0f);
    ExpectBox("inside", 0.0f, 0.0f, 100.0f, 10.0f);

    Step(0.5);
    ExpectBox("box", 0.0f, 0.0f, 300.0f, 50.0f);
  }

  TEST_F(UiAnimationTest, MovesBetweenPixelsAndAPercentage)
  {
    ShowBox(
      "#box { width: 100px; height: 50px; transition: width 1s linear; }\n"
      "#box.wide { width: 50%; }\n");

    _ui->AddClass(Box(), "wide");
    Step(0.0);
    Step(0.5);

    // half of 100, and half of half of 1920
    EXPECT_EQ(Style().layout.width, LayoutLength::Sum(50.0f, 25.0f));
    ExpectBox("box", 0.0f, 0.0f, 530.0f, 50.0f);
  }

  TEST_F(UiAnimationTest, ChangesAtOnceWhatCannotBeMoved)
  {
    ShowBox(
      "#box { width: 100px; flex-direction: row; background-image: none; transition: all 1s linear; }\n"
      "#box.other { width: auto; flex-direction: column; background-image: url(assets://ui/a.png); }\n");

    _ui->AddClass(Box(), "other");
    Step(0.0);

    EXPECT_TRUE(Style().layout.width.IsAuto());
    EXPECT_EQ(Style().layout.flex_direction, neon::FlexDirection::Column);
    EXPECT_EQ(Style().background_image, "assets://ui/a.png");
    EXPECT_EQ(_ui->GetMovingCount(), 0u);
  }

  TEST_F(UiAnimationTest, MovesWhenAStateChangesAProperty)
  {
    ShowBox(
      "#box { width: 100px; height: 100px; pointer-events: auto; opacity: 0.5; transition: opacity 1s linear; }\n"
      "#box:hover { opacity: 1; }\n");

    PointAt(50, 50);
    Step(0.0);
    EXPECT_FLOAT_EQ(Opacity(), 0.5f);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.75f, near);

    Step(0.5);
    EXPECT_FLOAT_EQ(Opacity(), 1.0f);
  }

  TEST_F(UiAnimationTest, MovesWhenTheGameChangesAProperty)
  {
    ShowBox("#box { opacity: 0; transition: opacity 1s linear; }");

    ASSERT_TRUE(_ui->Set(Box(), "opacity", "1"));
    Step(0.0);
    Step(0.25);

    EXPECT_NEAR(Opacity(), 0.25f, near);
  }

  TEST_F(UiAnimationTest, MovesWhatIsWrittenInAFileOfYaml)
  {
    ASSERT_GE(Show(
      "root:\n"
      "  type: button\n"
      "  name: box\n"
      "  text: a\n"
      "  width: 100\n"
      "  height: 100\n"
      "  opacity: 0.5\n"
      "  transition: opacity 1s linear\n"
      "  hover:\n"
      "    opacity: 1\n"), 0);
    Step(0.0);

    PointAt(50, 50);
    Step(0.0);
    Step(0.5);

    EXPECT_NEAR(Opacity(), 0.75f, near);
  }

  // transitions that are turned around

  TEST_F(UiAnimationTest, TurnsAroundFromWhereItIs)
  {
    ShowBox(
      "#box { opacity: 0; transition: opacity 1s linear; }\n"
      "#box.shown { opacity: 1; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    // halfway, it is sent back
    _ui->RemoveClass(Box(), "shown");
    Step(0.0);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    // it takes as long to get back as it took to get here: half a second
    // for half of the way
    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.0f, near);

    Step(0.25);
    EXPECT_FLOAT_EQ(Opacity(), 0.0f);
  }

  TEST_F(UiAnimationTest, TurnsAroundTwice)
  {
    ShowBox(
      "#box { opacity: 0; transition: opacity 1s linear; }\n"
      "#box.shown { opacity: 1; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    Step(0.5);

    _ui->RemoveClass(Box(), "shown");
    Step(0.0);
    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    // and forwards again, from a quarter: three quarters of a second are
    // left of the way
    _ui->AddClass(Box(), "shown");
    Step(0.0);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    Step(0.375);
    EXPECT_NEAR(Opacity(), 0.625f, near);

    Step(0.375);
    EXPECT_NEAR(Opacity(), 1.0f, near);
  }

  TEST_F(UiAnimationTest, GoesSomewhereElseFromWhereItIs)
  {
    ShowBox(
      "#box { opacity: 0; transition: opacity 1s linear; }\n"
      "#box.shown { opacity: 1; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    Step(0.5);

    // neither where it came from nor where it was going
    ASSERT_TRUE(_ui->Set(Box(), "opacity", "0.25"));
    Step(0.0);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    // all of a second, for a way that is new
    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.375f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.25f, near);
  }

  TEST_F(UiAnimationTest, KeepsGoingWhenItIsSentWhereItIsGoing)
  {
    ShowBox(
      "#box { opacity: 0; z-index: 0; transition: opacity 1s linear; }\n"
      "#box.shown { opacity: 1; }\n"
      "#box.other { z-index: 3; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    Step(0.5);

    // the style is worked out again, and the opacity is what it was
    _ui->AddClass(Box(), "other");
    Step(0.25);

    EXPECT_NEAR(Opacity(), 0.75f, near);
    EXPECT_EQ(Style().z_index, 3);
  }

  // what is told of a transition

  TEST_F(UiAnimationTest, TellsThatATransitionEnded)
  {
    ShowBox(
      "#box { opacity: 0; outline-width: 0; transition: opacity 1s linear, outline-width 2s linear; }\n"
      "#box.shown { opacity: 1; outline-width: 4px; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);

    Step(0.5);
    EXPECT_THAT(Happened(), IsEmpty());

    Step(0.5);
    EXPECT_THAT(Happened(), ElementsAre("transition_ended opacity"));
    EXPECT_EQ(_ui->GetElementEvents()[0].target, Box());

    Step(0.5);
    EXPECT_THAT(Happened(), IsEmpty());

    Step(0.5);
    EXPECT_THAT(Happened(), ElementsAre("transition_ended outline-width"));

    Step(0.5);
    EXPECT_THAT(Happened(), IsEmpty());
  }

  TEST_F(UiAnimationTest, TellsNothingOfATransitionThatWasTurnedAroundUntilItIsBack)
  {
    ShowBox(
      "#box { opacity: 0; transition: opacity 1s linear; }\n"
      "#box.shown { opacity: 1; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    Step(0.5);

    _ui->RemoveClass(Box(), "shown");
    Step(0.0);
    Step(0.25);
    EXPECT_THAT(Happened(), IsEmpty());

    Step(0.25);
    EXPECT_THAT(Happened(), ElementsAre("transition_ended opacity"));
  }

  TEST_F(UiAnimationTest, DoesNoWorkOnceNothingMovesAnyMore)
  {
    ShowBox(
      "#box { opacity: 0; transition: opacity 1s linear; }\n"
      "#box.shown { opacity: 1; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);
    EXPECT_EQ(_ui->GetMovingCount(), 1u);

    Step(0.5);
    Step(0.5);
    Step(0.1);
    EXPECT_EQ(_ui->GetMovingCount(), 0u);

    const auto before = _ui->GetStatistics();
    for (int i = 0; i < 10; i++) { Step(0.1); }

    const auto &after = _ui->GetStatistics();
    EXPECT_EQ(after.styles, before.styles);
    EXPECT_EQ(after.layouts, before.layouts);
    EXPECT_EQ(after.paints, before.paints);
    EXPECT_EQ(after.replays, before.replays + 10);
  }

  TEST_F(UiAnimationTest, PlacesNothingForAPropertyThatOnlyChangesWhatIsDrawn)
  {
    ShowBox(
      "#box { opacity: 0; width: 100px; height: 100px; transition: opacity 1s linear; }\n"
      "#box.shown { opacity: 1; }\n");

    _ui->AddClass(Box(), "shown");
    Step(0.0);

    const auto before = _ui->GetStatistics();
    for (int i = 0; i < 5; i++) { Step(0.1); }

    const auto &after = _ui->GetStatistics();
    EXPECT_EQ(after.layouts, before.layouts);
    EXPECT_EQ(after.paints, before.paints + 5);
  }

  TEST_F(UiAnimationTest, PlacesOnlyWhatIsAroundAPropertyThatResizes)
  {
    WriteAsset(
      "ui/theme.css",
      "#grows { width: 10px; height: 10px; transition: width 1s linear; }\n"
      "#grows.wide { width: 100px; }\n");

    ASSERT_GE(Show(
      "styles: [theme.css]\n"
      "root:\n"
      "  type: panel\n"
      "  width: 100%\n"
      "  height: 100%\n"
      "  children:\n"
      "    - type: panel\n"
      "      name: around\n"
      "      width: 400\n"
      "      height: 300\n"
      "      children:\n"
      "        - {type: panel, name: grows}\n"
      "        - {type: panel, name: next, width: 10, height: 10}\n"
      "    - {type: panel, name: other, width: 50, height: 50}\n"
      "    - {type: label, name: text, text: Hello}\n"), 0);
    Step(0.0);

    _ui->AddClass(_ui->FindByName("grows"), "wide");
    Step(0.0);

    const auto before = _ui->GetStatistics();
    for (int i = 0; i < 4; i++) { Step(0.1); }

    const auto &after = _ui->GetStatistics();
    EXPECT_EQ(after.layouts, before.layouts + 4);
    EXPECT_EQ(after.full_layouts, before.full_layouts);

    // the panel around it, and the two inside of that
    EXPECT_EQ(after.laid_out_elements, before.laid_out_elements + 4 * 3);

    ExpectBox("grows", 0.0f, 0.0f, 46.0f, 10.0f);
    ExpectBox("next", 46.0f, 0.0f, 10.0f, 10.0f);
  }

  // animations

  TEST_F(UiAnimationTest, RunsTheKeyframesOfAnAnimation)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear; }\n");

    // it runs from the moment the element has its style
    EXPECT_FLOAT_EQ(Opacity(), 0.0f);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.75f, near);

    // behind its end, the element is what it is without it
    Step(0.5);
    EXPECT_FLOAT_EQ(Opacity(), 1.0f);
    EXPECT_FLOAT_EQ(Element("box").GetComputedStyle().opacity, 1.0f);
  }

  TEST_F(UiAnimationTest, GoesBackToWhatTheElementIsWhenItEnded)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0.2; } to { opacity: 0.8; } }\n"
      "#box { opacity: 0.5; animation: fade 1s linear; }\n");

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.65f, near);

    Step(0.25);
    EXPECT_FLOAT_EQ(Opacity(), 0.5f);
  }

  TEST_F(UiAnimationTest, GoesThroughTheKeyframesBetweenTheEnds)
  {
    ShowBox(
      "@keyframes pulse { 0% { opacity: 0; } 25% { opacity: 1; } 75% { opacity: 0.5; } 100% { opacity: 0; } }\n"
      "#box { animation: pulse 4s linear; }\n");

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 1.0f, near);

    Step(1.0);
    EXPECT_NEAR(Opacity(), 0.75f, near);

    Step(1.0);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.25f, near);
  }

  TEST_F(UiAnimationTest, StartsFromAndEndsAtWhatTheElementIsWhereTheKeyframesSayNothing)
  {
    ShowBox(
      "@keyframes flash { 50% { opacity: 1; } }\n"
      "#box { opacity: 0.2; animation: flash 2s linear; }\n");

    EXPECT_NEAR(Opacity(), 0.2f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.6f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 1.0f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.6f, near);
  }

  TEST_F(UiAnimationTest, TakesTheTimingFunctionForEveryStepBetweenTwoKeyframes)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } 50% { opacity: 0.5; } to { opacity: 1; } }\n"
      "#box { animation: fade 2s ease; }\n");

    // half of the first step, which is eased by itself
    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.5f * 0.802403f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.5f + 0.5f * 0.408511f, near);
  }

  TEST_F(UiAnimationTest, TakesTheTimingFunctionOfAKeyframeForTheStepThatStartsThere)
  {
    ShowBox(
      "@keyframes fade {\n"
      "  from { opacity: 0; animation-timing-function: linear; }\n"
      "  50% { opacity: 0.5; animation-timing-function: steps(1, jump-end); }\n"
      "  to { opacity: 1; }\n"
      "}\n"
      "#box { animation: fade 2s ease; }\n");

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    // from half on, in one jump at the end
    Step(1.0);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    Step(0.4);
    EXPECT_NEAR(Opacity(), 0.5f, near);
  }

  TEST_F(UiAnimationTest, SwitchesHalfwayWhatCannotBeMoved)
  {
    ShowBox(
      "@keyframes turn { from { flex-direction: row; } to { flex-direction: column; } }\n"
      "#box { animation: turn 1s linear; }\n");

    Step(0.25);
    EXPECT_EQ(Style().layout.flex_direction, neon::FlexDirection::Row);

    Step(0.24);
    EXPECT_EQ(Style().layout.flex_direction, neon::FlexDirection::Row);

    Step(0.02);
    EXPECT_EQ(Style().layout.flex_direction, neon::FlexDirection::Column);
  }

  TEST_F(UiAnimationTest, AnimatesSeveralPropertiesAndWhatAShorthandStandsFor)
  {
    ShowBox(
      "@keyframes grow {\n"
      "  from { width: 100px; padding: 0; background-color: #000000; }\n"
      "  to { width: 200px; padding: 10px 20px; background-color: #ffffff; }\n"
      "}\n"
      "#box { height: 50px; animation: grow 1s linear; }\n");

    Step(0.5);

    EXPECT_EQ(Style().layout.width, LayoutLength::Pixels(150.0f));
    EXPECT_EQ(Style().layout.padding.top, LayoutLength::Pixels(5.0f));
    EXPECT_EQ(Style().layout.padding.left, LayoutLength::Pixels(10.0f));
    EXPECT_NEAR(Style().background_color.r, 0.5f, near);

    ExpectBox("box", 0.0f, 0.0f, 170.0f, 60.0f);
  }

  TEST_F(UiAnimationTest, WorksOutTheValuesOfAKeyframeForTheElement)
  {
    ShowBox(
      ":root { --full: 0.8; }\n"
      "@keyframes grow { from { width: 0; opacity: 0; } to { width: 10em; opacity: var(--full); } }\n"
      "#box { font-size: 20px; animation: grow 1s linear; }\n");

    Step(0.5);

    EXPECT_EQ(Style().layout.width, LayoutLength::Pixels(100.0f));
    EXPECT_NEAR(Opacity(), 0.4f, near);
  }

  // how often, and which way

  TEST_F(UiAnimationTest, RunsAsOftenAsItIsToldTo)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear 3; }\n");

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    Step(0.75);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    Step(1.5);
    EXPECT_NEAR(Opacity(), 0.75f, near);

    // three runs are over
    Step(0.5);
    EXPECT_FLOAT_EQ(Opacity(), 1.0f);
    EXPECT_EQ(_ui->GetMovingCount(), 0u);
  }

  TEST_F(UiAnimationTest, RunsAPartOfARun)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear 1.5 forwards; }\n");

    Step(1.25);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    // it ends in the middle of its second run, and holds that
    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.5f, near);
  }

  TEST_F(UiAnimationTest, RunsWithoutEnd)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear infinite; }\n");

    Step(100.25);
    EXPECT_NEAR(Opacity(), 0.25f, 0.001f);

    Step(1000.5);
    EXPECT_NEAR(Opacity(), 0.75f, 0.001f);
    EXPECT_EQ(_ui->GetMovingCount(), 1u);
  }

  TEST_F(UiAnimationTest, RunsBackwards)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear 2 reverse; }\n");

    EXPECT_NEAR(Opacity(), 1.0f, near);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.75f, near);

    Step(1.0);
    EXPECT_NEAR(Opacity(), 0.75f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.25f, near);
  }

  TEST_F(UiAnimationTest, RunsThereAndBack)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear 4 alternate; }\n");

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    // the second run, backwards
    Step(1.0);
    EXPECT_NEAR(Opacity(), 0.75f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    // the third, forwards again
    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.75f, near);
  }

  TEST_F(UiAnimationTest, RunsBackAndThere)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear 4 alternate-reverse; }\n");

    EXPECT_NEAR(Opacity(), 1.0f, near);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.75f, near);

    // the second run, forwards
    Step(1.0);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.75f, near);
  }

  TEST_F(UiAnimationTest, TurnsTheTimingFunctionAroundWithTheRun)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s ease-in reverse; }\n");

    // a quarter of the time of a run that goes backwards is three quarters
    // of the way of the keyframes
    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.621862f, near);
  }

  // what holds in front of an animation and behind it

  TEST_F(UiAnimationTest, HoldsNothingInFrontOfItAndBehindItWithoutAFillMode)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0.2; } to { opacity: 0.8; } }\n"
      "#box { opacity: 0.5; animation: fade 1s linear 1s; }\n");

    EXPECT_FLOAT_EQ(Opacity(), 0.5f);

    Step(0.5);
    EXPECT_FLOAT_EQ(Opacity(), 0.5f);

    Step(1.0);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.65f, near);

    Step(0.5);
    EXPECT_FLOAT_EQ(Opacity(), 0.5f);
  }

  TEST_F(UiAnimationTest, HoldsItsEndWithForwards)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0.2; } to { opacity: 0.8; } }\n"
      "#box { opacity: 0.5; animation: fade 1s linear 1s forwards; }\n");

    Step(0.5);
    EXPECT_FLOAT_EQ(Opacity(), 0.5f);

    Step(2.0);
    EXPECT_NEAR(Opacity(), 0.8f, near);

    Step(5.0);
    EXPECT_NEAR(Opacity(), 0.8f, near);
  }

  TEST_F(UiAnimationTest, HoldsItsStartWithBackwards)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0.2; } to { opacity: 0.8; } }\n"
      "#box { opacity: 0.5; animation: fade 1s linear 1s backwards; }\n");

    EXPECT_NEAR(Opacity(), 0.2f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.2f, near);

    Step(1.0);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    Step(1.0);
    EXPECT_FLOAT_EQ(Opacity(), 0.5f);
  }

  TEST_F(UiAnimationTest, HoldsBothWithBoth)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0.2; } to { opacity: 0.8; } }\n"
      "#box { opacity: 0.5; animation: fade 1s linear 1s both; }\n");

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.2f, near);

    Step(5.0);
    EXPECT_NEAR(Opacity(), 0.8f, near);
  }

  TEST_F(UiAnimationTest, HoldsTheEndOfTheRunItEndedIn)
  {
    // backwards, the end is what the keyframes start with
    ShowBox(
      "@keyframes fade { from { opacity: 0.2; } to { opacity: 0.8; } }\n"
      "#box { animation: fade 1s linear reverse both; }\n");

    EXPECT_NEAR(Opacity(), 0.8f, near);
    Step(2.0);
    EXPECT_NEAR(Opacity(), 0.2f, near);
  }

  TEST_F(UiAnimationTest, HoldsTheEndOfTheLastRunThereAndBack)
  {
    // there, back, and there: three runs end where the keyframes end
    ShowBox(
      "@keyframes fade { from { opacity: 0.2; } to { opacity: 0.8; } }\n"
      "#box { animation: fade 1s linear 3 alternate forwards; }\n");

    Step(5.0);
    EXPECT_NEAR(Opacity(), 0.8f, near);

    // and two where they start
    ShowBox(
      "@keyframes fade { from { opacity: 0.2; } to { opacity: 0.8; } }\n"
      "#box { animation: fade 1s linear 2 alternate forwards; }\n");

    Step(5.0);
    EXPECT_NEAR(Opacity(), 0.2f, near);
  }

  TEST_F(UiAnimationTest, HoldsItsStartWhenItNeverRuns)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0.2; } to { opacity: 0.8; } }\n"
      "#box { opacity: 0.5; animation: fade 1s linear 0 both; }\n");

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.2f, near);
  }

  TEST_F(UiAnimationTest, StartsPartOfTheWayInWithADelayBelowZeroOfItsOwn)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear -0.25s; }\n");

    EXPECT_NEAR(Opacity(), 0.25f, near);

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.75f, near);
  }

  // standing still

  TEST_F(UiAnimationTest, StandsStillWhileItIsPaused)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear; }\n"
      "#box.held { animation-play-state: paused; }\n");

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    _ui->AddClass(Box(), "held");
    Step(0.25);
    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    // and goes on from where it stood
    _ui->RemoveClass(Box(), "held");
    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.5f, near);
  }

  TEST_F(UiAnimationTest, StandsStillWhileTheTimeOfTheUserInterfaceDoes)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear; }\n");

    Step(0.25);

    _ui->SetTimeScale(0.0);
    Step(0.25);
    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.25f, near);
    EXPECT_NEAR(_ui->GetTime(), 0.25, 1e-9);

    _ui->SetTimeScale(2.0);
    Step(0.125);
    EXPECT_NEAR(Opacity(), 0.5f, near);

    _ui->SetTimeScale(1.0);
    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.75f, near);
  }

  TEST_F(UiAnimationTest, StandsStillUntilItIsToldHowMuchTimePasses)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear; }\n");

    for (int i = 0; i < 5; i++) { Frame(); }

    EXPECT_FLOAT_EQ(Opacity(), 0.0f);
    EXPECT_EQ(_ui->GetTime(), 0.0);
  }

  // several at once

  TEST_F(UiAnimationTest, RunsSeveralAnimationsOnOneElement)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "@keyframes grow { from { outline-width: 0; } to { outline-width: 10px; } }\n"
      "#box { animation: fade 1s linear, grow 2s linear; }\n");

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.5f, near);
    EXPECT_NEAR(Style().outline_width, 2.5f, near);

    Step(1.0);
    EXPECT_FLOAT_EQ(Opacity(), 1.0f);
    EXPECT_NEAR(Style().outline_width, 7.5f, near);
  }

  TEST_F(UiAnimationTest, LetsTheAnimationThatIsNamedLastWinForAProperty)
  {
    ShowBox(
      "@keyframes low { from { opacity: 0; } to { opacity: 0.2; } }\n"
      "@keyframes high { from { opacity: 0.8; } to { opacity: 1; } }\n"
      "#box { animation: low 1s linear, high 1s linear; }\n");

    Step(0.5);
    EXPECT_NEAR(Opacity(), 0.9f, near);
  }

  TEST_F(UiAnimationTest, GoesOnWhenTheStyleIsWorkedOutAgain)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear; }\n"
      "#box.other { z-index: 3; }\n");

    Step(0.25);
    _ui->AddClass(Box(), "other");
    Step(0.25);

    // it did not start again
    EXPECT_NEAR(Opacity(), 0.5f, near);
  }

  TEST_F(UiAnimationTest, StartsAndStopsWithTheStyleThatNamesIt)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { opacity: 0.5; }\n"
      "#box.fading { animation: fade 1s linear; }\n");

    Step(0.5);
    EXPECT_FLOAT_EQ(Opacity(), 0.5f);

    _ui->AddClass(Box(), "fading");
    Step(0.0);
    EXPECT_NEAR(Opacity(), 0.0f, near);

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    _ui->RemoveClass(Box(), "fading");
    Step(0.0);
    EXPECT_FLOAT_EQ(Opacity(), 0.5f);
    EXPECT_EQ(_ui->GetMovingCount(), 0u);

    // named again, it starts again
    _ui->AddClass(Box(), "fading");
    Step(0.0);
    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.25f, near);
  }

  TEST_F(UiAnimationTest, RunsNothingForKeyframesThatAreNotThere)
  {
    ShowBox("#box { opacity: 0.5; animation: missing 1s linear; }");

    Step(0.5);
    EXPECT_FLOAT_EQ(Opacity(), 0.5f);
  }

  // what is told of an animation

  TEST_F(UiAnimationTest, TellsThatAnAnimationEnded)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear 2; }\n");

    Step(1.5);
    EXPECT_THAT(Happened(), IsEmpty());

    Step(0.5);
    EXPECT_THAT(Happened(), ElementsAre("animation_ended fade"));
    EXPECT_EQ(_ui->GetElementEvents()[0].target, Box());

    Step(0.5);
    EXPECT_THAT(Happened(), IsEmpty());
  }

  TEST_F(UiAnimationTest, TellsOnceOfAnAnimationThatHoldsItsEnd)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear forwards; }\n");

    int ended = 0;
    _ui->On(Box(), "animation_ended", [&](const UiElementEvent &event)
    {
      EXPECT_EQ(event.value, "fade");
      ended++;
    });

    for (int i = 0; i < 30; i++) { Step(0.1); }
    EXPECT_EQ(ended, 1);
  }

  // started by the game

  TEST_F(UiAnimationTest, StartsAnAnimationByName)
  {
    ShowBox(
      "@keyframes shake { from { left: -10px; } to { left: 10px; } }\n"
      "#box { position: absolute; left: 0; top: 0; width: 10px; height: 10px; }\n");

    ASSERT_TRUE(_ui->StartAnimation(Box(), "shake", "1s linear"));
    Step(0.0);
    EXPECT_EQ(Style().layout.inset.left, LayoutLength::Pixels(-10.0f));

    Step(0.75);
    EXPECT_EQ(Style().layout.inset.left, LayoutLength::Pixels(5.0f));
    ExpectBox("box", 5.0f, 0.0f, 10.0f, 10.0f);

    Step(0.25);
    EXPECT_THAT(Happened(), ElementsAre("animation_ended shake"));
    EXPECT_EQ(Style().layout.inset.left, LayoutLength::Pixels(0.0f));
    EXPECT_EQ(_ui->GetMovingCount(), 0u);
  }

  TEST_F(UiAnimationTest, StartsAnAnimationWithEverythingThatCanBeSaidAboutOne)
  {
    ShowBox("@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n#box { opacity: 0.5; }");

    ASSERT_TRUE(_ui->StartAnimation(Box(), "fade", "1s linear 0.5s 2 alternate both"));
    Step(0.0);
    EXPECT_NEAR(Opacity(), 0.0f, near);

    Step(0.75);
    EXPECT_NEAR(Opacity(), 0.25f, near);

    Step(1.0);
    EXPECT_NEAR(Opacity(), 0.75f, near);

    Step(5.0);
    EXPECT_NEAR(Opacity(), 0.0f, near);
  }

  TEST_F(UiAnimationTest, StopsAnAnimationByName)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "@keyframes grow { from { outline-width: 0; } to { outline-width: 10px; } }\n"
      "#box { opacity: 0.5; }\n");

    ASSERT_TRUE(_ui->StartAnimation(Box(), "fade", "1s linear infinite"));
    ASSERT_TRUE(_ui->StartAnimation(Box(), "grow", "1s linear infinite"));
    Step(0.25);

    EXPECT_NEAR(Opacity(), 0.25f, near);
    EXPECT_NEAR(Style().outline_width, 2.5f, near);

    ASSERT_TRUE(_ui->StopAnimation(Box(), "fade"));
    Step(0.25);

    EXPECT_FLOAT_EQ(Opacity(), 0.5f);
    EXPECT_NEAR(Style().outline_width, 5.0f, near);

    // all that the game started
    ASSERT_TRUE(_ui->StopAnimation(Box()));
    Step(0.25);
    EXPECT_FLOAT_EQ(Style().outline_width, 0.0f);

    EXPECT_FALSE(_ui->StopAnimation(Box(), "fade"));
    EXPECT_FALSE(_ui->StopAnimation(Box()));
  }

  TEST_F(UiAnimationTest, StartsAnAnimationAgainThatIsStartedTwice)
  {
    ShowBox("@keyframes fade { from { opacity: 0; } to { opacity: 1; } }");

    ASSERT_TRUE(_ui->StartAnimation(Box(), "fade", "1s linear"));
    Step(0.75);
    EXPECT_NEAR(Opacity(), 0.75f, near);

    ASSERT_TRUE(_ui->StartAnimation(Box(), "fade", "1s linear"));
    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.25f, near);
  }

  TEST_F(UiAnimationTest, LeavesTheAnimationsOfTheStyleAloneWhenItStopsWhatTheGameStarted)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear infinite; }\n");

    Step(0.25);
    EXPECT_FALSE(_ui->StopAnimation(Box(), "fade"));

    Step(0.25);
    EXPECT_NEAR(Opacity(), 0.5f, near);
  }

  TEST_F(UiAnimationTest, SaysThatThereAreNoSuchKeyframesAndThatWhatFollowsTheNameIsWrong)
  {
    ShowBox("@keyframes fade { from { opacity: 0; } to { opacity: 1; } }");
    _logger->Clear();

    EXPECT_FALSE(_ui->StartAnimation(Box(), "missing", "1s"));
    EXPECT_FALSE(_ui->StartAnimation(Box(), "fade", "1s 2s 3s"));
    EXPECT_FALSE(_ui->StartAnimation(Box(), "fade", "quickly"));

    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn, "There are no keyframes 'missing' for panel 'box', and nothing is started"));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn, "'1s 2s 3s' cannot be read as what follows the name of an animation"));
    EXPECT_EQ(_ui->GetMovingCount(), 0u);
  }

  TEST_F(UiAnimationTest, ForgetsWhatMovedOnAnElementThatIsGone)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 1s linear infinite; }\n");

    Step(0.25);
    EXPECT_EQ(_ui->GetMovingCount(), 1u);

    ASSERT_TRUE(_ui->Remove(Box()));
    Step(0.25);

    EXPECT_EQ(_ui->GetMovingCount(), 0u);
  }

  // less motion

  TEST_F(UiAnimationTest, LetsASheetLeaveOutWhatMovesWhenLessMotionIsWanted)
  {
    ShowBox(
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { opacity: 0; transition: opacity 1s linear; }\n"
      "#box.shown { opacity: 1; }\n"
      "@media (prefers-reduced-motion: reduce) {\n"
      "  #box { transition: none; animation: none; }\n"
      "}\n");

    _ui->SetReducedMotion(true);
    Step(0.0);

    _ui->AddClass(Box(), "shown");
    Step(0.0);

    EXPECT_FLOAT_EQ(Opacity(), 1.0f);
    EXPECT_EQ(_ui->GetMovingCount(), 0u);
  }

  // the same every time

  TEST_F(UiAnimationTest, GivesTheSameFramesForTheSameStepsOfTime)
  {
    const std::string css =
      "@keyframes pulse { 0% { opacity: 0.1; width: 10px; } 40% { opacity: 1; width: 300px; } "
      "100% { opacity: 0.3; width: 50px; } }\n"
      "#box { height: 20px; background-color: #102030; animation: pulse 0.7s ease-in-out infinite alternate; "
      "transition: background-color 0.4s ease; }\n"
      "#box.lit { background-color: #f0e0d0; }\n";

    const auto run = [&]
    {
      std::vector<float> values;

      TearDown();
      Create({});
      ShowBox(css);

      for (int frame = 0; frame < 240; frame++)
      {
        if (frame == 30) { _ui->AddClass(Box(), "lit"); }
        if (frame == 45) { _ui->RemoveClass(Box(), "lit"); }

        Step(1.0 / 60.0);

        values.push_back(Opacity());
        values.push_back(Style().layout.width.value);
        values.push_back(Style().background_color.r);
        values.push_back(Element("box").GetBox().Width());
      }

      return values;
    };

    const auto first = run();
    const auto second = run();

    // to the last bit
    ASSERT_EQ(first.size(), second.size());
    for (std::size_t i = 0; i < first.size(); i++) { ASSERT_EQ(first[i], second[i]) << i; }
  }

  TEST_F(UiAnimationTest, IsWhereItIsByTheTimeThatPassedHoweverItWasCutUp)
  {
    const std::string css =
      "@keyframes fade { from { opacity: 0; } to { opacity: 1; } }\n"
      "#box { animation: fade 2s ease-in-out; }\n";

    ShowBox(css);
    Step(1.2);
    const float in_one_step = Opacity();

    TearDown();
    Create({});
    ShowBox(css);
    for (int i = 0; i < 72; i++) { Step(1.0 / 60.0); }

    EXPECT_NEAR(Opacity(), in_one_step, 0.0001f);
  }
} // namespace
