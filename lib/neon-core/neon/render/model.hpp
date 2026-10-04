#ifndef MODEL_HPP
#define MODEL_HPP

#include <span>
#include <assimp/scene.h>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

#include "mesh.hpp"
#include "model-fit.hpp"
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
    ModelFit _fit;
    std::vector<ModelMaterial> _materials;

    bool ProcessNode(aiNode *root, const aiScene *scene);

    /// Reads the materials of a scene into `_materials`.
    void LoadMaterials(const aiScene *scene);

    // the material of every mesh, in the order the meshes were handed to
    // the backend, and the materials they use, in the order of first use
    std::vector<int> _mesh_materials;
    std::vector<int> _used_materials;

  protected:
    /// The matrix the fit gives the model, see GetNormalizedModelMatrix().
    glm::mat4 _model_matrix{1.0f};

    /// The middle of the model, see GetMiddle().
    glm::vec3 _middle{0.0f};
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

    /// The vertices of a mesh as the file gives them: position, normal, and
    /// the first set of texture coordinates, with what the mesh has not
    /// left at zero, and the first set of vertex colours (`COLOR_0` of
    /// glTF), white when the mesh has none.
    static std::vector<Vertex> ReadVertices(const aiMesh *mesh);

    /// Moves vertices where a node of the model places its mesh. The normals
    /// turn with it. A transform that mirrors turns the triangles round, so
    /// the winding of every face is reversed to keep the front in front.
    static void ApplyNodeTransform(
      const glm::mat4 &transform, std::vector<Vertex> &vertices, std::vector<unsigned int> &indices);

    /// Sets `_model_matrix` from the meshes that were loaded and the fit.
    virtual void GenerateNormalizationMatrix() = 0;

    /// The matrix that gives a model its fit: for `Unit`, the one that moves
    /// the model to the origin and scales it so that its longest side has
    /// length 1; for `None`, the identity, which leaves the model as the
    /// file says. Backends call this from GenerateNormalizationMatrix() with
    /// the meshes they loaded.
    static glm::mat4 ComputeNormalizationMatrix(const std::vector<const Mesh *> &meshes, ModelFit fit);

    /// The middle of the box around vertices, as they are. The origin for
    /// none. Backends keep it in `_middle` for the meshes they loaded, and
    /// again when a mesh is changed.
    static glm::vec3 ComputeMiddle(std::span<const Vertex> vertices);

    /// The same for all the vertices of meshes.
    static glm::vec3 ComputeMiddle(const std::vector<const Mesh *> &meshes);

    /// Takes one mesh of the model. `transform` is where the node that
    /// carries the mesh places it, with every node above it applied: a
    /// backend bakes it into the vertices with ApplyNodeTransform(), since
    /// the renderer draws a model as one piece.
    virtual bool ProcessMesh(aiMesh *mesh, const aiScene *scene, const glm::mat4 &transform) = 0;

    ~Model() = default;

  public:
    /// `fit` says how the model is sized once it is loaded, see ModelFit.
    Model(
      const std::string &path,
      FileSystemContext *file_system_context,
      const std::shared_ptr<Logger> &logger,
      ModelFit fit = ModelFit::None);

    virtual bool Initialize() = 0;

    virtual void Use() const = 0;

    virtual void CleanUp() = 0;

    /// The matrix that gives the model its fit, which what draws it puts
    /// between the vertices and the Transform of the entity. The identity
    /// for a fit of `None`, and until the model is loaded.
    [[nodiscard]] glm::mat4 GetNormalizedModelMatrix() const;

    /// The middle of the box around the model, as its vertices are, before
    /// the fit: where the model is, as far as one place can say it. What
    /// draws see-through things orders them by it and not by where their
    /// entity is, since a mesh may lie far from the origin it is placed by.
    [[nodiscard]] glm::vec3 GetMiddle() const { return _middle; }

    [[nodiscard]] ModelFit GetFit() const { return _fit; }

    /// The materials of the file, in its order, with the one assimp adds
    /// for meshes without: behind them for a glTF, in front for an .obj.
    /// Empty until the model is loaded.
    [[nodiscard]] const std::vector<ModelMaterial> &GetMaterials() const { return _materials; }

    /// The materials the meshes use, as indices into GetMaterials(), each
    /// once, in the order a mesh first uses them. A renderer draws the
    /// model with one material of its own for each. Empty until the model
    /// is loaded, or when it has no mesh.
    [[nodiscard]] const std::vector<int> &GetUsedMaterials() const { return _used_materials; }

    /// The material of the mesh that was handed to the backend at `mesh`,
    /// as an index into GetMaterials(), or -1 for a mesh that was built.
    [[nodiscard]] int GetMaterialOfMesh(std::size_t mesh) const;

    /// The material of the first mesh, which is the one a scene's own
    /// textures replace. Nothing until the model is loaded, or when it has
    /// no mesh.
    [[nodiscard]] const ModelMaterial *GetFirstMaterial() const;

    /// The virtual path of the folder a model is in, with its slash, so that
    /// a file the model names is found next to it.
    [[nodiscard]] static std::string FolderOf(const std::string &model_path);
  };
} // neon

#endif //MODEL_HPP
