#include "editor-camera.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
  using neon::EditorCamera;
  using neon::EditorCameraInput;
  using neon::Rotation;

  TEST(EditorCamera, StartsWhereItIsPutAndLooksAlongMinusZ)
  {
    EditorCamera camera;
    camera.StartAt({1.0f, 2.0f, 3.0f}, Rotation{0.0f, 0.0f, 30.0f});

    EXPECT_EQ(camera.GetPosition(), glm::vec3(1.0f, 2.0f, 3.0f));
    // the roll of the scene's camera is not taken, the editor's is level
    EXPECT_EQ(camera.GetRotation().roll, 0.0f);
    EXPECT_NEAR(camera.Forward().z, -1.0f, 1e-5f);
  }

  TEST(EditorCamera, MovesForwardAndToTheRightAtItsSpeed)
  {
    EditorCamera camera;
    camera.StartAt({0.0f, 0.0f, 0.0f}, Rotation{});

    camera.Update({.move_forward = 1.0f}, 0.5);
    EXPECT_NEAR(camera.GetPosition().z, -camera.GetSpeed() * 0.5f, 1e-4f);

    camera.StartAt({0.0f, 0.0f, 0.0f}, Rotation{});
    camera.Update({.move_right = 1.0f, .move_up = 1.0f}, 1.0);
    EXPECT_NEAR(camera.GetPosition().x, camera.GetSpeed(), 1e-4f);
    EXPECT_NEAR(camera.GetPosition().y, camera.GetSpeed(), 1e-4f);
  }

  TEST(EditorCamera, MovesFasterWhileFastIsHeld)
  {
    EditorCamera camera;
    camera.StartAt({0.0f, 0.0f, 0.0f}, Rotation{});

    camera.Update({.move_forward = 1.0f, .fast = true}, 1.0);

    EXPECT_NEAR(camera.GetPosition().z, -camera.GetSpeed() * EditorCamera::fast_factor, 1e-3f);
  }

  TEST(EditorCamera, TheMouseTurnsTheViewAndThePitchStopsShortOfStraightUp)
  {
    EditorCamera camera;
    camera.StartAt({0.0f, 0.0f, 0.0f}, Rotation{});

    // to the right: the yaw goes down, and forward turns to +x
    camera.Update({.look_x = 90.0f / EditorCamera::look_degrees_per_pixel}, 0.016);
    EXPECT_NEAR(camera.GetRotation().yaw, -90.0f, 1e-3f);
    EXPECT_NEAR(camera.Forward().x, 1.0f, 1e-4f);

    // up, far past the top
    camera.Update({.look_y = -10000.0f}, 0.016);
    EXPECT_EQ(camera.GetRotation().pitch, 89.0f);
  }

  TEST(EditorCamera, TheWheelChangesTheSpeedWithinBounds)
  {
    EditorCamera camera;
    const float before = camera.GetSpeed();

    camera.Update({.speed_notches = 1.0f}, 0.016);
    EXPECT_NEAR(camera.GetSpeed(), before * EditorCamera::notch_factor, 1e-4f);

    camera.Update({.speed_notches = -100.0f}, 0.016);
    EXPECT_EQ(camera.GetSpeed(), EditorCamera::least_speed);

    camera.Update({.speed_notches = 1000.0f}, 0.016);
    EXPECT_EQ(camera.GetSpeed(), EditorCamera::most_speed);
  }

  TEST(EditorCamera, DrawsIntoTheWindowFromWhereItIs)
  {
    EditorCamera camera;
    camera.StartAt({0.0f, 2.0f, 5.0f}, Rotation{});

    const neon::CameraInfo info = camera.GetCameraInfo();
    EXPECT_EQ(info.target, neon::RenderTarget::Window);
    // the view takes the camera's place to the origin
    const glm::vec4 origin = info.view * glm::vec4(0.0f, 2.0f, 5.0f, 1.0f);
    EXPECT_NEAR(origin.x, 0.0f, 1e-5f);
    EXPECT_NEAR(origin.y, 0.0f, 1e-5f);
    EXPECT_NEAR(origin.z, 0.0f, 1e-5f);
  }
}
