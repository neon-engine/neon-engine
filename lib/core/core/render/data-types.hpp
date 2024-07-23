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
    int material_id;
    int mesh_id;
  };

  struct Transform
  {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;
  };
}

#endif //DATA_TYPES_HPP
