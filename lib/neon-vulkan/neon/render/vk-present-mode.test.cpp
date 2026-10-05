#include "vk-present-mode.hpp"

#include <vector>

#include <gtest/gtest.h>

namespace
{
  using neon::VK_PresentMode;

  const std::vector<VkPresentModeKHR> everything = {
    VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR};

  TEST(VkPresentModeTest, WaitsForTheScreenWithVerticalSyncWhateverElseIsOffered)
  {
    EXPECT_EQ(VK_PresentMode::Choose(everything, true), VK_PRESENT_MODE_FIFO_KHR);
    EXPECT_EQ(VK_PresentMode::Choose(std::vector{VK_PRESENT_MODE_FIFO_KHR}, true), VK_PRESENT_MODE_FIFO_KHR);
  }

  TEST(VkPresentModeTest, ShowsAFrameAtOnceWithoutVerticalSyncWhereTheDriverCan)
  {
    EXPECT_EQ(VK_PresentMode::Choose(everything, false), VK_PRESENT_MODE_IMMEDIATE_KHR);

    // as MoltenVK offers them
    const std::vector on_a_mac = {VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR};
    EXPECT_EQ(VK_PresentMode::Choose(on_a_mac, false), VK_PRESENT_MODE_IMMEDIATE_KHR);
  }

  TEST(VkPresentModeTest, WaitsForTheScreenWithoutVerticalSyncWhereTheDriverCannotShowAFrameAtOnce)
  {
    const std::vector no_immediate = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_KHR};
    EXPECT_EQ(VK_PresentMode::Choose(no_immediate, false), VK_PRESENT_MODE_FIFO_KHR);
    EXPECT_EQ(VK_PresentMode::Choose({}, false), VK_PRESENT_MODE_FIFO_KHR);
  }

  TEST(VkPresentModeTest, NamesTheModesForTheLog)
  {
    EXPECT_STREQ(VK_PresentMode::NameOf(VK_PRESENT_MODE_FIFO_KHR), "fifo");
    EXPECT_STREQ(VK_PresentMode::NameOf(VK_PRESENT_MODE_IMMEDIATE_KHR), "immediate");
  }
}
