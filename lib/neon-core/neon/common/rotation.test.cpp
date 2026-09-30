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
}
