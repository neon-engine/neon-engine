#include "vk-renderer-2d.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

// What needs no graphics card: what is handed to the shader for a corner,
// where triangles are cut off, and what happens when something cannot be
// created. Drawing itself is covered from the outside, by the
// functional test that renders a user interface.

namespace
{
  using neon::ClipRectangle;
  using neon::Triangles2D;
  using neon::Vertex2D;
  using neon::VK_Device;
  using neon::VK_Renderer2D;
    using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;

  class VkRenderer2DTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _file_system{SettingsConfig{}, _logger};

    // never initialized, so they stand for no graphics card
    VK_Device _device;
    neon::VK_Samplers _samplers;
    VK_Renderer2D _renderer;

    void SetUp() override
    {
      _file_system.Initialize();
      _renderer.Initialize(&_device, &_file_system, VK_NULL_HANDLE, &_samplers, {1920, 1080}, _logger);
    }

    static Triangles2D ClippedAt(const int x, const int y, const int width, const int height)
    {
      Triangles2D batch;
      batch.clipped = true;
      batch.clip = {x, y, width, height};
      return batch;
    }

    static void ExpectScissor(
      const VkRect2D &scissor,
      const int x,
      const int y,
      const unsigned int width,
      const unsigned int height)
    {
      EXPECT_EQ(scissor.offset.x, x);
      EXPECT_EQ(scissor.offset.y, y);
      EXPECT_EQ(scissor.extent.width, width);
      EXPECT_EQ(scissor.extent.height, height);
    }
  };

  TEST_F(VkRenderer2DTest, BindsTheTextureApartFromTheSamplerItIsReadThrough)
  {
    // what ui-shader.glsl declares: the texture at 0 and the sampler at 1,
    // so that every target of the shaders binds what the source says
    EXPECT_EQ(VK_Renderer2D::kBindings[0].binding, VK_Renderer2D::kImage_Binding);
    EXPECT_EQ(VK_Renderer2D::kBindings[0].descriptorType, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    EXPECT_EQ(VK_Renderer2D::kBindings[0].stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
    EXPECT_EQ(VK_Renderer2D::kBindings[1].binding, VK_Renderer2D::kSampler_Binding);
    EXPECT_EQ(VK_Renderer2D::kBindings[1].descriptorType, VK_DESCRIPTOR_TYPE_SAMPLER);
    EXPECT_EQ(VK_Renderer2D::kBindings[1].stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
    EXPECT_EQ(VK_Renderer2D::kImage_Binding, 0u);
    EXPECT_EQ(VK_Renderer2D::kSampler_Binding, 1u);
  }

  TEST_F(VkRenderer2DTest, ACornerIsAsLargeAsTheShaderExpects)
  {
    // two for the place, two for the texture, four for the color, one
    // that says whether the texture is read, one for the shape, and two
    // for the place in the shape, with nothing between them
    EXPECT_EQ(sizeof(Vertex2D), 12 * sizeof(float));
    EXPECT_EQ(offsetof(Vertex2D, x), 0u);
    EXPECT_EQ(offsetof(Vertex2D, u), 2 * sizeof(float));
    EXPECT_EQ(offsetof(Vertex2D, color), 4 * sizeof(float));
    EXPECT_EQ(offsetof(Vertex2D, textured), 8 * sizeof(float));
    EXPECT_EQ(offsetof(Vertex2D, shape), 9 * sizeof(float));
    EXPECT_EQ(offsetof(Vertex2D, local_x), 10 * sizeof(float));
    EXPECT_EQ(offsetof(Vertex2D, local_y), 11 * sizeof(float));
  }

  TEST_F(VkRenderer2DTest, TrianglesAreWellFormedWhenEveryCornerExists)
  {
    Triangles2D triangles;
    triangles.vertices.resize(4);
    triangles.indices = {0, 1, 2, 0, 2, 3};

    EXPECT_TRUE(VK_Renderer2D::IsWellFormed(triangles));
    EXPECT_TRUE(VK_Renderer2D::IsWellFormed(Triangles2D{}));
  }

  TEST_F(VkRenderer2DTest, TrianglesAreNotWellFormedWhenACornerIsMissing)
  {
    Triangles2D triangles;
    triangles.vertices.resize(4);

    triangles.indices = {0, 1, 4};
    EXPECT_FALSE(VK_Renderer2D::IsWellFormed(triangles));

    triangles.indices = {0, 1, 2, 3};
    EXPECT_FALSE(VK_Renderer2D::IsWellFormed(triangles));
  }

  TEST_F(VkRenderer2DTest, ABatchThatIsNotCutOffCoversTheFrame)
  {
    ExpectScissor(VK_Renderer2D::ScissorOf(Triangles2D{}, {1920, 1080}), 0, 0, 1920, 1080);
  }

  TEST_F(VkRenderer2DTest, ABatchIsCutOffWhereItSays)
  {
    ExpectScissor(VK_Renderer2D::ScissorOf(ClippedAt(100, 50, 300, 200), {1920, 1080}), 100, 50, 300, 200);
  }

  TEST_F(VkRenderer2DTest, WhatABatchIsCutOffAtStaysInsideTheFrame)
  {
    constexpr VkExtent2D frame{1920, 1080};

    ExpectScissor(VK_Renderer2D::ScissorOf(ClippedAt(-50, -20, 300, 200), frame), 0, 0, 250, 180);
    ExpectScissor(VK_Renderer2D::ScissorOf(ClippedAt(1800, 1000, 300, 200), frame), 1800, 1000, 120, 80);
    ExpectScissor(VK_Renderer2D::ScissorOf(ClippedAt(2000, 1200, 300, 200), frame), 1920, 1080, 0, 0);
    ExpectScissor(VK_Renderer2D::ScissorOf(ClippedAt(-500, -500, 300, 200), frame), 0, 0, 0, 0);
    ExpectScissor(VK_Renderer2D::ScissorOf(ClippedAt(100, 100, -30, -30), frame), 100, 100, 0, 0);
  }

  TEST_F(VkRenderer2DTest, RefusesPixelsThatDoNotMakeUpTheImage)
  {
    EXPECT_EQ(_renderer.CreateTexture(2, 2, std::vector<unsigned char>(15, 255)), neon::No_Texture);
    EXPECT_EQ(_renderer.CreateTexture(0, 2, {}), neon::No_Texture);
    EXPECT_EQ(_renderer.CreateTexture(2, -2, std::vector<unsigned char>(16, 255)), neon::No_Texture);

    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "A texture of 2 by 2 needs four bytes for each pixel, and was given 15 bytes"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(VkRenderer2DTest, FailsAndSaysSoWhenAnImageIsMissing)
  {
    EXPECT_EQ(_renderer.LoadTexture("assets://ui/missing.png"), neon::No_Texture);

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Failed to read texture assets://ui/missing.png"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(VkRenderer2DTest, FailsAndSaysSoWhenAFileIsNoImage)
  {
    _file_system.AddNativeFile("/assets/ui/notes.png", "this is not an image");

    EXPECT_EQ(_renderer.LoadTexture("assets://ui/notes.png"), neon::No_Texture);

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Failed to load texture assets://ui/notes.png"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(VkRenderer2DTest, SaysOnceThatItCannotDrawWithoutARenderer)
  {
    const std::vector<unsigned char> pixels(16, 255);

    EXPECT_EQ(_renderer.CreateTexture(2, 2, pixels), neon::No_Texture);
    EXPECT_EQ(_renderer.CreateTexture(2, 2, pixels), neon::No_Texture);
    EXPECT_EQ(_renderer.CreateTexture(2, 2, pixels), neon::No_Texture);

    // what could not be created is not tried again
    EXPECT_EQ(_logger->Count(LogLevel::Error), 2u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Drawing in two dimensions could not be set up"));
  }

  TEST_F(VkRenderer2DTest, KnowsNoSizeOfATextureThatDoesNotExist)
  {
    int width = 7;
    int height = 7;

    EXPECT_FALSE(_renderer.GetTextureSize(0, width, height));
    EXPECT_FALSE(_renderer.GetTextureSize(neon::No_Texture, width, height));
    EXPECT_EQ(width, 7);
  }

  TEST_F(VkRenderer2DTest, DrawsNothingOutsideAFrame)
  {
    Triangles2D triangles;
    triangles.vertices.resize(3);
    triangles.indices = {0, 1, 2};

    _renderer.Draw(triangles);
    _renderer.Draw(Triangles2D{});
    _renderer.DestroyTexture(3);
    _renderer.PrepareFrame();

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(VkRenderer2DTest, CanBeCleanedUpWithoutEverBeingUsed)
  {
    _renderer.CleanUp();
    _renderer.CleanUp();

    VK_Renderer2D without_anything;
    without_anything.CleanUp();

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
  }
}
