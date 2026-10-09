#include "mesh-uvs.hpp"

#include <cmath>

#include <glm/glm.hpp>

namespace neon
{
  void ProjectUvs(MeshData &mesh, const float texels_per_meter)
  {
    for (auto &vertex : mesh.vertices)
    {
      const glm::vec3 &p = vertex.position;
      const glm::vec3 n = vertex.normal;
      const float ax = std::fabs(n.x);
      const float ay = std::fabs(n.y);
      const float az = std::fabs(n.z);

      // The two coordinates across the face. The signs keep a texture
      // upright on a wall seen from its front, and north-up on a floor, so
      // that the same texture reads the same way round on every side of a
      // room. Textures are uploaded with their top at v = 0.
      glm::vec2 across;
      if (ay >= ax && ay >= az)
      {
        across = n.y >= 0.0f ? glm::vec2(p.x, p.z) : glm::vec2(p.x, -p.z);
      } else if (ax >= az)
      {
        across = n.x >= 0.0f ? glm::vec2(-p.z, -p.y) : glm::vec2(p.z, -p.y);
      } else
      {
        across = n.z >= 0.0f ? glm::vec2(p.x, -p.y) : glm::vec2(-p.x, -p.y);
      }

      vertex.tex_coords = across * texels_per_meter;
    }
  }
} // neon
