#ifndef RENDER_CAPABILITIES_HPP
#define RENDER_CAPABILITIES_HPP

#include <string>
#include <vector>

#include "api-version.hpp"

namespace neon
{
  /// How finished frames are handed to the display, which is what a menu
  /// offers as the choices of vertical sync.
  enum class PresentMode
  {
    /// Shown at once, in the middle of what the display is showing. The
    /// least delay, and lines where the picture tears. Vertical sync off.
    Immediate = 0,

    /// Shown when the display starts its next picture, and a newer frame
    /// replaces one that waits. No tearing, and the game is not held back.
    Mailbox,

    /// Shown when the display starts its next picture, and the game waits
    /// for it. Vertical sync on. Every graphics card offers it.
    Fifo,

    /// As Fifo, but a frame that is late is shown at once. Vertical sync
    /// that gives way when the game cannot keep up.
    FifoRelaxed
  };

  /// What the graphics card and its driver can do, for the parts of the
  /// engine that offer choices, such as a settings menu that should offer
  /// only what works.
  ///
  /// The render system fills it in when it starts, and it does not change
  /// after. It holds no type of a graphics API, so that what reads it works
  /// with every renderer.
  struct RenderCapabilities
  {
    /// The version of the API that is rendered with: the highest that was
    /// asked for and that the driver and the graphics card offer.
    ApiVersion api_version;

    /// The version the graphics card offers, which can be above the one
    /// that is rendered with.
    ApiVersion device_api_version;

    /// The name of the graphics card, such as `Apple M2 Pro`.
    std::string device_name;

    /// The widest and highest texture, in pixels.
    int max_texture_size = 0;

    /// The most samples of a pixel for anti-aliasing, of color and depth
    /// alike. 1 is none.
    int max_samples = 1;

    /// The most samples of anisotropic filtering. 0 when the graphics card
    /// cannot filter that way, and the choice should not be offered.
    float max_anisotropy = 0.0f;

    /// How frames can be shown, which are the choices of vertical sync.
    /// Empty without a window, where nothing is shown.
    std::vector<PresentMode> present_modes;

    /// Whether the scene can be lit in an image of 16 bit floating point
    /// numbers for red, green, blue, and alpha. The renderer needs it.
    bool has_scene_format = false;

    /// Whether textures in sRGB can be read and filtered. The renderer
    /// needs it.
    bool has_srgb_textures = false;

    /// Whether one image can be seen as sRGB and as plain bytes. Render
    /// targets need it.
    bool has_mutable_format_views = false;
  };
} // neon

#endif //RENDER_CAPABILITIES_HPP
