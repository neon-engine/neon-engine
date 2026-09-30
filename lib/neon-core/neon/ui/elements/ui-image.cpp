#include "ui-image.hpp"

#include <neon/ui/ui-box-paint.hpp>
#include <neon/ui/ui-image-paint.hpp>
#include <neon/ui/ui-fields.hpp>

#include <algorithm>
#include <cmath>
#include <format>

namespace neon
{
  bool UiImageElement::TellsWhenItChanged() const
  {
    return true;
  }

  bool UiImageElement::GetField(const std::string &name, FieldValue &value) const
  {
    if (name != "src") { return false; }

    value = _source;
    return true;
  }

  bool UiImageElement::SetField(const std::string &name, const FieldValue &value, std::string &error)
  {
    if (name != "src") { return UiElement::SetField(name, value, error); }

    if (!TakeText(value, "'src' of " + Describe(), _source, error)) { return false; }

    // another image may have another size
    Invalidate(UiDirty::Layout | UiDirty::Paint);
    return true;
  }

  std::vector<UiElement::Field> UiImageElement::GetFields() const
  {
    return {{"src", FieldKind::Text, "Virtual path of the image", {}}};
  }

  void UiImageElement::ApplyDefaults(UiStyle &style) const
  {
    style.pointer_events = UiPointerEvents::None;

    // an image keeps its size next to others, which shrink in its place
    style.layout.flex_shrink = 0.0f;
  }

  void UiImageElement::ReadAttributes(const DataReader &reader)
  {
    const auto *value = reader.ReadValue("src");

    if (value == nullptr)
    {
      reader.Report(std::format(
                      "{} has no 'src', where the virtual path of an image was expected", reader.GetWhere()));
      return;
    }

    if (!value->IsList())
    {
      reader.Read("src", _source);
      if (!_source.empty()) { _sources.push_back({_source, 1.0f}); }
      return;
    }

    // images for screens of several densities
    std::size_t number = 1;
    for (const auto &item : value->GetItems())
    {
      const DataReader source_reader(
        item, reader.GetDocument(), std::format("image {} of {}", number, reader.GetWhere()), reader.GetErrors());
      number++;

      UiImageSource source;
      const bool has_path = source_reader.Read("src", source.path);

      if (!has_path && !source_reader.Has("src"))
      {
        source_reader.Report(std::format(
          "{} has no 'src', where the virtual path of an image was expected", source_reader.GetWhere()));
      }

      if (source_reader.Read("scale", source.scale) && source.scale <= 0.0f)
      {
        source_reader.Report(*source_reader.ReadValue("scale"), std::format(
                               "'scale' of {} is {}, where a number above 0 was expected",
                               source_reader.GetWhere(), source.scale));
        source.scale = 1.0f;
      }

      source_reader.Finish();

      if (has_path && !source.path.empty()) { _sources.push_back(source); }
    }

    if (_sources.empty())
    {
      reader.Report(*value, std::format(
                      "'src' of {} is a list without an image, where at least one such as "
                      "{{ src: assets://ui/heart.png, scale: 1 }} was expected",
                      reader.GetWhere()));
      return;
    }

    _source = _sources.front().path;
  }

  bool UiImageElement::HasContent() const
  {
    return true;
  }

  UiImage UiImageElement::Choose(const UiFrame &frame, const float width, const float height) const
  {
    if (_sources.empty()) { return {}; }

    const UiImageSource &source = _sources[ChooseImageSource(_sources, frame.scale)];

    UiImage image = frame.resources->GetImageFor(source.path, width, height, frame.scale, Describe());

    // an image that was made for a denser screen has more pixels for each
    // unit of the file
    if (image.texture != No_Texture && source.scale != 1.0f)
    {
      image.natural_width = image.NaturalWidth() / source.scale;
      image.natural_height = image.NaturalHeight() / source.scale;
    }

    return image;
  }

  LayoutSize UiImageElement::Measure(const UiFrame &frame, float available_width, float available_height)
  {
    if (_sources.empty()) { return {}; }

    // a pixel of the image is a unit of the file, as it is a pixel of CSS
    const UiImage image = Choose(frame, 0.0f, 0.0f);
    return {image.NaturalWidth(), image.NaturalHeight()};
  }

  void UiImageElement::PaintContent(
    UiPainter &painter,
    const UiFrame &frame,
    const UiRectangle &content_box,
    const float opacity)
  {
    if (_sources.empty() || content_box.IsEmpty()) { return; }

    const UiStyle &style = GetStyle();

    // An image of shapes is drawn at the size it has on the screen. What
    // fills its box has the size of the box, and what keeps its shape is
    // no larger.
    const UiImage image = Choose(frame, content_box.Width(), content_box.Height());
    if (image.texture == No_Texture || image.width <= 0 || image.height <= 0) { return; }

    UiRectangle place;
    UiRectangle part;
    FitImage(
      style,
      image.NaturalWidth() * frame.scale,
      image.NaturalHeight() * frame.scale,
      content_box,
      frame.scale,
      place,
      part);

    if (place.IsEmpty()) { return; }

    // cut off at the corners of the element when they are round
    const UiBoxPaint box(
      style, ToPixels(GetBox(), frame.scale), ToPixels(GetPaddingBox(), frame.scale), frame.scale);

    box.PaintImage(painter, image, place, part, opacity);
  }

  const std::string &UiImageElement::GetSource() const
  {
    return _source;
  }
} // neon
