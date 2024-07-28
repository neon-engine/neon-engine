#ifndef DATA_TYPES_HPP
#define DATA_TYPES_HPP

#include <string>
#include <glm/glm.hpp>
#include <glm/detail/type_quat.hpp>

namespace core
{
  struct Color
  {
    float r = 0.5;
    float g = 0.5;
    float b = 0.5;
    float a = 1.0f;
  };

  struct MaterialInfo
  {
    Color color;
    std::string shader_path;
    std::string texture_paths[];
  };

  struct Vertex
  {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 uv;
  };

  struct RenderResolution
  {
    RenderResolution(const int width, const int height)
      : width(width),
        height(height) {}

    const int width;
    const int height;
  };

  struct RenderObject
  {
    int mesh_id = -1;
    int material_id = -1;
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
}

#endif //DATA_TYPES_HPP
