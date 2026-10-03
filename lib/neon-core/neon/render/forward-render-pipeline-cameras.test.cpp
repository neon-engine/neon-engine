#include "forward-render-pipeline.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

#include <neon/testing/recording-logger.hpp>

// Cameras that draw into a texture: what they see is drawn into a render
// target of that name, in front of what the camera of the window sees.

namespace
{
  using neon::CameraInfo;
  using neon::Color;
  using neon::Forward_RenderPipeline;
  using neon::LightSource;
  using neon::RenderContext;
  using neon::RenderInfo;
  using neon::RenderResolution;
  using neon::RenderTarget;
  using neon::SkyInfo;
  using neon::Transform;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;

  /// A renderer that keeps what it is asked to draw, and into what.
  class RecordingRenderContext final : public RenderContext
  {
    RenderResolution _resolution{1920, 1080};

  public:
    struct Drawn
    {
      int object = -1;
      int target = -1;
      glm::mat4 view{1.0f};
      glm::mat4 projection{1.0f};
    };

    struct Target
    {
      std::string name;
      int width = 0;
      int height = 0;
      int begun = 0;
    };

    /// A sky that was drawn: how many objects had been drawn by then,
    /// into what, and seen how.
    struct DrawnSky
    {
      std::string texture;
      std::size_t after = 0;
      int target = -1;
      glm::mat4 view{1.0f};
    };

    std::vector<Drawn> drawn;
    std::vector<DrawnSky> skies;
    std::map<int, Target> targets;
    std::vector<std::string> refused;
    int created = 0;
    int destroyed = 0;
    int current = -1;

    explicit RecordingRenderContext(const std::shared_ptr<neon::Logger> &logger) : RenderContext(16, logger) {}

    int CreateRenderObject(const RenderInfo &render_info) override { return 0; }

    void DrawRenderObject(
      const int render_object_id,
      const Transform &transform,
      const glm::mat4 &view,
      const glm::mat4 &projection,
      const std::vector<LightSource> &lights) override
    {
      drawn.push_back({render_object_id, current, view, projection});
    }

    void DestroyRenderObject(int render_object_id) override {}

    const RenderResolution &GetRenderResolution() override { return _resolution; }

    int CreateRenderTarget(const std::string &name, const int width, const int height) override
    {
      created++;
      for (const auto &each : refused)
      {
        if (each == name) { return -1; }
      }

      targets[created] = {name, width, height, 0};
      return created;
    }

    void DestroyRenderTarget(const int target) override
    {
      destroyed += static_cast<int>(targets.erase(target));
    }

    int FindRenderTarget(const std::string &name) override
    {
      for (const auto &[id, target] : targets)
      {
        if (target.name == name) { return id; }
      }
      return -1;
    }

    bool GetRenderTargetSize(const int target, int &width, int &height) override
    {
      const auto found = targets.find(target);
      if (found == targets.end()) { return false; }

      width = found->second.width;
      height = found->second.height;
      return true;
    }

    bool BeginRenderTarget(const int target, const Color &clear) override
    {
      const auto found = targets.find(target);
      if (found == targets.end() || current >= 0) { return false; }

      found->second.begun++;
      current = target;
      return true;
    }

    void EndRenderTarget() override { current = -1; }

    void DrawSky(const SkyInfo &sky, const glm::mat4 &view, const glm::mat4 &projection) override
    {
      skies.push_back({sky.texture, drawn.size(), current, view});
    }
  };

  class ForwardRenderPipelineCamerasTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    RecordingRenderContext _renderer{_logger};
    Forward_RenderPipeline _pipeline{&_renderer, 3, _logger};

    static CameraInfo Window(const float x)
    {
      CameraInfo camera;
      camera.view = translate(glm::mat4(1.0f), glm::vec3(x, 0.0f, 0.0f));
      return camera;
    }

    static CameraInfo Into(const std::string &texture, const float x, const int width = 512, const int height = 256)
    {
      CameraInfo camera;
      camera.target = RenderTarget::Texture;
      camera.texture = texture;
      camera.width = width;
      camera.height = height;
      camera.fov = 60.0f;
      camera.view = translate(glm::mat4(1.0f), glm::vec3(x, 0.0f, 0.0f));
      return camera;
    }

