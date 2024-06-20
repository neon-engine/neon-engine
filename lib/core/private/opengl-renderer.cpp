#include "opengl-renderer.hpp"

#include <iostream>
#include <stdexcept>
#include <GL/gl3w.h>

namespace core {
  OpenGl_Renderer::OpenGl_Renderer(const Window_Info &window_info) : Renderer(window_info) { }

  void OpenGl_Renderer::Initialize()
  {
    if (gl3wInit())
    {
      throw std::runtime_error("Failed to initialize gl3w");
    }

    glEnable(GL_DEPTH_TEST);

    int num_vertex_attributes_supported;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &num_vertex_attributes_supported);
    std::cout << "Maximum number of vertex attributes supported: " << num_vertex_attributes_supported << std::endl;

    glViewport(0, 0, _window_info.width, _window_info.height);
  }
} // core