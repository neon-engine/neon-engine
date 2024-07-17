#include "opengl-render-system.hpp"

#include <iostream>
#include <stdexcept>

#include <GL/gl3w.h>

namespace core
{
  void OpenGl_RenderSystem::Initialize()
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

  void OpenGl_RenderSystem::CleanUp()
  {
    std::cout << "Cleaning up OpenGL" << std::endl;
  }

  void OpenGl_RenderSystem::RenderFrame()
  {
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); for wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw the scene here
  }

  RenderContext* OpenGl_RenderSystem::GetContext()
  {
    return this;
  }

  int OpenGl_RenderSystem::InitGeometry(
    const std::vector<float> &vertices,
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
    GLuint ebo = 0;

    // this can be configurable to make data more compact, but hard coding it to 8 * float for now
    constexpr GLint stride = 8 * sizeof(GLfloat);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 &vertices[0],
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(int)),
                 &indices[0],
                 GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(
      0,
      // "layout (location = x)" in the vertex shader
      3,
      // how many components there are per vertex
      GL_FLOAT,
      // What type these components are
      GL_FALSE,
      // GL_TRUE means the values should be normalized. GL_FALSE means they shouldn't
      stride,
      static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);

    // indices
    glVertexAttribPointer(
      1,
      3,
      GL_FLOAT,
      GL_FALSE,
      stride,
      reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // texture coordinates
    glVertexAttribPointer(
      2,
      2,
      GL_FLOAT,
      GL_FALSE,
      stride,
      reinterpret_cast<void *>(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // We've sent the vertex data over to OpenGL, but there's still something missing.
    // In what order should it draw those vertices? That's why we'll need a GL_ELEMENT_ARRAY_BUFFER for this.


    // Unbind the currently bound buffer so that we don't accidentally make unwanted changes to it.
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Unbind the VAO now, so we don't accidentally tamper with it.
    // NOTE: You must NEVER unbind the element array buffer associated with a VAO!
    glBindVertexArray(0);
    _geometry_references[geometry_id] = std::make_tuple(vao, vbo, ebo, stride);
    return geometry_id;
  }

  void OpenGl_RenderSystem::DestroyGeometry(int geometry_id)
  {
    const auto tuple = _geometry_references[geometry_id];
    const auto vao = std::get<0>(tuple);
    const auto vbo = std::get<1>(tuple);
    const auto ebo = std::get<2>(tuple);

    glDeleteBuffers(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
  }

  int OpenGl_RenderSystem::InitShader()
  {
    const auto shader_id = _shader_index++;
    std::cout << "Initializing shader: " << shader_id << std::endl;
    return shader_id;
  }

  void OpenGl_RenderSystem::DestroyShader(int shader_id) {}

  int OpenGl_RenderSystem::InitTexture()
  {
    const auto texture_id = _texture_index++;
    std::cout << "Initializing texture: " << texture_id << std::endl;
    return texture_id;
  }

  void OpenGl_RenderSystem::DestroyTexture(int texture_id) {}

} // core
