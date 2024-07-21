#ifndef OPEN_GL_SHADER_HPP
#define OPEN_GL_SHADER_HPP

#include <GL/gl3w.h>

#include <string>

namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class OpenGL_Shader
  {
    std::string _shader_path;
    GLuint _shader_program_id = 0;
    bool _initialize = false;
  public:
    OpenGL_Shader();
    explicit OpenGL_Shader(const std::string &shader_path);

    ~OpenGL_Shader();

    bool Initialize();

    void Activate() const;

    void Deactivate();

    void CleanUp();

    void SetInt(const std::string &name, const int &value) const;

    void SetFloat(const std::string &name, const float &value) const;

    void SetBool(const std::string &name, const bool &value) const;
  };
} // core

#endif //OPEN_GL_SHADER_HPP
