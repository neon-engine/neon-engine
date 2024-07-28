#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP
#include "data-types.hpp"

namespace core
{
  class Controller
  {
  protected:
    Transform _transform{};
    glm::vec3 _forward{0.0f, 0.0f, -1.0f};
    glm::vec3 _move_direction{0, 0, 0};
    glm::vec3 _up{0.0f, 1.0f, 0.0f};
    float _move_speed = 0;
    float _look_speed = 0;

    // consider using vector 3 rotation for this
    float _yaw = -90.0;
    float _pitch = 0.0;
    ~Controller() = default;

  public:
    virtual void Update(double delta_time, const glm::mat4 &parent_matrix) = 0;
  };
} // core

#endif //CONTROLLER_HPP
