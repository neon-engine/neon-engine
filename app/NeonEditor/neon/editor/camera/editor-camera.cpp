#include "editor-camera.hpp"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <neon/common/transform.hpp>

namespace neon
{
  void EditorCamera::StartAt(const glm::vec3 &position, const Rotation &rotation)
  {
    _position = position;
    _rotation = rotation;
    _rotation.roll = 0.0f;
  }

  glm::vec3 EditorCamera::Forward() const
  {
    // as a Transform turns its forward: by the pitch, then by the yaw
    return normalize(glm::mat3_cast(_rotation.GetQuaternion()) * Transform::World_Forward());
  }

  void EditorCamera::Update(const EditorCameraInput &input, const double delta_time)
  {
    // the wheel changes the speed by a factor a notch, within bounds
    if (input.speed_notches != 0.0f)
    {
      _speed = std::clamp(_speed * std::pow(notch_factor, input.speed_notches), least_speed, most_speed);
    }

    if (input.look_x != 0.0f || input.look_y != 0.0f)
    {
      _rotation.yaw -= input.look_x * look_degrees_per_pixel;
      _rotation.pitch -= input.look_y * look_degrees_per_pixel;
      _rotation.pitch = std::clamp(_rotation.pitch, -89.0f, 89.0f);
    }

    const glm::vec3 forward = Forward();
    const glm::vec3 right = normalize(cross(forward, Transform::World_Up()));
    const glm::vec3 direction = forward * input.move_forward + right * input.move_right +
                                Transform::World_Up() * input.move_up;
    if (direction == glm::vec3{0.0f}) { return; }

    const float speed = _speed * (input.fast ? fast_factor : 1.0f);
    _position += direction * speed * static_cast<float>(delta_time);
  }

  CameraInfo EditorCamera::GetCameraInfo() const
  {
    CameraInfo info;
    info.target = RenderTarget::Window;
    info.fov = 60.0f;
    info.near = 0.1f;
    info.far = 1000.0f;
    info.view = lookAt(_position, _position + Forward(), Transform::World_Up());
    return info;
  }
} // neon
