#include <iostream>
#include <fstream>
#include <sstream>
#include <GL/gl3w.h>
#include "shader.h"

Shader::Shader(const char *vertex_path, const char *fragment_path) {
  // 1. retrieve the vertex/fragment source code from filePath
  std::string vertex_code;
  std::string fragment_code;
  std::ifstream vert_shader_file;
  std::ifstream frag_shader_file;
  // ensure ifstream objects can throw exceptions:
  vert_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  frag_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  try {
    // open files
    vert_shader_file.open(vertex_path);
    frag_shader_file.open(fragment_path);
    std::stringstream vert_shader_stream, frag_shader_stream;
    // read file's buffer contents into streams
    vert_shader_stream << vert_shader_file.rdbuf();
    frag_shader_stream << frag_shader_file.rdbuf();
    // close file handlers
    vert_shader_file.close();
    frag_shader_file.close();
    // convert stream into string
    vertex_code = vert_shader_stream.str();
    fragment_code = frag_shader_stream.str();
  }
  catch (const std::ifstream::failure &e) {
    std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
  }
  const char *vert_shader_code = vertex_code.c_str();
  const char *frag_shader_code = fragment_code.c_str();

  // 2. compile shaders
  unsigned int vertex, fragment;
  const int kBufSize = 512;
  int success;
  char info_log[kBufSize];

  // vertex Shader
  vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vert_shader_code, nullptr);
  glCompileShader(vertex);
  // print compile errors if any
  glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertex, kBufSize, nullptr, info_log);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << info_log << std::endl;
  }

  // similar for Fragment Shader
  fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &frag_shader_code, nullptr);
  glCompileShader(fragment);
  // print compile errors if any
  glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragment, kBufSize, nullptr, info_log);
    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << info_log << std::endl;
  }

  // shader Program
  program_id_ = glCreateProgram();
  glAttachShader(program_id_, vertex);
  glAttachShader(program_id_, fragment);
  glLinkProgram(program_id_);
  // print linking errors if any
  glGetProgramiv(program_id_, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program_id_, kBufSize, nullptr, info_log);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << info_log << std::endl;
  }

// delete the shaders as they're linked into our program now and no longer necessary
  glDeleteShader(vertex);
  glDeleteShader(fragment);
}

void Shader::Use() const {
  glUseProgram(program_id_);
}

void Shader::SetBool(const std::string &name, bool value) const {
  glUniform1i(glGetUniformLocation(program_id_, name.c_str()), (int) value);
}

void Shader::SetInt(const std::string &name, int value) const {
  glUniform1i(glGetUniformLocation(program_id_, name.c_str()), value);
}

void Shader::SetFloat(const std::string &name, float value) const {
  glUniform1f(glGetUniformLocation(program_id_, name.c_str()), value);
}
