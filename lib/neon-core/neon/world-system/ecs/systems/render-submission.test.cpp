#include "render-submission.hpp"

#include <memory>
#include <string>

#include <glm/gtc/matrix_transform.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/mock-render-pipeline.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/light.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>
#include <neon/world-system/ecs/components/sky.hpp>

namespace
{
  using neon::Camera;
  using neon::CameraInfo;
  using neon::Entity;
  using neon::Light;
  using neon::LightSource;
  using neon::Renderable;
  using neon::RenderInfo;
  using neon::RenderSubmission;
  using neon::RenderTarget;
  using neon::Sky;
  using neon::SkyInfo;
  using neon::SkyType;
  using neon::Transform;
  using neon::testing::FakeEntityStore;
  using neon::testing::MockRenderPipeline;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::AllOf;
  using ::testing::Field;
  using ::testing::InSequence;
  using ::testing::Return;
  using ::testing::StrictMock;

  /// A transform that was placed in the world already.
  Transform Placed(const float x, const float y, const float z, const float yaw = 0.0f)
  {
    Transform transform;
    transform.position = {x, y, z};
    transform.rotation.yaw = yaw;
    transform.world_coordinates =
      translate(glm::mat4(1.0f), transform.position) * mat4_cast(transform.rotation.GetQuaternion());
    return transform;
  }

  Renderable Model(const std::string &path)
  {
    Renderable renderable;
    renderable.render_info.model_path = path;
    return renderable;
  }

  ::testing::Matcher<const Transform &> IsPlacedAt(const float x, const float y, const float z)
  {
    return Field(
      "world_coordinates",
      &Transform::world_coordinates,
      translate(glm::mat4(1.0f), glm::vec3(x, y, z)));
  }

  ::testing::Matcher<const RenderInfo &> IsModel(const std::string &path)
  {
    return Field("model_path", &RenderInfo::model_path, path);
  }

  class RenderSubmissionTest : public ::testing::Test
  {
  protected:
    FakeEntityStore _store;
    // strict, so that everything that reaches the pipeline has to be expected
    StrictMock<MockRenderPipeline> _pipeline{std::make_shared<RecordingLogger>()};
    RenderSubmission _system{&_pipeline};

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Transform>("Transform");
      _store.Register<Camera>("Camera");
      _store.Register<Light>("Light");
      _store.Register<Sky>("Sky");
      _store.Register<Renderable>("Renderable");
      _system.Initialize(_store);

      // the time is handed over in every frame, whatever else a test expects
      EXPECT_CALL(_pipeline, SetTime(_, _)).Times(::testing::AnyNumber());
    }

