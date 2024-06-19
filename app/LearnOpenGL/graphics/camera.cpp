#include "camera.hpp"

#include <iostream>
#include <glm/ext/matrix_transform.hpp>

Camera::Camera() = default;

Camera::~Camera() = default;

glm::mat4 Camera::GetView() const
{
    return _view;
}

void Camera::Update(const float &delta_time, const glm::vec3 &move_direction)
{
    _camera_pos += move_direction * delta_time;
    _view = lookAt(_camera_pos, _camera_pos + _camera_front, _camera_up);
    std::cout << "camera position: <" << _camera_pos.x << ", " << _camera_pos.y << ", " << _camera_pos.z << ">" << std::endl;
}
