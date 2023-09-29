#ifndef TEXTURE_HPP
#define TEXTURE_HPP

class Texture {
 private:
  unsigned int _texture_id;
 public:
  Texture(const char *texture_path);
  ~Texture();

  void Use() const;
};

#endif //TEXTURE_HPP
