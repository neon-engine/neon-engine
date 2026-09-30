#ifndef WINDOW_METRICS_HPP
#define WINDOW_METRICS_HPP

namespace neon
{
  /// The three sizes of a window, which must not be confused.
  ///
  /// A platform counts a window in points, which are the same size to the
  /// eye on every display. What is drawn to is counted in pixels. On a
  /// display of high density a point is more than one pixel: two on a
  /// Retina display, one and a quarter on a laptop that is set to 125 %.
  /// The density is how many pixels a point has.
  ///
  /// The pointer arrives in points and everything that is drawn is placed
  /// in pixels, so whatever takes the one for the other is off by the
  /// density.
  struct WindowMetrics
  {
    /// The size of the window in points.
    int point_width = 0;
    int point_height = 0;

    /// The size of what is drawn to in pixels.
    int pixel_width = 0;
    int pixel_height = 0;

    /// Pixels for each point along the width and along the height. They
    /// differ by a part of a pixel where the platform rounds a size, which
    /// is why the pointer is scaled along each side by itself.
    [[nodiscard]] double ScaleX() const
    {
      return point_width > 0 && pixel_width > 0 ? static_cast<double>(pixel_width) / point_width : 1.0;
    }

    [[nodiscard]] double ScaleY() const
    {
      return point_height > 0 && pixel_height > 0 ? static_cast<double>(pixel_height) / point_height : 1.0;
    }

    /// The density: pixels for each point. It is taken along the height,
    /// which is the side a user interface is fitted to most often.
    [[nodiscard]] double Density() const
    {
      return ScaleY();
    }

    /// A place in points as a place in pixels.
    [[nodiscard]] double ToPixelsX(const double points) const
    {
      return points * ScaleX();
    }

    [[nodiscard]] double ToPixelsY(const double points) const
    {
      return points * ScaleY();
    }

    /// A place in pixels as a place in points.
    [[nodiscard]] double ToPointsX(const double pixels) const
    {
      return pixels / ScaleX();
    }

    [[nodiscard]] double ToPointsY(const double pixels) const
    {
      return pixels / ScaleY();
    }

    bool operator==(const WindowMetrics &other) const = default;
  };

  /// The shape of the cursor, named as `cursor` of CSS names it.
  enum class CursorShape
  {
    Default = 0,
    Pointer,
    Text,
    Wait,
    Progress,
    Crosshair,
    Move,
    NotAllowed,
    EwResize,
    NsResize,
    NeswResize,
    NwseResize,
    Grab,
    Grabbing,

    /// No cursor is shown.
    None
  };
} // neon

#endif //WINDOW_METRICS_HPP
