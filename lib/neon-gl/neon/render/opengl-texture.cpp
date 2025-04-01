#include "opengl-texture.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>
#include <neon/common/util.hpp>

namespace neon
{
  OpenGL_Texture::OpenGL_Texture() = default;

  OpenGL_Texture::OpenGL_Texture(const std::string &texture_path, const std::shared_ptr<Logger> &logger)
  {
    _texture_path = texture_path;
    _logger = logger;
  }

  bool OpenGL_Texture::Initialize()
  {

    if (_initialized)
    {
      _logger->Error("Texture {} already initialized with opengl id {}", _texture_path, _texture_id);
      return true;
    }

    _logger->Info("Initializing texture from {}", _texture_path);

    const auto file_extension = get_file_extension(_texture_path);
    auto format = GL_RGB;

    if (file_extension == "png")
    {
      format = GL_RGBA;
    }
    else
    {
      format = GL_RGB;
    }

    GLint tex_width, tex_height;
    int nr_channels;
    stbi_set_flip_vertically_on_load(false);
    if (unsigned char *data = stbi_load(
      _texture_path.c_str(),
      &tex_width,
      &tex_height,
      &nr_channels,
      STBI_default))
    {
      glGenTextures(1, &_texture_id);
      glBindTexture(GL_TEXTURE_2D, _texture_id);
      glGenerateMipmap(GL_TEXTURE_2D);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D,
                   0,
                   static_cast<GLint>(format),
                   tex_width,
                   tex_height,
                   0,
                   // consider making this configurable
                   format,
                   GL_UNSIGNED_BYTE,
                   data);
      glGenerateMipmap(GL_TEXTURE_2D);

      // free the memory after we are done creating texture
      stbi_image_free(data);
    } else
    {
      _logger->Error("Failed to load texture {}", _texture_path);
      return false;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    _initialized = true;
    return true;
  }

  void OpenGL_Texture::Use(const GLint unit) const
  {
    const GLenum texture_unit = GL_TEXTURE0 + unit;
    glActiveTexture(texture_unit);
    glBindTexture(GL_TEXTURE_2D, _texture_id);
    // TODO [issues/5] add check to make sure we are not exceeding max_texture_units
    // if (unit >= 0 && unit < _max_texture_units)
    // {
    //   const GLenum texture_unit = GL_TEXTURE0 + unit;
    //   glActiveTexture(texture_unit);
    //   glBindTexture(GL_TEXTURE_2D, _texture_id);
    // } else
    // {
    //   auto max_val = std::to_string(_max_texture_units - 1);
    //   _logger->Error("Error: Texture unit {} is out of range. Supported range: 0 to {}", unit, max_val);
    // }
  }

  void OpenGL_Texture::CleanUp()
  {
    if (!_initialized) { return; }
    _logger->Info("Cleaning up texture {} with opengl id {}", _texture_path, _texture_id);
    glDeleteTextures(1, &_texture_id);
    _initialized = false;
  }
} // neon
