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
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
  };
}

#endif //DATA_TYPES_HPP
