#include "vk-culling.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

namespace
{
  using neon::AlphaMode;
  using neon::VK_Culling;

  TEST(VkCullingTest, LeavesOutTheBackOfAOneSidedMaterial)
  {
    EXPECT_EQ(VK_Culling::CullModeFor(false), VK_CULL_MODE_BACK_BIT);
  }

  TEST(VkCullingTest, DrawsBothSidesOfADoubleSidedMaterial)
  {
    EXPECT_EQ(VK_Culling::CullModeFor(true), VK_CULL_MODE_NONE);
  }

  TEST(VkCullingTest, TakesTheCounterclockwiseSideAsTheFront)
  {
    EXPECT_EQ(VK_Culling::FrontFaceFor(false), VK_FRONT_FACE_COUNTER_CLOCKWISE);
  }

  TEST(VkCullingTest, TakesTheClockwiseSideAsTheFrontOfWhatIsMirrored)
  {
    EXPECT_EQ(VK_Culling::FrontFaceFor(true), VK_FRONT_FACE_CLOCKWISE);
  }

  TEST(VkCullingTest, DoesNotTakeMovingTurningOrScalingForMirroring)
  {
    glm::mat4 model = translate(glm::mat4{1.0f}, glm::vec3(1.0f, -2.0f, 3.0f));
    model = rotate(model, glm::radians(135.0f), glm::vec3(0.3f, 1.0f, -0.2f));
    model = scale(model, glm::vec3(2.0f, 0.5f, 7.0f));

    EXPECT_FALSE(VK_Culling::IsMirrored(model));
  }

  TEST(VkCullingTest, TakesOneAxisScaledBelowZeroForMirroring)
  {
    EXPECT_TRUE(VK_Culling::IsMirrored(scale(glm::mat4{1.0f}, glm::vec3(-1.0f, 1.0f, 1.0f))));
  }

  TEST(VkCullingTest, TakesTwoAxesScaledBelowZeroForATurn)
  {
    // the same as half a turn about the third axis
    EXPECT_FALSE(VK_Culling::IsMirrored(scale(glm::mat4{1.0f}, glm::vec3(-1.0f, -1.0f, 1.0f))));
  }

  TEST(VkCullingTest, TakesThreeAxesScaledBelowZeroForMirroring)
  {
    EXPECT_TRUE(VK_Culling::IsMirrored(scale(glm::mat4{1.0f}, glm::vec3(-1.0f, -2.0f, -0.5f))));
  }

  TEST(VkCullingTest, GivesEveryWayOfDrawingAShaderAPipelineOfItsOwn)
  {
    const std::string shader = "engine://shaders/basic-lit";

    const std::vector keys{
      VK_Culling::PipelineKey(shader, AlphaMode::Opaque, false, false),
      VK_Culling::PipelineKey(shader, AlphaMode::Opaque, false, true),
      VK_Culling::PipelineKey(shader, AlphaMode::Opaque, true, false),
      VK_Culling::PipelineKey(shader, AlphaMode::Blend, false, false),
      VK_Culling::PipelineKey(shader, AlphaMode::Blend, false, true),
      VK_Culling::PipelineKey(shader, AlphaMode::Blend, true, false),
    };

    for (std::size_t i = 0; i < keys.size(); i++)
    {
      for (std::size_t j = i + 1; j < keys.size(); j++) { EXPECT_NE(keys[i], keys[j]) << i << " and " << j; }
    }
  }

  TEST(VkCullingTest, SharesThePipelineOfADoubleSidedMaterialMirroredOrNot)
  {
    EXPECT_EQ(
      VK_Culling::PipelineKey("s", AlphaMode::Opaque, true, false),
      VK_Culling::PipelineKey("s", AlphaMode::Opaque, true, true));
  }

  TEST(VkCullingTest, KeepsTheKeyOfAnOpaqueOneSidedMaterialAsTheShader)
  {
    EXPECT_EQ(VK_Culling::PipelineKey("s", AlphaMode::Opaque, false, false), "s");
  }
} // namespace
