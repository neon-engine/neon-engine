#ifndef MESH_NORMALS_HPP
#define MESH_NORMALS_HPP

#include <glm/glm.hpp>

#include "mesh-data.hpp"

namespace neon
{
  /// Works out the normals of a mesh, flat or smooth.
  class MeshNormals
  {
    /// The normal of a triangle, with the length of twice its area, which
    /// is what weights a smooth normal by the triangle's size.
    static glm::vec3 AreaNormal(const MeshData &mesh, std::size_t triangle);

    static glm::vec3 NormalizedOrUp(const glm::vec3 &vector);

  public:
    /// Gives every vertex the normal of the one triangle it belongs to. A
    /// vertex that several triangles share is split first, so that every
    /// triangle owns its three vertices and every edge is a hard edge. For
    /// walls, boxes, and anything built from flat faces.
    static void ComputeFlat(MeshData &mesh);

    /// Gives every vertex the normal of the triangles that share it, each
    /// weighted by its area, so that a curved surface is lit as one. Vertices
    /// are shared as they are; nothing is split or joined. For terrain, a
    /// bent plane, and anything meant to look round.
    static void ComputeSmooth(MeshData &mesh);
  };
} // neon

#endif //MESH_NORMALS_HPP
