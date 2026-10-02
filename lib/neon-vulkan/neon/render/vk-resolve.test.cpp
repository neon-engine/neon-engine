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

  TEST(VkResolveTest, BindsTheImageFirstAndTheSamplerNext)
  {
    EXPECT_EQ(VK_Resolve::kImage_Binding, 0u);
    EXPECT_EQ(VK_Resolve::kSampler_Binding, 1u);
  }

  TEST(VkResolveTest, CanBeCleanedUpWithoutEverBeingInitialized)
  {
    VK_Resolve resolve;
    resolve.CleanUp();
    resolve.CleanUp();
  }
}
