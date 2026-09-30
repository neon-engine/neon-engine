#ifndef UI_SCROLLBARS_HPP
#define UI_SCROLLBARS_HPP

#include "ui-painter.hpp"
#include "ui-style.hpp"

namespace neon
{
  class UiElement;
  struct UiFrame;

  /// The scrollbars of an element whose content is larger than its box.
  /// They are drawn by the engine and styled by CSS:
  ///
  ///   - `scrollbar_width` says how wide they are, and `none` hides them
  ///   - `scrollbar_color` says the colours of the thumb and of the track
  ///   - `::scrollbar-track` and `::scrollbar-thumb` of a style sheet reach
  ///     the two parts, of which `background_color` is what is drawn
  ///
  /// A scrollbar lies at the right and at the bottom of the padding box.
  /// With `overflow: scroll` its room is kept free of what is inside. With
  /// `overflow: auto` it lies on top of what is inside, and is only there
  /// while there is something to scroll.
  class UiScrollbars
  {
  public:
    enum class Axis
    {
      Horizontal = 0,
      Vertical
    };

    /// In units of the file.
    static constexpr float width_auto = 12.0f;
    static constexpr float width_thin = 8.0f;

    /// The least a thumb is long, so that it can be taken hold of.
    static constexpr float least_thumb = 24.0f;

    /// How wide the scrollbars of a style are. 0 for one that has none.
    [[nodiscard]] static float WidthOf(const UiStyle &style);

    /// What an element is cut off at: its padding box along the sides that
    /// cut off, and nothing along the others. In pixels.
    [[nodiscard]] static UiRectangle ClipOf(const UiStyle &style, const UiRectangle &padding_box);

    /// Whether the element shows a scrollbar along a side.
    [[nodiscard]] static bool IsShown(const UiElement &element, Axis axis);

    /// What the thumb is dragged along, in units of the file. Empty for a
    /// scrollbar that is not shown.
    [[nodiscard]] static UiRectangle TrackOf(const UiElement &element, Axis axis);

    /// What is dragged, in units of the file.
    [[nodiscard]] static UiRectangle ThumbOf(const UiElement &element, Axis axis);

    /// How far the element is scrolled when its thumb was dragged to a
    /// place: `thumb_start` is where the thumb starts along its track, in
    /// units of the file from the start of the track.
    [[nodiscard]] static float ScrollFor(const UiElement &element, Axis axis, float thumb_start);

    static void Paint(const UiElement &element, UiPainter &painter, const UiFrame &frame, float opacity);
  };
} // neon

#endif //UI_SCROLLBARS_HPP
