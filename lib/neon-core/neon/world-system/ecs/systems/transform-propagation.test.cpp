#include "transform-propagation.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>

namespace
{
  using neon::Entity;
  using neon::No_Entity;
  using neon::Transform;
  using neon::TransformPropagation;
  using neon::testing::FakeEntityStore;

  constexpr float tolerance = 1e-4f;

  void ExpectMatrix(const glm::mat4 &actual, const glm::mat4 &expected)
  {
    for (int column = 0; column < 4; column++)
    {
      for (int row = 0; row < 4; row++)
      {
        EXPECT_NEAR(actual[column][row], expected[column][row], tolerance)
          << "column " << column << ", row " << row;
      }
    }
  }

  void ExpectVector(const glm::vec3 &actual, const float x, const float y, const float z)
  {
    EXPECT_NEAR(actual.x, x, tolerance);
    EXPECT_NEAR(actual.y, y, tolerance);
    EXPECT_NEAR(actual.z, z, tolerance);
  }

  Transform At(const float x, const float y, const float z)
  {
    Transform transform;
    transform.position = {x, y, z};
    return transform;
  }

  class TransformPropagationTest : public ::testing::Test
  {
  protected:
    FakeEntityStore _store;
    TransformPropagation _system;

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Transform>("Transform");
      _system.Initialize(_store);
    }

    Entity Create(const Transform &transform, const Entity parent = No_Entity)
    {
      const Entity entity = _store.CreateEntity("", parent);
      _store.Set(entity, transform);
      return entity;
    }

    const glm::mat4 &WorldOf(const Entity entity)
    {
      return _store.Get<Transform>(entity)->world_coordinates;
    }

    /// Where the origin of the entity ends up in the world.
    glm::vec3 PlaceOf(const Entity entity)
    {
      return WorldOf(entity)[3];
    }

