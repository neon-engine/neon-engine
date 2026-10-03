#include "vk-resolve.hpp"

#include <gtest/gtest.h>

// The layout the shader of the resolve step is bound to. It has to match
// what resolve.frag declares, so it is written down here. Making it needs a
// device and is not tested.

namespace
{
  using neon::VK_Resolve;

  TEST(VkResolveTest, BindsTheSceneImageApartFromTheSamplerItIsReadThrough)
  {
    EXPECT_EQ(VK_Resolve::kBindings[0].binding, VK_Resolve::kImage_Binding);
    EXPECT_EQ(VK_Resolve::kBindings[0].descriptorType, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    EXPECT_EQ(VK_Resolve::kBindings[0].stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
    EXPECT_EQ(VK_Resolve::kBindings[1].binding, VK_Resolve::kSampler_Binding);
    EXPECT_EQ(VK_Resolve::kBindings[1].descriptorType, VK_DESCRIPTOR_TYPE_SAMPLER);
    EXPECT_EQ(VK_Resolve::kBindings[1].stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
  }

  TEST(VkResolveTest, BindsTheSettingsOfTheStepAsOneUniformBufferAfterThem)
  {
    EXPECT_EQ(VK_Resolve::kBindings[2].binding, VK_Resolve::kData_Binding);
    EXPECT_EQ(VK_Resolve::kBindings[2].descriptorType, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    EXPECT_EQ(VK_Resolve::kBindings[2].stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
    EXPECT_EQ(VK_Resolve::kBindings[2].descriptorCount, 1u);
  }

  TEST(VkResolveTest, BindsTheImageFirstTheSamplerNextAndTheSettingsLast)
  {
    EXPECT_EQ(VK_Resolve::kImage_Binding, 0u);
    EXPECT_EQ(VK_Resolve::kSampler_Binding, 1u);
    EXPECT_EQ(VK_Resolve::kData_Binding, 2u);
  }

  TEST(VkResolveTest, NumbersTheTonemappersAsTheSettingDoes)
  {
    // the shader reads the number, so the enum and resolve.frag agree
    EXPECT_EQ(static_cast<int>(neon::Tonemapper::None), 0);
    EXPECT_EQ(static_cast<int>(neon::Tonemapper::Aces), 1);
    EXPECT_EQ(static_cast<int>(neon::Tonemapper::Agx), 2);
  }

  TEST(VkResolveTest, HandsTheShaderOneVec4OfSettings)
  {
    const VK_Resolve::Data data;

    EXPECT_EQ(sizeof(data), 16u);
    EXPECT_EQ(data.tonemapper, 0.0f);
    EXPECT_EQ(data.exposure, 1.0f);
  }

  TEST(VkResolveTest, CanBeCleanedUpWithoutEverBeingInitialized)
  {
    VK_Resolve resolve;
    resolve.CleanUp();
    resolve.CleanUp();
  }
}
