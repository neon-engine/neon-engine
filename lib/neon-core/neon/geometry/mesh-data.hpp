#ifndef MESH_DATA_HPP
#define MESH_DATA_HPP

#include <vector>

#include <neon/render/vertex.hpp>

namespace neon
{
  /// A mesh as numbers: vertices, and three indices for every triangle,
  /// wound anticlockwise seen from outside, as the renderer expects. It is
  /// what the engine makes when geometry is built at run time, in metres,
  /// and what a tool or an importer hands over to be drawn or to collide.
  /// Nothing of it belongs to a graphics card or a file.
  struct MeshData
  {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    [[nodiscard]] bool IsEmpty() const
    {
      return indices.empty();
    }

    [[nodiscard]] std::size_t TriangleCount() const
    {
      return indices.size() / 3;
    }
  };
} // neon

#endif //MESH_DATA_HPP
