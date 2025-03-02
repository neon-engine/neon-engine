#include "opengl-model.hpp"

#include "opengl-mesh.hpp"

namespace core
{
  OpenGL_Model::OpenGL_Model(
    const std::string &path,
    const std::shared_ptr<Logger> &logger): Model(path, logger) {}

  bool OpenGL_Model::Initialize()
  {
    if (!LoadModel())
    {
      return false;
    }

    return true;
  }

  void OpenGL_Model::Use()
  {
    for (auto &mesh : _meshes)
    {
      mesh.Use();
    }
  }

  void OpenGL_Model::CleanUp()
  {
    // the Model load may have been interrupted so some meshes may have been initialized
    while (!_meshes.empty())
    {
      _meshes.back().CleanUp();
      _meshes.pop_back();
    }
  }

  bool OpenGL_Model::ProcessMesh(aiMesh *mesh, const aiScene *scene)
  {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
      Vertex vertex{};
      glm::vec3 vector;
      vector.x = mesh->mVertices[i].x;
      vector.y = mesh->mVertices[i].y;
      vector.z = mesh->mVertices[i].z;
      vertex.position = vector;

      vector.x = mesh->mNormals[i].x;
      vector.y = mesh->mNormals[i].y;
      vector.z = mesh->mNormals[i].z;
      vertex.normal = vector;

      if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
      {
        glm::vec2 vec;
        vec.x = mesh->mTextureCoords[0][i].x;
        vec.y = mesh->mTextureCoords[0][i].y;
        vertex.tex_coords = vec;
      }
      else
      {
        vertex.tex_coords = glm::vec2(0.0f, 0.0f);
      }
    }

    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
      aiFace face = mesh->mFaces[i];
      for(unsigned int j = 0; j < face.mNumIndices; j++)
      {
        indices.push_back(face.mIndices[j]);
      }
    }

    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    LoadMaterialTextures(material, aiTextureType_DIFFUSE, textures);
    LoadMaterialTextures(material, aiTextureType_SPECULAR, textures);

    OpenGL_Mesh opengl_mesh(vertices, indices, textures, _logger);
    opengl_mesh.Initialize();
    _meshes.push_back(opengl_mesh);

    return true;
  }
} // core
