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
    explicit OpenGL_Shader(const std::string &shader_path);

    ~OpenGL_Shader();

    bool Initialize();

    void Use() const;

    void CleanUp();
  };
} // core

#endif //OPEN_GL_SHADER_HPP
