#ifndef ROTATION_HPP
#define ROTATION_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace core
{
  struct Rotation
  {
    float pitch{};
    float yaw{};
    float roll{};

    [[nodiscard]] glm::vec3 GetEulerAngle() const
    {
      return eulerAngles(GetQuaternion());
    }

    [[nodiscard]] glm::quat GetQuaternion() const
    {
      const float yaw_rad = glm::radians(yaw);
      const float pitch_rad = glm::radians(pitch);
      const float roll_rad = glm::radians(roll);
      const glm::quat yaw_quat = angleAxis(yaw_rad, glm::vec3(0.0f, 1.0f, 0.0f));     // Yaw around Y-axis
      const glm::quat pitch_quat = angleAxis(pitch_rad, glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch around X-axis
      const glm::quat roll_quat = angleAxis(roll_rad, glm::vec3(0.0f, 0.0f, 1.0f));   // Roll around Z-axis

      const glm::quat orientation = yaw_quat * pitch_quat * roll_quat;

      return orientation;
    }
  };
} // core

#endif //ROTATION_HPP
