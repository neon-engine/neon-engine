#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <glm/glm.hpp>

namespace neon
{
  struct Vertex
  {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;
  };
} // neon

#endif //VERTEX_HPP
