#include "vk-samplers.hpp"

#include <gtest/gtest.h>

// What every sampler is made with is worked out before Vulkan is called, so
// it is checked here. Making one needs a device and is not tested.

namespace
{
  using neon::VK_Samplers;
  using neon::VK_Sampling;

  TEST(VkSamplersTest, HasNothingUntilItIsInitialized)
  {
    const VK_Samplers samplers;

    EXPECT_EQ(samplers.Of(VK_Sampling::AnisotropicRepeat), VK_NULL_HANDLE);
    EXPECT_EQ(samplers.Of(VK_Sampling::NearestClamp), VK_NULL_HANDLE);
  }

  TEST(VkSamplersTest, CanBeCleanedUpWithoutEverBeingInitialized)
  {
    VK_Samplers samplers;
    samplers.CleanUp();
    samplers.CleanUp();

    EXPECT_EQ(samplers.Of(VK_Sampling::LinearClamp), VK_NULL_HANDLE);
  }

  TEST(VkSamplersTest, ReadsTheTextureOfAModelSmoothlyFromTheSideAndAgainPastItsEdge)
  {
    const VkSamplerCreateInfo description = VK_Samplers::DescriptionOf(VK_Sampling::AnisotropicRepeat, 16.0f);

    EXPECT_EQ(description.magFilter, VK_FILTER_LINEAR);
    EXPECT_EQ(description.minFilter, VK_FILTER_LINEAR);
    EXPECT_EQ(description.mipmapMode, VK_SAMPLER_MIPMAP_MODE_LINEAR);
    EXPECT_EQ(description.addressModeU, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    EXPECT_EQ(description.addressModeV, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    EXPECT_EQ(description.addressModeW, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    EXPECT_EQ(description.anisotropyEnable, VK_TRUE);
    EXPECT_FLOAT_EQ(description.maxAnisotropy, 8.0f);
    EXPECT_FLOAT_EQ(description.maxLod, VK_LOD_CLAMP_NONE);
  }

  TEST(VkSamplersTest, DrawsTheEdgeOfARenderTargetOnPastIt)
  {
    const VkSamplerCreateInfo description = VK_Samplers::DescriptionOf(VK_Sampling::AnisotropicClamp, 16.0f);

    EXPECT_EQ(description.addressModeU, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    EXPECT_EQ(description.addressModeV, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    EXPECT_EQ(description.anisotropyEnable, VK_TRUE);
    EXPECT_FLOAT_EQ(description.maxAnisotropy, 8.0f);
  }

  TEST(VkSamplersTest, TakesNoMoreSamplesFromTheSideThanTheGraphicsCardAllows)
  {
    const VkSamplerCreateInfo description = VK_Samplers::DescriptionOf(VK_Sampling::AnisotropicRepeat, 4.0f);

    EXPECT_EQ(description.anisotropyEnable, VK_TRUE);
    EXPECT_FLOAT_EQ(description.maxAnisotropy, 4.0f);
  }

  TEST(VkSamplersTest, ReadsStraightOnWhenTheGraphicsCardCannotReadFromTheSide)
  {
    const VkSamplerCreateInfo description = VK_Samplers::DescriptionOf(VK_Sampling::AnisotropicRepeat, 0.0f);

    EXPECT_EQ(description.anisotropyEnable, VK_FALSE);
    EXPECT_FLOAT_EQ(description.maxAnisotropy, 1.0f);
  }

  TEST(VkSamplersTest, ReadsAnImageOfAUserInterfaceSmoothlyAndStraightOn)
  {
    const VkSamplerCreateInfo clamped = VK_Samplers::DescriptionOf(VK_Sampling::LinearClamp, 16.0f);
    const VkSamplerCreateInfo repeating = VK_Samplers::DescriptionOf(VK_Sampling::LinearRepeat, 16.0f);

    EXPECT_EQ(clamped.magFilter, VK_FILTER_LINEAR);
    EXPECT_EQ(clamped.mipmapMode, VK_SAMPLER_MIPMAP_MODE_LINEAR);
    EXPECT_EQ(clamped.addressModeU, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    EXPECT_EQ(clamped.anisotropyEnable, VK_FALSE);
    EXPECT_FLOAT_EQ(clamped.maxAnisotropy, 1.0f);
    EXPECT_FLOAT_EQ(clamped.maxLod, VK_LOD_CLAMP_NONE);

    EXPECT_EQ(repeating.addressModeU, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    EXPECT_EQ(repeating.addressModeV, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    EXPECT_EQ(repeating.anisotropyEnable, VK_FALSE);
  }

  TEST(VkSamplersTest, ReadsPixelByPixelFromTheImageItselfAlone)
  {
    const VkSamplerCreateInfo description = VK_Samplers::DescriptionOf(VK_Sampling::NearestClamp, 16.0f);

    EXPECT_EQ(description.magFilter, VK_FILTER_NEAREST);
    EXPECT_EQ(description.minFilter, VK_FILTER_NEAREST);
    EXPECT_EQ(description.mipmapMode, VK_SAMPLER_MIPMAP_MODE_NEAREST);
    EXPECT_EQ(description.addressModeU, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    EXPECT_EQ(description.anisotropyEnable, VK_FALSE);
    EXPECT_FLOAT_EQ(description.maxLod, 0.0f);
    EXPECT_EQ(description.compareEnable, VK_FALSE);
  }

  TEST(VkSamplersTest, ComparesTheShadowMapAndLightsWhatIsPastItsEdge)
  {
    const VkSamplerCreateInfo description = VK_Samplers::DescriptionOf(VK_Sampling::ShadowCompare, 16.0f);

    EXPECT_EQ(description.compareEnable, VK_TRUE);
    EXPECT_EQ(description.compareOp, VK_COMPARE_OP_LESS_OR_EQUAL);
    EXPECT_EQ(description.magFilter, VK_FILTER_LINEAR);
    EXPECT_EQ(description.minFilter, VK_FILTER_LINEAR);
    EXPECT_EQ(description.addressModeU, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    EXPECT_EQ(description.addressModeV, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
    EXPECT_EQ(description.borderColor, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
    EXPECT_EQ(description.anisotropyEnable, VK_FALSE);
    EXPECT_FLOAT_EQ(description.maxLod, 0.0f);
  }

  TEST(VkSamplersTest, ComparesNothingButTheShadowMap)
  {
    for (const auto sampling : {
           VK_Sampling::AnisotropicRepeat, VK_Sampling::AnisotropicClamp, VK_Sampling::LinearRepeat,
           VK_Sampling::LinearClamp, VK_Sampling::NearestClamp})
    {
      EXPECT_EQ(VK_Samplers::DescriptionOf(sampling, 16.0f).compareEnable, VK_FALSE);
    }
  }
}
