#include "rotation.hpp"

#include <cmath>

#include <gtest/gtest.h>

namespace
{
  using neon::Rotation;

  constexpr float tolerance = 1e-5f;

  // half of a right angle, which is what a quaternion of 90 degrees holds
  const float half = std::sqrt(0.5f);

  void ExpectQuaternion(const glm::quat &actual, const float w, const float x, const float y, const float z)
  {
    EXPECT_NEAR(actual.w, w, tolerance);
    EXPECT_NEAR(actual.x, x, tolerance);
    EXPECT_NEAR(actual.y, y, tolerance);
    EXPECT_NEAR(actual.z, z, tolerance);
  }

  void ExpectVector(const glm::vec3 &actual, const float x, const float y, const float z)
  {
    EXPECT_NEAR(actual.x, x, tolerance);
    EXPECT_NEAR(actual.y, y, tolerance);
    EXPECT_NEAR(actual.z, z, tolerance);
  }

  TEST(Rotation, IsNoRotationByDefault)
  {
    ExpectQuaternion(Rotation{}.GetQuaternion(), 1.0f, 0.0f, 0.0f, 0.0f);
  }

  TEST(Rotation, PitchTurnsAroundTheXAxis)
  {
    ExpectQuaternion(Rotation{.pitch = 90.0f}.GetQuaternion(), half, half, 0.0f, 0.0f);
  }

  TEST(Rotation, YawTurnsAroundTheYAxis)
  {
    ExpectQuaternion(Rotation{.yaw = 90.0f}.GetQuaternion(), half, 0.0f, half, 0.0f);
  }

  TEST(Rotation, RollTurnsAroundTheZAxis)
  {
    ExpectQuaternion(Rotation{.roll = 90.0f}.GetQuaternion(), half, 0.0f, 0.0f, half);
  }

  TEST(Rotation, AHalfTurnHasNoRealPart)
  {
    ExpectQuaternion(Rotation{.yaw = 180.0f}.GetQuaternion(), 0.0f, 0.0f, 1.0f, 0.0f);
  }

  TEST(Rotation, ANegativeAngleTurnsTheOtherWay)
  {
    ExpectQuaternion(Rotation{.yaw = -90.0f}.GetQuaternion(), half, 0.0f, -half, 0.0f);
  }

  TEST(Rotation, QuaternionHasLengthOne)
  {
    const auto quaternion = Rotation{.pitch = 33.0f, .yaw = 127.0f, .roll = -71.0f}.GetQuaternion();

    EXPECT_NEAR(length(quaternion), 1.0f, tolerance);
  }

  TEST(Rotation, YawTurnsForwardToTheLeft)
  {
    const auto turned = Rotation{.yaw = 90.0f}.GetQuaternion() * glm::vec3(0.0f, 0.0f, -1.0f);

    ExpectVector(turned, -1.0f, 0.0f, 0.0f);
  }

  TEST(Rotation, PitchTurnsForwardUp)
  {
    const auto turned = Rotation{.pitch = 90.0f}.GetQuaternion() * glm::vec3(0.0f, 0.0f, -1.0f);

    ExpectVector(turned, 0.0f, 1.0f, 0.0f);
  }

  TEST(Rotation, RollTurnsRightUp)
  {
    const auto turned = Rotation{.roll = 90.0f}.GetQuaternion() * glm::vec3(1.0f, 0.0f, 0.0f);

    ExpectVector(turned, 0.0f, 1.0f, 0.0f);
  }

  TEST(Rotation, AppliesRollFirstThenPitchThenYaw)
  {
    const auto quaternion = Rotation{.pitch = 90.0f, .yaw = 90.0f, .roll = 90.0f}.GetQuaternion();

    // right is rolled up, pitched to the back, and the yaw turns that to the right
    ExpectVector(quaternion * glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 0.0f, 0.0f);
    // forward is left alone by the roll, pitched up, and left alone by the yaw
    ExpectVector(quaternion * glm::vec3(0.0f, 0.0f, -1.0f), 0.0f, 1.0f, 0.0f);
  }

  TEST(Rotation, EulerAnglesAreInRadians)
  {
    ExpectVector(Rotation{.pitch = 30.0f}.GetEulerAngle(), glm::radians(30.0f), 0.0f, 0.0f);
    ExpectVector(Rotation{.yaw = 45.0f}.GetEulerAngle(), 0.0f, glm::radians(45.0f), 0.0f);
    ExpectVector(Rotation{.roll = 60.0f}.GetEulerAngle(), 0.0f, 0.0f, glm::radians(60.0f));
  }

  TEST(Rotation, EulerAnglesAreZeroByDefault)
  {
    ExpectVector(Rotation{}.GetEulerAngle(), 0.0f, 0.0f, 0.0f);
  }

  // from a quaternion

