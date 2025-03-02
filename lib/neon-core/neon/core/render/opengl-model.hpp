#ifndef OPEN_GL_MODEL_HPP
#define OPEN_GL_MODEL_HPP
#include "model.hpp"
#include "opengl-mesh.hpp"

namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class OpenGL_Model final : public Model
  {
    std::vector<OpenGL_Mesh> _meshes;
  protected:
    bool Initialize() override;

    void Use() override;

    void CleanUp() override;

    bool ProcessMesh(aiMesh *mesh, const aiScene *scene) override;

  public:
    OpenGL_Model(const std::string &path, const std::shared_ptr<Logger> &logger);
  };
} // core

#endif //OPEN_GL_MODEL_HPP
