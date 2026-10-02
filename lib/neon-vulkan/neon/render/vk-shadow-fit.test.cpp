#include "vk-shadow-fit.hpp"

#include <algorithm>
#include <cmath>

#include <gtest/gtest.h>

// Where the shadow map looks is worked out before Vulkan is called, so it
// is checked here: the box is around the camera, it is seen along the
// light, and it moves in whole texels.

namespace
{
  using neon::VK_ShadowFit;

  constexpr float map_size = 2048.0f;

  // where a point of the world lands in the map: x and y from -1 to 1
  // across it, z from -1 to 1 through it
  glm::vec3 Projected(const glm::mat4 &view_projection, const glm::vec3 &point)
  {
    const glm::vec4 clip = view_projection * glm::vec4(point, 1.0f);
    return glm::vec3(clip) / clip.w;
  }

  TEST(VkShadowFitTest, PutsTheCameraInTheMiddleOfTheMapAtItsMiddleDepth)
  {
    const glm::vec3 camera{3.0f, 1.5f, -7.0f};
    const glm::mat4 fit = VK_ShadowFit::ViewProjection(glm::vec3(0.0f, -1.0f, 0.0f), camera, map_size);
    const glm::vec3 at = Projected(fit, camera);

    // within a texel across, since the box is moved by whole texels
    const float texel = 2.0f / map_size;
    EXPECT_NEAR(at.x, 0.0f, texel);
    EXPECT_NEAR(at.y, 0.0f, texel);
    EXPECT_NEAR(at.z, 0.0f, 1e-4f);
  }

  TEST(VkShadowFitTest, LooksAlongTheLightSoThatWhatIsNearerTheLightIsNearerInTheMap)
  {
    const glm::vec3 camera{0.0f, 0.0f, 0.0f};
    const glm::vec3 direction{0.0f, -1.0f, 0.0f};
    const glm::mat4 fit = VK_ShadowFit::ViewProjection(direction, camera, map_size);

    const glm::vec3 high = Projected(fit, glm::vec3(0.0f, 10.0f, 0.0f));
    const glm::vec3 low = Projected(fit, glm::vec3(0.0f, -10.0f, 0.0f));

    EXPECT_LT(high.z, low.z);
    EXPECT_NEAR(high.z, -10.0f / VK_ShadowFit::kDepth, 1e-4f);
    EXPECT_NEAR(low.z, 10.0f / VK_ShadowFit::kDepth, 1e-4f);
  }

  TEST(VkShadowFitTest, CoversTheBoxAroundTheCameraAndNoMore)
  {
    const glm::vec3 camera{0.0f, 0.0f, 0.0f};
    const glm::mat4 fit = VK_ShadowFit::ViewProjection(glm::vec3(0.0f, -1.0f, 0.0f), camera, map_size);

    constexpr float half = VK_ShadowFit::kBox / 2.0f;
    const float texel = 2.0f / map_size;

    // along whichever axis of the map the world's x and z lie: half the
    // box from the camera is the edge of the map, and a step past it is
    // outside
    const auto across = [&fit](const glm::vec3 &point)
    {
      const glm::vec3 at = Projected(fit, point);
      return std::max(std::abs(at.x), std::abs(at.y));
    };

    EXPECT_NEAR(across(glm::vec3(half, 0.0f, 0.0f)), 1.0f, texel);
    EXPECT_NEAR(across(glm::vec3(-half, 0.0f, 0.0f)), 1.0f, texel);
    EXPECT_NEAR(across(glm::vec3(0.0f, 0.0f, half)), 1.0f, texel);
    EXPECT_GT(across(glm::vec3(half + 1.0f, 0.0f, 0.0f)), 1.0f);
    EXPECT_LT(across(glm::vec3(half - 1.0f, 0.0f, 0.0f)), 1.0f);
  }

  TEST(VkShadowFitTest, MovesInWholeTexelsAsTheCameraMoves)
  {
    const glm::vec3 direction{0.0f, -1.0f, 0.0f};
    const glm::vec3 point{4.0f, 0.0f, 2.0f};
    const float texel_in_metres = VK_ShadowFit::kBox / map_size;

    // a move of less than a texel does not move the point in the map,
    // a move of several moves it by whole texels, and only ever by whole
    // texels
    const glm::vec3 camera{0.0f, 0.0f, 0.0f};
    const glm::vec3 nearby = camera + glm::vec3(texel_in_metres * 0.3f, 0.0f, 0.0f);
    const glm::vec3 far = camera + glm::vec3(texel_in_metres * 7.0f, 0.0f, 0.0f);

    const glm::vec3 at = Projected(VK_ShadowFit::ViewProjection(direction, camera, map_size), point);
    const glm::vec3 at_nearby = Projected(VK_ShadowFit::ViewProjection(direction, nearby, map_size), point);
    const glm::vec3 at_far = Projected(VK_ShadowFit::ViewProjection(direction, far, map_size), point);

    EXPECT_NEAR(at.x, at_nearby.x, 1e-5f);
    EXPECT_NEAR(at.y, at_nearby.y, 1e-5f);

    // along whichever axis of the map the world's x lies
    const float texel = 2.0f / map_size;
    EXPECT_NEAR(glm::length(glm::vec2(at) - glm::vec2(at_far)) / texel, 7.0f, 1e-2f);
  }

  TEST(VkShadowFitTest, HasAnUpForALightStraightDown)
  {
    const glm::mat4 view = VK_ShadowFit::View(glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f));

    // a view that is finite, and a point to the side that lands to the side
    for (int column = 0; column < 4; column++)
    {
      for (int row = 0; row < 4; row++) { EXPECT_TRUE(std::isfinite(view[column][row])); }
    }

    const glm::vec4 aside = view * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_NEAR(std::abs(aside.x) + std::abs(aside.y), 1.0f, 1e-4f);
  }

  TEST(VkShadowFitTest, TakesTheDirectionWhateverItsLength)
  {
    const glm::vec3 camera{1.0f, 2.0f, 3.0f};
    const glm::mat4 unit = VK_ShadowFit::ViewProjection(glm::vec3(-0.6f, -0.8f, 0.0f), camera, map_size);
    const glm::mat4 longer = VK_ShadowFit::ViewProjection(glm::vec3(-6.0f, -8.0f, 0.0f), camera, map_size);

    const glm::vec3 point{5.0f, 0.0f, -2.0f};
    const glm::vec3 a = Projected(unit, point);
    const glm::vec3 b = Projected(longer, point);

    EXPECT_NEAR(a.x, b.x, 1e-4f);
    EXPECT_NEAR(a.y, b.y, 1e-4f);
    EXPECT_NEAR(a.z, b.z, 1e-4f);
  }
}
