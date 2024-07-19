#include "opengl-render-system.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include <fstream>
#include <iostream>
#include <sstream>
#include <stb_image.h>
#include <stdexcept>
#include <GL/gl3w.h>


namespace core
{
  void OpenGL_RenderSystem::Initialize()
  {
    std::cout << "Initializing OpenGL" << std::endl;
    if (gl3wInit())
    {
      throw std::runtime_error("Failed to initialize gl3w");
    }

    glEnable(GL_DEPTH_TEST);

    int num_vertex_attributes_supported;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &num_vertex_attributes_supported);
    std::cout << "Maximum number of vertex attributes supported: " << num_vertex_attributes_supported << std::endl;

    glViewport(0, 0, _settings_config.width, _settings_config.height);
  }

  void OpenGL_RenderSystem::CleanUp()
  {
    std::cout << "Cleaning up OpenGL" << std::endl;
  }

  void OpenGL_RenderSystem::PrepareFrame()
  {
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); for wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  RenderContext *OpenGL_RenderSystem::GetContext()
  {
    return this;
  }

  int OpenGL_RenderSystem::InitGeometry(
    const std::vector<float> &vertices,
    const std::vector<float> &normals,
    const std::vector<float> &tex_coordinates,
    const std::vector<int> &indices)
  {
    const auto geometry_id = _geometry_index++;
    std::cout << "Initializing mesh: " << geometry_id << std::endl;

    if (geometry_id >= _geometry_references.size())
    {
      throw std::runtime_error("geometry id exceeded geometry db size");
    }

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint nvbo = 0;
    GLuint uvbo = 0;
    GLuint ebo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &nvbo);
    glGenBuffers(1, &uvbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    // vbo - vertices
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
      GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
      &vertices[0],
      GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
      0,
      3,
      GL_FLOAT,
      GL_FALSE,
      3 * sizeof(GLfloat),
      static_cast<void *>(nullptr));

    // nvbo - normals
    glBindBuffer(GL_ARRAY_BUFFER, nvbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(normals.size() * sizeof(float)),
                 &normals[0],
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
      1,
      3,
      GL_FLOAT,
      GL_FALSE,
      3 * sizeof(GLfloat),
      static_cast<void *>(nullptr));

    // uvbo - texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, uvbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(tex_coordinates.size() * sizeof(float)),
                 &tex_coordinates[0],
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
      2,
      2,
      GL_FLOAT,
      GL_FALSE,
      2 * sizeof(GLfloat),
      static_cast<void *>(nullptr));


    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(int)),
                 &indices[0],
                 GL_STATIC_DRAW);

    // Unbind the currently bound buffer so that we don't accidentally make unwanted changes to it.
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Unbind the VAO now, so we don't accidentally tamper with it.
    // NOTE: You must NEVER unbind the element array buffer associated with a VAO!
    glBindVertexArray(0);
    _geometry_references[geometry_id] = OpenGL_Geometry{vao, vbo, nvbo, uvbo, ebo};
    return geometry_id;
  }

  void OpenGL_RenderSystem::DestroyGeometry(const int geometry_id)
  {
    const auto [
      vao,
      vbo,
      nvbo,
      uvbo,
      ebo] = _geometry_references[geometry_id];

    glDeleteBuffers(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &nvbo);
    glDeleteBuffers(1, &uvbo);
    glDeleteBuffers(1, &ebo);
  }

  int OpenGL_RenderSystem::InitShader(const std::string shader_path)
  {
    const auto shader_id = _shader_index++;
    std::cout << "Initializing shader from " << shader_path << " with shader id " << shader_id << std::endl;

    std::string vertex_path = shader_path + ".vert";
    std::string fragment_path = shader_path + ".frag";
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
      std::cout << "Error opening shader " << shader_path << std::endl;
      return -1;
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
      std::cout << "Error compiling vertex shader:\n" << info_log << std::endl;
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
      std::cout << "Error compiling fragment shader:\n" << info_log << std::endl;
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
      std::cout << "Error linking shader program:\n" << info_log << std::endl;
      error = true;
    }

    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (error) { return -1; }

    _shader_references[shader_id] = program_id;

    return shader_id;
  }

  void OpenGL_RenderSystem::DestroyShader(const int shader_id)
  {
    const auto program_id = _shader_references[shader_id];
    glDeleteProgram(program_id);
  }

  int OpenGL_RenderSystem::InitTexture(const std::string texture_path)
  {
    const auto texture_id = _texture_index++;
    std::cout << "Initializing texture from " << texture_path << " with texture id " << texture_id << std::endl;

    GLint tex_width, tex_height;
    int nr_channels;
    stbi_set_flip_vertically_on_load(true);
    GLuint opengl_texture_id;
    if (unsigned char *data = stbi_load(
      texture_path.c_str(),
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
                   GL_RGBA,
                   tex_width,
                   tex_height,
                   0,
                   GL_RGBA, // consider making this configurable
                   GL_UNSIGNED_BYTE,
                   data);
      glGenerateMipmap(GL_TEXTURE_2D);

      // free the memory after we are done creating texture
      stbi_image_free(data);
    } else
    {
      std::cout << "Failed to load texture" << std::endl;
      return -1;
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    _texture_references[texture_id] = opengl_texture_id;

    return texture_id;
  }

  void OpenGL_RenderSystem::DestroyTexture(const int texture_id)
  {
    const auto opengl_texture_id = _texture_references[texture_id];
    glDeleteTextures(1, &opengl_texture_id);
  }
} // core
