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
    VK_Model(
      const std::string &path,
      FileSystemContext *file_system_context,
      VK_Device *device,
      const std::shared_ptr<Logger> &logger);

    /// A model from a mesh that was built at run time. It is drawn in metres
    /// as it is, without the centring and scaling a file gets, since what
    /// built it meant every number.
    VK_Model(std::shared_ptr<const MeshData> mesh, VK_Device *device, const std::shared_ptr<Logger> &logger);

    bool Initialize() override;

    /// Draws the model as part of the frame that is being recorded.
    void Use() const override;

    void CleanUp() override;
  };
} // neon

#endif //VK_MODEL_HPP
