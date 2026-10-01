#ifndef UI_SURFACE_VIEW_HPP
#define UI_SURFACE_VIEW_HPP

#include <string>

namespace neon
{
  /// Shows a user interface on a surface of its own for as long as the
  /// entity that carries it is there: a screen in the world, a sign, a
  /// terminal.
  ///
  ///     UiSurface:
  ///       ui: assets://ui/terminal.ui.yml
  ///       size: [1024, 768]
  ///       name: terminal
  ///
  /// What shows the surface names it. A model does so with the texture
  /// `surface://terminal`, which is how the entity itself or any other
  /// carries the user interface on its surface.
  struct UiSurfaceView
  {
    /// Virtual path of the file.
    std::string ui;

    /// What the surface is called. Given once.
    std::string name;

    /// The size of the surface in pixels.
    int width = 1024;
    int height = 1024;

    /// How much larger everything on the surface is drawn.
    float scale = 1.0f;

    /// How near the player has to be to point at it, in units of the
    /// world. 0 lets it be pointed at from anywhere.
    float reach = 3.0f;

    /// What the user interface knows the surface and the file as once they
    /// are shown. -1 before that.
    int surface = -1;
    int document = -1;

    /// Whether showing it was tried, so that a surface that cannot be made
    /// is not tried again in every frame.
    bool is_tried = false;
  };
} // neon

#endif //UI_SURFACE_VIEW_HPP
