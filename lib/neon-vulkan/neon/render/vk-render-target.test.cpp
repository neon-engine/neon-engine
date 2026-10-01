#include "vk-render-target.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

#include "vk-material.hpp"
#include "vk-texture.hpp"

// What needs no graphics card: what a target is called, how many smaller
// copies it has, and what happens without a renderer. That a target is
// drawn to and shown is covered from the outside, by the frames the runtime
// renders.

namespace
{
  using neon::VK_Device;
  using neon::VK_Material;
  using neon::VK_RenderTarget;
  using neon::VK_Texture;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;

  TEST(VkRenderTargetTest, KnowsTheNameOfTheTargetATextureAsksFor)
  {
    EXPECT_EQ(VK_RenderTarget::NameOf("surface://terminal"), "terminal");
    EXPECT_EQ(VK_RenderTarget::NameOf("surface://screens/left"), "screens/left");

    EXPECT_EQ(VK_RenderTarget::NameOf("assets://textures/wood.png"), "");
    EXPECT_EQ(VK_RenderTarget::NameOf("surface:/terminal"), "");
    EXPECT_EQ(VK_RenderTarget::NameOf("terminal"), "");
    EXPECT_EQ(VK_RenderTarget::NameOf(""), "");
    EXPECT_EQ(VK_RenderTarget::NameOf("surface://"), "");
  }

  TEST(VkRenderTargetTest, HasSmallerCopiesDownToOnePixel)
  {
    EXPECT_EQ(VK_RenderTarget::LevelsFor(1, 1), 1u);
    EXPECT_EQ(VK_RenderTarget::LevelsFor(2, 2), 2u);
    EXPECT_EQ(VK_RenderTarget::LevelsFor(1024, 768), 11u);
    EXPECT_EQ(VK_RenderTarget::LevelsFor(768, 1024), 11u);
    EXPECT_EQ(VK_RenderTarget::LevelsFor(1000, 10), 10u);
  }

  TEST(VkRenderTargetTest, IsReadByModelsThroughTheSrgbFormatOfItsBytes)
  {
    // what a user interface drew is sRGB, and a model lights linear light
    EXPECT_EQ(VK_RenderTarget::SampledFormatOf(VK_FORMAT_R8G8B8A8_UNORM), VK_FORMAT_R8G8B8A8_SRGB);
    EXPECT_EQ(VK_RenderTarget::SampledFormatOf(VK_FORMAT_B8G8R8A8_UNORM), VK_FORMAT_B8G8R8A8_SRGB);
    EXPECT_EQ(VK_RenderTarget::SampledFormatOf(VK_FORMAT_R16G16B16A16_SFLOAT), VK_FORMAT_R16G16B16A16_SFLOAT);
  }

  TEST(VkRenderTargetTest, FailsAndSaysSoWithoutARenderer)
  {
    const auto logger = std::make_shared<RecordingLogger>();

    // never initialized, so it stands for no graphics card
    VK_Device device;
    VK_RenderTarget target("terminal", &device, logger);

    EXPECT_FALSE(target.Initialize(64, 64, VK_NULL_HANDLE, VK_FORMAT_R8G8B8A8_UNORM));
    EXPECT_TRUE(logger->Contains(
      LogLevel::Error, "The render target 'terminal' needs a renderer that is initialized"));

    target.CleanUp();
    target.CleanUp();

    VK_RenderTarget without_anything;
    without_anything.CleanUp();
  }

  TEST(VkRenderTargetTest, ATextureThatIsBorrowedLeavesItsImageAlone)
  {
    // what stands for the image of a target, which the target releases
    const auto view = reinterpret_cast<VkImageView>(static_cast<std::uintptr_t>(0x1234));
    const auto sampler = reinterpret_cast<VkSampler>(static_cast<std::uintptr_t>(0x5678));

    VK_Texture texture = VK_Texture::Borrowed(view, sampler, 640, 480);

    EXPECT_EQ(texture.View(), view);
    EXPECT_EQ(texture.Sampler(), sampler);
    EXPECT_EQ(texture.Width(), 640u);
    EXPECT_EQ(texture.Height(), 480u);

    // without a device, which releasing an image of its own would ask for
    texture.CleanUp();
    EXPECT_EQ(texture.View(), VK_NULL_HANDLE);
  }

