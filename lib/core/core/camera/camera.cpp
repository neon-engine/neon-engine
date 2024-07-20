#include "camera.hpp"

#include "glm/ext/matrix_transform.hpp"

namespace core {
  Camera::Camera() = default;
  Camera::~Camera() = default;
  glm::mat4 Camera::GetView() const { return _view; }
  void Camera::Update(const float &delta_time, const glm::vec3 &move_direction, const glm::vec3 &look_direction)
  {
    _camera_pos += move_direction * delta_time;
    _view = glm::lookAt(_camera_pos, _camera_pos + look_direction, _camera_up);
  }
} // core