#include "ui-timing.hpp"

#include <gtest/gtest.h>

// What is expected here is what https://www.w3.org/TR/css-easing-1/ says.
// The values of the curves were worked out from their definition with
// another program, by halving to 200 steps.

namespace
{
  using neon::UiTimingFunction;
  using Position = neon::UiTimingFunction::StepPosition;

  constexpr float near = 0.0001f;

  UiTimingFunction Read(const std::string &text)
  {
    UiTimingFunction function;
    EXPECT_TRUE(neon::ParseCssTimingFunction(text, function)) << text;
    return function;
  }

  bool Reads(const std::string &text)
  {
    UiTimingFunction function = UiTimingFunction::Linear();
    const bool read = neon::ParseCssTimingFunction(text, function);

    // what cannot be read leaves what it was to be read into alone
    if (!read) { EXPECT_EQ(function, UiTimingFunction::Linear()) << text; }
    return read;
  }

  TEST(UiTimingFunctionTest, StartsAsEase)
  {
    EXPECT_EQ(UiTimingFunction{}, UiTimingFunction::Ease());
  }

  TEST(UiTimingFunctionTest, GoesStraightWhenLinear)
  {
    const UiTimingFunction linear = UiTimingFunction::Linear();

    for (const float time : {0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 1.0f})
    {
      EXPECT_FLOAT_EQ(linear.At(time), time);
    }
  }

  TEST(UiTimingFunctionTest, StartsAtTheStartAndEndsAtTheEnd)
  {
    for (const auto &function : {
           UiTimingFunction::Linear(), UiTimingFunction::Ease(), UiTimingFunction::EaseIn(),
           UiTimingFunction::EaseOut(), UiTimingFunction::EaseInOut(),
           UiTimingFunction::CubicBezier(0.1f, 0.7f, 1.0f, 0.1f),
           UiTimingFunction::CubicBezier(0.34f, 1.56f, 0.64f, 1.0f)
         })
    {
      EXPECT_NEAR(function.At(0.0f), 0.0f, near);
      EXPECT_NEAR(function.At(1.0f), 1.0f, near);
    }
  }

  TEST(UiTimingFunctionTest, FollowsEase)
  {
    const UiTimingFunction ease = UiTimingFunction::Ease();

    EXPECT_NEAR(ease.At(0.1f), 0.094796f, near);
    EXPECT_NEAR(ease.At(0.25f), 0.408511f, near);
    EXPECT_NEAR(ease.At(0.5f), 0.802403f, near);
    EXPECT_NEAR(ease.At(0.75f), 0.960459f, near);
    EXPECT_NEAR(ease.At(0.9f), 0.994316f, near);
  }

  TEST(UiTimingFunctionTest, FollowsEaseIn)
  {
    const UiTimingFunction ease_in = UiTimingFunction::EaseIn();

    EXPECT_NEAR(ease_in.At(0.1f), 0.017027f, near);
    EXPECT_NEAR(ease_in.At(0.25f), 0.093465f, near);
    EXPECT_NEAR(ease_in.At(0.5f), 0.315357f, near);
    EXPECT_NEAR(ease_in.At(0.75f), 0.621862f, near);
    EXPECT_NEAR(ease_in.At(0.9f), 0.839428f, near);
  }

  TEST(UiTimingFunctionTest, FollowsEaseOut)
  {
    const UiTimingFunction ease_out = UiTimingFunction::EaseOut();

    EXPECT_NEAR(ease_out.At(0.1f), 0.160572f, near);
    EXPECT_NEAR(ease_out.At(0.25f), 0.378138f, near);
    EXPECT_NEAR(ease_out.At(0.5f), 0.684643f, near);
    EXPECT_NEAR(ease_out.At(0.75f), 0.906535f, near);
    EXPECT_NEAR(ease_out.At(0.9f), 0.982973f, near);
  }

  TEST(UiTimingFunctionTest, FollowsEaseInOut)
  {
    const UiTimingFunction ease_in_out = UiTimingFunction::EaseInOut();

    EXPECT_NEAR(ease_in_out.At(0.1f), 0.019722f, near);
    EXPECT_NEAR(ease_in_out.At(0.25f), 0.129162f, near);
    EXPECT_NEAR(ease_in_out.At(0.5f), 0.5f, near);
    EXPECT_NEAR(ease_in_out.At(0.75f), 0.870838f, near);
    EXPECT_NEAR(ease_in_out.At(0.9f), 0.980278f, near);
  }

