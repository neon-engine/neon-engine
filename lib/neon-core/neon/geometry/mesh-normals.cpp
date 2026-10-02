#include "mesh-normals.hpp"

namespace neon
{
  glm::vec3 MeshNormals::AreaNormal(const MeshData &mesh, const std::size_t triangle)
  {
    const glm::vec3 &a = mesh.vertices[mesh.indices[triangle * 3]].position;
    const glm::vec3 &b = mesh.vertices[mesh.indices[triangle * 3 + 1]].position;
    const glm::vec3 &c = mesh.vertices[mesh.indices[triangle * 3 + 2]].position;
    return cross(b - a, c - a);
  }

  glm::vec3 MeshNormals::NormalizedOrUp(const glm::vec3 &vector)
  {
    // a degenerate triangle has no normal of its own; up is as good as any
    return dot(vector, vector) > 0.0f ? normalize(vector) : glm::vec3(0.0f, 1.0f, 0.0f);
  }

  void MeshNormals::ComputeFlat(MeshData &mesh)
  {
    // every triangle gets vertices of its own, so that an edge between two
    // faces stays a hard edge
    std::vector<Vertex> split;
    split.reserve(mesh.indices.size());

    for (std::size_t triangle = 0; triangle < mesh.TriangleCount(); triangle++)
    {
      const glm::vec3 normal = NormalizedOrUp(AreaNormal(mesh, triangle));
      for (std::size_t corner = 0; corner < 3; corner++)
      {
        Vertex vertex = mesh.vertices[mesh.indices[triangle * 3 + corner]];
        vertex.normal = normal;
        split.push_back(vertex);
      }
    }

    mesh.vertices = std::move(split);
    for (std::size_t i = 0; i < mesh.indices.size(); i++) { mesh.indices[i] = static_cast<unsigned int>(i); }
  }

  void MeshNormals::ComputeSmooth(MeshData &mesh)
  {
    for (auto &vertex : mesh.vertices) { vertex.normal = glm::vec3(0.0f); }

    for (std::size_t triangle = 0; triangle < mesh.TriangleCount(); triangle++)
    {
      const glm::vec3 weighted = AreaNormal(mesh, triangle);
      for (std::size_t corner = 0; corner < 3; corner++)
      {
        mesh.vertices[mesh.indices[triangle * 3 + corner]].normal += weighted;
      }
    }

    for (auto &vertex : mesh.vertices) { vertex.normal = NormalizedOrUp(vertex.normal); }
  }
} // neon
