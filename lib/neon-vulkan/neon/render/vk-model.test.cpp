#include "vk-model.hpp"

#include <memory>

#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

// What happens before Vulkan is called: a model whose file cannot be used
// fails with a message. Loading one that can be used uploads its meshes,
// which needs a device and is not tested. How a model is read is tested in
// neon-core, in model.test.cpp.

namespace
{
  using neon::ModelFit;
  using neon::VK_Device;
  using neon::VK_Model;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;

  class VkModelTest : public ::testing::Test
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

  TEST_F(VkModelTest, IsLeftAsItIsUntilItIsInitialized)
  {
    const VK_Model model("assets://models/cube.obj", ModelFit::Unit, &_file_system, &_device, _logger);

    EXPECT_EQ(model.GetNormalizedModelMatrix(), glm::mat4(1.0f));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
  }

  TEST_F(VkModelTest, KeepsTheFitItWasGiven)
  {
    const VK_Model fitted("assets://models/cube.obj", ModelFit::Unit, &_file_system, &_device, _logger);
    const VK_Model as_it_is("assets://models/cube.obj", ModelFit::None, &_file_system, &_device, _logger);

    EXPECT_EQ(fitted.GetFit(), ModelFit::Unit);
    EXPECT_EQ(as_it_is.GetFit(), ModelFit::None);
  }

  TEST_F(VkModelTest, AMeshThatWasBuiltHasNoFit)
  {
    const VK_Model model(std::make_shared<const neon::MeshData>(), &_device, _logger);

    EXPECT_EQ(model.GetFit(), ModelFit::None);
  }

  TEST_F(VkModelTest, FailsAndSaysSoWhenTheFileIsMissing)
  {
    VK_Model model("assets://models/missing.obj", ModelFit::Unit, &_file_system, &_device, _logger);

    EXPECT_FALSE(model.Initialize());

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Error importing model assets://models/missing.obj: "))
      << _logger->Messages(LogLevel::Error);
    EXPECT_EQ(model.GetNormalizedModelMatrix(), glm::mat4(1.0f));
  }

  TEST_F(VkModelTest, FailsWhenTheFormatOfTheFileIsNotKnown)
  {
    _file_system.AddNativeFile("/assets/models/notes.txt", "this is not a model\n");
    VK_Model model("assets://models/notes.txt", ModelFit::Unit, &_file_system, &_device, _logger);

    EXPECT_FALSE(model.Initialize());

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Error importing model assets://models/notes.txt: "));
  }

  TEST_F(VkModelTest, DrawsNothingAndCleansUpNothingWithoutMeshes)
  {
    VK_Model model("assets://models/missing.obj", ModelFit::Unit, &_file_system, &_device, _logger);

    model.Use();
    model.CleanUp();
    model.CleanUp();

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
  }
}
