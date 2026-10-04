#include "vk-shader-data.hpp"

#include <cstddef>
#include <memory>

#include <gtest/gtest.h>

// The structures are copied to the graphics card byte by byte, and read there
// by scene-data.glsl. A field that moves breaks every shader without an
// error, so where each one lies is written down here.

namespace
{
  using neon::VK_DirectionLight;
  using neon::VK_ObjectData;
  using neon::VK_PointLight;
  using neon::VK_SceneData;
  using neon::VK_SpotLight;

  constexpr std::size_t vec4 = 16;
  constexpr std::size_t mat4 = 64;

  TEST(VkShaderData, HoldsAsManyLightsAsTheShadersDo)
  {
    EXPECT_EQ(neon::kMax_Point_Lights, 64);
    EXPECT_EQ(neon::kMax_Spot_Lights, 64);
  }

  TEST(VkShaderData, LaysOutADirectionLightAsTheShadersReadIt)
  {
    EXPECT_EQ(offsetof(VK_DirectionLight, direction), 0 * vec4);
    EXPECT_EQ(offsetof(VK_DirectionLight, ambient), 1 * vec4);
    EXPECT_EQ(offsetof(VK_DirectionLight, diffuse), 2 * vec4);
    EXPECT_EQ(offsetof(VK_DirectionLight, specular), 3 * vec4);
    EXPECT_EQ(offsetof(VK_DirectionLight, cascades), 4 * vec4);
    EXPECT_EQ(offsetof(VK_DirectionLight, splits), 4 * vec4 + neon::kMax_Shadow_Cascades * mat4);
    EXPECT_EQ(offsetof(VK_DirectionLight, shadow), 5 * vec4 + neon::kMax_Shadow_Cascades * mat4);
    EXPECT_EQ(sizeof(VK_DirectionLight), 6 * vec4 + neon::kMax_Shadow_Cascades * mat4);
  }

  TEST(VkShaderData, LaysOutAPointLightAsTheShadersReadIt)
  {
    EXPECT_EQ(offsetof(VK_PointLight, position), 0 * vec4);
    EXPECT_EQ(offsetof(VK_PointLight, ambient), 1 * vec4);
    EXPECT_EQ(offsetof(VK_PointLight, diffuse), 2 * vec4);
    EXPECT_EQ(offsetof(VK_PointLight, specular), 3 * vec4);
    EXPECT_EQ(offsetof(VK_PointLight, attenuation), 4 * vec4);
    EXPECT_EQ(sizeof(VK_PointLight), 5 * vec4);
  }

  TEST(VkShaderData, LaysOutASpotLightAsTheShadersReadIt)
  {
    EXPECT_EQ(offsetof(VK_SpotLight, position), 0 * vec4);
    EXPECT_EQ(offsetof(VK_SpotLight, direction), 1 * vec4);
    EXPECT_EQ(offsetof(VK_SpotLight, ambient), 2 * vec4);
    EXPECT_EQ(offsetof(VK_SpotLight, diffuse), 3 * vec4);
    EXPECT_EQ(offsetof(VK_SpotLight, specular), 4 * vec4);
    EXPECT_EQ(offsetof(VK_SpotLight, attenuation), 5 * vec4);
    EXPECT_EQ(offsetof(VK_SpotLight, cutoff), 6 * vec4);
    EXPECT_EQ(sizeof(VK_SpotLight), 7 * vec4);
  }

  TEST(VkShaderData, LaysOutTheSceneAsTheShadersReadIt)
  {
    const std::size_t lights = 2 * mat4 + vec4 + sizeof(VK_DirectionLight) + vec4;

    EXPECT_EQ(offsetof(VK_SceneData, view), 0u);
    EXPECT_EQ(offsetof(VK_SceneData, projection), mat4);
    EXPECT_EQ(offsetof(VK_SceneData, view_position), 2 * mat4);
    EXPECT_EQ(offsetof(VK_SceneData, direction_light), 2 * mat4 + vec4);
    EXPECT_EQ(offsetof(VK_SceneData, light_counts), 2 * mat4 + vec4 + sizeof(VK_DirectionLight));
    EXPECT_EQ(offsetof(VK_SceneData, point_lights), lights);
    EXPECT_EQ(offsetof(VK_SceneData, spot_lights), lights + 64 * sizeof(VK_PointLight));
    EXPECT_EQ(sizeof(VK_SceneData), lights + 64 * sizeof(VK_PointLight) + 64 * sizeof(VK_SpotLight));
  }

  TEST(VkShaderData, LaysOutAnObjectAsTheShadersReadIt)
  {
    EXPECT_EQ(offsetof(VK_ObjectData, model), 0u);
    EXPECT_EQ(offsetof(VK_ObjectData, normal_matrix), mat4);
    EXPECT_EQ(offsetof(VK_ObjectData, color), 2 * mat4);
    EXPECT_EQ(offsetof(VK_ObjectData, texture_scale), 2 * mat4 + vec4);
    EXPECT_EQ(offsetof(VK_ObjectData, material), 2 * mat4 + 2 * vec4);
    EXPECT_EQ(offsetof(VK_ObjectData, surface), 2 * mat4 + 3 * vec4);
    EXPECT_EQ(offsetof(VK_ObjectData, emissive), 2 * mat4 + 4 * vec4);
    EXPECT_EQ(offsetof(VK_ObjectData, lightmap), 2 * mat4 + 5 * vec4);
    EXPECT_EQ(sizeof(VK_ObjectData), 2 * mat4 + 6 * vec4);
  }

  TEST(VkShaderData, StartsASceneWithoutLights)
  {
    const auto scene = std::make_unique<VK_SceneData>();

    EXPECT_EQ(scene->view, glm::mat4(1.0f));
    EXPECT_EQ(scene->projection, glm::mat4(1.0f));
    EXPECT_EQ(scene->view_position, glm::vec4(0.0f));
    EXPECT_EQ(scene->light_counts, glm::ivec4(0));
    EXPECT_EQ(scene->direction_light.diffuse, glm::vec4(0.0f));
    EXPECT_EQ(scene->direction_light.cascades[3], glm::mat4(1.0f));
    EXPECT_EQ(scene->direction_light.splits, glm::vec4(0.0f));
    EXPECT_EQ(scene->direction_light.shadow, glm::vec4(0.0f));
    EXPECT_EQ(scene->point_lights[63].attenuation, glm::vec4(0.0f));
    EXPECT_EQ(scene->spot_lights[63].cutoff, glm::vec4(0.0f));
  }

  TEST(VkShaderData, StartsAnObjectWhiteAtItsPlaceWithTexturesAtTheirSize)
  {
    const VK_ObjectData object;

    EXPECT_EQ(object.model, glm::mat4(1.0f));
    EXPECT_EQ(object.normal_matrix, glm::mat4(1.0f));
    EXPECT_EQ(object.color, glm::vec4(1.0f));
    EXPECT_EQ(object.texture_scale, glm::vec4(1.0f));
    EXPECT_EQ(object.material, glm::vec4(0.0f));
  }
}
