#ifndef DATA_TYPES_HPP
#define DATA_TYPES_HPP

#include <glm/glm.hpp>

namespace core
{
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

  struct Transform
  {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
  };
}

#endif //DATA_TYPES_HPP
