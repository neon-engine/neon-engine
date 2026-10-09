#include "vk-shadow-fit.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include <gtest/gtest.h>

// Where the shadow map looks is worked out before Vulkan is called, so it
// is checked here: the cascades slice what the camera sees, each box
// holds its slice, the near slice is the finest, and a box moves in whole
// texels.

namespace
{
  using neon::VK_ShadowFit;
  using neon::VK_ShadowCascades;

  constexpr float map_size = 2048.0f;
  constexpr float distance = VK_ShadowFit::kDistance;
  const glm::vec3 down{0.0f, -1.0f, 0.0f};

  // where a point of the world lands in a box: x and y from -1 to 1
  // across it, z from -1 to 1 through it
  glm::vec3 Projected(const glm::mat4 &view_projection, const glm::vec3 &point)
  {
    const glm::vec4 clip = view_projection * glm::vec4(point, 1.0f);
    return glm::vec3(clip) / clip.w;
  }

  bool Inside(const glm::mat4 &view_projection, const glm::vec3 &point)
  {
    const glm::vec3 at = Projected(view_projection, point);
    return std::abs(at.x) <= 1.0001f && std::abs(at.y) <= 1.0001f && std::abs(at.z) <= 1.0001f;
  }

  // a camera at the origin looking down -z, as the scene has it
  const glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 2.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  const glm::mat4 projection = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 500.0f);

  TEST(VkShadowFitTest, SplitsEndAtTheDistanceAndGrowWithIt)
  {
    const auto splits = VK_ShadowFit::Splits(0.1f, 100.0f, 4);
    EXPECT_FLOAT_EQ(splits[3], 100.0f);
    EXPECT_LT(splits[0], splits[1]);
    EXPECT_LT(splits[1], splits[2]);
    EXPECT_LT(splits[2], splits[3]);
    // the near slice is thinner than an even quarter, the far one wider
    EXPECT_LT(splits[0], 25.0f);
    EXPECT_GT(splits[3] - splits[2], 25.0f);
  }

  TEST(VkShadowFitTest, OneCascadeIsTheWholeDistance)
  {
    const auto splits = VK_ShadowFit::Splits(0.1f, 40.0f, 1);
    EXPECT_FLOAT_EQ(splits[0], 40.0f);
  }

  TEST(VkShadowFitTest, EachCascadeHoldsItsSliceOfTheView)
  {
    const VK_ShadowCascades cascades = VK_ShadowFit::Cascades(view, projection, down, distance, 4, map_size);
    ASSERT_EQ(cascades.count, 4);

    const glm::mat4 inverse = glm::inverse(projection * view);
    float from = 0.1f;
    for (int i = 0; i < 4; i++)
    {
      const float to = cascades.splits[static_cast<std::size_t>(i)];
      for (const glm::vec3 &corner : VK_ShadowFit::SliceCorners(inverse, 0.1f, 500.0f, from, to))
      {
        EXPECT_TRUE(Inside(cascades.view_projections[static_cast<std::size_t>(i)], corner)) << "cascade " << i;
      }
      from = to;
    }
  }

  TEST(VkShadowFitTest, TheNearCascadeIsTheFinest)
  {
    const VK_ShadowCascades cascades = VK_ShadowFit::Cascades(view, projection, down, distance, 4, map_size);

    // a meter across the world is more of the near box than of the far one
    const auto across = [&](const int i)
    {
      const glm::mat4 &box = cascades.view_projections[static_cast<std::size_t>(i)];
      return glm::length(glm::vec2(Projected(box, glm::vec3(1.0f, 0.0f, 0.0f))) - glm::vec2(Projected(box, glm::vec3(0.0f))));
    };
    EXPECT_GT(across(0), across(1));
    EXPECT_GT(across(1), across(2));
    EXPECT_GT(across(2), across(3));
  }

  TEST(VkShadowFitTest, WhatIsAboveTheSliceTowardsTheLightIsInTheBox)
  {
    const VK_ShadowCascades cascades = VK_ShadowFit::Cascades(view, projection, down, distance, 4, map_size);

    // a caster high above the near slice, within the distance, is nearer
    // the light than the slice and still inside the box
    const glm::vec3 above(0.0f, 2.0f + distance * 0.9f, -2.0f);
    EXPECT_TRUE(Inside(cascades.view_projections[0], above));
  }

  TEST(VkShadowFitTest, ABoxMovesInWholeTexelsAsItsCenterMoves)
  {
    const glm::vec3 point{4.0f, 0.0f, 2.0f};
    const float texel_in_meters = 2.0f * 10.0f / map_size;

    const glm::mat4 before = VK_ShadowFit::BoxViewProjection(down, glm::vec3(0.0f), 10.0f, 50.0f, map_size);
    const glm::mat4 after = VK_ShadowFit::BoxViewProjection(down, glm::vec3(texel_in_meters * 7.3f, 0.0f, 0.0f), 10.0f, 50.0f, map_size);

    // the point moved in the map by whole texels, seven of them
    const glm::vec3 at = Projected(before, point);
    const glm::vec3 at_moved = Projected(after, point);
    const float texel = 2.0f / map_size;
    EXPECT_NEAR(glm::length(glm::vec2(at) - glm::vec2(at_moved)) / texel, 7.0f, 1e-2f);
  }

  TEST(VkShadowFitTest, HasAnUpForALightStraightDown)
  {
    const glm::mat4 light = VK_ShadowFit::View(down, glm::vec3(0.0f), 10.0f, 50.0f);
    for (int column = 0; column < 4; column++)
    {
      for (int row = 0; row < 4; row++) { EXPECT_TRUE(std::isfinite(light[column][row])); }
    }
    const glm::vec4 side = light * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_GT(std::abs(side.x) + std::abs(side.y), 0.5f);
  }

  TEST(VkShadowFitTest, TheCountIsHeldToWhatTheMapHas)
  {
    EXPECT_EQ(VK_ShadowFit::Cascades(view, projection, down, distance, 9, map_size).count, neon::kMax_Shadow_Cascades);
    EXPECT_EQ(VK_ShadowFit::Cascades(view, projection, down, distance, 0, map_size).count, 1);
  }
}