  TEST(UiTimingFunctionTest, FollowsACurveOfItsOwn)
  {
    const UiTimingFunction curve = UiTimingFunction::CubicBezier(0.1f, 0.7f, 1.0f, 0.1f);

    EXPECT_NEAR(curve.At(0.1f), 0.244779f, near);
    EXPECT_NEAR(curve.At(0.25f), 0.350421f, near);
    EXPECT_NEAR(curve.At(0.5f), 0.417277f, near);
    EXPECT_NEAR(curve.At(0.75f), 0.489876f, near);
    EXPECT_NEAR(curve.At(0.9f), 0.609904f, near);
  }

  TEST(UiTimingFunctionTest, SwingsOutAboveTheEndAndBelowTheStart)
  {
    const UiTimingFunction over = UiTimingFunction::CubicBezier(0.34f, 1.56f, 0.64f, 1.0f);
    EXPECT_NEAR(over.At(0.5f), 1.087401f, near);
    EXPECT_NEAR(over.At(0.75f), 1.059647f, near);

    const UiTimingFunction back = UiTimingFunction::CubicBezier(0.6f, -0.28f, 0.735f, 0.045f);
    EXPECT_NEAR(back.At(0.25f), -0.086946f, near);
    EXPECT_NEAR(back.At(0.5f), -0.063622f, near);
    EXPECT_NEAR(back.At(0.75f), 0.240393f, near);
  }

  TEST(UiTimingFunctionTest, IsTheSameAsLinearForACurveThatIsStraight)
  {
    const UiTimingFunction curve = UiTimingFunction::CubicBezier(0.0f, 0.0f, 1.0f, 1.0f);

    for (const float time : {0.1f, 0.25f, 0.5f, 0.75f, 0.9f}) { EXPECT_NEAR(curve.At(time), time, near); }
  }

  TEST(UiTimingFunctionTest, NeverGoesBackwardsForACurveThatRisesAllTheWay)
  {
    for (const auto &function : {
           UiTimingFunction::Ease(), UiTimingFunction::EaseIn(), UiTimingFunction::EaseOut(),
           UiTimingFunction::EaseInOut()
         })
    {
      float before = 0.0f;
      for (int i = 1; i <= 1000; i++)
      {
        const float now = function.At(static_cast<float>(i) / 1000.0f);
        EXPECT_GE(now, before - 1e-6f) << i;
        before = now;
      }
    }
  }

  // steps, with the values of the specification for every position

