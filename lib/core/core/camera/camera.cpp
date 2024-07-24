#include "camera.hpp"

#include <glm/ext/matrix_transform.hpp>
#include "util.hpp"

namespace core
{
  Camera::Camera() = default;

  glm::mat4 Camera::GetView() const { return _view; }

  // ReSharper disable once CppDFAConstantFunctionResult
  float Camera::GetFov() const
  {
    return _fov;
  }

  void Camera::Update(const float &delta_time, const glm::vec3 &move_direction)
  {
    _camera_pos += move_direction * delta_time;
    _view = lookAt(_camera_pos, _camera_pos + _look_direction, _camera_up);
  }

  glm::vec3 Camera::GetLookDirection() const
  {
    return _look_direction;
  }
} // core
