#include "ui-scrollbars.hpp"

#include <algorithm>
#include <cmath>

#include "ui-element.hpp"

namespace neon
{
  // Helpers of UiElement, for this file alone.
  namespace
  {
    constexpr float far_away = 1.0e7f;

    float ViewportOf(const UiElement &element, const UiScrollbars::Axis axis)
    {
      const UiRectangle box = element.GetPaddingBox();
      return axis == UiScrollbars::Axis::Horizontal ? box.Width() : box.Height();
    }

    float ContentOf(const UiElement &element, const UiScrollbars::Axis axis)
    {
      return axis == UiScrollbars::Axis::Horizontal ? element.GetContentWidth() : element.GetContentHeight();
    }

    float LengthOf(const UiRectangle &rectangle, const UiScrollbars::Axis axis)
    {
      return axis == UiScrollbars::Axis::Horizontal ? rectangle.Width() : rectangle.Height();
    }

    float ThumbLength(const UiElement &element, const UiScrollbars::Axis axis, const float track)
    {
      const float content = ContentOf(element, axis);
      if (content <= 0.0f) { return track; }

      const float length = track * std::min(1.0f, ViewportOf(element, axis) / content);
      return std::min(track, std::max(UiScrollbars::least_thumb, length));
    }
  }

  float UiScrollbars::WidthOf(const UiStyle &style)
  {
    if (!style.ScrollsX() && !style.ScrollsY()) { return 0.0f; }

    switch (style.scrollbar_width)
    {
      case UiScrollbarWidth::Thin: return width_thin;
      case UiScrollbarWidth::None: return 0.0f;
      default: return width_auto;
    }
  }

  UiRectangle UiScrollbars::ClipOf(const UiStyle &style, const UiRectangle &padding_box)
  {
    UiRectangle clip{-far_away, -far_away, far_away, far_away};

    if (style.ClipsX())
    {
      clip.left = padding_box.left;
      clip.right = padding_box.right;
    }

    if (style.ClipsY())
    {
      clip.top = padding_box.top;
      clip.bottom = padding_box.bottom;
    }

    return clip;
  }

  bool UiScrollbars::IsShown(const UiElement &element, const Axis axis)
  {
    const UiStyle &style = element.GetStyle();
    if (WidthOf(style) <= 0.0f) { return false; }

    const UiOverflow overflow = axis == Axis::Horizontal ? style.OverflowX() : style.OverflowY();

    if (overflow == UiOverflow::Scroll) { return true; }
    if (overflow != UiOverflow::Auto) { return false; }

    return axis == Axis::Horizontal ? element.CanScrollX() : element.CanScrollY();
  }

  UiRectangle UiScrollbars::TrackOf(const UiElement &element, const Axis axis)
  {
    if (!IsShown(element, axis)) { return {}; }

    const float width = WidthOf(element.GetStyle());
    const UiRectangle box = element.GetPaddingBox();

    // where the two meet, neither is
    const bool has_other = IsShown(element, axis == Axis::Horizontal ? Axis::Vertical : Axis::Horizontal);
    const float corner = has_other ? width : 0.0f;

    if (axis == Axis::Vertical)
    {
      return {box.right - width, box.top, box.right, std::max(box.top, box.bottom - corner)};
    }

    return {box.left, box.bottom - width, std::max(box.left, box.right - corner), box.bottom};
  }

  UiRectangle UiScrollbars::ThumbOf(const UiElement &element, const Axis axis)
  {
    const UiRectangle track = TrackOf(element, axis);
    if (track.IsEmpty()) { return {}; }

    const float track_length = LengthOf(track, axis);
    const float thumb_length = ThumbLength(element, axis, track_length);

    const float most = axis == Axis::Horizontal ? element.GetMaxScrollX() : element.GetMaxScrollY();
    const float scrolled = axis == Axis::Horizontal ? element.GetScrollX() : element.GetScrollY();

    const float start = most > 0.0f ? (track_length - thumb_length) * scrolled / most : 0.0f;

    if (axis == Axis::Vertical)
    {
      return {track.left, track.top + start, track.right, track.top + start + thumb_length};
    }

    return {track.left + start, track.top, track.left + start + thumb_length, track.bottom};
  }

  float UiScrollbars::ScrollFor(const UiElement &element, const Axis axis, const float thumb_start)
  {
    const UiRectangle track = TrackOf(element, axis);
    if (track.IsEmpty()) { return 0.0f; }

    const float track_length = LengthOf(track, axis);
    const float thumb_length = ThumbLength(element, axis, track_length);
    const float room = track_length - thumb_length;

    const float most = axis == Axis::Horizontal ? element.GetMaxScrollX() : element.GetMaxScrollY();
    if (room <= 0.0f || most <= 0.0f) { return 0.0f; }

    return std::clamp(thumb_start / room, 0.0f, 1.0f) * most;
  }

  void UiScrollbars::Paint(
    const UiElement &element,
    UiPainter &painter,
    const UiFrame &frame,
    const float opacity)
  {
    const UiStyle &style = element.GetStyle();
    if (!style.ScrollsX() && !style.ScrollsY()) { return; }

    for (const Axis axis : {Axis::Vertical, Axis::Horizontal})
    {
      const UiRectangle track = TrackOf(element, axis);
      if (track.IsEmpty()) { continue; }

      const Color &track_color = element.GetPartStyle("scrollbar-track").background_color;
      const UiStyle &thumb_style = element.GetPartStyle("scrollbar-thumb");
      const Color &thumb_color = thumb_style.background_color;

      painter.FillRectangle(
        ToPixels(track, frame.scale),
        {track_color.r, track_color.g, track_color.b, track_color.a * opacity});

      painter.FillRectangle(
        ToPixels(ThumbOf(element, axis), frame.scale),
        {thumb_color.r, thumb_color.g, thumb_color.b, thumb_color.a * opacity * thumb_style.opacity});
    }
  }
} // neon
