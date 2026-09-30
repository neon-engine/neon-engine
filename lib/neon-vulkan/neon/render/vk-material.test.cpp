#include "vk-material.hpp"

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

// No graphics card is needed for these: what a material hands to the shaders
// is worked out from its settings alone, and a texture that cannot be read
// fails before Vulkan is called. Loading a texture that can be read is not
// tested, it needs a device.

namespace
{
  using neon::MaterialInfo;
  using neon::Transform;
  using neon::VK_Device;
  using neon::VK_Material;
  using neon::VK_ObjectData;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::IsEmpty;

  class VkMaterialTest : public ::testing::Test
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

    VK_Material Create(
      const MaterialInfo &material_info,
      const bool scale_textures,
      const std::vector<std::string> &texture_paths = {})
    {
      return {
        "assets://shaders/basic-lit",
        texture_paths,
        material_info,
        scale_textures,
        &_file_system,
        &_device,
        _logger
      };
    }

    static Transform Scaled(const float x, const float y, const float z)
    {
      Transform transform;
      transform.scale = {x, y, z};
      return transform;
    }
  };

  TEST_F(VkMaterialTest, KeepsThePathOfItsShader)
  {
    const VK_Material material = Create({}, false);

    EXPECT_EQ(material.ShaderPath(), "assets://shaders/basic-lit");
  }

  TEST_F(VkMaterialTest, HasNeitherTexturesNorPipelineUntilItIsGivenThem)
  {
    const VK_Material material = Create({}, false, {"assets://textures/wood.png"});

    EXPECT_THAT(material.Textures(), IsEmpty());
    EXPECT_EQ(material.Pipeline(), VK_NULL_HANDLE);
    EXPECT_EQ(material.DescriptorSet(), VK_NULL_HANDLE);
  }

  // GetObjectData

  TEST_F(VkMaterialTest, HandsTheModelMatrixToTheShaders)
  {
    const VK_Material material = Create({}, false);
    const auto model = translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));

    const VK_ObjectData data = material.GetObjectData(model, Transform{});

    EXPECT_EQ(data.model, model);
  }

  TEST_F(VkMaterialTest, WorksOutTheMatrixForNormalsFromTheModelMatrix)
  {
    const VK_Material material = Create({}, false);
    const auto model =
      translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f)) *
      rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
      scale(glm::mat4(1.0f), glm::vec3(2.0f, 1.0f, 0.5f));

    const VK_ObjectData data = material.GetObjectData(model, Transform{});

    EXPECT_EQ(data.normal_matrix, transpose(inverse(model)));
  }

  TEST_F(VkMaterialTest, KeepsANormalAtRightAnglesToASurfaceThatIsStretched)
  {
    const VK_Material material = Create({}, false);
    const auto model = scale(glm::mat4(1.0f), glm::vec3(4.0f, 1.0f, 1.0f));

    const VK_ObjectData data = material.GetObjectData(model, Transform{});

    // a surface that leans by 45 degrees, and what stands on it
    const glm::vec3 along = glm::mat3(model) * glm::vec3(1.0f, -1.0f, 0.0f);
    const glm::vec3 normal = glm::mat3(data.normal_matrix) * glm::vec3(1.0f, 1.0f, 0.0f);
    EXPECT_NEAR(dot(along, normal), 0.0f, 1e-5f);
  }

  TEST_F(VkMaterialTest, HandsTheColorToTheShaders)
  {
    MaterialInfo info;
    info.color = {.r = 0.1f, .g = 0.2f, .b = 0.3f, .a = 0.4f};
    const VK_Material material = Create(info, false);

    const VK_ObjectData data = material.GetObjectData(glm::mat4(1.0f), Transform{});

    EXPECT_EQ(data.color, glm::vec4(0.1f, 0.2f, 0.3f, 0.4f));
  }

  TEST_F(VkMaterialTest, HandsShininessAndWhetherTexturesAreUsedToTheShaders)
  {
    MaterialInfo with_textures;
    with_textures.shininess = 32.0f;
    with_textures.use_textures = true;
    MaterialInfo without_textures;
    without_textures.shininess = 8.0f;
    without_textures.use_textures = false;

    EXPECT_EQ(
      Create(with_textures, false).GetObjectData(glm::mat4(1.0f), Transform{}).material,
      glm::vec4(32.0f, 1.0f, 0.0f, 0.0f));
    EXPECT_EQ(
      Create(without_textures, false).GetObjectData(glm::mat4(1.0f), Transform{}).material,
      glm::vec4(8.0f, 0.0f, 0.0f, 0.0f));
  }

  TEST_F(VkMaterialTest, LeavesTexturesAtTheirSizeUnlessToldToScaleThem)
  {
    const VK_Material material = Create({}, false);

    const VK_ObjectData data = material.GetObjectData(glm::mat4(1.0f), Scaled(10.0f, 20.0f, 30.0f));

    EXPECT_EQ(data.texture_scale, glm::vec4(1.0f, 1.0f, 0.0f, 0.0f));
  }

  struct ScaleCase
  {
    glm::vec3 scale;
    glm::vec2 expected;
  };

  class VkMaterialTextureScale : public VkMaterialTest, public ::testing::WithParamInterface<ScaleCase> {};

  TEST_P(VkMaterialTextureScale, IsTheTwoLongestSidesOfTheObjectWithTheLongestFirst)
  {
    const VK_Material material = Create({}, true);
    const auto [x, y, z] = std::tuple{GetParam().scale.x, GetParam().scale.y, GetParam().scale.z};

    const VK_ObjectData data = material.GetObjectData(glm::mat4(1.0f), Scaled(x, y, z));

    EXPECT_EQ(data.texture_scale, glm::vec4(GetParam().expected, 0.0f, 0.0f));
  }

  INSTANTIATE_TEST_SUITE_P(
    VkMaterial,
    VkMaterialTextureScale,
    ::testing::Values(
      ScaleCase{{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
      ScaleCase{{3.0f, 2.0f, 1.0f}, {3.0f, 2.0f}},
      ScaleCase{{1.0f, 2.0f, 3.0f}, {3.0f, 2.0f}},
      ScaleCase{{2.0f, 3.0f, 1.0f}, {3.0f, 2.0f}},
      ScaleCase{{2.0f, 1.0f, 3.0f}, {3.0f, 2.0f}},
      ScaleCase{{1.0f, 3.0f, 2.0f}, {3.0f, 2.0f}},
      ScaleCase{{3.0f, 1.0f, 2.0f}, {3.0f, 2.0f}},
      // a floor: wide and deep, and flat
      ScaleCase{{20.0f, 0.1f, 20.0f}, {20.0f, 20.0f}},
      ScaleCase{{5.0f, 5.0f, 1.0f}, {5.0f, 5.0f}},
      // what is turned inside out counts as nothing
      ScaleCase{{-4.0f, 2.0f, 1.0f}, {2.0f, 1.0f}},
      ScaleCase{{-4.0f, -2.0f, 3.0f}, {3.0f, 0.0f}},
      ScaleCase{{-1.0f, -2.0f, -3.0f}, {0.0f, 0.0f}},
      ScaleCase{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}));

  TEST_F(VkMaterialTest, ScalesTexturesByTheScaleOfTheTransformAndNotByTheModelMatrix)
  {
    const VK_Material material = Create({}, true);
    const auto model = scale(glm::mat4(1.0f), glm::vec3(9.0f));

    const VK_ObjectData data = material.GetObjectData(model, Scaled(2.0f, 3.0f, 1.0f));

    EXPECT_EQ(data.texture_scale, glm::vec4(3.0f, 2.0f, 0.0f, 0.0f));
  }

  // Initialize

  TEST_F(VkMaterialTest, InitializesWithoutTextures)
  {
    VK_Material material = Create({}, false);

    EXPECT_TRUE(material.Initialize());
    EXPECT_THAT(material.Textures(), IsEmpty());
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);

    material.CleanUp();
  }

  TEST_F(VkMaterialTest, SaysSoWhenItIsInitializedTwice)
  {
    VK_Material material = Create({}, false);
    ASSERT_TRUE(material.Initialize());

    EXPECT_TRUE(material.Initialize());

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Material is already initialized"));
  }

  TEST_F(VkMaterialTest, FailsWhenATextureIsMissing)
  {
    VK_Material material = Create({}, false, {"assets://textures/missing.png"});

    EXPECT_FALSE(material.Initialize());

    EXPECT_THAT(material.Textures(), IsEmpty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Failed to read texture assets://textures/missing.png"))
      << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Could not initialize texture"));
  }

  TEST_F(VkMaterialTest, FailsWhenATextureIsNoImage)
  {
    _file_system.AddNativeFile("/assets/textures/notes.png", "this is not an image");
    VK_Material material = Create({}, false, {"assets://textures/notes.png"});

    EXPECT_FALSE(material.Initialize());

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Failed to load texture assets://textures/notes.png"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(VkMaterialTest, CanBeInitializedAgainAfterItFailed)
  {
    VK_Material material = Create({}, false, {"assets://textures/missing.png"});
    ASSERT_FALSE(material.Initialize());
    _logger->Clear();

    EXPECT_FALSE(material.Initialize());

    EXPECT_FALSE(_logger->Contains(LogLevel::Error, "Material is already initialized"));
  }
}
