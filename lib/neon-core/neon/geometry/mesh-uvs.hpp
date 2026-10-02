#ifndef MESH_UVS_HPP
#define MESH_UVS_HPP

#include "mesh-data.hpp"

namespace neon
{
  /// Gives every vertex texture coordinates by projecting it along the axis
  /// its normal points most along: a wall facing z takes its x and y, a
  /// floor its x and z. One metre of surface is `texels_per_metre` repeats
  /// of the texture, so that a texture looks the same on a wall of two
  /// metres and one of twenty, which is what a blocked-out level wants.
  /// Textures stay upright on walls and north-up on floors.
  void ProjectUvs(MeshData &mesh, float texels_per_metre);
} // neon

#endif //MESH_UVS_HPP
