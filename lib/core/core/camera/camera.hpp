#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>

namespace core
{
  class Camera
  {
    glm::vec3 _camera_pos = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 _camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 _view = glm::mat4(1.0f);

  public:
    Camera();

    ~Camera();

    [[nodiscard]] glm::mat4 GetView() const;

    void Update(
      const float &delta_time,
      const glm::vec3 &move_direction,
      const glm::vec3 &look_direction);
  };
} // core

#endif //CAMERA_HPP
