#ifndef MESH_HPP
#define MESH_HPP

#include <string>
#include <vector>

#include "vertex.hpp"


namespace core
{
  class Mesh
  {
  protected:
    std::vector<Vertex> _vertices;
    std::vector<unsigned int> _indices;
    std::vector<std::string> _textures;

    ~Mesh() = default;

  public:
    Mesh(const std::vector<Vertex> &vertices,
         const std::vector<unsigned int> &indices,
         const std::vector<std::string> &textures)
    {
      _vertices = vertices;
      _indices = indices;
      _textures = textures;
    }

    virtual void Initialize() = 0;

    virtual void CleanUp() = 0;

    virtual void Draw() = 0;
  };
} // core

#endif //MESH_HPP