    void Frame(const std::vector<CameraInfo> &cameras, const std::vector<int> &objects)
    {
      _renderer.drawn.clear();

      for (const auto &camera : cameras) { _pipeline.SetCameraInfo(camera); }
      for (const int object : objects) { _pipeline.EnqueueForRendering(object, Transform{}); }

      _pipeline.RenderFrame();
    }
  };

  TEST_F(ForwardRenderPipelineCamerasTest, WithoutSuchACameraNothingIsDrawnIntoATexture)
  {
    Frame({Window(1)}, {7, 8});

    ASSERT_EQ(_renderer.drawn.size(), 2u);
    EXPECT_EQ(_renderer.drawn[0].target, -1);
    EXPECT_EQ(_renderer.created, 0);
  }

  TEST_F(ForwardRenderPipelineCamerasTest, DrawsWhatACameraSeesIntoItsTextureAndThenTheWindow)
  {
    Frame({Window(1), Into("mirror", 5)}, {7, 8});

    ASSERT_EQ(_renderer.targets.size(), 1u);
    const int target = _renderer.targets.begin()->first;
    EXPECT_EQ(_renderer.targets[target].name, "mirror");
    EXPECT_EQ(_renderer.targets[target].width, 512);
    EXPECT_EQ(_renderer.targets[target].height, 256);
    EXPECT_EQ(_renderer.targets[target].begun, 1);

    // every object twice: for the texture first, which the window may show
    ASSERT_EQ(_renderer.drawn.size(), 4u);

    EXPECT_EQ(_renderer.drawn[0].object, 7);
    EXPECT_EQ(_renderer.drawn[0].target, target);
    EXPECT_EQ(_renderer.drawn[1].object, 8);
    EXPECT_EQ(_renderer.drawn[1].target, target);
    EXPECT_FLOAT_EQ(_renderer.drawn[0].view[3].x, 5) << "as the camera of the texture sees it";

    EXPECT_EQ(_renderer.drawn[2].object, 7);
    EXPECT_EQ(_renderer.drawn[2].target, -1);
    EXPECT_EQ(_renderer.drawn[3].target, -1);
    EXPECT_FLOAT_EQ(_renderer.drawn[2].view[3].x, 1) << "as the camera of the window sees it";

    EXPECT_EQ(_renderer.current, -1);
  }

  TEST_F(ForwardRenderPipelineCamerasTest, ACameraOfATextureDoesNotTakeThePlaceOfTheOneOfTheWindow)
  {
    // whichever is handed over last
    Frame({Into("mirror", 5), Window(1)}, {7});
    EXPECT_FLOAT_EQ(_renderer.drawn.back().view[3].x, 1);

    Frame({Window(2), Into("mirror", 5)}, {7});
    EXPECT_FLOAT_EQ(_renderer.drawn.back().view[3].x, 2);
  }

  TEST_F(ForwardRenderPipelineCamerasTest, WhatACameraSeesHasTheShapeOfItsTexture)
  {
    Frame({Window(1), Into("mirror", 5, 512, 256)}, {7});

    const glm::mat4 of_texture = glm::perspective(glm::radians(60.0f), 2.0f, 0.1f, 1000.0f);
    const glm::mat4 of_window = glm::perspective(glm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 1000.0f);

    ASSERT_EQ(_renderer.drawn.size(), 2u);
    EXPECT_EQ(_renderer.drawn[0].projection, of_texture);
    EXPECT_EQ(_renderer.drawn[1].projection, of_window);
  }

  TEST_F(ForwardRenderPipelineCamerasTest, MakesTheTextureOfACameraOnce)
  {
    Frame({Window(1), Into("mirror", 5)}, {7});
    Frame({Window(1), Into("mirror", 6)}, {7});
    Frame({Window(1), Into("mirror", 7)}, {7});

    EXPECT_EQ(_renderer.created, 1);
    EXPECT_EQ(_renderer.targets.begin()->second.begun, 3);
    EXPECT_FLOAT_EQ(_renderer.drawn[0].view[3].x, 7) << "and draws into it in every frame";
  }

  TEST_F(ForwardRenderPipelineCamerasTest, SeveralCamerasDrawIntoTexturesOfTheirOwn)
  {
    Frame({Window(1), Into("mirror", 5), Into("minimap", 9, 256, 256)}, {7});

    ASSERT_EQ(_renderer.targets.size(), 2u);
    ASSERT_EQ(_renderer.drawn.size(), 3u);

    EXPECT_EQ(_renderer.targets[_renderer.drawn[0].target].name, "mirror");
    EXPECT_EQ(_renderer.targets[_renderer.drawn[1].target].name, "minimap");
    EXPECT_EQ(_renderer.drawn[2].target, -1);
  }

  TEST_F(ForwardRenderPipelineCamerasTest, DrawsIntoATargetThatIsThereAlready)
  {
    // as the surface of a user interface is
    const int target = _renderer.CreateRenderTarget("shared", 128, 64);
    _renderer.created = 0;

    Frame({Window(1), Into("shared", 5, 512, 512)}, {7});

    EXPECT_EQ(_renderer.created, 0);
    EXPECT_EQ(_renderer.drawn[0].target, target);

    // with the shape the target has, and not the one the camera asked for
    EXPECT_EQ(_renderer.drawn[0].projection, glm::perspective(glm::radians(60.0f), 2.0f, 0.1f, 1000.0f));
  }

  TEST_F(ForwardRenderPipelineCamerasTest, SaysOnceThatATextureCannotBeCreated)
  {
    _renderer.refused.push_back("mirror");

    Frame({Window(1), Into("mirror", 5)}, {7});
    Frame({Window(1), Into("mirror", 5)}, {7});
    Frame({Window(1), Into("mirror", 5)}, {7});

    EXPECT_EQ(_renderer.created, 1) << "and does not try again in every frame";
    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The texture 'mirror' of a camera cannot be created at 512 by 256 pixels, what the camera sees is not "
      "drawn"));

    // the window is drawn all the same
    ASSERT_EQ(_renderer.drawn.size(), 1u);
    EXPECT_EQ(_renderer.drawn[0].target, -1);
  }

  TEST_F(ForwardRenderPipelineCamerasTest, SaysOnceThatATextureHasNoName)
  {
    Frame({Window(1), Into("", 5)}, {7});
    Frame({Window(1), Into("", 5)}, {7});

    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u);
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "A camera draws into a texture that has no name"));
    EXPECT_EQ(_renderer.created, 0);
    EXPECT_EQ(_renderer.drawn.size(), 1u);
  }

  TEST_F(ForwardRenderPipelineCamerasTest, ACameraThatIsGoneDrawsNoMore)
  {
    Frame({Window(1), Into("mirror", 5)}, {7});
    Frame({Window(1)}, {7});

    ASSERT_EQ(_renderer.drawn.size(), 1u);
    EXPECT_EQ(_renderer.drawn[0].target, -1);
    EXPECT_EQ(_renderer.targets.begin()->second.begun, 1);
  }

  TEST_F(ForwardRenderPipelineCamerasTest, ReleasesTheTexturesItMadeWhenItIsCleanedUp)
  {
    (void) _renderer.CreateRenderTarget("of-another", 64, 64);

    Frame({Window(1), Into("mirror", 5), Into("minimap", 9), Into("of-another", 2)}, {7});

    _pipeline.CleanUp();

    // those it found are its own no less, since it draws into them
    EXPECT_EQ(_renderer.destroyed, 3);
  }

  // the sky

  TEST_F(ForwardRenderPipelineCamerasTest, WithoutASkyNoneIsDrawn)
  {
    Frame({Window(1)}, {7, 8});

    EXPECT_TRUE(_renderer.skies.empty());
  }

  TEST_F(ForwardRenderPipelineCamerasTest, DrawsTheSkyAsTheCameraOfTheWindowSeesIt)
  {
    SkyInfo sky;
    sky.texture = "day";
    _pipeline.SetSky(sky);

    Frame({Window(1)}, {7, 8});

    ASSERT_EQ(_renderer.skies.size(), 1u);
    EXPECT_EQ(_renderer.skies[0].texture, "day");
    EXPECT_EQ(_renderer.skies[0].target, -1);
    EXPECT_FLOAT_EQ(_renderer.skies[0].view[3].x, 1);
  }

  TEST_F(ForwardRenderPipelineCamerasTest, DrawsTheSkyOfASceneThatHasNoModel)
  {
    _pipeline.SetSky(SkyInfo{});

    Frame({Window(1)}, {});

    EXPECT_EQ(_renderer.skies.size(), 1u);
  }

  TEST_F(ForwardRenderPipelineCamerasTest, EveryCameraSeesTheSky)
  {
    _pipeline.SetSky(SkyInfo{});

    Frame({Window(1), Into("mirror", 5)}, {7, 8});

    // into the texture while it is drawn to, then into the window
    ASSERT_EQ(_renderer.skies.size(), 2u);
    EXPECT_EQ(_renderer.skies[0].target, _renderer.targets.begin()->first);
    EXPECT_EQ(_renderer.skies[0].after, 2u);
    EXPECT_FLOAT_EQ(_renderer.skies[0].view[3].x, 5);
    EXPECT_EQ(_renderer.skies[1].target, -1);
    EXPECT_EQ(_renderer.skies[1].after, 4u);
    EXPECT_FLOAT_EQ(_renderer.skies[1].view[3].x, 1);
  }

  TEST_F(ForwardRenderPipelineCamerasTest, TheSkyIsAskedForInEveryFrame)
  {
    _pipeline.SetSky(SkyInfo{});
    Frame({Window(1)}, {7});
    ASSERT_EQ(_renderer.skies.size(), 1u);

    // a frame that names no sky has none
    Frame({Window(1)}, {7});

    EXPECT_EQ(_renderer.skies.size(), 1u);
  }
}
