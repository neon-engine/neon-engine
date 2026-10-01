#include "vk-swapchain-sizing.hpp"

#include <cstdint>

#include <gtest/gtest.h>

// When the swapchain of a window is made again, and at which size. Making
// it needs a graphics card and a window, and is left to running the
// runtime and resizing its window.

namespace
{
  using neon::VK_FrameSizing;
  using neon::VK_SwapchainSizing;
  using neon::WindowSize;

  VkSurfaceCapabilitiesKHR CapabilitiesWith(const VkExtent2D current)
  {
    VkSurfaceCapabilitiesKHR capabilities{};
    capabilities.currentExtent = current;
    capabilities.minImageExtent = {1, 1};
    capabilities.maxImageExtent = {4096, 4096};
    return capabilities;
  }

  TEST(VkSwapchainSizingTest, DrawsWhenTheWindowKeepsItsSize)
  {
    EXPECT_EQ(VK_SwapchainSizing::Decide({1280, 720}, {1280, 720}, false), VK_FrameSizing::Draw);
  }

  TEST(VkSwapchainSizingTest, MakesTheSwapchainAgainWhenTheWindowWasResized)
  {
    EXPECT_EQ(VK_SwapchainSizing::Decide({1600, 900}, {1280, 720}, false), VK_FrameSizing::Recreate);
    EXPECT_EQ(VK_SwapchainSizing::Decide({1280, 900}, {1280, 720}, false), VK_FrameSizing::Recreate);
    EXPECT_EQ(VK_SwapchainSizing::Decide({1600, 720}, {1280, 720}, false), VK_FrameSizing::Recreate);
  }

  TEST(VkSwapchainSizingTest, MakesTheSwapchainAgainWhenVulkanSaysItNoLongerFits)
  {
    EXPECT_EQ(VK_SwapchainSizing::Decide({1280, 720}, {1280, 720}, true), VK_FrameSizing::Recreate);
  }

  TEST(VkSwapchainSizingTest, SkipsTheFrameWhileTheWindowHasNoArea)
  {
    EXPECT_EQ(VK_SwapchainSizing::Decide({0, 0}, {1280, 720}, false), VK_FrameSizing::Skip);
    EXPECT_EQ(VK_SwapchainSizing::Decide({0, 0}, {1280, 720}, true), VK_FrameSizing::Skip);
    EXPECT_EQ(VK_SwapchainSizing::Decide({1280, 0}, {1280, 720}, false), VK_FrameSizing::Skip);
    EXPECT_EQ(VK_SwapchainSizing::Decide({0, 720}, {1280, 720}, false), VK_FrameSizing::Skip);
  }

  TEST(VkSwapchainSizingTest, MakesTheSwapchainAgainWhenAMinimizedWindowComesBack)
  {
    // A window that is restored at another size than it was minimized at.
    // The swapchain still has the size from before.
    EXPECT_EQ(VK_SwapchainSizing::Decide({0, 0}, {1280, 720}, false), VK_FrameSizing::Skip);
    EXPECT_EQ(VK_SwapchainSizing::Decide({800, 600}, {1280, 720}, false), VK_FrameSizing::Recreate);
  }

  TEST(VkSwapchainSizingTest, KnowsWhichResultsSayTheSwapchainNoLongerFits)
  {
    EXPECT_TRUE(VK_SwapchainSizing::IsStale(VK_ERROR_OUT_OF_DATE_KHR));
    EXPECT_TRUE(VK_SwapchainSizing::IsStale(VK_SUBOPTIMAL_KHR));

    EXPECT_FALSE(VK_SwapchainSizing::IsStale(VK_SUCCESS));
    EXPECT_FALSE(VK_SwapchainSizing::IsStale(VK_TIMEOUT));
    EXPECT_FALSE(VK_SwapchainSizing::IsStale(VK_ERROR_SURFACE_LOST_KHR));
    EXPECT_FALSE(VK_SwapchainSizing::IsStale(VK_ERROR_DEVICE_LOST));
  }

  TEST(VkSwapchainSizingTest, TakesTheSizeTheWindowDictates)
  {
    const VkExtent2D extent = VK_SwapchainSizing::ExtentOf(CapabilitiesWith({1600, 900}), {1280, 720});

    EXPECT_EQ(extent.width, 1600u);
    EXPECT_EQ(extent.height, 900u);
  }

  TEST(VkSwapchainSizingTest, TakesTheSizeOfAMinimizedWindowAsNoArea)
  {
    const VkExtent2D extent = VK_SwapchainSizing::ExtentOf(CapabilitiesWith({0, 0}), {1280, 720});

    EXPECT_EQ(extent.width, 0u);
    EXPECT_EQ(extent.height, 0u);
  }

  TEST(VkSwapchainSizingTest, TakesTheWantedSizeWhenTheWindowLeavesItOpen)
  {
    const VkExtent2D extent = VK_SwapchainSizing::ExtentOf(CapabilitiesWith({UINT32_MAX, UINT32_MAX}), {1280, 720});

    EXPECT_EQ(extent.width, 1280u);
    EXPECT_EQ(extent.height, 720u);
  }

  TEST(VkSwapchainSizingTest, KeepsTheWantedSizeWithinWhatTheWindowAllows)
  {
    const auto capabilities = CapabilitiesWith({UINT32_MAX, UINT32_MAX});

    const VkExtent2D large = VK_SwapchainSizing::ExtentOf(capabilities, {8000, 6000});
    EXPECT_EQ(large.width, 4096u);
    EXPECT_EQ(large.height, 4096u);

    const VkExtent2D small = VK_SwapchainSizing::ExtentOf(capabilities, {0, 0});
    EXPECT_EQ(small.width, 1u);
    EXPECT_EQ(small.height, 1u);
  }
} // namespace
