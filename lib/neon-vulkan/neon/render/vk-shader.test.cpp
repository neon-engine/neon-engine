#include "vk-shader.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/mock-file-system-context.hpp>
#include <neon/testing/recording-logger.hpp>

// What happens before Vulkan is called: which files a shader reads, and what
// it does with files it cannot use. Creating the modules needs a device and
// is not tested.

namespace
{
  using neon::VK_Device;
  using neon::VK_Shader;
  using neon::testing::LogLevel;
  using neon::testing::MockFileSystemContext;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::DoAll;
  using ::testing::Return;
  using ::testing::SetArgReferee;
  using ::testing::StrictMock;

  class VkShaderTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    StrictMock<MockFileSystemContext> _file_system;

    // never initialized, so it stands for no graphics card
    VK_Device _device;

    VK_Shader _shader{"assets://shaders/basic-lit", &_file_system, &_device, _logger};
  };

  TEST_F(VkShaderTest, ReadsNothingWhenItIsCreated)
  {
    EXPECT_EQ(_shader.Vertex(), VK_NULL_HANDLE);
    EXPECT_EQ(_shader.Fragment(), VK_NULL_HANDLE);
  }

  TEST_F(VkShaderTest, ReadsTheVertexShaderFromThePathWithItsExtensionAdded)
  {
    EXPECT_CALL(_file_system, ReadBytes("assets://shaders/basic-lit.vert.spv", _)).WillOnce(Return(false));

    EXPECT_FALSE(_shader.Initialize());

    EXPECT_TRUE(
      _logger->Contains(
        LogLevel::Error,
        "Could not read shader assets://shaders/basic-lit.vert.spv, was it compiled by the build?"))
      << _logger->Messages(LogLevel::Error);
    EXPECT_EQ(_shader.Vertex(), VK_NULL_HANDLE);
    EXPECT_EQ(_shader.Fragment(), VK_NULL_HANDLE);
  }

  TEST_F(VkShaderTest, DoesNotReadTheFragmentShaderWhenTheVertexShaderFailed)
  {
    // the file system is strict, a second read would fail the test
    EXPECT_CALL(_file_system, ReadBytes("assets://shaders/basic-lit.vert.spv", _)).WillOnce(Return(false));

    EXPECT_FALSE(_shader.Initialize());
  }

  TEST_F(VkShaderTest, RefusesAShaderThatIsEmpty)
  {
    EXPECT_CALL(_file_system, ReadBytes("assets://shaders/basic-lit.vert.spv", _))
      .WillOnce(DoAll(SetArgReferee<1>(std::vector<unsigned char>{}), Return(true)));

    EXPECT_FALSE(_shader.Initialize());

    EXPECT_TRUE(
      _logger->Contains(LogLevel::Error, "Shader assets://shaders/basic-lit.vert.spv is not valid SPIR-V"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(VkShaderTest, RefusesAShaderThatIsNotMadeOfWholeWords)
  {
    for (const std::size_t size : {1u, 2u, 3u, 5u, 6u, 7u, 1023u})
    {
      EXPECT_CALL(_file_system, ReadBytes("assets://shaders/basic-lit.vert.spv", _))
        .WillOnce(DoAll(SetArgReferee<1>(std::vector<unsigned char>(size, 0x07)), Return(true)));
      _logger->Clear();

      EXPECT_FALSE(_shader.Initialize()) << size;

      EXPECT_TRUE(
        _logger->Contains(LogLevel::Error, "Shader assets://shaders/basic-lit.vert.spv is not valid SPIR-V"))
        << size;
    }
  }

  TEST_F(VkShaderTest, CanBeCleanedUpWithoutEverBeingInitialized)
  {
    _shader.CleanUp();
    _shader.CleanUp();

    VK_Shader without_anything;
    without_anything.CleanUp();

    EXPECT_EQ(_shader.Vertex(), VK_NULL_HANDLE);
  }
}
