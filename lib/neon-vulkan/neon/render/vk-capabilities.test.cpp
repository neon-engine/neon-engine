#include "vk-capabilities.hpp"

#include <cstring>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// What a graphics card can do, from what Vulkan says about it. Asking
// Vulkan needs a graphics card, and is left to running the runtime.

namespace
{
  using neon::ApiVersion;
  using neon::PresentMode;
  using neon::VK_Capabilities;
  using neon::VK_DeviceAnswers;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  constexpr VkFormatFeatureFlags scene_features =
    VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT |
    VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
  constexpr VkFormatFeatureFlags texture_features =
    VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

  /// What a graphics card of today says: everything the engine uses.
  VK_DeviceAnswers AnswersOfACard()
  {
    VK_DeviceAnswers answers;
    std::strcpy(answers.properties.deviceName, "Some Graphics Card");
    answers.properties.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 290);
    answers.properties.limits.maxImageDimension2D = 16384;
    answers.properties.limits.framebufferColorSampleCounts =
      VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT;
    answers.properties.limits.framebufferDepthSampleCounts =
      VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT;
    answers.properties.limits.maxSamplerAnisotropy = 16.0f;
    answers.features.samplerAnisotropy = VK_TRUE;
    answers.api_version = VK_API_VERSION_1_2;
    answers.present_modes = {VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_MAILBOX_KHR};
    answers.scene_format.optimalTilingFeatures = scene_features;
    answers.srgb_format.optimalTilingFeatures = texture_features;
    answers.has_mutable_format_views = true;
    return answers;
  }

  TEST(VkCapabilitiesTest, TakesTheVersionThatIsRenderedWith)
  {
    EXPECT_EQ(VK_Capabilities::Fill(AnswersOfACard()).api_version, (ApiVersion{1, 2}));
  }

  TEST(VkCapabilitiesTest, TakesTheVersionTheGraphicsCardOffersWithItsPatch)
  {
    EXPECT_EQ(VK_Capabilities::Fill(AnswersOfACard()).device_api_version, (ApiVersion{1, 3, 290}));
  }

  TEST(VkCapabilitiesTest, TakesTheNameOfTheGraphicsCard)
  {
    EXPECT_EQ(VK_Capabilities::Fill(AnswersOfACard()).device_name, "Some Graphics Card");
  }

  TEST(VkCapabilitiesTest, TakesTheLargestTextureOfTwoDimensions)
  {
    EXPECT_EQ(VK_Capabilities::Fill(AnswersOfACard()).max_texture_size, 16384);
  }

  TEST(VkCapabilitiesTest, TakesTheMostSamplesThatColorAndDepthBothHave)
  {
    VK_DeviceAnswers answers = AnswersOfACard();
    answers.properties.limits.framebufferColorSampleCounts |= VK_SAMPLE_COUNT_16_BIT;

    EXPECT_EQ(VK_Capabilities::Fill(answers).max_samples, 8);
  }

  TEST(VkCapabilitiesTest, CountsOneSampleWhenThereIsNoAntiAliasing)
  {
    EXPECT_EQ(VK_Capabilities::MostSamples(VK_SAMPLE_COUNT_1_BIT), 1);
    EXPECT_EQ(VK_Capabilities::MostSamples(0), 1);
  }

  TEST(VkCapabilitiesTest, CountsUpToSixtyFourSamples)
  {
    EXPECT_EQ(VK_Capabilities::MostSamples(VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_64_BIT), 64);
  }

  TEST(VkCapabilitiesTest, TakesTheLargestAnisotropy)
  {
    EXPECT_EQ(VK_Capabilities::Fill(AnswersOfACard()).max_anisotropy, 16.0f);
  }

  TEST(VkCapabilitiesTest, HasNoAnisotropyWhenTheFeatureIsMissing)
  {
    VK_DeviceAnswers answers = AnswersOfACard();
    answers.features.samplerAnisotropy = VK_FALSE;

    EXPECT_EQ(VK_Capabilities::Fill(answers).max_anisotropy, 0.0f);
  }

  TEST(VkCapabilitiesTest, ListsThePresentModesInTheOrderOfTheEngine)
  {
    VK_DeviceAnswers answers = AnswersOfACard();
    answers.present_modes = {
      VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_MAILBOX_KHR,
      VK_PRESENT_MODE_IMMEDIATE_KHR
    };

    EXPECT_THAT(
      VK_Capabilities::Fill(answers).present_modes,
      ElementsAre(PresentMode::Immediate, PresentMode::Mailbox, PresentMode::Fifo, PresentMode::FifoRelaxed));
  }

  TEST(VkCapabilitiesTest, LeavesOutPresentModesThatAreNoChoiceOfVerticalSync)
  {
    VK_DeviceAnswers answers = AnswersOfACard();
    answers.present_modes = {VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR, VK_PRESENT_MODE_FIFO_KHR};

    EXPECT_THAT(VK_Capabilities::Fill(answers).present_modes, ElementsAre(PresentMode::Fifo));
  }

  TEST(VkCapabilitiesTest, ListsAPresentModeOnceWhenItIsOfferedTwice)
  {
    VK_DeviceAnswers answers = AnswersOfACard();
    answers.present_modes = {VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_FIFO_KHR};

    EXPECT_THAT(VK_Capabilities::Fill(answers).present_modes, ElementsAre(PresentMode::Fifo));
  }

  TEST(VkCapabilitiesTest, HasNoPresentModesWithoutAWindow)
  {
    VK_DeviceAnswers answers = AnswersOfACard();
    answers.present_modes.clear();

    EXPECT_THAT(VK_Capabilities::Fill(answers).present_modes, IsEmpty());
  }

  TEST(VkCapabilitiesTest, HasTheFormatsTheEngineReliesOn)
  {
    const auto capabilities = VK_Capabilities::Fill(AnswersOfACard());

    EXPECT_TRUE(capabilities.has_scene_format);
    EXPECT_TRUE(capabilities.has_srgb_textures);
    EXPECT_TRUE(capabilities.has_mutable_format_views);
  }

  TEST(VkCapabilitiesTest, HasNoSceneFormatThatCannotBeBlended)
  {
    VK_DeviceAnswers answers = AnswersOfACard();
    answers.scene_format.optimalTilingFeatures &= ~VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;

    EXPECT_FALSE(VK_Capabilities::Fill(answers).has_scene_format);
  }

  TEST(VkCapabilitiesTest, HasNoSceneFormatWhenOnlyImagesInLinearTilingHaveIt)
  {
    VK_DeviceAnswers answers = AnswersOfACard();
    answers.scene_format.optimalTilingFeatures = 0;
    answers.scene_format.linearTilingFeatures = scene_features;

    EXPECT_FALSE(VK_Capabilities::Fill(answers).has_scene_format);
  }

  TEST(VkCapabilitiesTest, HasNoSrgbTexturesThatCannotBeFiltered)
  {
    VK_DeviceAnswers answers = AnswersOfACard();
    answers.srgb_format.optimalTilingFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;

    EXPECT_FALSE(VK_Capabilities::Fill(answers).has_srgb_textures);
  }

  TEST(VkCapabilitiesTest, HasNoMutableViewsWhenTheImageCannotBeMade)
  {
    VK_DeviceAnswers answers = AnswersOfACard();
    answers.has_mutable_format_views = false;

    EXPECT_FALSE(VK_Capabilities::Fill(answers).has_mutable_format_views);
  }

  TEST(VkCapabilitiesTest, DescribesWhatTheGraphicsCardCanDo)
  {
    EXPECT_THAT(
      VK_Capabilities::Describe(VK_Capabilities::Fill(AnswersOfACard())),
      ElementsAre(
        "Textures up to 16384 pixels wide, anti-aliasing up to 8 samples, anisotropic filtering up to 16",
        "Present modes: mailbox, fifo",
        "Scene image of R16G16B16A16_SFLOAT: yes, sRGB textures: yes, views of another format: yes"));
  }

  TEST(VkCapabilitiesTest, DescribesAGraphicsCardWithoutAWindowOrAnisotropy)
  {
    VK_DeviceAnswers answers = AnswersOfACard();
    answers.present_modes.clear();
    answers.features.samplerAnisotropy = VK_FALSE;

    EXPECT_THAT(
      VK_Capabilities::Describe(VK_Capabilities::Fill(answers)),
      ElementsAre(
        "Textures up to 16384 pixels wide, anti-aliasing up to 8 samples, no anisotropic filtering",
        "Present modes: none without a window",
        "Scene image of R16G16B16A16_SFLOAT: yes, sRGB textures: yes, views of another format: yes"));
  }
}