    /// Where a point of the entity ends up in the world.
    glm::vec3 PlaceOf(const Entity entity, const glm::vec3 &point)
    {
      return WorldOf(entity) * glm::vec4(point, 1.0f);
    }
  };

  // without a parent

  TEST_F(TransformPropagationTest, LeavesAnEntityThatWasNotMovedAtTheOrigin)
  {
    const Entity entity = Create(Transform{});

    _system.Update(_store, 0.0);

    ExpectMatrix(WorldOf(entity), glm::mat4(1.0f));
  }

  TEST_F(TransformPropagationTest, PlacesAnEntityAtItsPosition)
  {
    const Entity entity = Create(At(1.0f, 2.0f, 3.0f));

    _system.Update(_store, 0.0);

    ExpectMatrix(WorldOf(entity), translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f)));
    ExpectVector(PlaceOf(entity), 1.0f, 2.0f, 3.0f);
  }

  TEST_F(TransformPropagationTest, TurnsAnEntityByItsRotation)
  {
    Transform transform;
    transform.rotation = {.yaw = 90.0f};
    const Entity entity = Create(transform);

    _system.Update(_store, 0.0);

    // what pointed forward points to the left
    ExpectVector(PlaceOf(entity, {0.0f, 0.0f, -1.0f}), -1.0f, 0.0f, 0.0f);
    ExpectVector(_store.Get<Transform>(entity)->Forward(), -1.0f, 0.0f, 0.0f);
  }

  TEST_F(TransformPropagationTest, SizesAnEntityByItsScale)
  {
    Transform transform;
    transform.scale = {2.0f, 3.0f, 4.0f};
    const Entity entity = Create(transform);

    _system.Update(_store, 0.0);

    ExpectVector(PlaceOf(entity, {1.0f, 1.0f, 1.0f}), 2.0f, 3.0f, 4.0f);
  }

  TEST_F(TransformPropagationTest, SizesFirstThenTurnsThenMoves)
  {
    Transform transform;
    transform.position = {10.0f, 0.0f, 0.0f};
    transform.rotation = {.yaw = 90.0f};
    transform.scale = {2.0f, 1.0f, 1.0f};
    const Entity entity = Create(transform);

    _system.Update(_store, 0.0);

    // 1 to the right is sized to 2, turned to 2 to the front, and moved
    ExpectVector(PlaceOf(entity, {1.0f, 0.0f, 0.0f}), 10.0f, 0.0f, -2.0f);
    ExpectMatrix(
      WorldOf(entity),
      translate(glm::mat4(1.0f), transform.position) *
      mat4_cast(transform.rotation.GetQuaternion()) *
      scale(glm::mat4(1.0f), transform.scale));
  }

  TEST_F(TransformPropagationTest, ReplacesWhatTheWorldCoordinatesHeldBefore)
  {
    Transform transform = At(1.0f, 2.0f, 3.0f);
    transform.world_coordinates = glm::mat4(7.0f);
    const Entity entity = Create(transform);

    _system.Update(_store, 0.0);

    ExpectMatrix(WorldOf(entity), translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f)));
  }

  TEST_F(TransformPropagationTest, LeavesPositionRotationAndScaleAsTheyAre)
  {
    Transform transform = At(1.0f, 2.0f, 3.0f);
    transform.rotation = {.pitch = 10.0f, .yaw = 20.0f, .roll = 30.0f};
    transform.scale = {2.0f, 2.0f, 2.0f};
    const Entity parent = Create(At(5.0f, 5.0f, 5.0f));
    const Entity entity = Create(transform, parent);

    _system.Update(_store, 0.0);

    const auto &after = *_store.Get<Transform>(entity);
    ExpectVector(after.position, 1.0f, 2.0f, 3.0f);
    ExpectVector(after.scale, 2.0f, 2.0f, 2.0f);
    EXPECT_EQ(after.rotation.pitch, 10.0f);
    EXPECT_EQ(after.rotation.yaw, 20.0f);
    EXPECT_EQ(after.rotation.roll, 30.0f);
  }

  TEST_F(TransformPropagationTest, PlacesEveryEntityWithoutParentByItself)
  {
    const Entity first = Create(At(1.0f, 0.0f, 0.0f));
    const Entity second = Create(At(0.0f, 2.0f, 0.0f));

    _system.Update(_store, 0.0);

    ExpectVector(PlaceOf(first), 1.0f, 0.0f, 0.0f);
    ExpectVector(PlaceOf(second), 0.0f, 2.0f, 0.0f);
  }

  // with a parent

  TEST_F(TransformPropagationTest, PlacesAChildRelativeToItsParent)
  {
    const Entity parent = Create(At(10.0f, 0.0f, 0.0f));
    const Entity child = Create(At(1.0f, 2.0f, 3.0f), parent);

    _system.Update(_store, 0.0);

    ExpectVector(PlaceOf(parent), 10.0f, 0.0f, 0.0f);
    ExpectVector(PlaceOf(child), 11.0f, 2.0f, 3.0f);
  }

  TEST_F(TransformPropagationTest, PlacesAChildAsTheParentTimesTheChild)
  {
    Transform of_parent = At(10.0f, -4.0f, 2.0f);
    of_parent.rotation = {.pitch = 30.0f, .yaw = 45.0f, .roll = 10.0f};
    of_parent.scale = {2.0f, 3.0f, 0.5f};
    Transform of_child = At(1.0f, 2.0f, 3.0f);
    of_child.rotation = {.pitch = -15.0f, .yaw = 80.0f};
    of_child.scale = {0.5f, 0.5f, 4.0f};
    const Entity parent = Create(of_parent);
    const Entity child = Create(of_child, parent);

    _system.Update(_store, 0.0);

    const auto local = [](const Transform &transform)
    {
      return translate(glm::mat4(1.0f), transform.position) *
             mat4_cast(transform.rotation.GetQuaternion()) *
             scale(glm::mat4(1.0f), transform.scale);
    };
    ExpectMatrix(WorldOf(parent), local(of_parent));
    ExpectMatrix(WorldOf(child), local(of_parent) * local(of_child));
  }

  TEST_F(TransformPropagationTest, TurnsAChildAroundItsParent)
  {
    Transform of_parent = At(10.0f, 0.0f, 0.0f);
    of_parent.rotation = {.yaw = 90.0f};
    const Entity parent = Create(of_parent);
    const Entity child = Create(At(0.0f, 0.0f, -1.0f), parent);

    _system.Update(_store, 0.0);

    // in front of the parent, which looks to the left
    ExpectVector(PlaceOf(child), 9.0f, 0.0f, 0.0f);
    ExpectVector(_store.Get<Transform>(child)->Forward(), -1.0f, 0.0f, 0.0f);
  }

  TEST_F(TransformPropagationTest, SizesAChildAndItsDistanceWithItsParent)
  {
    Transform of_parent;
    of_parent.scale = {2.0f, 2.0f, 2.0f};
    const Entity parent = Create(of_parent);
    const Entity child = Create(At(1.0f, 0.0f, 0.0f), parent);

    _system.Update(_store, 0.0);

    ExpectVector(PlaceOf(child), 2.0f, 0.0f, 0.0f);
    ExpectVector(PlaceOf(child, {1.0f, 1.0f, 1.0f}), 4.0f, 2.0f, 2.0f);
  }

  TEST_F(TransformPropagationTest, PlacesSeveralLevelsEachRelativeToTheOneAbove)
  {
    const Entity body = Create(At(100.0f, 0.0f, 0.0f));
    const Entity arm = Create(At(0.0f, 10.0f, 0.0f), body);
    const Entity hand = Create(At(0.0f, 0.0f, 1.0f), arm);
    const Entity finger = Create(At(0.5f, 0.5f, 0.5f), hand);

    _system.Update(_store, 0.0);

    ExpectVector(PlaceOf(body), 100.0f, 0.0f, 0.0f);
    ExpectVector(PlaceOf(arm), 100.0f, 10.0f, 0.0f);
    ExpectVector(PlaceOf(hand), 100.0f, 10.0f, 1.0f);
    ExpectVector(PlaceOf(finger), 100.5f, 10.5f, 1.5f);
  }

  TEST_F(TransformPropagationTest, PlacesSeveralLevelsWithRotationAndScale)
  {
    Transform of_body = At(10.0f, 0.0f, 0.0f);
    of_body.rotation = {.yaw = 90.0f};
    Transform of_arm = At(0.0f, 0.0f, -2.0f);
    of_arm.scale = {3.0f, 3.0f, 3.0f};
    const Entity body = Create(of_body);
    const Entity arm = Create(of_arm, body);
    const Entity hand = Create(At(0.0f, 0.0f, -1.0f), arm);

    _system.Update(_store, 0.0);

    // the arm is 2 in front of the body, which looks to the left
    ExpectVector(PlaceOf(arm), 8.0f, 0.0f, 0.0f);
    // the hand is 1 in front of the arm, which is 3 in the size of the arm
    ExpectVector(PlaceOf(hand), 5.0f, 0.0f, 0.0f);
  }

  TEST_F(TransformPropagationTest, PlacesChildrenWhateverOrderTheEntitiesWereCreatedIn)
  {
    const Entity hand = Create(At(0.0f, 0.0f, 1.0f));
    const Entity arm = Create(At(0.0f, 10.0f, 0.0f));
    const Entity body = Create(At(100.0f, 0.0f, 0.0f));
    _store.SetParent(hand, arm);
    _store.SetParent(arm, body);

    _system.Update(_store, 0.0);

    ExpectVector(PlaceOf(hand), 100.0f, 10.0f, 1.0f);
  }

  TEST_F(TransformPropagationTest, PlacesEveryChildOfAParent)
  {
    const Entity parent = Create(At(10.0f, 0.0f, 0.0f));
    const Entity left = Create(At(-1.0f, 0.0f, 0.0f), parent);
    const Entity right = Create(At(1.0f, 0.0f, 0.0f), parent);

    _system.Update(_store, 0.0);

    ExpectVector(PlaceOf(left), 9.0f, 0.0f, 0.0f);
    ExpectVector(PlaceOf(right), 11.0f, 0.0f, 0.0f);
  }

  // from frame to frame

  TEST_F(TransformPropagationTest, FollowsAnEntityThatMoves)
  {
    const Entity entity = Create(At(1.0f, 0.0f, 0.0f));
    _system.Update(_store, 0.0);

    _store.Get<Transform>(entity)->position = {5.0f, 0.0f, 0.0f};
    _system.Update(_store, 0.0);

    ExpectVector(PlaceOf(entity), 5.0f, 0.0f, 0.0f);
  }

  TEST_F(TransformPropagationTest, MovesTheChildrenWithTheirParentInTheSameFrame)
  {
    const Entity body = Create(At(100.0f, 0.0f, 0.0f));
    const Entity arm = Create(At(0.0f, 10.0f, 0.0f), body);
    const Entity hand = Create(At(0.0f, 0.0f, 1.0f), arm);
    _system.Update(_store, 0.0);

    _store.Get<Transform>(body)->position = {200.0f, 0.0f, 0.0f};
    _system.Update(_store, 0.0);

    ExpectVector(PlaceOf(arm), 200.0f, 10.0f, 0.0f);
    ExpectVector(PlaceOf(hand), 200.0f, 10.0f, 1.0f);
  }

  TEST_F(TransformPropagationTest, GivesTheSameResultWhenNothingChanged)
  {
    Transform of_parent = At(10.0f, 0.0f, 0.0f);
    of_parent.rotation = {.yaw = 30.0f};
    of_parent.scale = {2.0f, 2.0f, 2.0f};
    const Entity parent = Create(of_parent);
    const Entity child = Create(At(1.0f, 2.0f, 3.0f), parent);
    _system.Update(_store, 0.0);
    const glm::mat4 first = WorldOf(child);

    for (int frame = 0; frame < 10; frame++) { _system.Update(_store, 0.016); }

    EXPECT_EQ(WorldOf(child), first);
  }

  TEST_F(TransformPropagationTest, FollowsAnEntityThatIsMovedUnderAnotherParent)
  {
    const Entity player = Create(At(10.0f, 0.0f, 0.0f));
    const Entity enemy = Create(At(-10.0f, 0.0f, 0.0f));
    const Entity sword = Create(At(1.0f, 0.0f, 0.0f), player);
    _system.Update(_store, 0.0);
    ExpectVector(PlaceOf(sword), 11.0f, 0.0f, 0.0f);

    _store.SetParent(sword, enemy);
    _system.Update(_store, 0.0);
    ExpectVector(PlaceOf(sword), -9.0f, 0.0f, 0.0f);

    _store.SetParent(sword, No_Entity);
    _system.Update(_store, 0.0);
    ExpectVector(PlaceOf(sword), 1.0f, 0.0f, 0.0f);
  }

  TEST_F(TransformPropagationTest, DoesNothingInAWorldWithoutTransforms)
  {
    _store.CreateEntity("folder");

    _system.Update(_store, 0.0);

    EXPECT_EQ(_store.EntityCount(), 1u);
  }
}
