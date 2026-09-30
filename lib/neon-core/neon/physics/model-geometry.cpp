#include "model-geometry.hpp"

#include <neon/render/mesh.hpp>
#include <neon/render/model.hpp>

namespace neon
{
  namespace
  {
    /// A mesh that is kept in memory and never drawn.
    class PlainMesh final : public Mesh
    {
    public:
      using Mesh::Mesh;

      bool Initialize() override { return true; }

      void CleanUp() override {}

      void Use() const override {}

      [[nodiscard]] const std::vector<unsigned int> &GetIndices() const { return _indices; }
    };

    /// A model that keeps the points it is handed. It is read by what reads
    /// the models of the renderer, so both see the same points.
    class PlainModel final : public Model
    {
      std::vector<PlainMesh> _meshes;

    protected:
      bool ProcessMesh(aiMesh *mesh, const aiScene *) override
      {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        vertices.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
          Vertex vertex{};
          vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
          vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
          const aiFace &face = mesh->mFaces[i];

          // points and lines have no surface to collide with
          if (face.mNumIndices != 3) { continue; }

          for (unsigned int j = 0; j < 3; j++) { indices.push_back(face.mIndices[j]); }
        }

        _meshes.emplace_back(vertices, indices, std::vector<TextureInfo>{}, _logger);
        return true;
      }

      void GenerateNormalizationMatrix() override
      {
        std::vector<const Mesh *> meshes;
        for (const auto &mesh : _meshes) { meshes.push_back(&mesh); }

        _model_matrix = ComputeNormalizationMatrix(meshes);
      }

    public:
      using Model::Model;

      bool Initialize() override
      {
        if (!LoadModel()) { return false; }

        GenerateNormalizationMatrix();
        return true;
      }

      void Use() const override {}

      void CleanUp() override { _meshes.clear(); }

      void CopyTo(ModelGeometry &geometry) const
      {
        for (const auto &mesh : _meshes)
        {
          const auto first = static_cast<std::uint32_t>(geometry.points.size());

          for (const auto &vertex : mesh.GetVertices())
          {
            geometry.points.emplace_back(_model_matrix * glm::vec4(vertex.position, 1.0f));
          }

          for (const auto index : mesh.GetIndices()) { geometry.triangles.push_back(first + index); }
        }
      }
    };
  }

  bool LoadModelGeometry(
    const std::string &path,
    FileSystemContext *file_system_context,
    const std::shared_ptr<Logger> &logger,
    ModelGeometry &geometry)
  {
    geometry = {};

    PlainModel model(path, file_system_context, logger);
    if (!model.Initialize()) { return false; }

    model.CopyTo(geometry);

    if (geometry.triangles.empty())
    {
      logger->Error("The model {} holds no triangle to make a shape from", path);
      geometry = {};
      return false;
    }

    return true;
  }
} // neon
