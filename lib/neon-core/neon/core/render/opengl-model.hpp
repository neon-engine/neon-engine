#ifndef OPEN_GL_MODEL_HPP
#define OPEN_GL_MODEL_HPP
#include "model.hpp"
#include "opengl-mesh.hpp"

namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class OpenGL_Model final : public Model
  {
    std::vector<OpenGL_Mesh> _meshes{};
  protected:
    bool ProcessMesh(aiMesh *mesh, const aiScene *scene) override;

  public:
    OpenGL_Model(const std::string &path, const std::shared_ptr<Logger> &logger);

    bool Initialize() override;

    void Use() const override;

    void CleanUp() override;

  protected:
    void GenerateNormalizationMatrix() override;
  };
} // core

#endif //OPEN_GL_MODEL_HPP