    template<typename Component>
    Entity Create(const Transform &transform, const Component &component)
    {
      const Entity entity = _store.CreateEntity("");
      _store.Set(entity, transform);
      _store.Set(entity, component);
      return entity;
    }
  };

  TEST_F(RenderSubmissionTest, HandsOverNothingFromAnEmptyWorld)
  {
    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, HandsOverNothingWhenItIsInitialized)
  {
    Create(Placed(0.0f, 0.0f, 0.0f), Model("assets://models/cube.obj"));
    Create(Placed(0.0f, 0.0f, 0.0f), Camera{});
    Create(Placed(0.0f, 0.0f, 0.0f), Light{});

    RenderSubmission system(&_pipeline);
    system.Initialize(_store);
  }

  TEST_F(RenderSubmissionTest, LeavesRenderingTheFrameToTheWorld)
  {
    Create(Placed(0.0f, 0.0f, 0.0f), Camera{});
    EXPECT_CALL(_pipeline, SetCameraInfo(_));

    // the pipeline is strict, RenderFrame would fail the test
    _system.Update(_store, 0.016);
  }

  // cameras

  TEST_F(RenderSubmissionTest, HandsOverTheSettingsOfTheCamera)
  {
    Create(
      Placed(0.0f, 0.0f, 0.0f),
      Camera{.target = RenderTarget::Texture, .fov = 60.0f, .near_plane = 0.5f, .far_plane = 250.0f});

    EXPECT_CALL(
      _pipeline,
      SetCameraInfo(
        AllOf(
          Field("target", &CameraInfo::target, RenderTarget::Texture),
          Field("fov", &CameraInfo::fov, 60.0f),
          Field("near", &CameraInfo::near, 0.5f),
          Field("far", &CameraInfo::far, 250.0f))));

    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, HandsOverTheSettingsACameraHasByDefault)
  {
    Create(Placed(0.0f, 0.0f, 0.0f), Camera{});

    EXPECT_CALL(
      _pipeline,
      SetCameraInfo(
        AllOf(
          Field("target", &CameraInfo::target, RenderTarget::Window),
          Field("fov", &CameraInfo::fov, 45.0f),
          Field("near", &CameraInfo::near, 0.1f),
          Field("far", &CameraInfo::far, 1000.0f))));

    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, LooksFromWhereTheCameraIsInTheDirectionItFaces)
  {
    Create(Placed(1.0f, 2.0f, 3.0f, 90.0f), Camera{});

    // at 1, 2, 3 and turned to the left
    const auto view = lookAt(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(0.0f, 2.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    CameraInfo handed_over;
    EXPECT_CALL(_pipeline, SetCameraInfo(_)).WillOnce(::testing::SaveArg<0>(&handed_over));

    _system.Update(_store, 0.016);

    for (int column = 0; column < 4; column++)
    {
      for (int row = 0; row < 4; row++)
      {
        EXPECT_NEAR(handed_over.view[column][row], view[column][row], 1e-5f)
          << "column " << column << ", row " << row;
      }
    }
  }

  TEST_F(RenderSubmissionTest, HandsOverTheEffectsOfACameraInTheirOrder)
  {
    Camera camera;
    camera.effects = {"assets://shaders/effects/vignette", "extensions://game/assets/shaders/waves"};
    camera.screen_effects = {"assets://shaders/effects/scan-lines"};
    Create(Placed(0.0f, 0.0f, 0.0f, 0.0f), camera);

    CameraInfo handed_over;
    EXPECT_CALL(_pipeline, SetCameraInfo(_)).WillOnce(::testing::SaveArg<0>(&handed_over));

    _system.Update(_store, 0.016);

    EXPECT_EQ(handed_over.effects, camera.effects);
    EXPECT_EQ(handed_over.screen_effects, camera.screen_effects);
  }

  TEST_F(RenderSubmissionTest, LooksFromWhereTheCameraIsInTheWorldAndNotFromItsPosition)
  {
    // the child of something that was moved: its own position is 0
    Transform transform;
    transform.world_coordinates = translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f));
    Create(transform, Camera{});

    const auto view =
      lookAt(glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(10.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    EXPECT_CALL(_pipeline, SetCameraInfo(Field("view", &CameraInfo::view, view)));

    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, LooksWithTheUpOfTheCamera)
  {
    Create(Placed(0.0f, 0.0f, 0.0f), Camera{.up = {1.0f, 0.0f, 0.0f}});

    const auto view = lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    EXPECT_CALL(_pipeline, SetCameraInfo(Field("view", &CameraInfo::view, view)));

    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, StaysLevelWhenTheEntityRollsUnlessTheCameraRollsWithIt)
  {
    // rolled a quarter turn, as what a camera over a shoulder hangs from
    // may be: the view is as if it were not
    Transform transform;
    transform.world_coordinates =
      rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    Create(transform, Camera{});

    const auto view = lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    EXPECT_CALL(_pipeline, SetCameraInfo(Field("view", &CameraInfo::view, view)));

    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, RollsTheViewWithTheRollOfTheEntity)
  {
    // rolled a quarter turn around where it looks: what was up points left
    Transform transform;
    transform.world_coordinates =
      rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    Create(transform, Camera{.rolls_with_entity = true});

    const auto view = lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
    CameraInfo handed_over;
    EXPECT_CALL(_pipeline, SetCameraInfo(_)).WillOnce(::testing::SaveArg<0>(&handed_over));

    _system.Update(_store, 0.016);

    for (int column = 0; column < 4; column++)
    {
      for (int row = 0; row < 4; row++)
      {
        EXPECT_NEAR(handed_over.view[column][row], view[column][row], 1e-5f)
          << "column " << column << ", row " << row;
      }
    }
  }

  TEST_F(RenderSubmissionTest, StaysLevelWhenAnEntityItRollsWithLooksUpOrTurnsAround)
  {
    // turned to the left and looking up by 30 degrees: no roll in it
    Transform transform;
    transform.world_coordinates =
      rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
      rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    Create(transform, Camera{.rolls_with_entity = true});

    const glm::vec3 forward = glm::mat3(transform.world_coordinates) * glm::vec3(0.0f, 0.0f, -1.0f);
    const auto view = lookAt(glm::vec3(0.0f), forward, glm::vec3(0.0f, 1.0f, 0.0f));
    CameraInfo handed_over;
    EXPECT_CALL(_pipeline, SetCameraInfo(_)).WillOnce(::testing::SaveArg<0>(&handed_over));

    _system.Update(_store, 0.016);

    for (int column = 0; column < 4; column++)
    {
      for (int row = 0; row < 4; row++)
      {
        EXPECT_NEAR(handed_over.view[column][row], view[column][row], 1e-5f)
          << "column " << column << ", row " << row;
      }
    }
  }

  TEST_F(RenderSubmissionTest, TellsTheShadersHowLongTheWorldHasRun)
  {
    {
      const InSequence in_order;
      EXPECT_CALL(_pipeline, SetTime(::testing::DoubleEq(0.25), ::testing::DoubleEq(0.25)));
      EXPECT_CALL(_pipeline, SetTime(::testing::DoubleEq(0.75), ::testing::DoubleEq(0.5)));
    }

    // a frame of a quarter of a second, and one of half a second
    _system.Update(_store, 0.25);
    _system.Update(_store, 0.5);
  }

  TEST_F(RenderSubmissionTest, HandsOverTheCameraInEveryFrame)
  {
    const Entity camera = Create(Placed(0.0f, 0.0f, 0.0f), Camera{.fov = 45.0f});

    {
      InSequence in_order;
      EXPECT_CALL(_pipeline, SetCameraInfo(Field("fov", &CameraInfo::fov, 45.0f)));
      EXPECT_CALL(_pipeline, SetCameraInfo(Field("fov", &CameraInfo::fov, 90.0f)));
    }

    _system.Update(_store, 0.016);
    _store.Get<Camera>(camera)->fov = 90.0f;
    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, HandsOverEveryCameraOfTheWorld)
  {
    Create(Placed(0.0f, 0.0f, 0.0f), Camera{.fov = 30.0f});
    Create(Placed(0.0f, 0.0f, 0.0f), Camera{.fov = 60.0f});

    // the pipeline keeps the one it was handed last
    EXPECT_CALL(_pipeline, SetCameraInfo(Field("fov", &CameraInfo::fov, 30.0f)));
    EXPECT_CALL(_pipeline, SetCameraInfo(Field("fov", &CameraInfo::fov, 60.0f)));

    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, IgnoresACameraThatHasNoPlaceInTheWorld)
  {
    _store.Set(_store.CreateEntity("camera"), Camera{});

    _system.Update(_store, 0.016);
  }

  // lights

  TEST_F(RenderSubmissionTest, HandsOverALightWhereItIsInTheWorld)
  {
    Light light;
    light.source.id = "lamp";
    light.source.light_type = neon::LightType::Point;
    light.source.linear = 0.09f;
    light.source.diffuse = {1.0f, 0.5f, 0.25f};
    light.source.position = {9.0f, 9.0f, 9.0f};
    Create(Placed(1.0f, 2.0f, 3.0f), light);

    EXPECT_CALL(
      _pipeline,
      EnqueueLightSource(
        AllOf(
          Field("id", &LightSource::id, "lamp"),
          Field("light_type", &LightSource::light_type, neon::LightType::Point),
          Field("linear", &LightSource::linear, 0.09f),
          Field("diffuse", &LightSource::diffuse, glm::vec3(1.0f, 0.5f, 0.25f)),
          Field("position", &LightSource::position, glm::vec3(1.0f, 2.0f, 3.0f)))));

    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, KeepsWhereTheLightIsInItsComponent)
  {
    const Entity lamp = Create(Placed(1.0f, 2.0f, 3.0f), Light{});
    EXPECT_CALL(_pipeline, EnqueueLightSource(_));

    _system.Update(_store, 0.016);

    EXPECT_EQ(_store.Get<Light>(lamp)->source.position, glm::vec3(1.0f, 2.0f, 3.0f));
  }

  TEST_F(RenderSubmissionTest, LeavesTheDirectionOfALightAsItWasSet)
  {
    Light light;
    light.source.light_type = neon::LightType::Direction;
    light.source.direction = {0.0f, -1.0f, 0.0f};
    Create(Placed(1.0f, 2.0f, 3.0f, 90.0f), light);

    EXPECT_CALL(
      _pipeline,
      EnqueueLightSource(Field("direction", &LightSource::direction, glm::vec3(0.0f, -1.0f, 0.0f))));

    _system.Update(_store, 0.016);
  }

  // the sky

  TEST_F(RenderSubmissionTest, HandsOverTheSkyOfTheWorld)
  {
    Sky sky;
    sky.info.type = SkyType::Sphere;
    sky.info.texture = "assets://textures/sky/panorama.png";
    sky.info.rotation = 90.0f;
    sky.info.brightness = 0.5f;
    _store.Set(_store.CreateEntity("sky"), sky);

    EXPECT_CALL(
      _pipeline,
      SetSky(
        AllOf(
          Field("type", &SkyInfo::type, SkyType::Sphere),
          Field("texture", &SkyInfo::texture, "assets://textures/sky/panorama.png"),
          Field("rotation", &SkyInfo::rotation, 90.0f),
          Field("brightness", &SkyInfo::brightness, 0.5f))));

    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, HandsOverTheSkyInEveryFrame)
  {
    _store.Set(_store.CreateEntity("sky"), Sky{});

    EXPECT_CALL(_pipeline, SetSky(_)).Times(3);

    for (int frame = 0; frame < 3; frame++) { _system.Update(_store, 0.016); }
  }

  TEST_F(RenderSubmissionTest, HandsOverOneSkyWhenTheWorldHasTwo)
  {
    _store.Set(_store.CreateEntity("day"), Sky{});
    _store.Set(_store.CreateEntity("night"), Sky{});

    EXPECT_CALL(_pipeline, SetSky(_)).Times(1);

    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, HandsOverEveryLightInEveryFrame)
  {
    Light sun;
    sun.source.id = "sun";
    Light lamp;
    lamp.source.id = "lamp";
    Create(Placed(0.0f, 100.0f, 0.0f), sun);
    Create(Placed(1.0f, 2.0f, 3.0f), lamp);

    EXPECT_CALL(_pipeline, EnqueueLightSource(Field("id", &LightSource::id, "sun"))).Times(3);
    EXPECT_CALL(_pipeline, EnqueueLightSource(Field("id", &LightSource::id, "lamp"))).Times(3);

    for (int frame = 0; frame < 3; frame++) { _system.Update(_store, 0.016); }
  }

  TEST_F(RenderSubmissionTest, FollowsALightThatMoves)
  {
    const Entity lamp = Create(Placed(1.0f, 0.0f, 0.0f), Light{});

    {
      InSequence in_order;
      EXPECT_CALL(
        _pipeline,
        EnqueueLightSource(Field("position", &LightSource::position, glm::vec3(1.0f, 0.0f, 0.0f))));
      EXPECT_CALL(
        _pipeline,
        EnqueueLightSource(Field("position", &LightSource::position, glm::vec3(5.0f, 0.0f, 0.0f))));
    }

    _system.Update(_store, 0.016);
    _store.Set(lamp, Placed(5.0f, 0.0f, 0.0f));
    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, IgnoresALightThatHasNoPlaceInTheWorld)
  {
    _store.Set(_store.CreateEntity("lamp"), Light{});

    _system.Update(_store, 0.016);
  }

  // what is visible

  TEST_F(RenderSubmissionTest, MakesAnEntityKnownToTheRendererTheFirstTimeItIsDrawn)
  {
    const Entity cube = Create(Placed(1.0f, 2.0f, 3.0f), Model("assets://models/cube.obj"));

    {
      InSequence in_order;
      EXPECT_CALL(_pipeline, CreateRenderObject(IsModel("assets://models/cube.obj"))).WillOnce(Return(7));
      EXPECT_CALL(_pipeline, EnqueueForRendering(7, IsPlacedAt(1.0f, 2.0f, 3.0f)));
    }

    _system.Update(_store, 0.016);

    EXPECT_EQ(_store.Get<Renderable>(cube)->render_object_id, 7);
  }

  TEST_F(RenderSubmissionTest, HandsEverythingThatDescribesTheEntityToTheRenderer)
  {
    Renderable renderable;
    renderable.render_info.model_path = "assets://models/cube.obj";
    renderable.render_info.shader_path = "assets://shaders/basic-lit";
    renderable.render_info.texture_paths = {"assets://textures/wood.png", "assets://textures/gold.png"};
    renderable.render_info.scale_textures = true;
    renderable.render_info.material_info.shininess = 32.0f;
    renderable.render_info.material_info.use_textures = false;
    Create(Placed(0.0f, 0.0f, 0.0f), renderable);

    RenderInfo handed_over;
    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(
      ::testing::DoAll(::testing::SaveArg<0>(&handed_over), Return(1)));
    EXPECT_CALL(_pipeline, EnqueueForRendering(1, _));

    _system.Update(_store, 0.016);

    EXPECT_EQ(handed_over.model_path, "assets://models/cube.obj");
    EXPECT_EQ(handed_over.shader_path, "assets://shaders/basic-lit");
    EXPECT_THAT(
      handed_over.texture_paths,
      ::testing::ElementsAre("assets://textures/wood.png", "assets://textures/gold.png"));
    EXPECT_TRUE(handed_over.scale_textures);
    EXPECT_EQ(handed_over.material_info.shininess, 32.0f);
    EXPECT_FALSE(handed_over.material_info.use_textures);
  }

  TEST_F(RenderSubmissionTest, MakesAnEntityKnownToTheRendererOnce)
  {
    Create(Placed(1.0f, 2.0f, 3.0f), Model("assets://models/cube.obj"));

    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(7));
    EXPECT_CALL(_pipeline, EnqueueForRendering(7, _)).Times(5);

    for (int frame = 0; frame < 5; frame++) { _system.Update(_store, 0.016); }
  }

  TEST_F(RenderSubmissionTest, AcceptsZeroAsTheIdOfARenderObject)
  {
    Create(Placed(1.0f, 2.0f, 3.0f), Model("assets://models/cube.obj"));

    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(0));
    EXPECT_CALL(_pipeline, EnqueueForRendering(0, _)).Times(2);

    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, DrawsAnEntityThatIsKnownToTheRendererAlready)
  {
    Renderable renderable = Model("assets://models/cube.obj");
    renderable.render_object_id = 12;
    Create(Placed(1.0f, 2.0f, 3.0f), renderable);

    EXPECT_CALL(_pipeline, EnqueueForRendering(12, IsPlacedAt(1.0f, 2.0f, 3.0f)));

    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, MakesEveryEntityKnownToTheRendererByItself)
  {
    const Entity cube = Create(Placed(1.0f, 0.0f, 0.0f), Model("assets://models/cube.obj"));
    const Entity sphere = Create(Placed(2.0f, 0.0f, 0.0f), Model("assets://models/sphere.obj"));
    // the same model again is a render object of its own
    const Entity other_cube = Create(Placed(3.0f, 0.0f, 0.0f), Model("assets://models/cube.obj"));

    EXPECT_CALL(_pipeline, CreateRenderObject(IsModel("assets://models/cube.obj")))
      .WillOnce(Return(1))
      .WillOnce(Return(3));
    EXPECT_CALL(_pipeline, CreateRenderObject(IsModel("assets://models/sphere.obj"))).WillOnce(Return(2));
    EXPECT_CALL(_pipeline, EnqueueForRendering(1, IsPlacedAt(1.0f, 0.0f, 0.0f)));
    EXPECT_CALL(_pipeline, EnqueueForRendering(2, IsPlacedAt(2.0f, 0.0f, 0.0f)));
    EXPECT_CALL(_pipeline, EnqueueForRendering(3, IsPlacedAt(3.0f, 0.0f, 0.0f)));

    _system.Update(_store, 0.016);

    EXPECT_EQ(_store.Get<Renderable>(cube)->render_object_id, 1);
    EXPECT_EQ(_store.Get<Renderable>(sphere)->render_object_id, 2);
    EXPECT_EQ(_store.Get<Renderable>(other_cube)->render_object_id, 3);
  }

  TEST_F(RenderSubmissionTest, DrawsAnEntityThatJoinsTheWorldLater)
  {
    Create(Placed(1.0f, 0.0f, 0.0f), Model("assets://models/cube.obj"));
    EXPECT_CALL(_pipeline, CreateRenderObject(IsModel("assets://models/cube.obj"))).WillOnce(Return(1));
    EXPECT_CALL(_pipeline, EnqueueForRendering(1, _)).Times(2);
    _system.Update(_store, 0.016);

    Create(Placed(2.0f, 0.0f, 0.0f), Model("assets://models/sphere.obj"));
    EXPECT_CALL(_pipeline, CreateRenderObject(IsModel("assets://models/sphere.obj"))).WillOnce(Return(2));
    EXPECT_CALL(_pipeline, EnqueueForRendering(2, IsPlacedAt(2.0f, 0.0f, 0.0f)));
    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, DrawsAnEntityWhereItIsInEveryFrame)
  {
    const Entity cube = Create(Placed(1.0f, 0.0f, 0.0f), Model("assets://models/cube.obj"));
    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(1));

    {
      InSequence in_order;
      EXPECT_CALL(_pipeline, EnqueueForRendering(1, IsPlacedAt(1.0f, 0.0f, 0.0f)));
      EXPECT_CALL(_pipeline, EnqueueForRendering(1, IsPlacedAt(5.0f, 0.0f, 0.0f)));
    }

    _system.Update(_store, 0.016);
    *_store.Get<Transform>(cube) = Placed(5.0f, 0.0f, 0.0f);
    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, TellsTheRendererOnceWhatAnEntityLooksLikeWhenItsVersionWasCountedUp)
  {
    Renderable face;
    face.render_info.model_path = "assets://models/head.glb";
    face.render_info.texture_paths = {"assets://textures/face-open.png"};
    const Entity head = Create(Placed(0.0f, 0.0f, 0.0f), face);

    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(4));
    EXPECT_CALL(_pipeline, EnqueueForRendering(4, _)).Times(4);

    int told = 0;
    std::vector<std::string> told_textures;
    EXPECT_CALL(_pipeline, UpdateRenderObject(4, _))
      .WillRepeatedly([&](const int, const neon::RenderInfo &info)
      {
        told++;
        told_textures = info.texture_paths;
      });

    // drawn as it was made, as long as nothing counts its version up
    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
    EXPECT_EQ(told, 0);

    // another texture: told once, before it is drawn, and not again after
    Renderable *shown = _store.Get<Renderable>(head);
    shown->render_info.texture_paths = {"assets://textures/face-closed.png"};
    shown->render_info.version++;

    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
    EXPECT_EQ(told, 1);
    EXPECT_THAT(told_textures, ::testing::ElementsAre("assets://textures/face-closed.png"));
  }

  TEST_F(RenderSubmissionTest, DoesNotTellTheRendererAgainOfWhatWasWrittenBeforeAnEntityWasFirstDrawn)
  {
    Renderable crate;
    crate.render_info.model_path = "assets://models/crate.glb";
    crate.render_info.version = 5;
    Create(Placed(0.0f, 0.0f, 0.0f), crate);

    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(4));
    EXPECT_CALL(_pipeline, EnqueueForRendering(4, _)).Times(2);
    EXPECT_CALL(_pipeline, UpdateRenderObject(_, _)).Times(0);

    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, HandsAMeshThatWasChangedToTheRendererAgain)
  {
    const auto mesh = std::make_shared<neon::MeshData>();
    mesh->vertices.resize(3);
    mesh->indices = {0, 1, 2};

    Renderable built;
    built.render_info.mesh = mesh;
    const Entity rope = Create(Placed(0.0f, 0.0f, 0.0f), built);

    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(4));
    EXPECT_CALL(_pipeline, EnqueueForRendering(4, _)).Times(4);

    // drawn as it was made, and as long as nothing counts its version up
    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);

    // changed: handed over once, before it is drawn, and not again after
    mesh->vertices[1].position = {1.0f, 2.0f, 3.0f};
    _store.Get<Renderable>(rope)->render_info.mesh_version++;
    EXPECT_CALL(
      _pipeline,
      UpdateRenderObjectMesh(4, Field("vertices", &neon::MeshData::vertices, ::testing::SizeIs(3)))).Times(1);

    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, DoesNotHandOverAMeshAgainThatChangedBeforeItWasFirstDrawn)
  {
    Renderable built;
    built.render_info.mesh = std::make_shared<neon::MeshData>();
    built.render_info.mesh_version = 3;
    Create(Placed(0.0f, 0.0f, 0.0f), built);

    // the renderer is created with the mesh as it is by then
    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(4));
    EXPECT_CALL(_pipeline, EnqueueForRendering(4, _));

    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, StopsDrawingAnEntityThatIsNoLongerVisible)
  {
    const Entity cube = Create(Placed(1.0f, 0.0f, 0.0f), Model("assets://models/cube.obj"));
    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(1));
    EXPECT_CALL(_pipeline, EnqueueForRendering(1, _)).Times(1);
    _system.Update(_store, 0.016);

    _store.Remove<Renderable>(cube);
    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, IgnoresWhatIsVisibleAndHasNoPlaceInTheWorld)
  {
    _store.Set(_store.CreateEntity("cube"), Model("assets://models/cube.obj"));

    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, DoesNotDrawAnEntityTheRendererCouldNotCreate)
  {
    const Entity broken = Create(Placed(1.0f, 0.0f, 0.0f), Model("assets://models/missing.obj"));
    Create(Placed(2.0f, 0.0f, 0.0f), Model("assets://models/cube.obj"));

    EXPECT_CALL(_pipeline, CreateRenderObject(IsModel("assets://models/missing.obj"))).WillOnce(Return(-1));
    EXPECT_CALL(_pipeline, CreateRenderObject(IsModel("assets://models/cube.obj"))).WillOnce(Return(1));
    // and nothing with the id -1, which the renderer does not know
    EXPECT_CALL(_pipeline, EnqueueForRendering(1, _));

    _system.Update(_store, 0.016);

    EXPECT_EQ(_store.Get<Renderable>(broken)->render_object_id, -1);
  }

  // An entity the renderer could not create is handed to it again in every
  // frame. A model that is missing is then looked for, and reported in the
  // log, sixty times a second. Whether to try once, or again after a while,
  // or when the Renderable changes is a question of design. It needs a way
  // to tell "not created yet" from "could not be created", which the id of
  // -1 stands for both.
  TEST_F(RenderSubmissionTest, DISABLED_DoesNotAskTheRendererAgainForAnEntityItCouldNotCreate)
  {
    Create(Placed(1.0f, 0.0f, 0.0f), Model("assets://models/missing.obj"));

    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(-1));

    for (int frame = 0; frame < 3; frame++) { _system.Update(_store, 0.016); }
  }

  // everything together

  TEST_F(RenderSubmissionTest, HandsOverTheCameraThenTheLightsThenWhatIsVisible)
  {
    Create(Placed(1.0f, 0.0f, 0.0f), Model("assets://models/cube.obj"));
    Create(Placed(0.0f, 5.0f, 0.0f), Light{});
    Create(Placed(0.0f, 0.0f, 5.0f), Camera{});

    {
      InSequence in_order;
      EXPECT_CALL(_pipeline, SetCameraInfo(_));
      EXPECT_CALL(_pipeline, EnqueueLightSource(_));
      EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(1));
      EXPECT_CALL(_pipeline, EnqueueForRendering(1, _));
    }

    _system.Update(_store, 0.016);
  }

  TEST_F(RenderSubmissionTest, HandsOverAnEntityThatIsSeveralThingsAtOnce)
  {
    const Entity entity = Create(Placed(1.0f, 2.0f, 3.0f), Model("assets://models/lamp.obj"));
    _store.Set(entity, Light{});
    _store.Set(entity, Camera{});

    EXPECT_CALL(_pipeline, SetCameraInfo(_));
    EXPECT_CALL(
      _pipeline,
      EnqueueLightSource(Field("position", &LightSource::position, glm::vec3(1.0f, 2.0f, 3.0f))));
    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(1));
    EXPECT_CALL(_pipeline, EnqueueForRendering(1, IsPlacedAt(1.0f, 2.0f, 3.0f)));

    _system.Update(_store, 0.016);
  }
}
