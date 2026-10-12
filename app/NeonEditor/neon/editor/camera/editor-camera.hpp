#ifndef EDITOR_CAMERA_HPP
#define EDITOR_CAMERA_HPP

#include <glm/glm.hpp>
#include <neon/common/rotation.hpp>
#include <neon/render/camera-info.hpp>

#include "editor-camera-input.hpp"

namespace neon
{
  /// The camera the viewport looks through: the editor's own, which flies
  /// through the scene as the engine's Spectator does, and is no entity of
  /// the scene, so that saving the scene does not carry it. One for every
  /// viewport (#571). It starts where the scene's camera is, see
  /// EditorCameraSubmission, and the scene's camera stays what the game
  /// plays with.
  class EditorCamera final
  {
    glm::vec3 _position{0.0f, 2.0f, 5.0f};
    Rotation _rotation{};

    // units a second, and what the wheel multiplies it by for each notch
    float _speed = 5.0f;

  public:
    /// How far the speed goes, in units a second, and by how much a notch
    /// of the wheel changes it.
    static constexpr float least_speed = 0.25f;
    static constexpr float most_speed = 200.0f;
    static constexpr float notch_factor = 1.25f;

    /// How much faster the camera moves while `fast` is held.
    static constexpr float fast_factor = 4.0f;

    /// Degrees the view turns for each pixel the mouse moved.
    static constexpr float look_degrees_per_pixel = 0.15f;

    /// Places the camera, as at the scene's camera when the editor opens.
    void StartAt(const glm::vec3 &position, const Rotation &rotation);

    /// Moves and turns the camera by the input of a frame of `delta_time`
    /// seconds.
    void Update(const EditorCameraInput &input, double delta_time);

    [[nodiscard]] const glm::vec3 &GetPosition() const { return _position; }

    [[nodiscard]] const Rotation &GetRotation() const { return _rotation; }

    [[nodiscard]] float GetSpeed() const { return _speed; }

    /// Where the camera looks, as a unit vector.
    [[nodiscard]] glm::vec3 Forward() const;

    /// The camera as the pipeline draws with it: into the window, which
    /// the viewport turns into its target.
    [[nodiscard]] CameraInfo GetCameraInfo() const;
  };
} // neon

#endif //EDITOR_CAMERA_HPP
