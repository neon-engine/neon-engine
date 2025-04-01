#ifndef GL_MODEL_HPP
#define GL_MODEL_HPP

#include <neon/render/model.hpp>
#include "gl-mesh.hpp"

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class GL_Model final : public Model
  {
    std::vector<GL_Mesh> _meshes{};
    bool _has_texture_coordinates = true;
  protected:
    bool ProcessMesh(aiMesh *mesh, const aiScene *scene) override;

  public:
    GL_Model(const std::string &path, const std::shared_ptr<Logger> &logger);

    bool Initialize() override;

    void Use() const override;

    void CleanUp() override;

  protected:
    void GenerateNormalizationMatrix() override;
  };
} // neon

#endif //GL_MODEL_HPP
