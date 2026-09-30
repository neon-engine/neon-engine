#include "fixed-clock.hpp"

#include <cstdint>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

namespace
{
  using neon::FixedClock;

  /// Steps that frames of the same length ask for in all.
  std::uint64_t StepsOf(FixedClock &clock, const double frames_per_second, const double seconds)
  {
    const auto frames = static_cast<std::size_t>(frames_per_second * seconds + 0.5);

    std::uint64_t steps = 0;
    for (std::size_t frame = 0; frame < frames; frame++) { steps += clock.Advance(1.0 / frames_per_second); }
    return steps;
  }

  TEST(FixedClock, StepsSixtyTimesASecondUnlessItIsSet)
  {
    const FixedClock clock;

    EXPECT_DOUBLE_EQ(clock.GetStep(), 1.0 / 60.0);
    EXPECT_EQ(clock.GetMostStepsPerFrame(), 8u);
  }

  TEST(FixedClock, HasTakenNoStepAtTheStart)
  {
    const FixedClock clock;

    EXPECT_EQ(clock.GetStepCount(), 0u);
    EXPECT_EQ(clock.GetHeldBackCount(), 0u);
    EXPECT_EQ(clock.GetBlend(), 0.0);
  }

  TEST(FixedClock, TakesAStepForAFrameOfTheLengthOfAStep)
  {
    FixedClock clock;

    EXPECT_EQ(clock.Advance(1.0 / 60.0), 1u);
    EXPECT_EQ(clock.Advance(1.0 / 60.0), 1u);
    EXPECT_EQ(clock.GetStepCount(), 2u);
  }

  TEST(FixedClock, TakesTwoStepsForAFrameOfTwiceTheLength)
  {
    FixedClock clock;

    EXPECT_EQ(clock.Advance(1.0 / 30.0), 2u);
  }

  TEST(FixedClock, TakesNoStepUntilEnoughTimeHasComeTogether)
  {
    FixedClock clock;

    EXPECT_EQ(clock.Advance(1.0 / 240.0), 0u);
    EXPECT_EQ(clock.Advance(1.0 / 240.0), 0u);
    EXPECT_EQ(clock.Advance(1.0 / 240.0), 0u);
    EXPECT_EQ(clock.Advance(1.0 / 240.0), 1u);
    EXPECT_EQ(clock.Advance(1.0 / 240.0), 0u);
  }

  TEST(FixedClock, TakesTheStepsOfTheTimeThatPassedAtEveryFrameRate)
  {
    for (const double frames_per_second : {24.0, 30.0, 50.0, 60.0, 90.0, 120.0, 144.0, 240.0, 1000.0})
    {
      FixedClock clock;

      EXPECT_EQ(StepsOf(clock, frames_per_second, 2.0), 120u) << frames_per_second << " frames per second";
      EXPECT_EQ(clock.GetStepCount(), 120u);
    }
  }

  TEST(FixedClock, TakesTheStepsOfTheTimeThatPassedWhenFramesDiffer)
  {
    FixedClock clock;
    const std::vector<double> frames = {0.004, 0.031, 0.0167, 0.002, 0.05, 0.0083, 0.011, 0.0274};

    double passed = 0.0;
    std::uint64_t steps = 0;
    for (int round = 0; round < 50; round++)
    {
      for (const double frame : frames)
      {
        passed += frame;
        steps += clock.Advance(frame);
      }
    }

    EXPECT_EQ(steps, static_cast<std::uint64_t>(passed * 60.0));
    EXPECT_EQ(clock.GetHeldBackCount(), 0u);
  }

  TEST(FixedClock, NeverTakesAStepOfAnotherLength)
  {
    FixedClock clock;

    clock.Advance(0.0001);
    EXPECT_DOUBLE_EQ(clock.GetStep(), 1.0 / 60.0);

    clock.Advance(3.0);
    EXPECT_DOUBLE_EQ(clock.GetStep(), 1.0 / 60.0);
  }

  TEST(FixedClock, StepsAsOftenAsItIsSetTo)
  {
    FixedClock clock;
    clock.SetStepsPerSecond(120.0);

    EXPECT_DOUBLE_EQ(clock.GetStep(), 1.0 / 120.0);
    EXPECT_EQ(StepsOf(clock, 60.0, 1.0), 120u);
  }

