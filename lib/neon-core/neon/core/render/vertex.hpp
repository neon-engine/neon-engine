#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <glm/glm.hpp>

namespace core
{
  struct Vertex
  {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 uv;
  };
} // core

#endif //VERTEX_HPP
