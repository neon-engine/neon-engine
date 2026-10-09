#ifndef VK_MODEL_HPP
#define VK_MODEL_HPP

#include <memory>

#include <neon/geometry/mesh-data.hpp>
#include <neon/render/model.hpp>

#include "vk-mesh.hpp"

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class VK_Model final : public Model
  {
    VK_Device *_device;
    std::vector<VK_Mesh> _meshes{};

    // a mesh that was built rather than read, drawn as it is
    std::shared_ptr<const MeshData> _mesh_data;

  protected:
    bool ProcessMesh(aiMesh *mesh, const aiScene *scene, const glm::mat4 &transform) override;

    void GenerateNormalizationMatrix() override;

  public:
    /// A model from a file, sized as `fit` says once it is loaded.
    VK_Model(
      const std::string &path,
      ModelFit fit,
      FileSystemContext *file_system_context,
      VK_Device *device,
      const std::shared_ptr<Logger> &logger);

    /// A model from a mesh that was built at run time. It is drawn in meters
    /// as it is, with a fit of `None`, since what built it meant every
    /// number.
    VK_Model(std::shared_ptr<const MeshData> mesh, VK_Device *device, const std::shared_ptr<Logger> &logger);

    bool Initialize() override;

    /// Draws the model as part of the frame that is being recorded.
    void Use() const override;

    /// Draws the meshes that use the material at `material` of the file
    /// into `commands` `instances` times over, the shaders told which
    /// object each is by gl_InstanceIndex, counted from `first_instance`.
    /// Every mesh when `material` is below 0, which is what a mesh that
    /// was built has. The shadow pass records apart from the frame.
    void Draw(VkCommandBuffer commands, uint32_t instances = 1, uint32_t first_instance = 0, int material = -1) const;

    /// How many draws that takes: one for each mesh that uses the
    /// material, or for every mesh when `material` is below 0.
    [[nodiscard]] std::size_t MeshCount(int material = -1) const;

    /// Draws `mesh` from now on, for a model that was made from a mesh
    /// that has changed since. A model from a file is left as it is.
    void UpdateMesh(const MeshData &mesh);

    void CleanUp() override;
  };
} // neon

#endif //VK_MODEL_HPP
