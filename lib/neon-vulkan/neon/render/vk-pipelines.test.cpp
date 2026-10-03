#include "vk-pipelines.hpp"

#include <gtest/gtest.h>

// The layout every shader of a model is bound to. It has to match what the
// shaders declare, and a binding that moves breaks every shader without an
// error, so it is written down here. Making the layout needs a device and
// is not tested.

namespace
{
  using neon::VK_Pipelines;

  TEST(VkPipelinesTest, HasNoLayoutUntilItIsInitialized)
  {
    const VK_Pipelines pipelines;

    EXPECT_EQ(pipelines.DescriptorLayout(), VK_NULL_HANDLE);
    EXPECT_EQ(pipelines.Layout(), VK_NULL_HANDLE);
  }

  TEST(VkPipelinesTest, BindsTheSceneAndTheObjectFirstForBothHalvesOfAShader)
  {
    // the scene by an offset into its buffer, the objects of the frame whole,
    // read by index
    constexpr VkShaderStageFlags both = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    EXPECT_EQ(VK_Pipelines::kBindings[0].binding, 0u);
    EXPECT_EQ(VK_Pipelines::kBindings[0].descriptorType, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    EXPECT_EQ(VK_Pipelines::kBindings[0].stageFlags, both);
    EXPECT_EQ(VK_Pipelines::kBindings[1].binding, 1u);
    EXPECT_EQ(VK_Pipelines::kBindings[1].descriptorType, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    EXPECT_EQ(VK_Pipelines::kBindings[1].stageFlags, both);
  }

  TEST(VkPipelinesTest, BindsTheTexturesApartFromTheirSamplersTwoBindingsOn)
  {
    // a texture is bound as an image alone, and the sampler it is read
    // through two bindings on, so that every target of the shaders binds
    // what the source says
    EXPECT_EQ(VK_Pipelines::kTexture_Count, 2u);
    EXPECT_EQ(VK_Pipelines::kFirst_Sampler_Binding, VK_Pipelines::kFirst_Texture_Binding + VK_Pipelines::kTexture_Count);

    for (uint32_t i = 0; i < VK_Pipelines::kTexture_Count; i++)
    {
      const auto &texture = VK_Pipelines::kBindings[2 + i];
      const auto &sampler = VK_Pipelines::kBindings[2 + VK_Pipelines::kTexture_Count + i];

      EXPECT_EQ(texture.binding, VK_Pipelines::kFirst_Texture_Binding + i);
      EXPECT_EQ(texture.descriptorType, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
      EXPECT_EQ(texture.stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
      EXPECT_EQ(texture.descriptorCount, 1u);

      EXPECT_EQ(sampler.binding, VK_Pipelines::kFirst_Sampler_Binding + i);
      EXPECT_EQ(sampler.descriptorType, VK_DESCRIPTOR_TYPE_SAMPLER);
      EXPECT_EQ(sampler.stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
      EXPECT_EQ(sampler.descriptorCount, 1u);
    }
  }

  TEST(VkPipelinesTest, BindsTheShadowMapApartFromItsSamplerAfterTheTextures)
  {
    EXPECT_EQ(VK_Pipelines::kShadow_Map_Binding, VK_Pipelines::kFirst_Sampler_Binding + VK_Pipelines::kTexture_Count);
    EXPECT_EQ(VK_Pipelines::kShadow_Sampler_Binding, VK_Pipelines::kShadow_Map_Binding + 1);

    const auto &map = VK_Pipelines::kBindings[VK_Pipelines::kShadow_Map_Binding];
    const auto &sampler = VK_Pipelines::kBindings[VK_Pipelines::kShadow_Sampler_Binding];

    EXPECT_EQ(map.binding, VK_Pipelines::kShadow_Map_Binding);
    EXPECT_EQ(map.descriptorType, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    EXPECT_EQ(map.stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
    EXPECT_EQ(sampler.binding, VK_Pipelines::kShadow_Sampler_Binding);
    EXPECT_EQ(sampler.descriptorType, VK_DESCRIPTOR_TYPE_SAMPLER);
    EXPECT_EQ(sampler.stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
  }

  TEST(VkPipelinesTest, NamesTheShaderOfTheShadowPassWithoutAnExtension)
  {
    EXPECT_STREQ(VK_Pipelines::kShadow_Shader_Path, "assets://shaders/shadow");
    EXPECT_GT(VK_Pipelines::kShadow_Slope_Bias, 0.0f);
  }

  TEST(VkPipelinesTest, BindsNothingAsACombinedImageSampler)
  {
    for (const auto &binding : VK_Pipelines::kBindings)
    {
      EXPECT_NE(binding.descriptorType, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) << "binding " << binding.binding;
      EXPECT_EQ(binding.pImmutableSamplers, nullptr) << "binding " << binding.binding;
    }
  }

  TEST(VkPipelinesTest, NumbersItsBindingsInOrderWithoutAGap)
  {
    for (std::size_t i = 0; i < VK_Pipelines::kBindings.size(); i++)
    {
      EXPECT_EQ(VK_Pipelines::kBindings[i].binding, i);
    }
  }
}
