#include "vk-model.hpp"

namespace neon
{
  VK_Model::VK_Model(
    const std::string &path,
    FileSystemContext *file_system_context,
    VK_Device *device,
    const std::shared_ptr<Logger> &logger) : Model(path, file_system_context, logger)
  {
    _device = device;
  }

  bool VK_Model::Initialize()
  {
    if (!LoadModel())
    {
      // the load may have stopped half way, with some meshes already uploaded
      CleanUp();
      return false;
    }

    GenerateNormalizationMatrix();
    return true;
  }

  void VK_Model::Use() const
  {
    for (const auto &mesh : _meshes) { mesh.Use(); }
  }

  void VK_Model::CleanUp()
  {
    while (!_meshes.empty())
    {
      _meshes.back().CleanUp();
      _meshes.pop_back();
    }
  }

  void VK_Model::GenerateNormalizationMatrix()
  {
    _logger->Debug("Generating normalized matrix using the loaded vertices");

    std::vector<const Mesh *> meshes;
    for (const auto &mesh : _meshes) { meshes.push_back(&mesh); }

    _model_matrix = ComputeNormalizationMatrix(meshes);
  }

  bool VK_Model::ProcessMesh(aiMesh *mesh, const aiScene *scene)
  {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<TextureInfo> textures;

    vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
      Vertex vertex{};
      vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

      if (mesh->HasNormals())
      {
        vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
      }

      if (mesh->mTextureCoords[0])
      {
        vertex.tex_coords = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
      }

      vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
      const aiFace &face = mesh->mFaces[i];
      for (unsigned int j = 0; j < face.mNumIndices; j++) { indices.push_back(face.mIndices[j]); }
    }

    const aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    LoadMaterialTextures(material, aiTextureType_DIFFUSE, textures);
    LoadMaterialTextures(material, aiTextureType_SPECULAR, textures);

    VK_Mesh vulkan_mesh(vertices, indices, textures, _device, _logger);
    if (!vulkan_mesh.Initialize()) { return false; }

    _meshes.push_back(vulkan_mesh);
    return true;
  }
} // neon
