#include "camera.hpp"

#include <iostream>
#include <glm/ext/matrix_transform.hpp>

Camera::Camera() = default;

Camera::~Camera() = default;

glm::mat4 Camera::GetView() const
{
  return _view;
}

void Camera::Update(
  const float &delta_time,
  const glm::vec3 &move_direction,
  const glm::vec3 &look_direction)
{
  _camera_pos += move_direction * delta_time;
  std::cout << "camera position: " << _camera_pos.x << ", " << _camera_pos.y << ", " << _camera_pos.z << std::endl;
  std::cout << "look direction: " << look_direction.x << ", " << look_direction.y << ", " << look_direction.z << std::endl;
  std::cout << "camera up: " << _camera_up.x << ", " << _camera_up.y << ", " << _camera_up.z << std::endl;
  _view = lookAt(_camera_pos, _camera_pos + look_direction, _camera_up);
}
