#ifndef UI_IMAGE_HPP
#define UI_IMAGE_HPP

#include <string>
#include <vector>

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
  ///
  /// `src` is also a part of an atlas, an image of shapes, or a list of
  /// images that were made for screens of several densities, of which the
  /// one that suits the screen is taken:
  ///
  ///     src: assets://ui/icons.atlas.yml#coin
  ///     src: assets://ui/shield.svg
  ///     src:
  ///       - { src: assets://ui/gem-1x.png, scale: 1 }
  ///       - { src: assets://ui/gem-2x.png, scale: 2 }
  class UiImageElement final : public UiElement
  {
    std::string _source;

    // the images for screens of several densities. One without a list is
    // the one image, made for a scale of 1
    std::vector<UiImageSource> _sources;

    /// The image that suits the screen, as large as it is in units of the
    /// file.
    [[nodiscard]] UiImage Choose(const UiFrame &frame, float width, float height) const;

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
