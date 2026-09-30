#include "transform.hpp"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

namespace
{
  using neon::Transform;

  constexpr float tolerance = 1e-5f;

  void ExpectVector(const glm::vec3 &actual, const float x, const float y, const float z)
  {
    EXPECT_NEAR(actual.x, x, tolerance);
    EXPECT_NEAR(actual.y, y, tolerance);
    EXPECT_NEAR(actual.z, z, tolerance);
  }

  /// A transform that was placed in the world with a rotation.
  Transform Rotated(const float pitch, const float yaw)
  {
    Transform transform;
    transform.rotation = {.pitch = pitch, .yaw = yaw};
    transform.world_coordinates = mat4_cast(transform.rotation.GetQuaternion());
    return transform;
  }

  TEST(Transform, StartsAtTheOriginWithoutRotationAtFullSize)
  {
    const Transform transform;

    ExpectVector(transform.position, 0.0f, 0.0f, 0.0f);
    ExpectVector(transform.scale, 1.0f, 1.0f, 1.0f);
    EXPECT_EQ(transform.rotation.pitch, 0.0f);
    EXPECT_EQ(transform.rotation.yaw, 0.0f);
    EXPECT_EQ(transform.rotation.roll, 0.0f);
    EXPECT_EQ(transform.world_coordinates, glm::mat4(1.0f));
  }

  TEST(Transform, WorldLooksDownTheNegativeZAxisWithYUp)
  {
    ExpectVector(Transform::World_Forward(), 0.0f, 0.0f, -1.0f);
    ExpectVector(Transform::World_Up(), 0.0f, 1.0f, 0.0f);
  }

  TEST(Transform, LooksForwardInTheWorldByDefault)
  {
    const Transform transform;

    ExpectVector(transform.Forward(), 0.0f, 0.0f, -1.0f);
    ExpectVector(transform.Right(), 1.0f, 0.0f, 0.0f);
    ExpectVector(transform.Up(), 0.0f, 1.0f, 0.0f);
  }

  TEST(Transform, ForwardFollowsTheYaw)
  {
    ExpectVector(Rotated(0.0f, 90.0f).Forward(), -1.0f, 0.0f, 0.0f);
    ExpectVector(Rotated(0.0f, -90.0f).Forward(), 1.0f, 0.0f, 0.0f);
    ExpectVector(Rotated(0.0f, 180.0f).Forward(), 0.0f, 0.0f, 1.0f);
  }

  TEST(Transform, ForwardFollowsThePitch)
  {
    const float half = std::sqrt(0.5f);

    ExpectVector(Rotated(45.0f, 0.0f).Forward(), 0.0f, half, -half);
    ExpectVector(Rotated(-45.0f, 0.0f).Forward(), 0.0f, -half, -half);
  }

  TEST(Transform, RightFollowsTheYaw)
  {
    ExpectVector(Rotated(0.0f, 90.0f).Right(), 0.0f, 0.0f, -1.0f);
    ExpectVector(Rotated(0.0f, -90.0f).Right(), 0.0f, 0.0f, 1.0f);
  }

  TEST(Transform, RightStaysLevelWhenLookingUpOrDown)
  {
    ExpectVector(Rotated(60.0f, 0.0f).Right(), 1.0f, 0.0f, 0.0f);
    ExpectVector(Rotated(-60.0f, 90.0f).Right(), 0.0f, 0.0f, -1.0f);
  }

  TEST(Transform, ForwardIsReadFromTheWorldCoordinatesAndNotFromTheRotation)
  {
    Transform transform;
    transform.rotation = {.yaw = 90.0f};

    // nothing placed the transform in the world yet
    ExpectVector(transform.Forward(), 0.0f, 0.0f, -1.0f);
  }

  TEST(Transform, ForwardIgnoresWhereTheTransformIs)
  {
    Transform transform;
    transform.world_coordinates = translate(glm::mat4(1.0f), glm::vec3(5.0f, -3.0f, 12.0f));

    ExpectVector(transform.Forward(), 0.0f, 0.0f, -1.0f);
  }

  TEST(Transform, ForwardAndRightHaveLengthOneWhateverTheScale)
  {
    Transform transform = Rotated(20.0f, 35.0f);
    transform.world_coordinates = scale(transform.world_coordinates, glm::vec3(4.0f));

    EXPECT_NEAR(length(transform.Forward()), 1.0f, tolerance);
    EXPECT_NEAR(length(transform.Right()), 1.0f, tolerance);
  }

  // Right() is the cross product of Forward() and Up(). Straight up and
  // straight down they are parallel, the product is zero, and normalizing it
  // divides by zero, so every part of the result is NaN. The spectator never
  // gets there because its pitch is held between -89 and 89 degrees. Anything
  // else that turns a transform can. What Right() should be in that case is a
  // question of design: it would have to come from the rotation and not from
  // the world's up.
  TEST(Transform, DISABLED_RightIsANumberWhenLookingStraightUp)
  {
    const auto right = Rotated(90.0f, 0.0f).Right();

    EXPECT_FALSE(std::isnan(right.x));
    EXPECT_FALSE(std::isnan(right.y));
    EXPECT_FALSE(std::isnan(right.z));
  }
}
