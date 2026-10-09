#include "vk-model-cache.hpp"

#include <memory>

#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

// What the cache does with a file that cannot be read: it is read and
// logged once, and not again (#248, #507). Holding a model that can be read
// uploads its meshes, which needs a device and is not tested.

namespace
{
  using neon::ModelFit;
  using neon::RenderInfo;
  using neon::VK_Device;
  using neon::VK_ModelCache;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;

  class VkModelCacheTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _file_system{SettingsConfig{}, _logger};

    // never initialized, so it stands for no graphics card
    VK_Device _device;

    VK_ModelCache _cache{8};

    void SetUp() override
    {
      _file_system.Initialize();
      _cache.Initialize(&_file_system, &_device, _logger);
    }

    [[nodiscard]] static RenderInfo FileOf(const std::string &path, const ModelFit fit)
    {
      RenderInfo info;
      info.model_path = path;
      info.fit = fit;
      return info;
    }
  };

  TEST_F(VkModelCacheTest, ReadsAMissingFileOnceAndSaysSoOnce)
  {
    const RenderInfo missing = FileOf("assets://models/missing.obj", ModelFit::Unit);

    EXPECT_EQ(_cache.Acquire(missing), -1);
    const std::size_t errors = _logger->Count(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Error importing model assets://models/missing.obj: "))
      << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Could not initialize model assets://models/missing.obj"))
      << _logger->Messages(LogLevel::Error);

    // every placement after the first draws nothing and says nothing
    for (int i = 0; i < 60; i++) { EXPECT_EQ(_cache.Acquire(missing), -1); }

    EXPECT_EQ(_logger->Count(LogLevel::Error), errors) << _logger->Messages(LogLevel::Error);
    EXPECT_EQ(_cache.Failures(), 1u);
    EXPECT_EQ(_cache.Loads(), 0u);
    EXPECT_EQ(_cache.Size(), 0);
  }

  TEST_F(VkModelCacheTest, KeepsTheFailuresOfAnotherPathOrFitApart)
  {
    EXPECT_EQ(_cache.Acquire(FileOf("assets://models/missing.obj", ModelFit::Unit)), -1);
    _logger->Clear();

    // the same file at another fit is another model, and is read
    EXPECT_EQ(_cache.Acquire(FileOf("assets://models/missing.obj", ModelFit::None)), -1);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Error importing model assets://models/missing.obj: "));
    _logger->Clear();

    EXPECT_EQ(_cache.Acquire(FileOf("assets://models/gone.obj", ModelFit::Unit)), -1);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Error importing model assets://models/gone.obj: "));

    EXPECT_EQ(_cache.Failures(), 3u);
  }

  TEST_F(VkModelCacheTest, ReadsAFileThatFailedAgainAfterItIsCleanedUp)
  {
    const RenderInfo missing = FileOf("assets://models/missing.obj", ModelFit::Unit);
    EXPECT_EQ(_cache.Acquire(missing), -1);

    _cache.CleanUp();
    _logger->Clear();

    EXPECT_EQ(_cache.Failures(), 0u);
    EXPECT_EQ(_cache.Acquire(missing), -1);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Error importing model assets://models/missing.obj: "));
  }
} // namespace
