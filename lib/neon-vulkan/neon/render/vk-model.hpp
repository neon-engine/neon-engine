#ifndef VK_MODEL_HPP
#define VK_MODEL_HPP

#include <neon/render/model.hpp>

#include "vk-mesh.hpp"

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class VK_Model final : public Model
  {
    VK_Device *_device;
    std::vector<VK_Mesh> _meshes{};

  protected:
    bool ProcessMesh(aiMesh *mesh, const aiScene *scene) override;

    void GenerateNormalizationMatrix() override;

  public:
    VK_Model(
      const std::string &path,
      FileSystemContext *file_system_context,
      VK_Device *device,
      const std::shared_ptr<Logger> &logger);

    bool Initialize() override;

    /// Draws the model as part of the frame that is being recorded.
    void Use() const override;

    void CleanUp() override;
  };
} // neon

#endif //VK_MODEL_HPP
