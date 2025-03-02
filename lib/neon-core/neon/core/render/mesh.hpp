#ifndef MESH_HPP
#define MESH_HPP

#include <memory>
#include <string>
#include <vector>
#include <neon/core/logging/logger.hpp>

#include "texture-info.hpp"
#include "vertex.hpp"

namespace core
{
  class Mesh
  {
  protected:
    std::vector<Vertex> _vertices;
    std::vector<unsigned int> _indices;
    std::vector<TextureInfo> _textures;
    std::shared_ptr<Logger> _logger;

    bool _initialized = false;

    ~Mesh() = default;

  public:
    Mesh(const std::vector<Vertex> &vertices,
         const std::vector<unsigned int> &indices,
         const std::vector<TextureInfo> &textures,
         const std::shared_ptr<Logger> &logger)
    {
      _vertices = vertices;
      _indices = indices;
      _textures = textures;
      _logger = logger;
    }

    [[nodiscard]] const std::vector<TextureInfo>& GetTextures() const
    {
      return _textures;
    }

    [[nodiscard]] const std::vector<Vertex>& GetVertices() const
    {
      return _vertices;
    }

    virtual bool Initialize() = 0;

    virtual void CleanUp() = 0;

    virtual void Use() const = 0;
  };
} // core

#endif //MESH_HPP
