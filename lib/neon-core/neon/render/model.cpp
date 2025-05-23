#include "model.hpp"

#include <stack>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace neon
{
  Model::Model(const std::string &path, const std::shared_ptr<Logger> &logger)
  {
    _path = path;
    _logger = logger;
  }

  bool Model::LoadModel()
  {
    Assimp::Importer import;
    const auto *scene = import.ReadFile(_path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
    {
      auto error_message = std::string(import.GetErrorString());
      _logger->Error("Error importing model {}: {}", _path, error_message);
      return false;
    }

    return ProcessNode(scene->mRootNode, scene);
  }

  void Model::LoadMaterialTextures(
    const aiMaterial *material,
    const aiTextureType &type,
    std::vector<TextureInfo> textures) const
  {
    for(unsigned int i = 0; i < material->GetTextureCount(type); i++)
    {
      aiString str;
      material->GetTexture(type, i, &str);
      TextureInfo texture;
      texture.path = str.C_Str();
      switch (type)
      {
        case aiTextureType_DIFFUSE:
        {
          texture.texture_type = TextureType::Diffuse;
          break;
        }
        case aiTextureType_SPECULAR:
        {
          texture.texture_type = TextureType::Specular;
          break;
        }
        default:
        {
          auto type_index = static_cast<int>(type);
          _logger->Warn("Unsupported texture type {} at index {}", type_index, i);
          continue;
        }
      }
      textures.push_back(texture);
    }
  }

  bool Model::ProcessNode(aiNode *root, const aiScene *scene)
  {
    std::stack<aiNode *> node_stack;
    node_stack.push(root);

    while (!node_stack.empty()) {
      const aiNode *node = node_stack.top();
      node_stack.pop();

      if (node == nullptr) { continue; }

      // Process all meshes in this node
      for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        if (aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; !ProcessMesh(mesh, scene)) {
          return false;
        }
      }

      // Push all children onto the stack for later processing
      for (unsigned int i = 0; i < node->mNumChildren; i++) {
        node_stack.push(node->mChildren[i]);
      }
    }

    return true;
  }

  glm::mat4 Model::GetNormalizedModelMatrix() const
  {
    return _model_matrix;
  }
} // neon
