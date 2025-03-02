#ifndef MESH_HPP
#define MESH_HPP

#include <memory>
#include <string>
#include <vector>
#include <neon/core/logging/logger.hpp>

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
    glm::mat4 _model_matrix{1.0f};
    std::shared_ptr<Logger> _logger;

    bool _initialized = false;

    ~Mesh() = default;

  public:
    Mesh(const std::vector<Vertex> &vertices,
         const std::vector<unsigned int> &indices,
         const std::vector<Texture> &textures,
         const std::shared_ptr<Logger> &logger)
    {
      _vertices = vertices;
      _indices = indices;
      _textures = textures;
    }

    [[nodiscard]] const std::vector<Texture>& GetTextures() const;

    virtual bool Initialize() = 0;

    virtual void CleanUp() = 0;

    virtual void Use() = 0;

    [[nodiscard]] glm::mat4 GetModelMatrix() const;

  protected:
    void GenerateNormalizationMatrix();
  };
} // core

#endif //MESH_HPP
