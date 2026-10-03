#include "vk-shadow-casting.hpp"

#include <cstdint>

#include <gtest/gtest.h>

// Which draws of an object go into the shadow map is worked out before
// Vulkan is called, so it is checked here, with pipelines that are numbers.

namespace
{
  using neon::VK_ShadowCasting;

  VkPipeline Pipeline(const int n) { return reinterpret_cast<VkPipeline>(static_cast<std::uintptr_t>(n)); }

  TEST(VkShadowCastingTest, AnObjectWhoseMaterialsAllCastAlikeCastsItsWholeModelOnce)
  {
    const auto casts = VK_ShadowCasting::Plan({
      {.casts = true, .pipeline = Pipeline(1), .model_material = 0},
      {.casts = true, .pipeline = Pipeline(1), .model_material = 1},
      {.casts = true, .pipeline = Pipeline(1), .model_material = 2}});

    ASSERT_EQ(casts.size(), 3u);
    EXPECT_TRUE(casts[0].casts);
    EXPECT_EQ(casts[0].model_material, -1);
    EXPECT_FALSE(casts[1].casts);
    EXPECT_FALSE(casts[2].casts);
  }

  TEST(VkShadowCastingTest, AnObjectWithOneMaterialCastsAsItIs)
  {
    const auto casts = VK_ShadowCasting::Plan({{.casts = true, .pipeline = Pipeline(1), .model_material = -1}});

    ASSERT_EQ(casts.size(), 1u);
    EXPECT_TRUE(casts[0].casts);
    EXPECT_EQ(casts[0].model_material, -1);
  }

  TEST(VkShadowCastingTest, ASeeThroughMaterialAmongThemLeavesEachToCastItsOwnMeshes)
  {
    const auto casts = VK_ShadowCasting::Plan({
      {.casts = true, .pipeline = Pipeline(1), .model_material = 0},
      {.casts = false, .pipeline = VK_NULL_HANDLE, .model_material = 1}});

    ASSERT_EQ(casts.size(), 2u);
    EXPECT_TRUE(casts[0].casts);
    EXPECT_EQ(casts[0].model_material, 0);
    EXPECT_FALSE(casts[1].casts);
  }

  TEST(VkShadowCastingTest, MaterialsThatCastWithDifferentPipelinesEachCastTheirOwnMeshes)
  {
    // one material is double-sided, so its pipeline of the pass is another
    const auto casts = VK_ShadowCasting::Plan({
      {.casts = true, .pipeline = Pipeline(1), .model_material = 0},
      {.casts = true, .pipeline = Pipeline(2), .model_material = 3}});

    ASSERT_EQ(casts.size(), 2u);
    EXPECT_TRUE(casts[0].casts);
    EXPECT_EQ(casts[0].model_material, 0);
    EXPECT_TRUE(casts[1].casts);
    EXPECT_EQ(casts[1].model_material, 3);
  }

  TEST(VkShadowCastingTest, NothingCastsWhenTheLightCastsNoShadow)
  {
    const auto casts = VK_ShadowCasting::Plan({
      {.casts = false, .pipeline = VK_NULL_HANDLE, .model_material = 0},
      {.casts = false, .pipeline = VK_NULL_HANDLE, .model_material = 1}});

    EXPECT_FALSE(casts[0].casts);
    EXPECT_FALSE(casts[1].casts);
  }

  TEST(VkShadowCastingTest, AnObjectWithoutMaterialsCastsNothing)
  {
    EXPECT_TRUE(VK_ShadowCasting::Plan({}).empty());
  }
}
