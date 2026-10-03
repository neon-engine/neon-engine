#ifndef TUBE_HPP
#define TUBE_HPP

#include <vector>

#include <glm/glm.hpp>

#include "mesh-data.hpp"

namespace neon
{
  /// Adds a tube to `mesh`: a circle of `radius` with `sides` faces, swept
  /// along the line through `centres` and closed with a flat cap at each
  /// end. It is what a curve is drawn as, a rope or a pipe, once the curve
  /// is flattened to points, see docs/curves.md.
  ///
  /// The side shares its vertices and carries the normals of the round
  /// surface. A texture goes round the tube and along it, `texels_per_metre`
  /// times a metre both ways. A tube through as many points with as many
  /// sides always has as many vertices and triangles, in the same order,
  /// so one that moves is written over the one before. Nothing is added
  /// for fewer than two points.
  void AppendTube(
    const std::vector<glm::vec3> &centres,
    float radius,
    int sides,
    float texels_per_metre,
    MeshData &mesh);
} // neon

#endif //TUBE_HPP
