#include "frame-capture.hpp"

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/mock-render-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::FrameCapture;
  using neon::testing::MockRenderSystem;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::Return;
  using ::testing::StrictMock;

  // NumberedPath

  TEST(FrameCapture, PutsTheNumberInFrontOfTheExtension)
  {
    EXPECT_EQ(FrameCapture::NumberedPath("output://frame.png", 30), "output://frame-0030.png");
  }

  TEST(FrameCapture, WritesTheNumberWithAtLeastFourDigits)
  {
    EXPECT_EQ(FrameCapture::NumberedPath("output://frame.png", 1), "output://frame-0001.png");
    EXPECT_EQ(FrameCapture::NumberedPath("output://frame.png", 999), "output://frame-0999.png");
    EXPECT_EQ(FrameCapture::NumberedPath("output://frame.png", 9999), "output://frame-9999.png");
    EXPECT_EQ(FrameCapture::NumberedPath("output://frame.png", 0), "output://frame-0000.png");
  }

  TEST(FrameCapture, WritesANumberWithMoreThanFourDigitsInFull)
  {
    EXPECT_EQ(FrameCapture::NumberedPath("output://frame.png", 10000), "output://frame-10000.png");
    EXPECT_EQ(FrameCapture::NumberedPath("output://frame.png", 1234567), "output://frame-1234567.png");
  }

  TEST(FrameCapture, NumbersAFileInsideFolders)
  {
    EXPECT_EQ(
      FrameCapture::NumberedPath("user://shots/today/frame.png", 7),
      "user://shots/today/frame-0007.png");
  }

  TEST(FrameCapture, PutsTheNumberInFrontOfTheLastExtension)
  {
    EXPECT_EQ(FrameCapture::NumberedPath("output://frame.final.png", 7), "output://frame.final-0007.png");
  }

  TEST(FrameCapture, PutsTheNumberAtTheEndOfANameWithoutExtension)
  {
    EXPECT_EQ(FrameCapture::NumberedPath("output://frame", 7), "output://frame-0007");
  }

  TEST(FrameCapture, DoesNotTakeADotInAFolderForAnExtension)
  {
    EXPECT_EQ(FrameCapture::NumberedPath("output://shots.today/frame", 7), "output://shots.today/frame-0007");
  }

  TEST(FrameCapture, DoesNotTakeADotThatStartsTheNameForAnExtension)
  {
    EXPECT_EQ(FrameCapture::NumberedPath("output://.frame", 7), "output://.frame-0007");
    EXPECT_EQ(FrameCapture::NumberedPath("output://shots/.frame", 7), "output://shots/.frame-0007");
  }

  TEST(FrameCapture, NumbersANameThatStartsWithADotAndHasAnExtension)
  {
    EXPECT_EQ(FrameCapture::NumberedPath("output://.frame.png", 7), "output://.frame-0007.png");
  }

  TEST(FrameCapture, NumbersAPathWithoutScheme)
  {
    EXPECT_EQ(FrameCapture::NumberedPath("frame.png", 7), "frame-0007.png");
    EXPECT_EQ(FrameCapture::NumberedPath("frame", 7), "frame-0007");
    EXPECT_EQ(FrameCapture::NumberedPath(".png", 7), ".png-0007");
  }

  // AfterFrame

  class FrameCaptureTest : public ::testing::Test
  {
  protected:
    // every capture that a test does not expect fails the test
    StrictMock<MockRenderSystem> _render_system{std::make_shared<RecordingLogger>()};
  };

  TEST_F(FrameCaptureTest, SavesNothingWithoutAPath)
  {
    const FrameCapture capture(SettingsConfig{.max_frames = 3}, &_render_system);

    for (std::size_t frame = 1; frame <= 3; frame++) { EXPECT_TRUE(capture.AfterFrame(frame)); }
  }

  TEST_F(FrameCaptureTest, SavesNothingWithoutAPathEvenWithFrames)
  {
    const FrameCapture capture(SettingsConfig{.max_frames = 3, .screenshot_frames = {1, 2}}, &_render_system);

    for (std::size_t frame = 1; frame <= 3; frame++) { EXPECT_TRUE(capture.AfterFrame(frame)); }
  }

  TEST_F(FrameCaptureTest, SavesTheLastFrameUnderTheNameThatWasGiven)
  {
    const FrameCapture capture(
      SettingsConfig{.max_frames = 3, .screenshot_path = "output://frame.png"},
      &_render_system);

    EXPECT_TRUE(capture.AfterFrame(1));
    EXPECT_TRUE(capture.AfterFrame(2));

    EXPECT_CALL(_render_system, CaptureFrame("output://frame.png")).WillOnce(Return(true));
    EXPECT_TRUE(capture.AfterFrame(3));
  }

  TEST_F(FrameCaptureTest, SavesNothingAfterTheLastFrame)
  {
    const FrameCapture capture(
      SettingsConfig{.max_frames = 3, .screenshot_path = "output://frame.png"},
      &_render_system);

    EXPECT_TRUE(capture.AfterFrame(4));
  }

  TEST_F(FrameCaptureTest, SavesNothingInARunWithoutEnd)
  {
    const FrameCapture capture(SettingsConfig{.screenshot_path = "output://frame.png"}, &_render_system);

    EXPECT_TRUE(capture.AfterFrame(0));
    EXPECT_TRUE(capture.AfterFrame(1));
    EXPECT_TRUE(capture.AfterFrame(1000));
  }

  TEST_F(FrameCaptureTest, FailsWhenTheLastFrameCannotBeSaved)
  {
    const FrameCapture capture(
      SettingsConfig{.max_frames = 3, .screenshot_path = "output://frame.png"},
      &_render_system);

    EXPECT_CALL(_render_system, CaptureFrame("output://frame.png")).WillOnce(Return(false));

    EXPECT_FALSE(capture.AfterFrame(3));
  }

  TEST_F(FrameCaptureTest, SavesTheListedFramesUnderNumberedNames)
  {
    const FrameCapture capture(
      SettingsConfig{.max_frames = 30, .screenshot_path = "output://frame.png", .screenshot_frames = {1, 10, 30}},
      &_render_system);

    {
      ::testing::InSequence in_order;
      EXPECT_CALL(_render_system, CaptureFrame("output://frame-0001.png")).WillOnce(Return(true));
      EXPECT_CALL(_render_system, CaptureFrame("output://frame-0010.png")).WillOnce(Return(true));
      EXPECT_CALL(_render_system, CaptureFrame("output://frame-0030.png")).WillOnce(Return(true));
    }

    for (std::size_t frame = 1; frame <= 30; frame++) { EXPECT_TRUE(capture.AfterFrame(frame)) << frame; }
  }

  TEST_F(FrameCaptureTest, DoesNotSaveTheLastFrameWhenFramesAreListed)
  {
    const FrameCapture capture(
      SettingsConfig{.max_frames = 60, .screenshot_path = "output://frame.png", .screenshot_frames = {10}},
      &_render_system);

    EXPECT_TRUE(capture.AfterFrame(60));
  }

  TEST_F(FrameCaptureTest, FailsOnlyForTheListedFrameThatCannotBeSaved)
  {
    const FrameCapture capture(
      SettingsConfig{.max_frames = 3, .screenshot_path = "output://frame.png", .screenshot_frames = {1, 2, 3}},
      &_render_system);

    EXPECT_CALL(_render_system, CaptureFrame("output://frame-0001.png")).WillOnce(Return(true));
    EXPECT_CALL(_render_system, CaptureFrame("output://frame-0002.png")).WillOnce(Return(false));
    EXPECT_CALL(_render_system, CaptureFrame("output://frame-0003.png")).WillOnce(Return(true));

    EXPECT_TRUE(capture.AfterFrame(1));
    EXPECT_FALSE(capture.AfterFrame(2));
    EXPECT_TRUE(capture.AfterFrame(3));
  }

  TEST_F(FrameCaptureTest, SavesAFrameEachTimeItIsReported)
  {
    const FrameCapture capture(
      SettingsConfig{.max_frames = 3, .screenshot_path = "output://frame.png", .screenshot_frames = {2}},
      &_render_system);

    EXPECT_CALL(_render_system, CaptureFrame("output://frame-0002.png")).Times(2).WillRepeatedly(Return(true));

    EXPECT_TRUE(capture.AfterFrame(2));
    EXPECT_TRUE(capture.AfterFrame(2));
  }

  TEST_F(FrameCaptureTest, KeepsTheSettingsItWasCreatedWith)
  {
    SettingsConfig settings{.max_frames = 1, .screenshot_path = "output://frame.png"};
    const FrameCapture capture(settings, &_render_system);

    settings.screenshot_path = "output://other.png";
    settings.max_frames = 5;

    EXPECT_CALL(_render_system, CaptureFrame("output://frame.png")).WillOnce(Return(true));
    EXPECT_TRUE(capture.AfterFrame(1));
  }
}
