#include "vk-surface-format.hpp"

#include <vector>

#include <gtest/gtest.h>

// Which format the window is drawn in is chosen from the list it offers,
// which needs no graphics card.

namespace
{
  using neon::ChooseSurfaceFormat;
  using neon::IsSrgbFormat;

  constexpr VkColorSpaceKHR srgb_screen = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

  TEST(VkSurfaceFormatTest, TakesPlainBytesWhereverTheyAreInTheList)
  {
    // the frame holds sRGB colors already, and an sRGB format would
    // convert them again
    const std::vector<VkSurfaceFormatKHR> formats = {
      {VK_FORMAT_B8G8R8A8_SRGB, srgb_screen},
      {VK_FORMAT_B8G8R8A8_UNORM, srgb_screen},
    };

    EXPECT_EQ(ChooseSurfaceFormat(formats).format, VK_FORMAT_B8G8R8A8_UNORM);
  }

  TEST(VkSurfaceFormatTest, TakesPlainBytesInEitherColorOrder)
  {
    const std::vector<VkSurfaceFormatKHR> formats = {
      {VK_FORMAT_R8G8B8A8_SRGB, srgb_screen},
      {VK_FORMAT_R8G8B8A8_UNORM, srgb_screen},
    };

    EXPECT_EQ(ChooseSurfaceFormat(formats).format, VK_FORMAT_R8G8B8A8_UNORM);
  }

  TEST(VkSurfaceFormatTest, TakesTheFirstWhenThereAreNoPlainBytes)
  {
    const std::vector<VkSurfaceFormatKHR> formats = {
      {VK_FORMAT_B8G8R8A8_SRGB, srgb_screen},
      {VK_FORMAT_A2B10G10R10_UNORM_PACK32, srgb_screen},
    };

    EXPECT_EQ(ChooseSurfaceFormat(formats).format, VK_FORMAT_B8G8R8A8_SRGB);
  }

  TEST(VkSurfaceFormatTest, KnowsWhichFormatsConvertToSrgb)
  {
    EXPECT_TRUE(IsSrgbFormat(VK_FORMAT_B8G8R8A8_SRGB));
    EXPECT_TRUE(IsSrgbFormat(VK_FORMAT_R8G8B8A8_SRGB));
    EXPECT_FALSE(IsSrgbFormat(VK_FORMAT_B8G8R8A8_UNORM));
    EXPECT_FALSE(IsSrgbFormat(VK_FORMAT_R8G8B8A8_UNORM));
  }
} // namespace
