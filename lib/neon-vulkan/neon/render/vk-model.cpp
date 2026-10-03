#include "vk-model.hpp"

namespace neon
{
  VK_Model::VK_Model(
    const std::string &path,
    const ModelFit fit,
    FileSystemContext *file_system_context,
    VK_Device *device,
    const std::shared_ptr<Logger> &logger) : Model(path, file_system_context, logger, fit)
  {
    _device = device;
  }

  VK_Model::VK_Model(std::shared_ptr<const MeshData> mesh, VK_Device *device, const std::shared_ptr<Logger> &logger)
    : Model("", nullptr, logger, ModelFit::None)
  {
    _device = device;
    _mesh_data = std::move(mesh);
  }

  bool VK_Model::Initialize()
  {
    if (_mesh_data != nullptr)
    {
      if (_mesh_data->IsEmpty())
      {
        _logger->Error("The mesh that was built holds no triangle");
        return false;
      }

      VK_Mesh vulkan_mesh(_mesh_data->vertices, _mesh_data->indices, {}, _device, _logger);
      if (!vulkan_mesh.Initialize()) { return false; }
      _meshes.push_back(vulkan_mesh);

      // the numbers of a built mesh are meant as they are
      _model_matrix = glm::mat4(1.0f);
      return true;
    }

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

  void VK_Model::Draw(
    const VkCommandBuffer commands,
    const uint32_t instances,
    const uint32_t first_instance,
    const int material) const
  {
    // the meshes stand in the order they were read, which is the order the
    // model knows their materials in
    for (std::size_t i = 0; i < _meshes.size(); i++)
    {
      if (material < 0 || GetMaterialOfMesh(i) == material) { _meshes[i].Draw(commands, instances, first_instance); }
    }
  }

  std::size_t VK_Model::MeshCount(const int material) const
  {
    if (material < 0) { return _meshes.size(); }
    std::size_t count = 0;
    for (std::size_t i = 0; i < _meshes.size(); i++)
    {
      if (GetMaterialOfMesh(i) == material) { count++; }
    }
    return count;
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
    std::vector<const Mesh *> meshes;
    for (const auto &mesh : _meshes) { meshes.push_back(&mesh); }

    _model_matrix = ComputeNormalizationMatrix(meshes, GetFit());
  }

  bool VK_Model::ProcessMesh(aiMesh *mesh, const aiScene *scene, const glm::mat4 &transform)
  {
    std::vector<Vertex> vertices = ReadVertices(mesh);
    std::vector<unsigned int> indices;
    std::vector<TextureInfo> textures;

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
      const aiFace &face = mesh->mFaces[i];
      for (unsigned int j = 0; j < face.mNumIndices; j++) { indices.push_back(face.mIndices[j]); }
    }

    ApplyNodeTransform(transform, vertices, indices);

    const aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    LoadMaterialTextures(scene, material, aiTextureType_DIFFUSE, textures);
    LoadMaterialTextures(scene, material, aiTextureType_SPECULAR, textures);

    VK_Mesh vulkan_mesh(vertices, indices, textures, _device, _logger);
    if (!vulkan_mesh.Initialize()) { return false; }

    _meshes.push_back(vulkan_mesh);
    return true;
  }
} // neon
