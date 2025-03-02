#ifndef MODEL_HPP
#define MODEL_HPP

#include <assimp/scene.h>
#include <neon/core/logging/logger.hpp>

#include "mesh.hpp"
#include "texture.hpp"

namespace core
{
  class Model
  {
    std::string _path;

    bool ProcessNode(aiNode *root, const aiScene *scene);

  protected:
    std::shared_ptr<Logger> _logger;

    bool LoadModel();

    void LoadMaterialTextures(
      const aiMaterial *material,
      const aiTextureType &type,
      std::vector<Texture> textures) const;

    virtual bool Initialize() = 0;

    virtual void Use() = 0;

    virtual void CleanUp() = 0;

    virtual bool ProcessMesh(aiMesh *mesh, const aiScene *scene) = 0;

    ~Model() = default;

  public:
    Model(const std::string &path, const std::shared_ptr<Logger> &logger);
  };
} // core

#endif //MODEL_HPP
