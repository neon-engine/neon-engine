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

  int OpenGl_RenderSystem::InitGeometry()
  {
    const auto geometry_id = _geometry_index++;
    std::cout << "Initializing mesh: " << geometry_id << std::endl;

    // glGenVertexArrays(1, &_vao);
    // glGenBuffers(1, &_vbo);
    // glGenBuffers(1, &_ebo);
    //
    // glBindVertexArray(_vao);
    //
    // glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    // glBufferData(GL_ARRAY_BUFFER,
    //              static_cast<GLsizeiptr>(_vertices.size() * sizeof(float)),
    //              &_vertices[0],
    //              GL_STATIC_DRAW);
    //
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER,
    //              static_cast<GLsizeiptr>(_indices.size() * sizeof(int)),
    //              &_indices[0],
    //              GL_STATIC_DRAW);
    //
    // // position
    // glVertexAttribPointer(
    //   0,
    //   // "layout (location = x)" in the vertex shader
    //   3,
    //   // how many components there are per vertex
    //   GL_FLOAT,
    //   // What type these components are
    //   GL_FALSE,
    //   // GL_TRUE means the values should be normalized. GL_FALSE means they shouldn't
    //   _stride,
    //   static_cast<void *>(nullptr));
    // glEnableVertexAttribArray(0);
    //
    // // colors
    // glVertexAttribPointer(
    //   1,
    //   3,
    //   GL_FLOAT,
    //   GL_FALSE,
    //   _stride,
    //   reinterpret_cast<void *>(3 * sizeof(float)));
    // glEnableVertexAttribArray(1);
    //
    // // texture coordinates
    // glVertexAttribPointer(
    //   2,
    //   2,
    //   GL_FLOAT,
    //   GL_FALSE,
    //   _stride,
    //   reinterpret_cast<void *>(6 * sizeof(float)));
    // glEnableVertexAttribArray(2);
    //
    // // We've sent the vertex data over to OpenGL, but there's still something missing.
    // // In what order should it draw those vertices? That's why we'll need a GL_ELEMENT_ARRAY_BUFFER for this.
    //
    //
    // // Unbind the currently bound buffer so that we don't accidentally make unwanted changes to it.
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    //
    // // Unbind the VAO now, so we don't accidentally tamper with it.
    // // NOTE: You must NEVER unbind the element array buffer associated with a VAO!
    // glBindVertexArray(0);
    return geometry_id;
  }

  void OpenGl_RenderSystem::DestroyGeometry(int geometry_id) {}

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
