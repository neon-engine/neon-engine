#ifndef GL_RENDERER_HPP
#define GL_RENDERER_HPP



#include "core/window-info.hpp"

namespace core
{
  class OpenGl_Renderer final
  {
  public:
    static void Initialize(const Window_Info &window_info)
    {
      // if (gl3wInit())
      // {
      //   throw std::runtime_error("Failed to initialize gl3w");
      // }
      //
      // glEnable(GL_DEPTH_TEST);
      //
      // int num_vertex_attributes_supported;
      // glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &num_vertex_attributes_supported);
      // std::cout << "Maximum number of vertex attributes supported: " << num_vertex_attributes_supported << std::endl;
      //
      // glViewport(0, 0, window_info.width, window_info.height);
    }
  };
} // core

#endif //GL_RENDERER_HPP
