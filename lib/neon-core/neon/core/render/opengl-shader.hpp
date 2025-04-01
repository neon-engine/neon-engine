#ifndef OPEN_GL_SHADER_HPP
#define OPEN_GL_SHADER_HPP

#include <memory>
#include <glad/gl.h>

#include <string>
#include <glm/glm.hpp>

#include "neon/core/logging/logger.hpp"

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class OpenGL_Shader
  {
    std::string _shader_path;
    GLuint _shader_program_id = 0;
    bool _initialize = false;
    std::shared_ptr<Logger> _logger;

  public:
    OpenGL_Shader();

    explicit OpenGL_Shader(const std::string &shader_path, const std::shared_ptr<Logger> &logger);

    bool Initialize();

    void Activate() const;

    void Deactivate();

    void CleanUp();

    void SetInt(const std::string &name, int value) const;

    void SetFloat(const std::string &name, float value) const;

    void SetBool(const std::string &name, bool value) const;

    void SetVec2(const std::string &name, const glm::vec2 &value) const;

    void SetVec2(const std::string &name, float x, float y) const;

    void SetVec3(const std::string &name, const glm::vec3 &value) const;

    void SetVec3(const std::string &name, float x, float y, float z) const;

    void SetVec4(const std::string &name, const glm::vec4 &value) const;

    void SetVec4(const std::string &name, float x, float y, float z, float w) const;

    void SetMat2(const std::string &name, const glm::mat2 &mat) const;

    void SetMat3(const std::string &name, const glm::mat3 &mat) const;

    void SetMat4(const std::string &name, const glm::mat4 &mat) const;
  };
} // neon

#endif //OPEN_GL_SHADER_HPP