  /// Whether two quaternions are the same orientation. A quaternion and its
  /// negative are.
  void ExpectSameOrientation(const glm::quat &actual, const glm::quat &expected)
  {
    EXPECT_NEAR(std::abs(dot(actual, expected)), 1.0f, tolerance)
      << "(" << actual.w << ", " << actual.x << ", " << actual.y << ", " << actual.z << ") is not ("
      << expected.w << ", " << expected.x << ", " << expected.y << ", " << expected.z << ")";
  }

  TEST(Rotation, NoRotationHasNoAngles)
  {
    const auto rotation = Rotation::FromQuaternion(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    ExpectVector({rotation.pitch, rotation.yaw, rotation.roll}, 0.0f, 0.0f, 0.0f);
  }

  TEST(Rotation, FindsEachAngleByItself)
  {
    const auto pitched = Rotation::FromQuaternion(Rotation{.pitch = 30.0f}.GetQuaternion());
    const auto yawed = Rotation::FromQuaternion(Rotation{.yaw = -120.0f}.GetQuaternion());
    const auto rolled = Rotation::FromQuaternion(Rotation{.roll = 170.0f}.GetQuaternion());

    constexpr float degrees = 1e-3f;
    EXPECT_NEAR(pitched.pitch, 30.0f, degrees);
    EXPECT_NEAR(pitched.yaw, 0.0f, degrees);
    EXPECT_NEAR(pitched.roll, 0.0f, degrees);
    EXPECT_NEAR(yawed.pitch, 0.0f, degrees);
    EXPECT_NEAR(yawed.yaw, -120.0f, degrees);
    EXPECT_NEAR(yawed.roll, 0.0f, degrees);
    EXPECT_NEAR(rolled.pitch, 0.0f, degrees);
    EXPECT_NEAR(rolled.yaw, 0.0f, degrees);
    EXPECT_NEAR(rolled.roll, 170.0f, degrees);
  }

  TEST(Rotation, FindsTheAnglesItWasMadeFrom)
  {
    const auto rotation = Rotation::FromQuaternion(
      Rotation{.pitch = 33.0f, .yaw = 127.0f, .roll = -71.0f}.GetQuaternion());

    constexpr float degrees = 1e-3f;
    EXPECT_NEAR(rotation.pitch, 33.0f, degrees);
    EXPECT_NEAR(rotation.yaw, 127.0f, degrees);
    EXPECT_NEAR(rotation.roll, -71.0f, degrees);
  }

  TEST(Rotation, GivesBackEveryOrientation)
  {
    // every 15 degrees around each axis, which passes through straight up
    // and straight down
    for (float pitch = -180.0f; pitch <= 180.0f; pitch += 15.0f)
    {
      for (float yaw = -180.0f; yaw <= 180.0f; yaw += 15.0f)
      {
        for (float roll = -180.0f; roll <= 180.0f; roll += 15.0f)
        {
          const auto expected = Rotation{.pitch = pitch, .yaw = yaw, .roll = roll}.GetQuaternion();

          const auto rotation = Rotation::FromQuaternion(expected);

          ExpectSameOrientation(rotation.GetQuaternion(), expected);
          EXPECT_GE(rotation.pitch, -90.0f);
          EXPECT_LE(rotation.pitch, 90.0f);
        }
      }
    }
  }

  TEST(Rotation, GivesBackAnOrientationCloseToStraightUp)
  {
    for (const float pitch : {89.9f, 89.999f, 90.0f, -89.9f, -89.999f, -90.0f})
    {
      const auto expected = Rotation{.pitch = pitch, .yaw = 40.0f, .roll = 25.0f}.GetQuaternion();

      const auto rotation = Rotation::FromQuaternion(expected);

      ExpectSameOrientation(rotation.GetQuaternion(), expected);
    }
  }

  TEST(Rotation, GivesBackAnOrientationThatTumbles)
  {
    // what a body does that spins around an axis of its own
    const auto axis = normalize(glm::vec3(1.0f, 2.0f, 3.0f));
    auto orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    for (int step = 0; step < 1000; step++)
    {
      orientation = normalize(angleAxis(glm::radians(1.7f), axis) * orientation);

      ExpectSameOrientation(Rotation::FromQuaternion(orientation).GetQuaternion(), orientation);
    }
  }

  TEST(Rotation, LeavesTheRollAtZeroStraightUp)
  {
    const auto rotation = Rotation::FromQuaternion(Rotation{.pitch = 90.0f, .yaw = 40.0f, .roll = 25.0f}.GetQuaternion());

    EXPECT_NEAR(rotation.pitch, 90.0f, 1e-3f);
    EXPECT_EQ(rotation.roll, 0.0f);
  }

  TEST(Rotation, TakesAQuaternionOfAnyLength)
  {
    const auto expected = Rotation{.pitch = 10.0f, .yaw = 20.0f, .roll = 30.0f}.GetQuaternion();

    const auto rotation = Rotation::FromQuaternion(expected * 3.0f);

    ExpectSameOrientation(rotation.GetQuaternion(), expected);
  }
}
