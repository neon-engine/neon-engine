#include "ui-image.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace neon
{
  void UiImageElement::ApplyDefaults(UiStyle &style) const
  {
    style.pointer_events = UiPointerEvents::None;

    // an image keeps its size next to others, which shrink in its place
    style.layout.flex_shrink = 0.0f;
  }

  void UiImageElement::ReadAttributes(const DataReader &reader)
  {
    if (!reader.Read("src", _source) && !reader.Has("src"))
    {
      reader.Report(std::format(
                      "{} has no 'src', where the virtual path of an image was expected", reader.GetWhere()));
    }
  }

  bool UiImageElement::HasContent() const
  {
    return true;
  }

  LayoutSize UiImageElement::Measure(const UiFrame &frame, float available_width, float available_height)
  {
    if (_source.empty()) { return {}; }

    // a pixel of the image is a unit of the file, as it is a pixel of CSS
    const UiImage image = frame.resources->GetImage(_source);
    return {static_cast<float>(image.width), static_cast<float>(image.height)};
  }

  void UiImageElement::PaintContent(
    UiPainter &painter,
    const UiFrame &frame,
    const UiRectangle &content_box,
    const float opacity)
  {
    if (_source.empty() || content_box.IsEmpty()) { return; }

    const UiImage image = frame.resources->GetImage(_source);
    if (image.texture == No_Texture || image.width <= 0 || image.height <= 0) { return; }

    UiRectangle box = content_box;
    UiRectangle part{0.0f, 0.0f, 1.0f, 1.0f};

    const float image_ratio = static_cast<float>(image.width) / static_cast<float>(image.height);
    const float box_ratio = content_box.Width() / content_box.Height();

    switch (GetStyle().object_fit)
    {
      case UiObjectFit::Contain:
      {
        // the whole image, as large as it fits, in the middle
        if (image_ratio > box_ratio)
        {
          const float height = std::round(content_box.Width() / image_ratio);
          box.top = content_box.top + std::round((content_box.Height() - height) / 2.0f);
          box.bottom = box.top + height;
        } else
        {
          const float width = std::round(content_box.Height() * image_ratio);
          box.left = content_box.left + std::round((content_box.Width() - width) / 2.0f);
          box.right = box.left + width;
        }
        break;
      }
      case UiObjectFit::Cover:
      {
        // the whole box, with what does not fit cut off on both sides
        if (image_ratio > box_ratio)
        {
          const float shown = box_ratio / image_ratio;
          part.left = (1.0f - shown) / 2.0f;
          part.right = part.left + shown;
        } else
        {
          const float shown = image_ratio / box_ratio;
          part.top = (1.0f - shown) / 2.0f;
          part.bottom = part.top + shown;
        }
        break;
      }
      default:
        break;
    }

    painter.DrawImage(image, box, part, {1.0f, 1.0f, 1.0f, opacity});
  }

  const std::string &UiImageElement::GetSource() const
  {
    return _source;
  }
} // neon
