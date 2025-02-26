#ifndef MESH_HPP
#define MESH_HPP
#include <vector>

#include "texture.hpp"
#include "vertex.hpp"


namespace core
{
  class Mesh
  {
  protected:
    std::vector<Vertex> _vertices;
    std::vector<unsigned int> _indices;
    std::vector<Texture> _textures;

    ~Mesh() = default;

    virtual void SetupMesh() = 0;

  public:
    Mesh(const std::vector<Vertex> &vertices,
         const std::vector<unsigned int> &indices,
         const std::vector<Texture> &textures)
    {
      _vertices = vertices;
      _indices = indices;
      _textures = textures;
    }

    virtual void Draw() = 0;
  };
} // core

#endif //MESH_HPP
