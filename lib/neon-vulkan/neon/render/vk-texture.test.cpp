#include "vk-texture.hpp"

#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

// What happens before Vulkan is called: a texture whose file cannot be used
// fails with a message. Loading one that can be used needs a device and is
// not tested.

namespace
{
  using neon::VK_Device;
  using neon::VK_Texture;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;

  class VkTextureTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _file_system{SettingsConfig{}, _logger};

    // never initialized, so it stands for no graphics card
    VK_Device _device;

    void SetUp() override
    {
      _file_system.Initialize();
    }
  };

  TEST_F(VkTextureTest, HasNothingToShowUntilItIsInitialized)
  {
    const VK_Texture texture("assets://textures/wood.png", &_file_system, &_device, _logger);

    EXPECT_EQ(texture.View(), VK_NULL_HANDLE);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
  }

  TEST_F(VkTextureTest, IsReadAsTheSurfaceOfAModelUnlessToldOtherwise)
  {
    // smooth, from the side, and starting again past its edge, which the
    // shared sampler of that way does; a texture owns no sampler
    const VK_Texture texture("assets://textures/wood.png", &_file_system, &_device, _logger);

    EXPECT_EQ(texture.Sampling(), neon::VK_Sampling::AnisotropicRepeat);
  }

  TEST_F(VkTextureTest, FailsAndSaysSoWhenTheFileIsMissing)
  {
    VK_Texture texture("assets://textures/missing.png", &_file_system, &_device, _logger);

    EXPECT_FALSE(texture.Initialize());

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Failed to read texture assets://textures/missing.png"))
      << _logger->Messages(LogLevel::Error);
    EXPECT_EQ(texture.View(), VK_NULL_HANDLE);
  }

  TEST_F(VkTextureTest, ACubeFailsAndNamesTheFaceThatIsMissing)
  {
    VK_Texture texture("the faces of a sky", &_file_system, &_device, _logger);

    EXPECT_FALSE(texture.InitializeWithFaces({
      "assets://textures/sky/right.png", "assets://textures/sky/left.png", "assets://textures/sky/top.png",
      "assets://textures/sky/bottom.png", "assets://textures/sky/front.png", "assets://textures/sky/back.png"}));

    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "Failed to read assets://textures/sky/right.png, a face of the faces of a sky"))
      << _logger->Messages(LogLevel::Error);
    EXPECT_EQ(texture.View(), VK_NULL_HANDLE);
  }

  TEST_F(VkTextureTest, ACubeFailsAndNamesTheFaceThatIsNoImage)
  {
    _file_system.AddNativeFile("/assets/textures/sky/right.png", "this is not an image");
    VK_Texture texture("the faces of a sky", &_file_system, &_device, _logger);

    EXPECT_FALSE(texture.InitializeWithFaces({
      "assets://textures/sky/right.png", "assets://textures/sky/left.png", "assets://textures/sky/top.png",
      "assets://textures/sky/bottom.png", "assets://textures/sky/front.png", "assets://textures/sky/back.png"}));

    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "Failed to load assets://textures/sky/right.png, a face of the faces of a sky"))
      << _logger->Messages(LogLevel::Error);
    EXPECT_EQ(texture.View(), VK_NULL_HANDLE);
  }

  TEST_F(VkTextureTest, FailsAndSaysSoWhenThePathBreaksARule)
  {
    _file_system.AddNativeFile("/assets/textures/wood.png", "");
    VK_Texture texture("assets://Textures/wood.png", &_file_system, &_device, _logger);

    EXPECT_FALSE(texture.Initialize());

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "letter case has to match on every platform"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Failed to read texture assets://Textures/wood.png"));
  }

  TEST_F(VkTextureTest, FailsAndSaysSoWhenTheFileIsNoImage)
  {
    _file_system.AddNativeFile("/assets/textures/notes.png", "this is not an image");
    VK_Texture texture("assets://textures/notes.png", &_file_system, &_device, _logger);

    EXPECT_FALSE(texture.Initialize());

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Failed to load texture assets://textures/notes.png"))
      << _logger->Messages(LogLevel::Error);
    EXPECT_EQ(texture.View(), VK_NULL_HANDLE);
  }

  TEST_F(VkTextureTest, FailsAndSaysSoWhenTheFileIsEmpty)
  {
    _file_system.AddNativeFile("/assets/textures/empty.png", "");
    VK_Texture texture("assets://textures/empty.png", &_file_system, &_device, _logger);

    EXPECT_FALSE(texture.Initialize());

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Failed to load texture assets://textures/empty.png"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(VkTextureTest, FailsWhenAnImageStopsHalfWay)
  {
    // the first bytes of a PNG image, and nothing after them
    _file_system.AddNativeFile("/assets/textures/cut.png", std::string("\x89PNG\r\n\x1a\n", 8));
    VK_Texture texture("assets://textures/cut.png", &_file_system, &_device, _logger);

    EXPECT_FALSE(texture.Initialize());

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Failed to load texture assets://textures/cut.png"));
  }

  TEST_F(VkTextureTest, CanBeCleanedUpWithoutEverBeingInitialized)
  {
    VK_Texture texture("assets://textures/wood.png", &_file_system, &_device, _logger);
    texture.CleanUp();
    texture.CleanUp();

    VK_Texture without_anything;
    without_anything.CleanUp();

    EXPECT_EQ(texture.View(), VK_NULL_HANDLE);
  }

  TEST(VkTextureFormatTest, KeepsColorsInSrgbSoThatTheyAreReadAsLinearLight)
  {
    EXPECT_EQ(VK_Texture::FormatFor(true), VK_FORMAT_R8G8B8A8_SRGB);
  }

  TEST(VkTextureFormatTest, KeepsNumbersAsTheyAre)
  {
    EXPECT_EQ(VK_Texture::FormatFor(false), VK_FORMAT_R8G8B8A8_UNORM);
  }

  TEST(VkTextureFormatTest, TakesATextureForColorsUnlessToldOtherwise)
  {
    EXPECT_TRUE(neon::VK_TextureOptions{}.is_color);
  }
}
