#ifndef MODEL_HPP
#define MODEL_HPP

#include <assimp/scene.h>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

#include "mesh.hpp"
#include "model-material.hpp"
#include "texture-info.hpp"

namespace neon
{
  /// Reads a model file through the file system with assimp, and hands each
  /// mesh to a backend. What is read, and what of a file is left out, is
  /// described in docs/models.md.
  class Model
  {
    std::string _path;
    std::vector<ModelMaterial> _materials;

    bool ProcessNode(aiNode *root, const aiScene *scene);

    /// Reads the materials of a scene into `_materials`, and warns once
    /// about what the renderer cannot show of them.
    void LoadMaterials(const aiScene *scene);

    /// Warns once about what a mesh carries that the renderer cannot show.
    void NoteWhatIsNotShown(const aiMesh *mesh);

    bool _noted_vertex_colors = false;

    // the material of the first mesh, which the model is drawn with
    int _drawn_material = -1;

  protected:
    glm::mat4 _model_matrix{1.0f};
    FileSystemContext *_file_system_context;
    std::shared_ptr<Logger> _logger;

    bool LoadModel();

    /// Adds the textures of one kind that a material names to `textures`. A
    /// name that is a file is made a virtual path from the folder of the
    /// model. One that points into the scene, as `*0` does, brings the bytes
    /// of the image with it.
    void LoadMaterialTextures(
      const aiScene *scene,
      const aiMaterial *material,
      const aiTextureType &type,
      std::vector<TextureInfo> &textures) const;

    /// Moves vertices where a node of the model places its mesh. The normals
    /// turn with it. A transform that mirrors turns the triangles round, so
    /// the winding of every face is reversed to keep the front in front.
    static void ApplyNodeTransform(
      const glm::mat4 &transform, std::vector<Vertex> &vertices, std::vector<unsigned int> &indices);

    virtual void GenerateNormalizationMatrix() = 0;

    /// The matrix that moves a model to the origin and scales it so that its
    /// longest side has length 1. Backends call this from
    /// GenerateNormalizationMatrix() with the meshes they loaded.
    static glm::mat4 ComputeNormalizationMatrix(const std::vector<const Mesh *> &meshes);

    /// Takes one mesh of the model. `transform` is where the node that
    /// carries the mesh places it, with every node above it applied: a
    /// backend bakes it into the vertices with ApplyNodeTransform(), since
    /// the renderer draws a model as one piece.
    virtual bool ProcessMesh(aiMesh *mesh, const aiScene *scene, const glm::mat4 &transform) = 0;

    ~Model() = default;

  public:
    Model(
      const std::string &path,
      FileSystemContext *file_system_context,
      const std::shared_ptr<Logger> &logger);

    virtual bool Initialize() = 0;

    virtual void Use() const = 0;

    virtual void CleanUp() = 0;

    [[nodiscard]] glm::mat4 GetNormalizedModelMatrix() const;

    /// The materials of the file, in its order, with the one assimp adds
    /// for meshes without: behind them for a glTF, in front for an .obj.
    /// Empty until the model is loaded.
    [[nodiscard]] const std::vector<ModelMaterial> &GetMaterials() const { return _materials; }

    /// The material the model is drawn with, which is that of its first
    /// mesh. Nothing until the model is loaded, or when it has no mesh.
    [[nodiscard]] const ModelMaterial *GetDrawnMaterial() const;

    /// The virtual path of the folder a model is in, with its slash, so that
    /// a file the model names is found next to it.
    [[nodiscard]] static std::string FolderOf(const std::string &model_path);
  };
} // neon

#endif //MODEL_HPP
