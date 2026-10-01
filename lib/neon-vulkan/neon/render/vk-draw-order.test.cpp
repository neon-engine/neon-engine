#include "vk-draw-order.hpp"

#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

// The order see-through models are drawn in. Drawing them needs a graphics
// card, and is checked by the scene gamma-test.scene.yml.

namespace
{
  using neon::VK_DrawOrder;
  using neon::VK_SeeThroughDraw;

  VK_SeeThroughDraw At(const int model_id, const float distance)
  {
    VK_SeeThroughDraw draw;
    draw.model_id = model_id;
    draw.distance = distance;
    return draw;
  }

  std::vector<int> ModelsOf(const std::vector<VK_SeeThroughDraw> &draws)
  {
    std::vector<int> models;
    for (const auto &draw : draws) { models.push_back(draw.model_id); }
    return models;
  }

  TEST(VkDrawOrderTest, DrawsTheFarthestFirst)
  {
    std::vector draws{At(1, 2.0f), At(2, 10.0f), At(3, 5.0f)};

    VK_DrawOrder::BackToFront(draws);

    EXPECT_EQ(ModelsOf(draws), (std::vector{2, 3, 1}));
  }

  TEST(VkDrawOrderTest, KeepsTheOrderOfModelsAtTheSameDistance)
  {
    std::vector draws{At(1, 4.0f), At(2, 4.0f), At(3, 9.0f), At(4, 4.0f)};

    VK_DrawOrder::BackToFront(draws);

    EXPECT_EQ(ModelsOf(draws), (std::vector{3, 1, 2, 4}));
  }

  TEST(VkDrawOrderTest, MeasuresTheDistanceAlongWhereTheCameraLooks)
  {
    // a camera at 5 on z that looks towards the origin
    const glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    EXPECT_FLOAT_EQ(VK_DrawOrder::DistanceOf(view, glm::vec3(0.0f)), 5.0f);
    EXPECT_FLOAT_EQ(VK_DrawOrder::DistanceOf(view, glm::vec3(3.0f, 1.0f, -2.0f)), 7.0f)
      << "what is to the side is as far as its depth";
    EXPECT_LT(VK_DrawOrder::DistanceOf(view, glm::vec3(0.0f, 0.0f, 8.0f)), 0.0f) << "behind the camera";
  }
} // namespace
