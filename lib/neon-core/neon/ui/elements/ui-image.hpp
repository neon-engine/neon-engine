#ifndef UI_IMAGE_HPP
#define UI_IMAGE_HPP

#include <string>

#include <neon/ui/ui-element.hpp>

namespace neon
{
  /// An image from a file. It is as large as the image, unless the file
  /// gives it a size. It is what an `img` is in HTML.
  ///
  ///     type: image
  ///     src: assets://ui/heart.png
  ///     width: 32
  ///     height: 32
  ///     object_fit: contain
  class UiImageElement final : public UiElement
  {
    std::string _source;

  public:
    static constexpr const char *kType = "image";

    void ApplyDefaults(UiStyle &style) const override;

    void ReadAttributes(const DataReader &reader) override;

    [[nodiscard]] bool HasContent() const override;

    [[nodiscard]] LayoutSize Measure(
      const UiFrame &frame,
      float available_width,
      float available_height) override;

    void PaintContent(
      UiPainter &painter,
      const UiFrame &frame,
      const UiRectangle &content_box,
      float opacity) override;

    [[nodiscard]] const std::string &GetSource() const;
  };
} // neon

#endif //UI_IMAGE_HPP
