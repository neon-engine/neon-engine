#include "vk-api-version.hpp"

#include <gtest/gtest.h>

// Which version of Vulkan is rendered with. Asking the loader and the
// graphics card needs a driver, and is left to running the runtime.

namespace
{
  using neon::ApiVersion;
  using neon::VK_ApiVersion;

  constexpr uint32_t version_1_0 = VK_API_VERSION_1_0;
  constexpr uint32_t version_1_1 = VK_API_VERSION_1_1;
  constexpr uint32_t version_1_2 = VK_API_VERSION_1_2;
  constexpr uint32_t version_1_3 = VK_API_VERSION_1_3;

  TEST(VkApiVersionTest, NeedsVulkanOneOne)
  {
    EXPECT_EQ(VK_ApiVersion::kMinimum, version_1_1);
  }

  TEST(VkApiVersionTest, TurnsTheVersionOfTheSettingsIntoTheOneOfVulkan)
  {
    EXPECT_EQ(VK_ApiVersion::FromCore({1, 2}), version_1_2);
  }

  TEST(VkApiVersionTest, LeavesThePatchOutOfAVersionThatIsAskedFor)
  {
    EXPECT_EQ(VK_ApiVersion::FromCore({1, 2, 99}), version_1_2);
  }

  TEST(VkApiVersionTest, KeepsThePatchOfAVersionThatIsOffered)
  {
    EXPECT_EQ(VK_ApiVersion::ToCore(VK_MAKE_API_VERSION(0, 1, 3, 290)), (ApiVersion{1, 3, 290}));
  }

  TEST(VkApiVersionTest, CreatesTheInstanceForWhatIsAskedForWhenTheLoaderOffersMore)
  {
    EXPECT_EQ(VK_ApiVersion::ForInstance(version_1_2, version_1_3), version_1_2);
  }

  TEST(VkApiVersionTest, CreatesTheInstanceForWhatTheLoaderOffersWhenItIsLess)
  {
    EXPECT_EQ(VK_ApiVersion::ForInstance(version_1_3, version_1_1), version_1_1);
  }

  TEST(VkApiVersionTest, LeavesThePatchOfTheLoaderOut)
  {
    EXPECT_EQ(VK_ApiVersion::ForInstance(version_1_3, VK_MAKE_API_VERSION(0, 1, 3, 290)), version_1_3);
  }

  TEST(VkApiVersionTest, RendersWithTheVersionOfTheInstanceWhenTheGraphicsCardOffersMore)
  {
    EXPECT_EQ(VK_ApiVersion::ForDevice(version_1_2, VK_MAKE_API_VERSION(0, 1, 4, 300)), version_1_2);
  }

  TEST(VkApiVersionTest, RendersWithTheVersionOfTheGraphicsCardWhenItOffersLess)
  {
    EXPECT_EQ(VK_ApiVersion::ForDevice(version_1_3, VK_MAKE_API_VERSION(0, 1, 2, 198)), version_1_2);
  }

  TEST(VkApiVersionTest, TakesTheLowestOfAllThree)
  {
    const uint32_t instance = VK_ApiVersion::ForInstance(version_1_2, version_1_3);

    EXPECT_EQ(VK_ApiVersion::ForDevice(instance, version_1_3), version_1_2);
  }

  TEST(VkApiVersionTest, IsEnoughFromVulkanOneOne)
  {
    EXPECT_FALSE(VK_ApiVersion::IsEnough(version_1_0));
    EXPECT_FALSE(VK_ApiVersion::IsEnough(VK_MAKE_API_VERSION(0, 1, 0, 250)));
    EXPECT_TRUE(VK_ApiVersion::IsEnough(version_1_1));
    EXPECT_TRUE(VK_ApiVersion::IsEnough(version_1_3));
  }
}
