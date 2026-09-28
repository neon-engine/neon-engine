#ifndef GL_TEXTURE_HPP
#define GL_TEXTURE_HPP

#include <memory>
#include <string>
#include <glad/gl.h>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class GL_Texture
  {
    GLuint _texture_id = 0;
    std::string _texture_path;
    bool _initialized = false;
    FileSystemContext *_file_system_context = nullptr;
    std::shared_ptr<Logger> _logger;

  public:
    GL_Texture();

    explicit GL_Texture(
      const std::string &texture_path,
      FileSystemContext *file_system_context,
      const std::shared_ptr<Logger> &logger);

    bool Initialize();

    void Use(GLint unit) const;

    void CleanUp();
  };
} // neon

#endif //GL_TEXTURE_HPP
