#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <glm/glm.hpp>

namespace neon
{
  /// One vertex as every renderer lays it out: the layout the pipelines
  /// bind is this struct, field for field, see docs/models.md.
  struct Vertex
  {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;

    /// The colour painted on the vertex, in linear light, as `COLOR_0` of
    /// glTF. The shaders multiply it into the base colour, so white, the
    /// default, leaves a model without vertex colours as it is. The alpha
    /// is 1 for every vertex of a model file, see docs/models.md.
    glm::vec4 color{1.0f};
  };
} // neon

#endif //VERTEX_HPP
