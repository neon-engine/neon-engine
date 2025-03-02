#include "opengl-model.hpp"

#include "opengl-mesh.hpp"
#include <limits>
#include <glm/gtc/matrix_transform.hpp>

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
    GenerateNormalizationMatrix();
    return true;
  }

  void OpenGL_Model::Use() const
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

  void OpenGL_Model::GenerateNormalizationMatrix()
  {
    _logger->Debug("Generating normalized matrix using the loaded vertices");
    constexpr auto max_float_limit = std::numeric_limits<float>::max();
    constexpr auto min_float_limit = std::numeric_limits<float>::min();

    auto min_x = max_float_limit;
    auto min_y = max_float_limit;
    auto min_z = max_float_limit;

    auto max_x = min_float_limit;
    auto max_y = min_float_limit;
    auto max_z = min_float_limit;

    for (const auto &mesh : _meshes)
    {
      for (const auto vertices = mesh.GetVertices(); const auto &vertex : vertices) {
        const auto x = vertex.position.x;
        const auto y = vertex.position.y;
        const auto z = vertex.position.z;

        if (x < min_x) {min_x = x;}
        if (y < min_y) {min_y = y;}
        if (z < min_z) {min_z = z;}

        if (x > max_x) {max_x = x;}
        if (y > max_y) {max_y = y;}
        if (z > max_z) {max_z = z;}
      }
    }

    const auto x_center = (min_x + max_x) / 2.0f;
    const auto y_center = (min_y + max_y) / 2.0f;
    const auto z_center = (min_z + max_z) / 2.0f;

    const auto range_x = max_x - min_x;
    const auto range_y = max_y - min_y;
    const auto range_z = max_z - min_z;

    const auto max_range = std::max(std::max(range_x, range_y), range_z);

    const auto translation = translate(glm::mat4(1.0f), {-x_center, -y_center, -z_center});
    _model_matrix =
      scale(glm::mat4(1.0f), {1.0f / max_range, 1.0f / max_range, 1.0f / max_range}) * translation;
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
      vertices.push_back(vertex);
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
