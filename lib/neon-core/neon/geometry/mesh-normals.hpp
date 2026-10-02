#ifndef MESH_NORMALS_HPP
#define MESH_NORMALS_HPP

#include "mesh-data.hpp"

namespace neon
{
  /// Gives every vertex the normal of the one triangle it belongs to. A
  /// vertex that several triangles share is split first, so that every
  /// triangle owns its three vertices and every edge is a hard edge. For
  /// walls, boxes, and anything built from flat faces.
  void ComputeFlatNormals(MeshData &mesh);

  /// Gives every vertex the normal of the triangles that share it, each
  /// weighted by its area, so that a curved surface is lit as one. Vertices
  /// are shared as they are; nothing is split or joined. For terrain, a
  /// bent plane, and anything meant to look round.
  void ComputeSmoothNormals(MeshData &mesh);
} // neon

#endif //MESH_NORMALS_HPP
