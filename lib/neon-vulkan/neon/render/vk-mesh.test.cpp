#include "vk-mesh.hpp"

#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/recording-logger.hpp>

// What happens before Vulkan is called. Uploading a mesh and drawing it need
// a device and are not tested.

namespace
{
  using neon::TextureInfo;
  using neon::TextureType;
  using neon::Vertex;
  using neon::VK_Device;
  using neon::VK_Mesh;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;
  using ::testing::IsEmpty;

  class VkMeshTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();

    // never initialized, so it stands for no graphics card
    VK_Device _device;

    std::vector<Vertex> _triangle{
      Vertex{.position = {0.0f, 0.0f, 0.0f}},
      Vertex{.position = {1.0f, 0.0f, 0.0f}},
      Vertex{.position = {0.0f, 1.0f, 0.0f}}
    };
  };

  TEST_F(VkMeshTest, KeepsTheVerticesAndTexturesItWasCreatedWith)
  {
    const VK_Mesh mesh(
      _triangle,
      {0, 1, 2},
      {TextureInfo{.path = "textures/wood.png", .texture_type = TextureType::Diffuse}},
      &_device,
      _logger);

    ASSERT_EQ(mesh.GetVertices().size(), 3u);
    EXPECT_EQ(mesh.GetVertices()[1].position, glm::vec3(1.0f, 0.0f, 0.0f));
    ASSERT_EQ(mesh.GetTextures().size(), 1u);
    EXPECT_EQ(mesh.GetTextures()[0].path, "textures/wood.png");
    EXPECT_EQ(mesh.GetTextures()[0].texture_type, TextureType::Diffuse);
  }

  TEST_F(VkMeshTest, KeepsACopyOfItsVertices)
  {
    const VK_Mesh mesh(_triangle, {0, 1, 2}, {}, &_device, _logger);

    _triangle.clear();

    EXPECT_EQ(mesh.GetVertices().size(), 3u);
  }

  TEST_F(VkMeshTest, FailsAndSaysSoWithoutVertices)
  {
    VK_Mesh mesh({}, {0, 1, 2}, {}, &_device, _logger);

    EXPECT_FALSE(mesh.Initialize());

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Mesh has no vertices or no indices"));
  }

  TEST_F(VkMeshTest, FailsAndSaysSoWithoutIndices)
  {
    VK_Mesh mesh(_triangle, {}, {}, &_device, _logger);

    EXPECT_FALSE(mesh.Initialize());

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Mesh has no vertices or no indices"));
  }

  TEST_F(VkMeshTest, DrawsNothingOutsideOfAFrame)
  {
    const VK_Mesh mesh(_triangle, {0, 1, 2}, {}, &_device, _logger);

    // returns before Vulkan is called, which would end the test
    mesh.Use();

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
  }

  TEST_F(VkMeshTest, CanBeCleanedUpWithoutEverBeingInitialized)
  {
    VK_Mesh mesh(_triangle, {0, 1, 2}, {}, &_device, _logger);

    mesh.CleanUp();
    mesh.CleanUp();

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
  }
}
