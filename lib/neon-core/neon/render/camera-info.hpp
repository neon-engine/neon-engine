#ifndef CAMERA_INFO_HPP
#define CAMERA_INFO_HPP

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <neon/common/color.hpp>
#include "render-target.hpp"


namespace neon
{
  struct CameraInfo
  {
    RenderTarget target{RenderTarget::Window};
    float fov{45.0f};
    glm::mat4 view{1.0f};
    float near{0.1f};
    float far{1000.f};

    /// For a camera whose target is a texture: what the texture is called,
    /// and its size in pixels. A model shows it as the texture
    /// `surface://` and the name.
    std::string texture;
    int width{512};
    int height{512};

    /// The most levels of smaller copies the texture has: 1 for none, 0 for
    /// as many as its size allows. See RenderTargetOptions.
    int mipmaps{0};

    /// The effects that are run over what the camera drew, as the paths of
    /// their shaders: on the light of the scene, before the tonemapper, and
    /// on the colours a screen is given, after it. See
    /// RenderContext::SetEffects().
    std::vector<std::string> effects;
    std::vector<std::string> screen_effects;

    /// What the texture is cleared to before the camera draws.
    Color clear{0.0f, 0.0f, 0.0f, 1.0f};
  };
} // neon

#endif //CAMERA_INFO_HPP
