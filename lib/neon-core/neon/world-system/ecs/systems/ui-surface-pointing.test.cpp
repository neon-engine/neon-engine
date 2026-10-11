#include "ui-surface-pointing.hpp"

#include <memory>
#include <string>

#include <glm/gtc/matrix_transform.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/mock-ui-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/ray-cast.hpp>
#include <neon/world-system/ecs/components/ui-surface-view.hpp>

namespace
{
  using neon::Action;
  using neon::Entity;
  using neon::MouseButton;
  using neon::RayCast;
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

  /// The pointing over rays that RayCasting has already cast: the test
  /// writes what a ray hit, as the system would.
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
      _store.Register<RayCast>("RayCast");
      _store.Register<UiSurfaceView>("UiSurface");
      _system.Initialize(_store);

      // who points is said with every pointer; the tests of it ask for it
      EXPECT_CALL(_ui, SetPointerBy(_, _)).Times(::testing::AnyNumber());
    }

    void TearDown() override
    {
      _store.CleanUp();
    }

    /// A screen of 1 by 1 at a place, facing +Z, with its surface made.
    Entity CreateScreen(const int surface, const glm::vec3 &position, const glm::vec3 &scale = glm::vec3{1.0f})
    {
      const Entity screen = _store.CreateEntity("screen " + std::to_string(surface));
      _store.Set(screen, Placed(position, 0.0f, scale));
      _store.Set(screen, UiSurfaceView{.name = "screen", .surface = surface});
      return screen;
    }

    /// An entity with a ray, which hits `screen` at `point` in the world,
    /// or nothing.
    Entity CreateRay(
      const std::string &name,
      const Entity screen,
      const glm::vec3 &point,
      const Entity owner = neon::No_Entity,
      const float distance = 1.0f)
    {
      const Entity eye = _store.CreateEntity(name);
      Aim(eye, screen, point, owner, distance);
      return eye;
    }

    void Aim(const Entity eye, const Entity screen, const glm::vec3 &point, const Entity owner = neon::No_Entity, const float distance = 1.0f)
    {
      RayCast ray;
      ray.is_hitting = screen != neon::No_Entity;
      ray.hit = screen;
      ray.point = point;
      ray.distance = distance;
      ray.owner_entity = owner;
      _store.Set(eye, ray);
    }

    void Frame()
    {
      _system.Update(_store, 0.016);
    }
  };

  TEST_F(UiSurfacePointingTest, PutsThePointerWhereTheRayHitsTheScreen)
  {
    // a screen twice as wide, hit a quarter of its width right of its
    // middle and a tenth of its height above
    const Entity screen = CreateScreen(3, {1.0f, 2.0f, 0.0f}, {2.0f, 1.0f, 1.0f});
    CreateRay("eye", screen, {1.5f, 2.1f, 0.0f});

    EXPECT_CALL(_ui, SetPointerUv(3, FloatNear(0.75f, tolerance), FloatNear(0.4f, tolerance), false));

    Frame();
  }

  TEST_F(UiSurfacePointingTest, DoesNothingWhileNoRayHitsAScreen)
  {
    const Entity screen = CreateScreen(3, {0.0f, 0.0f, 0.0f});
    CreateRay("eye", neon::No_Entity, {0.0f, 0.0f, 0.0f});
    (void)screen;

    Frame();
    Frame();
  }

  TEST_F(UiSurfacePointingTest, PressesWithTheButtonOfThePointerAndWithAccept)
  {
    const Entity screen = CreateScreen(3, {0.0f, 0.0f, 0.0f});
    CreateRay("eye", screen, {0.0f, 0.0f, 0.0f});

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

  TEST_F(UiSurfacePointingTest, PressesWithTheLeftButtonOfTheMouseWhileThereIsNoPointer)
  {
    const Entity screen = CreateScreen(3, {0.0f, 0.0f, 0.0f});
    CreateRay("eye", screen, {0.0f, 0.0f, 0.0f});

    {
      InSequence in_order;
      EXPECT_CALL(_ui, SetPointerUv(3, _, _, true));
      EXPECT_CALL(_ui, SetPointerUv(3, _, _, false));
    }

    // a hidden cursor: the button is held, and the action of the pointer
    // is not set
    _input.state.SetMouseButtonDown(MouseButton::Left);
    Frame();

    // the other buttons press nothing
    _input.state.Reset();
    _input.state.SetMouseButtonDown(MouseButton::Right);
    Frame();
  }

  TEST_F(UiSurfacePointingTest, TakesThePointerAwayWhenTheRayHitsNothingAnyMore)
  {
    const Entity screen = CreateScreen(3, {0.0f, 0.0f, 0.0f});
    const Entity eye = CreateRay("eye", screen, {0.0f, 0.0f, 0.0f});

    {
      InSequence in_order;
      EXPECT_CALL(_ui, SetPointerUv(3, _, _, false));
      EXPECT_CALL(_ui, ClearPointer(3));
    }

    Frame();

    Aim(eye, neon::No_Entity, {0.0f, 0.0f, 0.0f});
    Frame();
    Frame();
  }

  TEST_F(UiSurfacePointingTest, MovesFromOneScreenToAnother)
  {
    const Entity first = CreateScreen(3, {0.0f, 0.0f, 0.0f});
    const Entity second = CreateScreen(4, {2.0f, 0.0f, 0.0f});
    const Entity eye = CreateRay("eye", first, {0.0f, 0.0f, 0.0f});

    {
      InSequence in_order;
      EXPECT_CALL(_ui, SetPointerUv(3, _, _, false));
      EXPECT_CALL(_ui, SetPointerUv(4, _, _, false));
      EXPECT_CALL(_ui, ClearPointer(3));
    }

    Frame();

    Aim(eye, second, {2.0f, 0.0f, 0.0f});
    Frame();
  }

  TEST_F(UiSurfacePointingTest, LeavesAScreenAloneThatIsHitFromBeyondItsReach)
  {
    const Entity screen = CreateScreen(3, {0.0f, 0.0f, 0.0f});
    _store.Get<UiSurfaceView>(screen)->reach = 3.0f;
    const Entity eye = CreateRay("eye", screen, {0.0f, 0.0f, 0.0f}, neon::No_Entity, 5.0f);

    Frame();

    // a reach of 0 is any distance
    _store.Get<UiSurfaceView>(screen)->reach = 0.0f;
    Aim(eye, screen, {0.0f, 0.0f, 0.0f}, neon::No_Entity, 5.0f);
    EXPECT_CALL(_ui, SetPointerUv(3, _, _, false));
    Frame();
  }

  TEST_F(UiSurfacePointingTest, LeavesAHitAloneThatIsNoScreenOrWhoseSurfaceIsNotMadeYet)
  {
    const Entity crate = _store.CreateEntity("crate");
    _store.Set(crate, Placed({0.0f, 0.0f, 0.0f}));
    CreateRay("eye", crate, {0.0f, 0.0f, 0.0f});

    const Entity unmade = CreateScreen(-1, {2.0f, 0.0f, 0.0f});
    CreateRay("other eye", unmade, {2.0f, 0.0f, 0.0f});

    Frame();
  }

  // who points

  TEST_F(UiSurfacePointingTest, TellsTheUserInterfaceThatTheOwnerOfTheRayPoints)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity screen = CreateScreen(3, {0.0f, 0.0f, 0.0f});
    CreateScreen(4, {5.0f, 0.0f, 0.0f});
    CreateRay("eye", screen, {0.0f, 0.0f, 0.0f}, player);

    EXPECT_CALL(_ui, SetPointerUv(3, _, _, false));
    EXPECT_CALL(_ui, SetPointerBy(3, player));
    Frame();
  }

  TEST_F(UiSurfacePointingTest, TellsTheUserInterfaceThatTheRaysEntityPointsWhenTheRayHasNoOwner)
  {
    const Entity screen = CreateScreen(3, {0.0f, 0.0f, 0.0f});
    const Entity eye = CreateRay("eye", screen, {0.0f, 0.0f, 0.0f});

    EXPECT_CALL(_ui, SetPointerUv(3, _, _, false));
    EXPECT_CALL(_ui, SetPointerBy(3, eye));
    Frame();
  }

  TEST_F(UiSurfacePointingTest, TwoRaysPointAtTwoScreensAndEachIsClearedOnItsOwn)
  {
    const Entity left = CreateScreen(3, {0.0f, 0.0f, 0.0f});
    const Entity right = CreateScreen(4, {2.0f, 0.0f, 0.0f});
    const Entity first = CreateRay("eye", left, {0.0f, 0.0f, 0.0f});
    const Entity second = CreateRay("other eye", right, {2.0f, 0.0f, 0.0f});

    {
      InSequence in_order;
      EXPECT_CALL(_ui, SetPointerUv(3, _, _, false));
      EXPECT_CALL(_ui, SetPointerUv(4, _, _, false));
      // one ray leaves: its screen loses the pointer
      EXPECT_CALL(_ui, SetPointerUv(4, _, _, false));
      EXPECT_CALL(_ui, ClearPointer(3));
      // the other leaves too
      EXPECT_CALL(_ui, ClearPointer(4));
    }

    Frame();

    Aim(first, neon::No_Entity, {0.0f, 0.0f, 0.0f});
    Frame();

    Aim(second, neon::No_Entity, {0.0f, 0.0f, 0.0f});
    Frame();
  }

  TEST_F(UiSurfacePointingTest, DoesNothingWithoutARay)
  {
    CreateScreen(3, {0.0f, 0.0f, 0.0f});

    Frame();
  }
}
