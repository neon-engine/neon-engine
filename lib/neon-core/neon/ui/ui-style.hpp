#ifndef UI_STYLE_HPP
#define UI_STYLE_HPP

#include <optional>
#include <string>
#include <vector>

#include <neon/common/color.hpp>
#include <neon/data/data-reader.hpp>
#include <neon/layout/layout-style.hpp>
#include <neon/text/shaped-text.hpp>
#include <neon/text/text-layout.hpp>

#include "ui-paint.hpp"

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
    Cover,

    /// As large as the image is.
    None,

    /// As `none` or as `contain`, whichever is smaller.
    ScaleDown
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

    // Text

    /// Added behind every character, and behind every space, in units of
    /// the file.
    float letter_spacing = 0.0f;
    float word_spacing = 0.0f;

    TextTransform text_transform = TextTransform::None;

    /// `text-decoration-line`: a line under the text, and one through it.
    bool text_underline = false;
    bool text_line_through = false;

    /// Without one, the lines have the colour of the text.
    std::optional<Color> text_decoration_color;

    /// 0 stands for what the font asks for.
    float text_decoration_thickness = 0.0f;

    /// From the one that is drawn on top to the one at the bottom, as CSS
    /// writes them.
    std::vector<UiShadow> text_shadow;

    WhiteSpace white_space = WhiteSpace::PreWrap;
    TextOverflow text_overflow = TextOverflow::Clip;

    /// `font-style: italic`.
    bool font_italic = false;

    /// `-webkit-text-stroke`: a line around every glyph, half of it over
    /// the glyph.
    float text_stroke_width = 0.0f;
    std::optional<Color> text_stroke_color;

    /// What the text is filled with when `color` is a gradient.
    std::optional<UiGradient> color_gradient;

    TextDirection direction = TextDirection::LeftToRight;

    // Images

    UiImageRendering image_rendering = UiImageRendering::Auto;
    UiPlace object_position;

    /// It starts as `100% 100%`, which stretches the image over the box,
    /// and not as `auto`.
    UiBackgroundSize background_size{
      UiBackgroundSize::Kind::Lengths, LayoutLength::Percent(100.0f), LayoutLength::Percent(100.0f)
    };
    UiPlace background_position{LayoutLength::Percent(0.0f), LayoutLength::Percent(0.0f)};
    UiBackgroundRepeat background_repeat = UiBackgroundRepeat::Repeat;
    UiBorderImageRepeat border_image_repeat = UiBorderImageRepeat::Stretch;

    /// What the box is filled with when `background_image` is a gradient.
    std::optional<UiGradient> background_gradient;

    // The box

    UiCornerRadii border_radius;

    /// The colour of each side where it differs from `border_color`.
    std::optional<Color> border_top_color;
    std::optional<Color> border_right_color;
    std::optional<Color> border_bottom_color;
    std::optional<Color> border_left_color;

    /// From the one that is drawn on top to the one at the bottom.
    std::vector<UiShadow> box_shadow;

    std::vector<UiTransformStep> transform;
    UiPlace transform_origin;

    /// Virtual path of the shader the element is drawn with, without an
    /// extension, and the values it is given. Empty for the shader of the
    /// engine.
    std::string shader;
    std::vector<UiShaderValue> shader_values;

    [[nodiscard]] bool HasSideColors() const
    {
      return border_top_color.has_value() || border_right_color.has_value() ||
             border_bottom_color.has_value() || border_left_color.has_value();
    }

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
