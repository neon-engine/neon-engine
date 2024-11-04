#ifndef DATA_TYPES_HPP
#define DATA_TYPES_HPP

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>
#include "glm/gtc/quaternion.hpp"

namespace core
{

  struct RenderObjectRef
  {
    int mesh_id = -1;
    int material_id = -1;
  };

  struct Color
  {
    float r = 0.5;
    float g = 0.5;
    float b = 0.5;
    float a = 1.0f;
  };

  struct Vertex
  {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 uv;
  };

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
      const glm::quat yaw_quat   = angleAxis(yaw_rad,   glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw around Y-axis
      const glm::quat pitch_quat = angleAxis(pitch_rad, glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch around X-axis
      const glm::quat roll_quat  = angleAxis(roll_rad,  glm::vec3(0.0f, 0.0f, 1.0f)); // Roll around Z-axis

      const glm::quat orientation = yaw_quat * pitch_quat * roll_quat;

      return orientation;
    }
  };

  struct Transform
  {
    glm::vec3 position{0.0f};
    Rotation rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    [[nodiscard]] glm::vec3 Forward() const { return normalize(rotation.GetQuaternion() * World_Forward()); }
    // ReSharper disable once CppMemberFunctionMayBeStatic
    [[nodiscard]] glm::vec3 Up() const { return World_Up(); } // NOLINT(*-convert-member-functions-to-static)
    [[nodiscard]] glm::vec3 Right() const { return normalize(cross(Forward(), Up())); }
    static glm::vec3 World_Forward() { return {0.0f, 0.0f, -1.0f}; }
    static glm::vec3 World_Up() { return { 0.0f, 1.0f, 0.0f }; }
  };

  struct RenderInfo
  {
    std::string model_path;
    std::string shader_path;
    std::vector<std::string> texture_paths;
    Color color;
  };

  enum class RenderTarget
  {
    Window,
    Texture
  };

  struct CameraInfo
  {
    RenderTarget target{RenderTarget::Window};
    float fov{45.0f};
    glm::mat4 view{1.0f};
    float near{0.1f};
    float far{1000.f};
  };
}

#endif //DATA_TYPES_HPP