  TEST(UiTimingFunctionTest, JumpsAtTheEndOfEveryStep)
  {
    const UiTimingFunction steps = UiTimingFunction::Steps(4, Position::JumpEnd);

    EXPECT_FLOAT_EQ(steps.At(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(steps.At(0.1f), 0.0f);
    EXPECT_FLOAT_EQ(steps.At(0.24f), 0.0f);
    EXPECT_FLOAT_EQ(steps.At(0.25f), 0.25f);
    EXPECT_FLOAT_EQ(steps.At(0.49f), 0.25f);
    EXPECT_FLOAT_EQ(steps.At(0.5f), 0.5f);
    EXPECT_FLOAT_EQ(steps.At(0.75f), 0.75f);
    EXPECT_FLOAT_EQ(steps.At(0.99f), 0.75f);
    EXPECT_FLOAT_EQ(steps.At(1.0f), 1.0f);
  }

  TEST(UiTimingFunctionTest, JumpsAtTheStartOfEveryStep)
  {
    const UiTimingFunction steps = UiTimingFunction::Steps(4, Position::JumpStart);

    EXPECT_FLOAT_EQ(steps.At(0.0f), 0.25f);
    EXPECT_FLOAT_EQ(steps.At(0.1f), 0.25f);
    EXPECT_FLOAT_EQ(steps.At(0.25f), 0.5f);
    EXPECT_FLOAT_EQ(steps.At(0.5f), 0.75f);
    EXPECT_FLOAT_EQ(steps.At(0.75f), 1.0f);
    EXPECT_FLOAT_EQ(steps.At(1.0f), 1.0f);

    // what has not started yet has not jumped
    EXPECT_FLOAT_EQ(steps.At(0.0f, true), 0.0f);
  }

  TEST(UiTimingFunctionTest, JumpsAtNeitherEnd)
  {
    const UiTimingFunction steps = UiTimingFunction::Steps(5, Position::JumpNone);

    EXPECT_FLOAT_EQ(steps.At(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(steps.At(0.19f), 0.0f);
    EXPECT_FLOAT_EQ(steps.At(0.2f), 0.25f);
    EXPECT_FLOAT_EQ(steps.At(0.4f), 0.5f);
    EXPECT_FLOAT_EQ(steps.At(0.6f), 0.75f);
    EXPECT_FLOAT_EQ(steps.At(0.8f), 1.0f);
    EXPECT_FLOAT_EQ(steps.At(1.0f), 1.0f);
  }

  TEST(UiTimingFunctionTest, JumpsAtBothEnds)
  {
    const UiTimingFunction steps = UiTimingFunction::Steps(3, Position::JumpBoth);

    EXPECT_FLOAT_EQ(steps.At(0.0f), 0.25f);
    EXPECT_FLOAT_EQ(steps.At(0.3f), 0.25f);
    EXPECT_FLOAT_EQ(steps.At(0.34f), 0.5f);
    EXPECT_FLOAT_EQ(steps.At(0.67f), 0.75f);
    EXPECT_FLOAT_EQ(steps.At(1.0f), 1.0f);
  }

  TEST(UiTimingFunctionTest, JumpsOnceWithOneStep)
  {
    const UiTimingFunction at_start = UiTimingFunction::Steps(1, Position::JumpStart);
    EXPECT_FLOAT_EQ(at_start.At(0.0f), 1.0f);
    EXPECT_FLOAT_EQ(at_start.At(0.5f), 1.0f);

    const UiTimingFunction at_end = UiTimingFunction::Steps(1, Position::JumpEnd);
    EXPECT_FLOAT_EQ(at_end.At(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(at_end.At(0.99f), 0.0f);
    EXPECT_FLOAT_EQ(at_end.At(1.0f), 1.0f);
  }

  // reading

  TEST(UiTimingFunctionTest, ReadsTheNamesOfCss)
  {
    EXPECT_EQ(Read("linear"), UiTimingFunction::Linear());
    EXPECT_EQ(Read("ease"), UiTimingFunction::Ease());
    EXPECT_EQ(Read("ease-in"), UiTimingFunction::EaseIn());
    EXPECT_EQ(Read("ease-out"), UiTimingFunction::EaseOut());
    EXPECT_EQ(Read("ease-in-out"), UiTimingFunction::EaseInOut());
    EXPECT_EQ(Read("step-start"), UiTimingFunction::Steps(1, Position::JumpStart));
    EXPECT_EQ(Read("step-end"), UiTimingFunction::Steps(1, Position::JumpEnd));
    EXPECT_EQ(Read(" Ease-In "), UiTimingFunction::EaseIn());
  }

  TEST(UiTimingFunctionTest, KnowsTheNamesAsTheCurvesTheSpecificationGivesThem)
  {
    EXPECT_EQ(Read("ease"), Read("cubic-bezier(0.25, 0.1, 0.25, 1)"));
    EXPECT_EQ(Read("ease-in"), Read("cubic-bezier(0.42, 0, 1, 1)"));
    EXPECT_EQ(Read("ease-out"), Read("cubic-bezier(0, 0, 0.58, 1)"));
    EXPECT_EQ(Read("ease-in-out"), Read("cubic-bezier(0.42, 0, 0.58, 1)"));
  }

  TEST(UiTimingFunctionTest, ReadsACurve)
  {
    const UiTimingFunction curve = Read("cubic-bezier( 0.1 , 0.7, 1.0,0.1 )");

    EXPECT_EQ(curve.kind, UiTimingFunction::Kind::CubicBezier);
    EXPECT_FLOAT_EQ(curve.x1, 0.1f);
    EXPECT_FLOAT_EQ(curve.y1, 0.7f);
    EXPECT_FLOAT_EQ(curve.x2, 1.0f);
    EXPECT_FLOAT_EQ(curve.y2, 0.1f);

    // up and down are not held to the way from 0 to 1
    EXPECT_FLOAT_EQ(Read("cubic-bezier(0.34, 1.56, 0.64, 1)").y1, 1.56f);
    EXPECT_FLOAT_EQ(Read("cubic-bezier(0.6, -0.28, 0.735, 0.045)").y1, -0.28f);
  }

  TEST(UiTimingFunctionTest, ReadsSteps)
  {
    EXPECT_EQ(Read("steps(4)"), UiTimingFunction::Steps(4, Position::JumpEnd));
    EXPECT_EQ(Read("steps(4, jump-end)"), UiTimingFunction::Steps(4, Position::JumpEnd));
    EXPECT_EQ(Read("steps(4, end)"), UiTimingFunction::Steps(4, Position::JumpEnd));
    EXPECT_EQ(Read("steps(4, jump-start)"), UiTimingFunction::Steps(4, Position::JumpStart));
    EXPECT_EQ(Read("steps(4, start)"), UiTimingFunction::Steps(4, Position::JumpStart));
    EXPECT_EQ(Read("steps(4, jump-none)"), UiTimingFunction::Steps(4, Position::JumpNone));
    EXPECT_EQ(Read("steps(4, jump-both)"), UiTimingFunction::Steps(4, Position::JumpBoth));
    EXPECT_EQ(Read("steps( 10 , jump-both )"), UiTimingFunction::Steps(10, Position::JumpBoth));
  }

  TEST(UiTimingFunctionTest, RefusesWhatIsNoTimingFunction)
  {
    EXPECT_FALSE(Reads(""));
    EXPECT_FALSE(Reads("fast"));
    EXPECT_FALSE(Reads("ease-in-out-back"));
    EXPECT_FALSE(Reads("cubic-bezier()"));
    EXPECT_FALSE(Reads("cubic-bezier(0.1, 0.7, 1.0)"));
    EXPECT_FALSE(Reads("cubic-bezier(0.1, 0.7, 1.0, 0.1, 0.5)"));
    EXPECT_FALSE(Reads("cubic-bezier(0.1, 0.7, 1.0, a)"));
    EXPECT_FALSE(Reads("cubic-bezier(0.1, 0.7, 1.0, 0.1"));
    EXPECT_FALSE(Reads("steps()"));
    EXPECT_FALSE(Reads("steps(0)"));
    EXPECT_FALSE(Reads("steps(-2)"));
    EXPECT_FALSE(Reads("steps(2.5)"));
    EXPECT_FALSE(Reads("steps(4, middle)"));
    EXPECT_FALSE(Reads("steps(4, jump-end, 2)"));
  }

  TEST(UiTimingFunctionTest, RefusesACurveWhoseTimeRunsBackwards)
  {
    EXPECT_FALSE(Reads("cubic-bezier(-0.1, 0, 1, 1)"));
    EXPECT_FALSE(Reads("cubic-bezier(1.1, 0, 1, 1)"));
    EXPECT_FALSE(Reads("cubic-bezier(0, 0, -0.1, 1)"));
    EXPECT_FALSE(Reads("cubic-bezier(0, 0, 1.1, 1)"));
  }

  TEST(UiTimingFunctionTest, RefusesOneStepWithoutAJumpAtEitherEnd)
  {
    EXPECT_FALSE(Reads("steps(1, jump-none)"));
    EXPECT_TRUE(Reads("steps(2, jump-none)"));
  }

  TEST(UiTimingFunctionTest, WritesWhatItReads)
  {
    for (const std::string text : {
           "linear", "ease", "ease-in", "ease-out", "ease-in-out", "cubic-bezier(0.1, 0.7, 1, 0.1)",
           "steps(4, jump-end)", "steps(3, jump-both)", "steps(1, jump-start)"
         })
    {
      EXPECT_EQ(neon::FormatCssTimingFunction(Read(text)), text);
    }
  }

  // times

  TEST(UiTimeTest, ReadsSecondsAndThousandthsOfThem)
  {
    float seconds = -1.0f;

    ASSERT_TRUE(neon::ParseCssTime("0.2s", seconds));
    EXPECT_FLOAT_EQ(seconds, 0.2f);

    ASSERT_TRUE(neon::ParseCssTime("150ms", seconds));
    EXPECT_FLOAT_EQ(seconds, 0.15f);

    ASSERT_TRUE(neon::ParseCssTime("2S", seconds));
    EXPECT_FLOAT_EQ(seconds, 2.0f);

    ASSERT_TRUE(neon::ParseCssTime(" 1.5s ", seconds));
    EXPECT_FLOAT_EQ(seconds, 1.5f);

    ASSERT_TRUE(neon::ParseCssTime(".5s", seconds));
    EXPECT_FLOAT_EQ(seconds, 0.5f);

    // a delay may be below 0, which starts a change part of the way in
    ASSERT_TRUE(neon::ParseCssTime("-250ms", seconds));
    EXPECT_FLOAT_EQ(seconds, -0.25f);
  }

  TEST(UiTimeTest, NeedsAUnitEvenForZero)
  {
    // https://www.w3.org/TR/css-values-4/#time
    float seconds = -1.0f;
    EXPECT_FALSE(neon::ParseCssTime("0", seconds));
    EXPECT_FLOAT_EQ(seconds, -1.0f);

    ASSERT_TRUE(neon::ParseCssTime("0s", seconds));
    EXPECT_FLOAT_EQ(seconds, 0.0f);
  }

  TEST(UiTimeTest, RefusesWhatIsNoTime)
  {
    float seconds = -1.0f;

    EXPECT_FALSE(neon::ParseCssTime("", seconds));
    EXPECT_FALSE(neon::ParseCssTime("2", seconds));
    EXPECT_FALSE(neon::ParseCssTime("s", seconds));
    EXPECT_FALSE(neon::ParseCssTime("ms", seconds));
    EXPECT_FALSE(neon::ParseCssTime("fast", seconds));
    EXPECT_FALSE(neon::ParseCssTime("2px", seconds));
    EXPECT_FALSE(neon::ParseCssTime("2 seconds", seconds));
    EXPECT_FLOAT_EQ(seconds, -1.0f);
  }
} // namespace
