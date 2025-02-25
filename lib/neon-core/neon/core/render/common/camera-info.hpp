#ifndef CAMERA_INFO_HPP
#define CAMERA_INFO_HPP

#include <glm/glm.hpp>
#include "render-target.hpp"


namespace core
{
  struct CameraInfo
  {
    RenderTarget target{RenderTarget::Window};
    float fov{45.0f};
    glm::mat4 view{1.0f};
    float near{0.1f};
    float far{1000.f};
  };
} // core

#endif //CAMERA_INFO_HPP
