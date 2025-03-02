#include "opengl-shader.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

namespace core
{
  OpenGL_Shader::OpenGL_Shader() = default;

  OpenGL_Shader::OpenGL_Shader(const std::string &shader_path, const std::shared_ptr<Logger> &logger)
  {
    _shader_path = shader_path;
    _logger = logger;
  }

  bool OpenGL_Shader::Initialize()
  {
    if (_initialize)
    {
      _logger->Error("Shader {} already loaded opengl id {}", _shader_path, _shader_program_id);
      return true;
    }

    _logger->Info("Initializing shader from {}", _shader_path);

    std::string vertex_path = _shader_path + ".vert";
    std::string fragment_path = _shader_path + ".frag";
    std::string vertex_code;
    std::string fragment_code;
    std::ifstream vert_shader_file;
    std::ifstream frag_shader_file;

    vert_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    frag_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
      vert_shader_file.open(vertex_path);
      frag_shader_file.open(fragment_path);
      std::stringstream vert_shader_stream, frag_shader_stream;

      vert_shader_stream << vert_shader_file.rdbuf();
      frag_shader_stream << frag_shader_file.rdbuf();

      vert_shader_file.close();
      frag_shader_file.close();

      vertex_code = vert_shader_stream.str();
      fragment_code = frag_shader_stream.str();
    } catch (const std::ifstream::failure &)
    {
      _logger->Error("Error opening shader {}", _shader_path);
      return false;
    }
    const char *vert_shader_code = vertex_code.c_str();
    const char *frag_shader_code = fragment_code.c_str();

    // 2. compile shaders
    GLuint vertex, fragment;
    constexpr GLsizei buf_size = 512;
    GLint success;
    char info_log[buf_size];
    auto error = false;

    // vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vert_shader_code, nullptr);
    glCompileShader(vertex);
    // print compile errors if any
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
      glGetShaderInfoLog(vertex, buf_size, nullptr, info_log);
      _logger->Error("Error compiling vertex shader: {}", info_log);
      error = true;
    }

    // similar for Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &frag_shader_code, nullptr);
    glCompileShader(fragment);
    // print compile errors if any
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
      glGetShaderInfoLog(fragment, buf_size, nullptr, info_log);
      _logger->Error("Error compiling fragment shader: {}", info_log);
      error = true;
    }

    // shader Program
    auto program_id = glCreateProgram();
    glAttachShader(program_id, vertex);
    glAttachShader(program_id, fragment);
    glLinkProgram(program_id);
    // print linking errors if any
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success)
    {
      glGetProgramInfoLog(program_id, buf_size, nullptr, info_log);
      _logger->Error("Error linking shader program: {}", info_log);
      error = true;
    }

    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (error) { return false; }

    _shader_program_id = program_id;
    _initialize = true;
    return true;
  }

  void OpenGL_Shader::Activate() const
  {
    glUseProgram(_shader_program_id);
  }

  // ReSharper disable once CppMemberFunctionMayBeStatic
  void OpenGL_Shader::Deactivate() // NOLINT(*-convert-member-functions-to-static)
  {
    glUseProgram(0);
  }

  void OpenGL_Shader::CleanUp()
  {
    if (!_initialize) { return; }
    _logger->Info("Cleaning up shader {} with opengl id {}", _shader_path, _shader_program_id);
    glDeleteProgram(_shader_program_id);
    _initialize = false;
  }

  void OpenGL_Shader::SetInt(const std::string &name, const int value) const
  {
    glUniform1i(glGetUniformLocation(_shader_program_id, name.c_str()), value);
  }

  void OpenGL_Shader::SetFloat(const std::string &name, const float value) const
  {
    glUniform1f(glGetUniformLocation(_shader_program_id, name.c_str()), value);
  }

  void OpenGL_Shader::SetBool(const std::string &name, const bool value) const
  {
    glUniform1i(glGetUniformLocation(_shader_program_id, name.c_str()), static_cast<int>(value));
  }

  void OpenGL_Shader::SetVec2(const std::string &name, const glm::vec2 &value) const
  {
    glUniform2fv(glGetUniformLocation(_shader_program_id, name.c_str()), 1, &value[0]);
  }

  void OpenGL_Shader::SetVec2(const std::string &name, const float x, const float y) const
  {
    glUniform2f(glGetUniformLocation(_shader_program_id, name.c_str()), x, y);
  }

  void OpenGL_Shader::SetVec3(const std::string &name, const glm::vec3 &value) const
  {
    glUniform3fv(glGetUniformLocation(_shader_program_id, name.c_str()), 1, &value[0]);
  }

  void OpenGL_Shader::SetVec3(const std::string &name, const float x, const float y, const float z) const
  {
    glUniform3f(glGetUniformLocation(_shader_program_id, name.c_str()), x, y, z);
  }

  void OpenGL_Shader::SetVec4(const std::string &name, const glm::vec4 &value) const
  {
    glUniform4fv(glGetUniformLocation(_shader_program_id, name.c_str()), 1, &value[0]);
  }

  void OpenGL_Shader::SetVec4(const std::string &name, const float x, const float y, const float z, const float w) const
  {
    glUniform4f(glGetUniformLocation(_shader_program_id, name.c_str()), x, y, z, w);
  }

  void OpenGL_Shader::SetMat2(const std::string &name, const glm::mat2 &mat) const
  {
    glUniformMatrix2fv(glGetUniformLocation(_shader_program_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
  }

  void OpenGL_Shader::SetMat3(const std::string &name, const glm::mat3 &mat) const
  {
    glUniformMatrix3fv(glGetUniformLocation(_shader_program_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
  }

  void OpenGL_Shader::SetMat4(const std::string &name, const glm::mat4 &mat) const
  {
    glUniformMatrix4fv(glGetUniformLocation(_shader_program_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
  }
} // core
