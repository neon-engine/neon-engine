#ifndef ROTATION_HPP
#define ROTATION_HPP

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

namespace neon
{
  /// An orientation as three angles in degrees. Roll is applied first, then
  /// pitch, then yaw.
  ///
  /// Three angles can name every orientation, so nothing is lost when an
  /// orientation is kept this way. What they cannot do is change smoothly:
  /// where the pitch is 90 degrees up or down, yaw and roll turn around the
  /// same axis, and the angles of two orientations that are close can be far
  /// apart. So angles are a way to write an orientation down. What has to
  /// turn freely, such as a body of the physics, keeps a quaternion and
  /// writes the angles with FromQuaternion().
  struct Rotation
  {
    float pitch{};
    float yaw{};
    float roll{};

    /// The angles of an orientation. GetQuaternion() of the result is the
    /// orientation that was given, or its negative, which is the same
    /// orientation. The pitch lies between -90 and 90 degrees. Where it is
    /// one of the two, the roll is 0 and the yaw holds the whole turn.
    [[nodiscard]] static Rotation FromQuaternion(const glm::quat &quaternion)
    {
      const glm::quat orientation = normalize(quaternion);

      // glm names an element by its column first
      const glm::mat3 matrix = mat3_cast(orientation);

      // the length of what is left of the axes once the pitch is taken
      // away. It stays exact where the sine of the pitch is close to 1,
      // which the arc sine does not
      const float cosine_of_pitch = std::sqrt(matrix[0][1] * matrix[0][1] + matrix[1][1] * matrix[1][1]);
      const float pitch = std::atan2(-matrix[2][1], cosine_of_pitch);

      // straight up and straight down, the yaw holds the whole turn
      if (cosine_of_pitch <= 1e-6f)
      {
        const float turn = std::atan2(-matrix[0][2], matrix[0][0]);
        return {glm::degrees(pitch), glm::degrees(turn), 0.0f};
      }

      const float yaw = std::atan2(matrix[2][0], matrix[2][2]);

      // The roll is what is left once yaw and pitch are taken away. Close to
      // straight up the yaw is found from two small numbers and is off by a
      // little. The roll turns around the same axis there, and found this
      // way it makes up for it.
      const glm::quat yaw_and_pitch =
        angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f)) * angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));
      const glm::quat rest = conjugate(yaw_and_pitch) * orientation;

      float roll = 2.0f * std::atan2(rest.z, rest.w);
      if (roll > glm::pi<float>()) { roll -= glm::two_pi<float>(); }
      if (roll < -glm::pi<float>()) { roll += glm::two_pi<float>(); }

      return {glm::degrees(pitch), glm::degrees(yaw), glm::degrees(roll)};
    }

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
} // neon

#endif //ROTATION_HPP