  class VkMaterialSurfaceTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _file_system{SettingsConfig{}, _logger};
    VK_Device _device;

    std::vector<std::string> _asked;
    bool _has_target = true;

    void SetUp() override
    {
      _file_system.Initialize();
    }

    VK_Material Material(const std::vector<std::string> &textures)
    {
      VK_Material material(
        "assets://shaders/basic-lit", textures, neon::MaterialInfo{}, false, &_file_system, &_device, _logger);

      material.SetSurfaceLookup([this](const std::string &name, VK_Texture &texture)
      {
        _asked.push_back(name);
        if (!_has_target) { return false; }

        const auto view = reinterpret_cast<VkImageView>(static_cast<std::uintptr_t>(0x1234));
        const auto sampler = reinterpret_cast<VkSampler>(static_cast<std::uintptr_t>(0x5678));
        texture = VK_Texture::Borrowed(view, sampler, 64, 64);
        return true;
      });

      return material;
    }
  };

  TEST_F(VkMaterialSurfaceTest, AsksForTheTargetOfATextureThatNamesOne)
  {
    VK_Material material = Material({"surface://terminal"});

    ASSERT_TRUE(material.Initialize());

    EXPECT_EQ(_asked, (std::vector<std::string>{"terminal"}));
    ASSERT_EQ(material.Textures().size(), 1u);
    EXPECT_NE(material.Textures()[0].View(), VK_NULL_HANDLE);

    EXPECT_TRUE(material.ShowsSurfaces());
    EXPECT_TRUE(material.Shows("terminal"));
    EXPECT_FALSE(material.Shows("other"));
    EXPECT_FALSE(material.Shows(""));

    // no file was asked for
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);

    material.CleanUp();
    EXPECT_FALSE(material.ShowsSurfaces());
  }

  TEST_F(VkMaterialSurfaceTest, ATargetThatIsNotThereYetIsNoMistake)
  {
    _has_target = false;
    VK_Material material = Material({"surface://terminal"});

    // the target may be made after the model that shows it
    ASSERT_TRUE(material.Initialize());

    ASSERT_EQ(material.Textures().size(), 1u);
    EXPECT_EQ(material.Textures()[0].View(), VK_NULL_HANDLE) << "which is drawn as plain white";
    EXPECT_TRUE(material.Shows("terminal"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);

    // and is shown once it is there
    _has_target = true;
    material.ResolveSurfaces(VK_Texture());

    EXPECT_NE(material.Textures()[0].View(), VK_NULL_HANDLE);

    // and no longer once it is gone
    _has_target = false;
    material.ResolveSurfaces(VK_Texture());

    EXPECT_EQ(material.Textures()[0].View(), VK_NULL_HANDLE);

    material.CleanUp();
  }

  TEST_F(VkMaterialSurfaceTest, AMaterialWithoutATargetAsksForNone)
  {
    VK_Material material = Material({});

    ASSERT_TRUE(material.Initialize());

    EXPECT_TRUE(_asked.empty());
    EXPECT_FALSE(material.ShowsSurfaces());

    material.ResolveSurfaces(VK_Texture());
    EXPECT_TRUE(_asked.empty());
  }

  TEST_F(VkMaterialSurfaceTest, StillFailsWhenATextureThatIsAFileIsMissing)
  {
    VK_Material material = Material({"surface://terminal", "assets://textures/missing.png"});

    EXPECT_FALSE(material.Initialize());
    EXPECT_FALSE(material.ShowsSurfaces()) << "and keeps nothing";
  }
}
