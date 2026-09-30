#ifndef UI_STYLE_HPP
#define UI_STYLE_HPP

#include <optional>
#include <string>

#include <neon/common/color.hpp>
#include <neon/data/data-reader.hpp>
#include <neon/layout/layout-style.hpp>
#include <neon/text/text-layout.hpp>

namespace neon
{
  /// `overflow` of CSS.
  enum class UiOverflow
  {
    Visible = 0,
    Hidden
  };

  /// `pointer-events` of CSS.
  enum class UiPointerEvents
  {
    Auto = 0,
    None
  };

  /// `object-fit` of CSS.
  enum class UiObjectFit
  {
    Fill = 0,
    Contain,
    Cover
  };

  /// What an element looks like and where it goes. The names and the
  /// meanings are those of the properties of CSS, with an underscore where
  /// CSS has a hyphen: `background-color` is `background_color`.
  ///
  /// Every element has every property, as in CSS. One that does not apply
  /// to an element, such as `font_size` to an image, does nothing.
  struct UiStyle
  {
    LayoutStyle layout;

    Color background_color{0.0f, 0.0f, 0.0f, 0.0f};

    /// Virtual path of an image that is stretched over the element.
    std::string background_image;

    /// Without one, the border has the colour of the text, which CSS calls
    /// `currentcolor`.
    std::optional<Color> border_color;

    /// Virtual path of an image that is drawn in nine parts: the corners as
    /// they are, the edges stretched along their side, and the middle
    /// stretched both ways.
    std::string border_image_source;

    /// How far the corners reach into the image from each side, in pixels
    /// of the image.
    LayoutEdges<float> border_image_slice;

    /// How wide the parts at the sides are drawn. Below zero stands for the
    /// width they have in the image.
    LayoutEdges<float> border_image_width{-1.0f, -1.0f, -1.0f, -1.0f};

    float outline_width = 0.0f;
    float outline_offset = 0.0f;
    std::optional<Color> outline_color;

    float opacity = 1.0f;
    UiOverflow overflow = UiOverflow::Visible;
    int z_index = 0;
    UiPointerEvents pointer_events = UiPointerEvents::Auto;

    /// The colour of text.
    Color color{1.0f, 1.0f, 1.0f, 1.0f};

    std::string font_family = "sans-serif";
    float font_size = 16.0f;
    int font_weight = 400;
    TextAlign text_align = TextAlign::Left;

    /// 0 stands for `normal`, which is what the font asks for. Otherwise a
    /// number of pixels, or a multiple of the size of the font.
    float line_height = 0.0f;
    bool line_height_is_multiple = false;

    /// What a bar is filled with.
    Color accent_color{0.30f, 0.69f, 0.31f, 1.0f};

    UiObjectFit object_fit = UiObjectFit::Fill;

    [[nodiscard]] Color BorderColor() const
    {
      return border_color.value_or(color);
    }

    [[nodiscard]] Color OutlineColor() const
    {
      return outline_color.value_or(color);
    }

    /// The height of a line in the units of `font_size`, or 0 for what the
    /// font asks for.
    [[nodiscard]] float LineHeight() const
    {
      return line_height_is_multiple ? line_height * font_size : line_height;
    }
  };

  /// Reads the properties that are written into `style`, and leaves the
  /// others as they are. What is written and cannot be read is reported,
  /// with the line and with what was expected.
  void ReadUiStyle(const DataReader &reader, UiStyle &style);
} // neon

#endif //UI_STYLE_HPP
