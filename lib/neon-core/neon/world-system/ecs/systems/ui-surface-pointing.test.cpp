#include "ui-surface-pointing.hpp"

#include <memory>
#include <string>

#include <glm/gtc/matrix_transform.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/mock-ui-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/ui-surface-view.hpp>

namespace
{
  using neon::Action;
  using neon::Camera;
  using neon::Entity;
  using neon::RenderTarget;
  using neon::Transform;
  using neon::UiSurfacePointing;
  using neon::UiSurfaceView;
  using neon::testing::FakeEntityStore;
  using neon::testing::FakeInputContext;
  using neon::testing::MockUiContext;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::FloatNear;
  using ::testing::InSequence;
  using ::testing::NiceMock;
  using ::testing::StrictMock;

  constexpr float tolerance = 1e-4f;

  /// A transform that was placed in the world: at a position, turned by a
  /// yaw in degrees, and scaled.
  Transform Placed(const glm::vec3 &position, const float yaw = 0.0f, const glm::vec3 &scale = glm::vec3{1.0f})
  {
    Transform transform;
    transform.position = position;
    transform.rotation.yaw = yaw;
    transform.scale = scale;
    transform.world_coordinates =
      translate(glm::mat4{1.0f}, position) * mat4_cast(transform.rotation.GetQuaternion()) *
      glm::scale(glm::mat4{1.0f}, scale);
    return transform;
  }

