#include "ui-image-paint.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace neon
{
  namespace
  {
    // more tiles than this are not drawn, and the image is stretched
    constexpr int max_tiles = 4096;
  }

  TextureFilter2D FilterOf(const UiStyle &style)
  {
    return style.image_rendering == UiImageRendering::Pixelated
      ? TextureFilter2D::Pixelated
      : TextureFilter2D::Smooth;
  }

  bool CutToBox(const UiRectangle &box, UiRectangle &place, UiRectangle &part)
  {
    if (place.IsEmpty()) { return false; }

    UiRectangle cut = place;
    cut.left = std::max(place.left, box.left);
    cut.top = std::max(place.top, box.top);
    cut.right = std::min(place.right, box.right);
    cut.bottom = std::min(place.bottom, box.bottom);

    if (cut.IsEmpty()) { return false; }

    const float across = part.Width() / place.Width();
    const float down = part.Height() / place.Height();

    UiRectangle shown = part;
    shown.left = part.left + (cut.left - place.left) * across;
    shown.right = part.right - (place.right - cut.right) * across;
    shown.top = part.top + (cut.top - place.top) * down;
    shown.bottom = part.bottom - (place.bottom - cut.bottom) * down;

    place = cut;
    part = shown;
    return true;
  }

  void FitImage(
    const UiStyle &style,
    const float width,
    const float height,
    const UiRectangle &box,
    const float scale,
    UiRectangle &place,
    UiRectangle &part)
  {
    place = box;
    part = {0.0f, 0.0f, 1.0f, 1.0f};

    if (width <= 0.0f || height <= 0.0f || box.IsEmpty()) { return; }

    const float image_ratio = width / height;
    const float box_ratio = box.Width() / box.Height();

    // where the image lies in the room that is left over, from 0 to 1
    // for a percentage
    const auto place_x = [&](const float room) { return std::round(ResolvePlace(style.object_position.x, room, scale)); };
    const auto place_y = [&](const float room) { return std::round(ResolvePlace(style.object_position.y, room, scale)); };

    UiObjectFit fit = style.object_fit;

    if (fit == UiObjectFit::ScaleDown)
    {
      const bool fits = width <= box.Width() + 0.5f && height <= box.Height() + 0.5f;
      fit = fits ? UiObjectFit::None : UiObjectFit::Contain;
    }

    switch (fit)
    {
      case UiObjectFit::Contain:
      {
        // the whole image, as large as it fits
        if (image_ratio > box_ratio)
        {
          const float drawn = std::round(box.Width() / image_ratio);
          place.top = box.top + place_y(box.Height() - drawn);
          place.bottom = place.top + drawn;
        } else
        {
          const float drawn = std::round(box.Height() * image_ratio);
          place.left = box.left + place_x(box.Width() - drawn);
          place.right = place.left + drawn;
        }
        break;
      }
      case UiObjectFit::Cover:
      {
        // the whole box, with what does not fit cut off
        const auto share = [](const LayoutLength &position)
        {
          return position.unit == LayoutLength::Unit::Percent ? position.value / 100.0f : 0.5f;
        };

        if (image_ratio > box_ratio)
        {
          const float shown = box_ratio / image_ratio;
          part.left = (1.0f - shown) * share(style.object_position.x);
          part.right = part.left + shown;
        } else
        {
          const float shown = image_ratio / box_ratio;
          part.top = (1.0f - shown) * share(style.object_position.y);
          part.bottom = part.top + shown;
        }
        break;
      }
      case UiObjectFit::None:
      {
        // as large as it is, and cut off where it sticks out
        const float drawn_width = std::round(width);
        const float drawn_height = std::round(height);

        place.left = box.left + place_x(box.Width() - drawn_width);
        place.top = box.top + place_y(box.Height() - drawn_height);
        place.right = place.left + drawn_width;
        place.bottom = place.top + drawn_height;

        if (!CutToBox(box, place, part)) { place = {box.left, box.top, box.left, box.top}; }
        break;
      }
      default:
        break;
    }
  }

  void PaintBackgroundImage(
    UiPainter &painter,
    const UiFrame &frame,
    const UiStyle &style,
    const UiBoxPaint &box,
    const UiRectangle &border_box,
    const float opacity,
    const std::string &element)
  {
    if (border_box.IsEmpty()) { return; }

    const UiBackgroundSize &size = style.background_size;
    const bool is_stretched =
      size.kind == UiBackgroundSize::Kind::Lengths &&
      size.width == LayoutLength::Percent(100.0f) && size.height == LayoutLength::Percent(100.0f);

    // An image of shapes is drawn at the size it has on the screen, which
    // is known for one that is stretched over the box.
    const UiImage image = frame.resources->GetImageFor(
      style.background_image,
      is_stretched ? border_box.Width() : 0.0f,
      is_stretched ? border_box.Height() : 0.0f,
      frame.scale,
      element);

    if (image.texture == No_Texture || image.width <= 0 || image.height <= 0) { return; }

    const float natural_width = image.NaturalWidth() * frame.scale;
    const float natural_height = image.NaturalHeight() * frame.scale;
    const float ratio = natural_width / natural_height;

    float width = natural_width;
    float height = natural_height;

    switch (size.kind)
    {
      case UiBackgroundSize::Kind::Cover:
      case UiBackgroundSize::Kind::Contain:
      {
        const bool is_wider = ratio > border_box.Width() / border_box.Height();
        const bool fills_width = size.kind == UiBackgroundSize::Kind::Contain ? is_wider : !is_wider;

        if (fills_width)
        {
          width = border_box.Width();
          height = width / ratio;
        } else
        {
          height = border_box.Height();
          width = height * ratio;
        }
        break;
      }
      case UiBackgroundSize::Kind::Lengths:
      {
        const auto resolve = [&frame](const LayoutLength &length, const float whole)
        {
          return length.unit == LayoutLength::Unit::Percent
            ? whole * length.value / 100.0f
            : length.value * frame.scale;
        };

        // a side that is `auto` follows the other one
        if (!size.width.IsAuto()) { width = resolve(size.width, border_box.Width()); }
        if (!size.height.IsAuto()) { height = resolve(size.height, border_box.Height()); }
        if (size.width.IsAuto()) { width = height * ratio; }
        if (size.height.IsAuto()) { height = width / ratio; }
        break;
      }
      default:
        break;
    }

    width = std::round(width);
    height = std::round(height);
    if (width < 1.0f || height < 1.0f) { return; }

    const float first_x =
      border_box.left + std::round(ResolvePlace(style.background_position.x, border_box.Width() - width, frame.scale));
    const float first_y =
      border_box.top + std::round(ResolvePlace(style.background_position.y, border_box.Height() - height, frame.scale));

    const bool repeats_x = style.background_repeat == UiBackgroundRepeat::Repeat ||
                           style.background_repeat == UiBackgroundRepeat::RepeatX;
    const bool repeats_y = style.background_repeat == UiBackgroundRepeat::Repeat ||
                           style.background_repeat == UiBackgroundRepeat::RepeatY;

    // the tiles that reach into the box, counted from the one that is
    // placed
    const auto tiles = [](const bool repeats, const float first, const float length, const float from, const float to)
    {
      if (!repeats) { return std::pair{0, 0}; }

      const int before = static_cast<int>(std::ceil((first - from) / length));
      const int after = static_cast<int>(std::ceil((to - (first + length)) / length));
      return std::pair{-std::max(before, 0), std::max(after, 0)};
    };

    auto [from_x, to_x] = tiles(repeats_x, first_x, width, border_box.left, border_box.right);
    auto [from_y, to_y] = tiles(repeats_y, first_y, height, border_box.top, border_box.bottom);

    painter.SetFilter(FilterOf(style));

    if (static_cast<long long>(to_x - from_x + 1) * (to_y - from_y + 1) > max_tiles)
    {
      frame.resources->WarnOnce(
        "tiles of " + style.background_image + " in " + element,
        std::format(
          "'background_image' of {} would be drawn more than {} times, and is stretched over the element "
          "in place of that. {} is {} by {} pixels where it is drawn",
          element, max_tiles, style.background_image, width, height));

      box.PaintImage(painter, image, border_box, {0.0f, 0.0f, 1.0f, 1.0f}, opacity);
      painter.SetFilter(TextureFilter2D::Smooth);
      return;
    }

    for (int row = from_y; row <= to_y; row++)
    {
      for (int column = from_x; column <= to_x; column++)
      {
        UiRectangle place;
        place.left = first_x + static_cast<float>(column) * width;
        place.top = first_y + static_cast<float>(row) * height;
        place.right = place.left + width;
        place.bottom = place.top + height;

        UiRectangle part{0.0f, 0.0f, 1.0f, 1.0f};
        if (!CutToBox(border_box, place, part)) { continue; }

        box.PaintImage(painter, image, place, part, opacity);
      }
    }

    painter.SetFilter(TextureFilter2D::Smooth);
  }

  void PaintNineSlice(
    UiPainter &painter,
    const UiImage &image,
    const UiRectangle &rectangle,
    const LayoutEdges<float> &slice,
    const LayoutEdges<float> &widths,
    const UiBorderImageRepeat repeat,
    const Color &tint)
  {
    if (image.texture == No_Texture || image.width <= 0 || image.height <= 0 || rectangle.IsEmpty()) { return; }

    const auto image_width = static_cast<float>(image.width);
    const auto image_height = static_cast<float>(image.height);

    // how far the corners reach into the image, held to the image
    const float slice_left = std::clamp(slice.left, 0.0f, image_width);
    const float slice_right = std::clamp(slice.right, 0.0f, image_width - slice_left);
    const float slice_top = std::clamp(slice.top, 0.0f, image_height);
    const float slice_bottom = std::clamp(slice.bottom, 0.0f, image_height - slice_top);

    float left = std::max(0.0f, widths.left);
    float right = std::max(0.0f, widths.right);
    float top = std::max(0.0f, widths.top);
    float bottom = std::max(0.0f, widths.bottom);

    // corners that overlap are made smaller by the same factor, all four
    float factor = 1.0f;
    if (left + right > rectangle.Width()) { factor = std::min(factor, rectangle.Width() / (left + right)); }
    if (top + bottom > rectangle.Height()) { factor = std::min(factor, rectangle.Height() / (top + bottom)); }

    left = std::round(left * factor);
    right = std::round(right * factor);
    top = std::round(top * factor);
    bottom = std::round(bottom * factor);

    const float xs[4] = {rectangle.left, rectangle.left + left, rectangle.right - right, rectangle.right};
    const float ys[4] = {rectangle.top, rectangle.top + top, rectangle.bottom - bottom, rectangle.bottom};
    const float us[4] = {0.0f, slice_left / image_width, 1.0f - slice_right / image_width, 1.0f};
    const float vs[4] = {0.0f, slice_top / image_height, 1.0f - slice_bottom / image_height, 1.0f};

    // How much larger a part is drawn than it is in the image. The edges
    // at the top and at the left say so for the middle, and an edge that
    // is not there leaves the part as large as it is.
    const float scale_x = slice_top > 0.0f ? top / slice_top : (slice_bottom > 0.0f ? bottom / slice_bottom : 1.0f);
    const float scale_y = slice_left > 0.0f ? left / slice_left : (slice_right > 0.0f ? right / slice_right : 1.0f);

    const float middle_width = image_width - slice_left - slice_right;
    const float middle_height = image_height - slice_top - slice_bottom;

    // The tiles along one side: where the first starts and how long each
    // is. They are laid out from the middle, as CSS does, so that what is
    // cut off is the same at both ends.
    const auto lay_out = [repeat](const float from, const float to, const float length, float &first, float &each)
    {
      const float room = to - from;
      each = std::max(length, 1.0f);

      if (repeat == UiBorderImageRepeat::Round)
      {
        // made larger or smaller so that a whole number fits
        const float count = std::max(1.0f, std::round(room / each));
        each = room / count;
        first = from;
        return;
      }

      const float count = std::ceil(room / each);
      first = from + (room - count * each) / 2.0f;
    };

    const auto draw_tiles = [&](const int column, const int row, const bool tiles_x, const bool tiles_y)
    {
      const UiRectangle cell{xs[column], ys[row], xs[column + 1], ys[row + 1]};
      const UiRectangle whole{us[column], vs[row], us[column + 1], vs[row + 1]};
      if (cell.IsEmpty()) { return; }

      float first_x = cell.left;
      float first_y = cell.top;
      float each_x = cell.Width();
      float each_y = cell.Height();

      if (tiles_x) { lay_out(cell.left, cell.right, middle_width * scale_x, first_x, each_x); }
      if (tiles_y) { lay_out(cell.top, cell.bottom, middle_height * scale_y, first_y, each_y); }

      const int count_x = tiles_x ? static_cast<int>(std::ceil((cell.right - first_x) / each_x - 0.001f)) : 1;
      const int count_y = tiles_y ? static_cast<int>(std::ceil((cell.bottom - first_y) / each_y - 0.001f)) : 1;

      if (static_cast<long long>(count_x) * count_y > max_tiles)
      {
        painter.DrawImage(image, cell, whole, tint);
        return;
      }

      for (int y = 0; y < count_y; y++)
      {
        for (int x = 0; x < count_x; x++)
        {
          UiRectangle place;
          place.left = first_x + static_cast<float>(x) * each_x;
          place.top = first_y + static_cast<float>(y) * each_y;
          place.right = place.left + each_x;
          place.bottom = place.top + each_y;

          UiRectangle part = whole;
          if (!CutToBox(cell, place, part)) { continue; }

          painter.DrawImage(image, place, part, tint);
        }
      }
    };

    for (int row = 0; row < 3; row++)
    {
      for (int column = 0; column < 3; column++)
      {
        // the corners are drawn as they are, the edges along their side,
        // and the middle both ways
        const bool tiles_x = column == 1 && repeat != UiBorderImageRepeat::Stretch;
        const bool tiles_y = row == 1 && repeat != UiBorderImageRepeat::Stretch;

        draw_tiles(column, row, tiles_x, tiles_y);
      }
    }
  }
} // neon
