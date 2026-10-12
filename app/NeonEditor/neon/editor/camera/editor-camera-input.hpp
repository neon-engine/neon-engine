#ifndef EDITOR_CAMERA_INPUT_HPP
#define EDITOR_CAMERA_INPUT_HPP

namespace neon
{
  /// What the editor camera is moved by in one frame, worked out by the
  /// viewport from the devices: it is the camera's whole input, so that
  /// the camera knows nothing of keys or windows.
  struct EditorCameraInput
  {
    /// Pixels the mouse moved while the view is turned, to the right and
    /// down.
    float look_x = 0.0f;
    float look_y = 0.0f;

    /// Where to move, from -1 to 1: to the right, forward, and up.
    float move_right = 0.0f;
    float move_forward = 0.0f;
    float move_up = 0.0f;

    /// Whether to move faster.
    bool fast = false;

    /// Notches the wheel turned, up for faster.
    float speed_notches = 0.0f;
  };
} // neon

#endif //EDITOR_CAMERA_INPUT_HPP