  class UiSurfacePointingTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    StrictMock<MockUiContext> _ui;
    NiceMock<FakeInputContext> _input{_logger};
    FakeEntityStore _store;
    UiSurfacePointing _system{&_ui, &_input};

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Transform>("Transform");
      _store.Register<Camera>("Camera");
      _store.Register<UiSurfaceView>("UiSurface");
      _system.Initialize(_store);
    }

    void TearDown() override
    {
      _store.CleanUp();
    }

    /// A camera that draws the window, at a place, looking along -Z.
    Entity CreateCamera(const glm::vec3 &position, const float yaw = 0.0f)
    {
      const Entity camera = _store.CreateEntity("camera");
      _store.Set(camera, Placed(position, yaw));
      _store.Set(camera, Camera{});
      return camera;
    }

    /// A screen of 1 by 1 at a place, facing +Z, with its surface made.
    Entity CreateScreen(
      const int surface,
      const glm::vec3 &position,
      const float yaw = 0.0f,
      const glm::vec3 &scale = glm::vec3{1.0f},
      const float reach = 3.0f)
    {
      // by its surface, since the store gives one name to one entity
      const Entity screen = _store.CreateEntity("screen " + std::to_string(surface));
      _store.Set(screen, Placed(position, yaw, scale));
      _store.Set(screen, UiSurfaceView{.name = "screen", .reach = reach, .surface = surface});
      return screen;
    }

    void Frame()
    {
      _system.Update(_store, 0.016);
    }
  };

  // the ray and the square

  TEST(UiSurfacePointingHit, HitsTheMiddleOfASquareInFrontOfIt)
  {
    float u = 0.0f;
    float v = 0.0f;
    float distance = 0.0f;

    EXPECT_TRUE(UiSurfacePointing::Hit(Placed({0.0f, 0.0f, 0.0f}), {0.0f, 0.0f, 2.0f}, {0.0f, 0.0f, -1.0f}, u, v, distance));
    EXPECT_NEAR(u, 0.5f, tolerance);
    EXPECT_NEAR(v, 0.5f, tolerance);
    EXPECT_NEAR(distance, 2.0f, tolerance);
  }

  TEST(UiSurfacePointingHit, CountsFromTheLeftTopCorner)
  {
    float u = 0.0f;
    float v = 0.0f;
    float distance = 0.0f;

    // the top right of the square, which is +X and +Y
    EXPECT_TRUE(UiSurfacePointing::Hit(Placed({0.0f, 0.0f, 0.0f}), {0.4f, 0.3f, 1.0f}, {0.0f, 0.0f, -1.0f}, u, v, distance));
    EXPECT_NEAR(u, 0.9f, tolerance);
    EXPECT_NEAR(v, 0.2f, tolerance);
  }

  TEST(UiSurfacePointingHit, MissesNextToTheSquare)
  {
    float u = 0.0f;
    float v = 0.0f;
    float distance = 0.0f;

    EXPECT_FALSE(UiSurfacePointing::Hit(Placed({0.0f, 0.0f, 0.0f}), {0.6f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, u, v, distance));
    EXPECT_FALSE(UiSurfacePointing::Hit(Placed({0.0f, 0.0f, 0.0f}), {0.0f, -0.51f, 1.0f}, {0.0f, 0.0f, -1.0f}, u, v, distance));
  }

  TEST(UiSurfacePointingHit, MissesFromBehindAndWhenLookingAway)
  {
    float u = 0.0f;
    float v = 0.0f;
    float distance = 0.0f;

    // behind the square, looking further away from it
    EXPECT_FALSE(UiSurfacePointing::Hit(Placed({0.0f, 0.0f, 0.0f}), {0.0f, 0.0f, -2.0f}, {0.0f, 0.0f, -1.0f}, u, v, distance));
    // behind it, looking at its back
    EXPECT_FALSE(UiSurfacePointing::Hit(Placed({0.0f, 0.0f, 0.0f}), {0.0f, 0.0f, -2.0f}, {0.0f, 0.0f, 1.0f}, u, v, distance));
    // in front, looking away
    EXPECT_FALSE(UiSurfacePointing::Hit(Placed({0.0f, 0.0f, 0.0f}), {0.0f, 0.0f, 2.0f}, {0.0f, 0.0f, 1.0f}, u, v, distance));
    // along the face
    EXPECT_FALSE(UiSurfacePointing::Hit(Placed({0.0f, 0.0f, 0.0f}), {2.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, u, v, distance));
  }

  TEST(UiSurfacePointingHit, FollowsWhereTheSquareWasPlacedTurnedAndScaled)
  {
    float u = 0.0f;
    float v = 0.0f;
    float distance = 0.0f;

    // a screen twice as wide, turned to face +X, and the ray comes from +X
    const Transform screen = Placed({3.0f, 1.0f, 0.0f}, 90.0f, {2.0f, 1.0f, 1.0f});
    EXPECT_TRUE(UiSurfacePointing::Hit(screen, {5.0f, 1.0f, -0.5f}, {-1.0f, 0.0f, 0.0f}, u, v, distance));
    // -Z of the world is +X of the turned square, a quarter of its width of 2
    EXPECT_NEAR(u, 0.75f, tolerance);
    EXPECT_NEAR(v, 0.5f, tolerance);
    EXPECT_NEAR(distance, 2.0f, tolerance);
  }

  // the system

  TEST_F(UiSurfacePointingTest, PointsWhereTheCameraLooksOnAScreenItHits)
  {
    CreateCamera({0.0f, 0.0f, 2.0f});
    CreateScreen(3, {0.0f, 0.0f, 0.0f});

    EXPECT_CALL(_ui, SetPointerUv(3, FloatNear(0.5f, tolerance), FloatNear(0.5f, tolerance), false));
    EXPECT_CALL(_ui, SetFlag("pointing", true));

    Frame();
  }

  TEST_F(UiSurfacePointingTest, DoesNothingWhileNothingIsPointedAt)
  {
    CreateCamera({2.0f, 0.0f, 2.0f});
    CreateScreen(3, {0.0f, 0.0f, 0.0f});

    Frame();
    Frame();
  }

  TEST_F(UiSurfacePointingTest, PressesWithTheButtonOfThePointerAndWithAccept)
  {
    CreateCamera({0.0f, 0.0f, 2.0f});
    CreateScreen(3, {0.0f, 0.0f, 0.0f});
    EXPECT_CALL(_ui, SetFlag("pointing", true));

    {
      InSequence in_order;
      EXPECT_CALL(_ui, SetPointerUv(3, _, _, true));
      EXPECT_CALL(_ui, SetPointerUv(3, _, _, false));
      EXPECT_CALL(_ui, SetPointerUv(3, _, _, true));
    }

    _input.state.SetAction(Action::Pointer_Primary);
    Frame();

    _input.state.Reset();
    Frame();

    _input.state.SetAction(Action::Ui_Accept);
    Frame();
  }

  TEST_F(UiSurfacePointingTest, TakesThePointerAwayWhenTheCameraLooksElsewhere)
  {
    const Entity camera = CreateCamera({0.0f, 0.0f, 2.0f});
    CreateScreen(3, {0.0f, 0.0f, 0.0f});

    {
      InSequence in_order;
      EXPECT_CALL(_ui, SetPointerUv(3, _, _, false));
      EXPECT_CALL(_ui, SetFlag("pointing", true));
      EXPECT_CALL(_ui, ClearPointer(3));
      EXPECT_CALL(_ui, SetFlag("pointing", false));
    }

    Frame();

    _store.Set(camera, Placed({0.0f, 0.0f, 2.0f}, 90.0f));
    Frame();
    Frame();
  }

  TEST_F(UiSurfacePointingTest, LeavesAScreenAloneThatIsOutOfReach)
  {
    CreateCamera({0.0f, 0.0f, 5.0f});
    CreateScreen(3, {0.0f, 0.0f, 0.0f}, 0.0f, glm::vec3{1.0f}, 3.0f);

    Frame();
  }

  TEST_F(UiSurfacePointingTest, ReachesAnyDistanceWhenTheReachIsZero)
  {
    CreateCamera({0.0f, 0.0f, 50.0f});
    CreateScreen(3, {0.0f, 0.0f, 0.0f}, 0.0f, glm::vec3{1.0f}, 0.0f);

    EXPECT_CALL(_ui, SetPointerUv(3, _, _, false));
    EXPECT_CALL(_ui, SetFlag("pointing", true));

    Frame();
  }

  TEST_F(UiSurfacePointingTest, TakesTheNearestOfTwoScreensInTheWay)
  {
    CreateCamera({0.0f, 0.0f, 4.0f});
    CreateScreen(3, {0.0f, 0.0f, 0.0f}, 0.0f, glm::vec3{1.0f}, 0.0f);
    CreateScreen(4, {0.0f, 0.0f, 2.0f}, 0.0f, glm::vec3{1.0f}, 0.0f);

    EXPECT_CALL(_ui, SetPointerUv(4, _, _, false));
    EXPECT_CALL(_ui, SetFlag("pointing", true));

    Frame();
  }

  TEST_F(UiSurfacePointingTest, MovesFromOneScreenToAnother)
  {
    const Entity camera = CreateCamera({0.0f, 0.0f, 2.0f});
    CreateScreen(3, {0.0f, 0.0f, 0.0f});
    CreateScreen(4, {2.0f, 0.0f, 0.0f});

    {
      InSequence in_order;
      EXPECT_CALL(_ui, SetPointerUv(3, _, _, false));
      EXPECT_CALL(_ui, SetFlag("pointing", true));
      EXPECT_CALL(_ui, SetPointerUv(4, _, _, false));
      EXPECT_CALL(_ui, ClearPointer(3));
    }

    Frame();

    _store.Set(camera, Placed({2.0f, 0.0f, 2.0f}));
    Frame();
  }

  TEST_F(UiSurfacePointingTest, LeavesAScreenAloneWhoseSurfaceIsNotMadeYet)
  {
    CreateCamera({0.0f, 0.0f, 2.0f});
    CreateScreen(-1, {0.0f, 0.0f, 0.0f});

    Frame();
  }

  TEST_F(UiSurfacePointingTest, LooksFromTheCameraThatDrawsTheWindowAndNotFromOneThatDrawsATexture)
  {
    const Entity security = _store.CreateEntity("security");
    _store.Set(security, Placed({0.0f, 0.0f, 2.0f}));
    _store.Set(security, Camera{.target = RenderTarget::Texture});
    CreateScreen(3, {0.0f, 0.0f, 0.0f});

    Frame();

    CreateCamera({0.0f, 0.0f, 2.0f});
    EXPECT_CALL(_ui, SetPointerUv(3, _, _, false));
    EXPECT_CALL(_ui, SetFlag("pointing", true));

    Frame();
  }

  TEST_F(UiSurfacePointingTest, DoesNothingWithoutACamera)
  {
    CreateScreen(3, {0.0f, 0.0f, 0.0f});

    Frame();
  }
}
