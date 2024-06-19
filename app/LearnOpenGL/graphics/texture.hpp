#ifndef TEXTURE_HPP
#define TEXTURE_HPP

class Texture
{
  GLuint _texture_id;
  static GLint _max_texture_units;

public:
  explicit Texture(const char *texture_path, GLenum format = GL_RGB);

  ~Texture();

  void Use(GLint value) const;
};

#endif //TEXTURE_HPP
