#ifndef OPEN_GL_TEXTURE_HPP
#define OPEN_GL_TEXTURE_HPP

#include <GL/gl3w.h>

#include <string>

namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class OpenGL_Texture
  {
    GLuint _texture_id = 0;
    std::string _texture_path;
    bool _initialized = false;

    static GLint _max_texture_units;

  public:
    explicit OpenGL_Texture(const std::string &texture_path);

    ~OpenGL_Texture();

    bool Initialize();

    void Use(GLint unit) const;

    void CleanUp();

  private:
    static std::string GetFileExtension(const std::string& filename);
  };
} // core

#endif //OPEN_GL_TEXTURE_HPP
