#include "model-geometry.hpp"

#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::LoadModelGeometry;
  using neon::ModelFit;
  using neon::ModelGeometry;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;

  // a ramp that is 4 long, 2 high, and 1 wide, away from the origin
  const std::string ramp =
    "v 10 0 0\n"
    "v 14 0 0\n"
    "v 14 2 0\n"
    "v 10 0 1\n"
    "v 14 0 1\n"
    "v 14 2 1\n"
    "f 1 2 3\n"
    "f 4 6 5\n"
    "f 1 4 5 2\n"
    "f 2 5 6 3\n"
    "f 1 3 6 4\n";

  class ModelGeometryTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    ModelGeometry _geometry;

    void SetUp() override
    {
      _files.Initialize();
      _files.AddNativeFile("/assets/models/ramp.obj", ramp);
    }

    bool Load(const std::string &path, const ModelFit fit = ModelFit::Unit)
    {
      return LoadModelGeometry(path, fit, &_files, _logger, _geometry);
    }

    void Measure(glm::vec3 &lowest, glm::vec3 &highest) const
    {
      lowest = glm::vec3{1000.0f};
      highest = glm::vec3{-1000.0f};
      for (const auto &point : _geometry.points)
      {
        lowest = glm::min(lowest, point);
        highest = glm::max(highest, point);
      }
    }
  };

  TEST_F(ModelGeometryTest, ReadsTheTrianglesOfAModel)
  {
    ASSERT_TRUE(Load("assets://models/ramp.obj"));

    // two triangles, and three sides of two triangles each
    EXPECT_EQ(_geometry.triangles.size(), 8u * 3u);
    EXPECT_FALSE(_geometry.points.empty());
  }

  TEST_F(ModelGeometryTest, NamesPointsThatExist)
  {
    ASSERT_TRUE(Load("assets://models/ramp.obj"));

    for (const auto index : _geometry.triangles) { EXPECT_LT(index, _geometry.points.size()); }
  }

  TEST_F(ModelGeometryTest, KeepsTheModelWhereAndHowBigTheFileSaysItIsWithoutAFit)
  {
    ASSERT_TRUE(Load("assets://models/ramp.obj", ModelFit::None));

    glm::vec3 lowest;
    glm::vec3 highest;
    Measure(lowest, highest);

    EXPECT_EQ(lowest, glm::vec3(10.0f, 0.0f, 0.0f));
    EXPECT_EQ(highest, glm::vec3(14.0f, 2.0f, 1.0f));
  }

  TEST_F(ModelGeometryTest, PutsTheMiddleOfTheModelAtTheOriginWithAUnitFit)
  {
    ASSERT_TRUE(Load("assets://models/ramp.obj", ModelFit::Unit));

    glm::vec3 lowest;
    glm::vec3 highest;
    Measure(lowest, highest);

    EXPECT_NEAR(lowest.x, -0.5f, 1e-5f);
    EXPECT_NEAR(highest.x, 0.5f, 1e-5f);
    EXPECT_NEAR(lowest.y, -0.25f, 1e-5f);
    EXPECT_NEAR(highest.y, 0.25f, 1e-5f);
    EXPECT_NEAR(lowest.z, -0.125f, 1e-5f);
    EXPECT_NEAR(highest.z, 0.125f, 1e-5f);
  }

  TEST_F(ModelGeometryTest, ReadsTheSameTwice)
  {
    ASSERT_TRUE(Load("assets://models/ramp.obj"));
    const ModelGeometry first = _geometry;

    ASSERT_TRUE(Load("assets://models/ramp.obj"));

    EXPECT_EQ(_geometry.points, first.points);
    EXPECT_EQ(_geometry.triangles, first.triangles);
  }

  TEST_F(ModelGeometryTest, RefusesAModelThatDoesNotExist)
  {
    EXPECT_FALSE(Load("assets://models/missing.obj"));

    EXPECT_TRUE(_geometry.points.empty());
    EXPECT_TRUE(_geometry.triangles.empty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "assets://models/missing.obj")) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(ModelGeometryTest, RefusesAModelWithoutTriangles)
  {
    _files.AddNativeFile("/assets/models/line.obj", "v 0 0 0\nv 1 0 0\nl 1 2\n");

    EXPECT_FALSE(Load("assets://models/line.obj"));

    EXPECT_TRUE(_geometry.points.empty());
    EXPECT_GE(_logger->Count(LogLevel::Error), 1u);
  }

  TEST_F(ModelGeometryTest, ForgetsWhatItHeldBefore)
  {
    ASSERT_TRUE(Load("assets://models/ramp.obj"));

    EXPECT_FALSE(Load("assets://models/missing.obj"));

    EXPECT_TRUE(_geometry.triangles.empty());
  }
}
