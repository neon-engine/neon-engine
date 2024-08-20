#ifndef DATA_TYPES_HPP
#define DATA_TYPES_HPP

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>

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
    float yaw{};
    float pitch{};
    float roll{};

    [[nodiscard]] glm::vec3 GetEulerAngle() const
    {
      return {yaw, pitch, roll};
    }

    [[nodiscard]] glm::quat GetQuaternion() const
    {
      return {GetEulerAngle()};
    }
  };

  struct Transform
  {
    glm::vec3 position{0.0f};
    Rotation rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
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
