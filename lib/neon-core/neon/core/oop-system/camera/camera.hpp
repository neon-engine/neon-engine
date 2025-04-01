#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>

namespace neon
{
  class Camera
  {
    glm::vec3 _camera_pos = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 _camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 _look_direction = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::mat4 _view = glm::mat4(1.0f);
    glm::mat4 _projection = glm::mat4(1.0f);
    float _fov = 45.0f;

  public:
    Camera();

    [[nodiscard]] const glm::mat4& GetView() const;

    [[nodiscard]] glm::mat4 GetProjection(int width, int height) const;

    [[nodiscard]] float GetFov() const;

    void Update(const glm::mat4 &parent_matrix);

    [[nodiscard]] glm::vec3 GetLookDirection() const;

    void SetLookDirection(const glm::vec3 &direction);
  };
} // neon

#endif //CAMERA_HPP
