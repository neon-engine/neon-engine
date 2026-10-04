#include "vk-effects.hpp"

#include <cstddef>
#include <gtest/gtest.h>

// The layout an effect of a camera is bound to. It has to match what
// effect.glsl declares, so it is written down here. Making it needs a
// device and is not tested.

namespace
{
  using neon::VK_Effects;

  TEST(VkEffectsTest, BindsThePictureApartFromTheSamplerItIsReadThrough)
  {
    EXPECT_EQ(VK_Effects::kBindings[0].binding, VK_Effects::kImage_Binding);
    EXPECT_EQ(VK_Effects::kBindings[0].descriptorType, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    EXPECT_EQ(VK_Effects::kBindings[0].stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
    EXPECT_EQ(VK_Effects::kBindings[1].binding, VK_Effects::kSampler_Binding);
    EXPECT_EQ(VK_Effects::kBindings[1].descriptorType, VK_DESCRIPTOR_TYPE_SAMPLER);
    EXPECT_EQ(VK_Effects::kBindings[1].stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
  }

  TEST(VkEffectsTest, BindsWhatTheShadersAreToldAsOneUniformBufferAfterThem)
  {
    EXPECT_EQ(VK_Effects::kBindings[2].binding, VK_Effects::kData_Binding);
    EXPECT_EQ(VK_Effects::kBindings[2].descriptorType, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    EXPECT_EQ(VK_Effects::kBindings[2].stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
    EXPECT_EQ(VK_Effects::kBindings[2].descriptorCount, 1u);
  }

  TEST(VkEffectsTest, BindsThePictureFirstTheSamplerNextAndTheNumbersLast)
  {
    EXPECT_EQ(VK_Effects::kImage_Binding, 0u);
    EXPECT_EQ(VK_Effects::kSampler_Binding, 1u);
    EXPECT_EQ(VK_Effects::kData_Binding, 2u);
  }

  TEST(VkEffectsTest, TellsAnEffectTheTimeAndThenTheEightNumbersOfTheGame)
  {
    // the block `FrameData` of effect.glsl: a vec4 and eight more
    EXPECT_EQ(offsetof(VK_Effects::Data, time), 0u);
    EXPECT_EQ(offsetof(VK_Effects::Data, numbers), 16u);
    EXPECT_EQ(sizeof(VK_Effects::Data), 144u);
    EXPECT_EQ(neon::RenderContext::kShader_Number_Places, 8);
  }

  TEST(VkEffectsTest, CanBeCleanedUpWithoutEverBeingInitialized)
  {
    VK_Effects effects;
    effects.CleanUp();
    effects.CleanUp();
  }
}
