#include "vk-frame-stages.hpp"

#include <gtest/gtest.h>

// The order of the stages of a frame. Recording them needs a graphics card,
// and is left to running the runtime.

namespace
{
  using neon::VK_FrameStage;
  using neon::VK_FrameStages;

  TEST(VkFrameStagesTest, BeginsTheSceneWithTheFirstModel)
  {
    const auto steps = VK_FrameStages::ToScene(VK_FrameStage::Nothing);

    EXPECT_TRUE(steps.begin_scene);
    EXPECT_FALSE(steps.refuse);
  }

  TEST(VkFrameStagesTest, KeepsDrawingIntoTheSceneThatIsBegun)
  {
    const auto steps = VK_FrameStages::ToScene(VK_FrameStage::Scene);

    EXPECT_FALSE(steps.begin_scene);
    EXPECT_FALSE(steps.refuse);
  }

  TEST(VkFrameStagesTest, LeavesOutAModelThatComesAfterWhatIsDrawnOnTop)
  {
    // the scene was resolved, and drawing into it again would cover what
    // is on top
    EXPECT_TRUE(VK_FrameStages::ToScene(VK_FrameStage::Overlay).refuse);
  }

  TEST(VkFrameStagesTest, ResolvesTheSceneBeforeDrawingOnTopOfIt)
  {
    const auto steps = VK_FrameStages::ToOverlay(VK_FrameStage::Scene);

    EXPECT_TRUE(steps.end_scene);
    EXPECT_TRUE(steps.resolve);
    EXPECT_TRUE(steps.begin_overlay);
  }

  TEST(VkFrameStagesTest, ClearsWhatIsShownWhenNoSceneWasDrawn)
  {
    // a user interface on a surface of its own draws no scene, and needs
    // no image of light
    const auto steps = VK_FrameStages::ToOverlay(VK_FrameStage::Nothing);

    EXPECT_TRUE(steps.begin_overlay);
    EXPECT_FALSE(steps.resolve);
    EXPECT_FALSE(steps.end_scene);
  }

  TEST(VkFrameStagesTest, KeepsDrawingOnTopOnceThere)
  {
    const auto steps = VK_FrameStages::ToOverlay(VK_FrameStage::Overlay);

    EXPECT_FALSE(steps.begin_overlay);
    EXPECT_FALSE(steps.resolve);
  }
} // namespace
