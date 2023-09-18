#include <string>

#ifndef PROJECTNEON_APP_LEARNOPENGL_GRAPHICS_SHADER_H_
#define PROJECTNEON_APP_LEARNOPENGL_GRAPHICS_SHADER_H_

class Shader {
 private:
  unsigned int program_id_;
 public:

  // constructor reads and builds the shader
  Shader(const char *vertex_path, const char *fragment_path);

  void Use() const;
  // utility uniform functions
  void SetBool(const std::string &name, bool value) const;
  void SetInt(const std::string &name, int value) const;
  void SetFloat(const std::string &name, float value) const;
};

#endif //PROJECTNEON_APP_LEARNOPENGL_GRAPHICS_SHADER_H_
