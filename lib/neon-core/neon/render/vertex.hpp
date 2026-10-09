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

    /// The color painted on the vertex, in linear light, as `COLOR_0` of
    /// glTF. The shaders multiply it into the base color, so white, the
    /// default, leaves a model without vertex colors as it is. The alpha
    /// is 1 for every vertex of a model file, see docs/models.md.
    glm::vec4 color{1.0f};

    /// Where in the lightmap of its material the vertex is, from 0 to 1: a
    /// second set of coordinates, apart from those of the textures, since
    /// light that was worked out ahead lies over a surface once, where a
    /// texture repeats. Not read by a material without a lightmap.
    glm::vec2 lightmap_coords{0.0f};
  };
} // neon

#endif //VERTEX_HPP