  TEST(FixedClock, IgnoresARateThatIsNotAboveZero)
  {
    FixedClock clock;

    clock.SetStepsPerSecond(0.0);
    clock.SetStepsPerSecond(-30.0);
    clock.SetStepsPerSecond(std::numeric_limits<double>::quiet_NaN());
    clock.SetStepsPerSecond(std::numeric_limits<double>::infinity());

    EXPECT_DOUBLE_EQ(clock.GetStep(), 1.0 / 60.0);
  }

  TEST(FixedClock, IgnoresAFrameThatTookNoTime)
  {
    FixedClock clock;

    EXPECT_EQ(clock.Advance(0.0), 0u);
    EXPECT_EQ(clock.Advance(-1.0), 0u);
    EXPECT_EQ(clock.Advance(std::numeric_limits<double>::quiet_NaN()), 0u);
    EXPECT_EQ(clock.Advance(std::numeric_limits<double>::infinity()), 0u);

    EXPECT_EQ(clock.GetBlend(), 0.0);
    EXPECT_EQ(clock.Advance(1.0 / 60.0), 1u);
  }

  // a frame that took long

  TEST(FixedClock, TakesNoMoreThanTheMostStepsInAFrame)
  {
    FixedClock clock;

    EXPECT_EQ(clock.Advance(5.0), 8u);
    EXPECT_EQ(clock.GetStepCount(), 8u);
    EXPECT_EQ(clock.GetHeldBackCount(), 292u);
  }

  TEST(FixedClock, GivesUpTheTimeAboveTheMostSteps)
  {
    FixedClock clock;
    clock.Advance(5.0);

    // the frames after it are as if the long one had not been
    EXPECT_EQ(clock.Advance(1.0 / 60.0), 1u);
    EXPECT_EQ(clock.Advance(1.0 / 60.0), 1u);
  }

  TEST(FixedClock, TakesExactlyTheMostStepsWithoutGivingUpAny)
  {
    FixedClock clock;

    EXPECT_EQ(clock.Advance(8.0 / 60.0), 8u);
    EXPECT_EQ(clock.GetHeldBackCount(), 0u);
  }

  TEST(FixedClock, TakesAsManyStepsInAFrameAsItIsSetTo)
  {
    FixedClock clock;
    clock.SetMostStepsPerFrame(2);

    EXPECT_EQ(clock.Advance(1.0), 2u);
  }

  TEST(FixedClock, IgnoresAMostOfNoSteps)
  {
    FixedClock clock;
    clock.SetMostStepsPerFrame(0);

    EXPECT_EQ(clock.GetMostStepsPerFrame(), 8u);
  }

  // between two steps

  TEST(FixedClock, BlendsByTheTimeThatWaits)
  {
    FixedClock clock;

    clock.Advance(0.25 / 60.0);
    EXPECT_NEAR(clock.GetBlend(), 0.25, 1e-9);

    clock.Advance(0.5 / 60.0);
    EXPECT_NEAR(clock.GetBlend(), 0.75, 1e-9);

    clock.Advance(0.5 / 60.0);
    EXPECT_NEAR(clock.GetBlend(), 0.25, 1e-9);
  }

  TEST(FixedClock, BlendsBetweenZeroAndOne)
  {
    FixedClock clock;
    const std::vector<double> frames = {0.004, 0.031, 0.0167, 0.002, 5.0, 0.0083, 0.011, 0.0274};

    for (int round = 0; round < 50; round++)
    {
      for (const double frame : frames)
      {
        clock.Advance(frame);

        EXPECT_GE(clock.GetBlend(), 0.0);
        EXPECT_LT(clock.GetBlend(), 1.0);
      }
    }
  }

  TEST(FixedClock, DoesTheSameForTheSameFrames)
  {
    FixedClock first;
    FixedClock second;
    const std::vector<double> frames = {0.004, 0.031, 0.0167, 0.002, 0.05, 0.0083, 0.5, 0.0274};

    for (int round = 0; round < 20; round++)
    {
      for (const double frame : frames)
      {
        EXPECT_EQ(first.Advance(frame), second.Advance(frame));
        EXPECT_EQ(first.GetBlend(), second.GetBlend());
      }
    }
  }
}
