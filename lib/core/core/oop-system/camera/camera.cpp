#include "camera.hpp"

#include <glm/ext/matrix_transform.hpp>
#include "util.hpp"

namespace core
{
  Camera::Camera() = default;

  const glm::mat4& Camera::GetView() const { return _view; }

  // ReSharper disable once CppDFAConstantFunctionResult
  float Camera::GetFov() const
  {
    return _fov;
  }

  void Camera::Update(const glm::mat4 &parent_matrix)
  {
    _camera_pos = parent_matrix[3];
    _look_direction = normalize(-parent_matrix[2]);
    _view = lookAt(_camera_pos, _camera_pos + _look_direction, _camera_up);
  }

  glm::vec3 Camera::GetLookDirection() const
  {
    return _look_direction;
  }
} // core
