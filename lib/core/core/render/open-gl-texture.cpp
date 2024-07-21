#include "open-gl-texture.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include <stb_image.h>

namespace core
{
  OpenGL_Texture::OpenGL_Texture() = default;

  OpenGL_Texture::OpenGL_Texture(const std::string &texture_path)
  {
    _texture_path = texture_path;
  }

  bool OpenGL_Texture::Initialize()
  {

    if (_initialized)
    {
      std::cerr << "Texture " << _texture_path << " already initialized with opengl id " << _texture_id << std::endl;
      return true;
    }

    std::cout << "Initializing texture from " << _texture_path << std::endl;

    const auto file_extension = GetFileExtension(_texture_path);
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
    stbi_set_flip_vertically_on_load(true);
    GLuint opengl_texture_id;
    if (unsigned char *data = stbi_load(
      _texture_path.c_str(),
      &tex_width,
      &tex_height,
      &nr_channels,
      STBI_default))
    {
      glGenTextures(1, &opengl_texture_id);
      glBindTexture(GL_TEXTURE_2D, opengl_texture_id);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D,
                   0,
                   format,
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
      std::cout << "Failed to load texture " << _texture_path << std::endl;
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
    // if (unit >= 0 && unit < _max_texture_units)
    // {
    //   const GLenum texture_unit = GL_TEXTURE0 + unit;
    //   glActiveTexture(texture_unit);
    //   glBindTexture(GL_TEXTURE_2D, _texture_id);
    // } else
    // {
    //   auto max_val = std::to_string(_max_texture_units - 1);
    //   std::cerr << "Error: Texture unit " << unit << " is out of range. Supported range: 0 to "
    //    << max_val << std::endl;
    // }
  }

  void OpenGL_Texture::CleanUp()
  {
    if (!_initialized) { return; }
    std::cout << "Cleaning up texture " << _texture_path << " with opengl id " << _texture_id << std::endl;
    glDeleteTextures(1, &_texture_id);
    _initialized = false;
  }

  std::string OpenGL_Texture::GetFileExtension(const std::string &filename)
  {
    if (const size_t dot_position = filename.find_last_of('.'); dot_position != std::string::npos) {
      return filename.substr(dot_position + 1);
    }
    return "";
  }
} // core
