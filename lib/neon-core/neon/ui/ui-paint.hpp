#ifndef UI_PAINT_HPP
#define UI_PAINT_HPP

#include <cstddef>
#include <string>
#include <vector>

#include <neon/common/color.hpp>
#include <neon/layout/layout-style.hpp>

// What an element is painted with beyond one color: gradients, shadows,
// round corners, and what moves and turns it. The names and the meanings
// are those of CSS.

namespace neon
{
  /// A color of a gradient and where along the gradient it lies, from 0
  /// to 1.
  struct UiGradientStop
  {
    Color color;
    float position = 0.0f;

    /// Whether the file says where the color lies. One that does not lies
    /// evenly between its neighbors.
    bool has_position = false;
  };

  /// `linear-gradient()` and `radial-gradient()` of CSS.
  struct UiGradient
  {
    enum class Kind
    {
      Linear = 0,
      Radial
    };

    /// As many colors as a renderer takes for one gradient.
    static constexpr std::size_t kMax_Stops = 8;

    Kind kind = Kind::Linear;

    /// The way a linear gradient runs, in degrees as CSS counts them: 0
    /// runs to the top, 90 to the right, 180 to the bottom.
    float angle = 180.0f;

    std::vector<UiGradientStop> stops;

    /// Every color with a place, which are in rising order.
    void Settle();

    /// The color at a place from 0 to 1.
    [[nodiscard]] Color At(float position) const;
  };

  /// One shadow of `box-shadow` or of `text-shadow`.
  struct UiShadow
  {
    float offset_x = 0.0f;
    float offset_y = 0.0f;

    /// As CSS counts it: twice the deviation of the blur.
    float blur = 0.0f;

    /// How much larger than the box the shadow is. Not read for text.
    float spread = 0.0f;

    Color color{0.0f, 0.0f, 0.0f, 1.0f};

    /// Without one, the shadow has the color of the text.
    bool has_color = false;

    /// Whether the shadow falls into the box and not around it.
    bool is_inset = false;
  };

  /// The radius of each corner of `border-radius`, from the left top one
  /// around to the left bottom one.
  struct UiCornerRadii
  {
    LayoutLength top_left = LayoutLength::Pixels(0.0f);
    LayoutLength top_right = LayoutLength::Pixels(0.0f);
    LayoutLength bottom_right = LayoutLength::Pixels(0.0f);
    LayoutLength bottom_left = LayoutLength::Pixels(0.0f);

    [[nodiscard]] bool IsRound() const
    {
      return top_left.value > 0.0f || top_right.value > 0.0f ||
             bottom_right.value > 0.0f || bottom_left.value > 0.0f;
    }
  };

  /// A place in a box from side to side and from top to bottom, as
  /// `object-position` and `background-position` write it. A percentage is
  /// of the room that is left over.
  struct UiPlace
  {
    LayoutLength x = LayoutLength::Percent(50.0f);
    LayoutLength y = LayoutLength::Percent(50.0f);
  };

  /// `background-size` of CSS.
  struct UiBackgroundSize
  {
    enum class Kind
    {
      /// As large as the image is.
      Auto = 0,
      Cover,
      Contain,

      /// As wide and as high as written. A side that is `auto` follows the
      /// other one.
      Lengths
    };

    Kind kind = Kind::Auto;
    LayoutLength width;
    LayoutLength height;
  };

  /// `background-repeat` of CSS.
  enum class UiBackgroundRepeat
  {
    Repeat = 0,
    NoRepeat,
    RepeatX,
    RepeatY
  };

  /// `border-image-repeat` of CSS.
  enum class UiBorderImageRepeat
  {
    Stretch = 0,

    /// The edge is drawn again and again at its size, and cut off where
    /// the side ends.
    Repeat,

    /// As `repeat`, scaled so that a whole number fits.
    Round
  };

  /// `image-rendering` of CSS.
  enum class UiImageRendering
  {
    Auto = 0,

    /// Every pixel of the image as a square, for art that is drawn pixel
    /// by pixel.
    Pixelated
  };

  /// One of the steps of `transform`.
  struct UiTransformStep
  {
    enum class Kind
    {
      Translate = 0,
      Rotate,
      Scale
    };

    Kind kind = Kind::Translate;

    /// How far it is moved. A percentage is of the size of the element.
    LayoutLength x = LayoutLength::Pixels(0.0f);
    LayoutLength y = LayoutLength::Pixels(0.0f);

    /// In degrees, clockwise.
    float angle = 0.0f;

    float scale_x = 1.0f;
    float scale_y = 1.0f;
  };

  /// What moves a point of an element to where it is drawn: two rows of
  /// three numbers.
  ///
  ///     x' = a * x + c * y + e
  ///     y' = b * x + d * y + f
  struct UiMatrix
  {
    float a = 1.0f;
    float b = 0.0f;
    float c = 0.0f;
    float d = 1.0f;
    float e = 0.0f;
    float f = 0.0f;

    [[nodiscard]] bool IsIdentity() const
    {
      return a == 1.0f && b == 0.0f && c == 0.0f && d == 1.0f && e == 0.0f && f == 0.0f;
    }

    void Apply(float &x, float &y) const
    {
      const float moved_x = a * x + c * y + e;
      const float moved_y = b * x + d * y + f;
      x = moved_x;
      y = moved_y;
    }

    /// This one after `first`.
    [[nodiscard]] UiMatrix After(const UiMatrix &first) const
    {
      return {
        a * first.a + c * first.b,
        b * first.a + d * first.b,
        a * first.c + c * first.d,
        b * first.c + d * first.d,
        a * first.e + c * first.f + e,
        b * first.e + d * first.f + f
      };
    }

    /// What undoes this one. Returns false for one that flattens
    /// everything onto a line, which nothing undoes.
    [[nodiscard]] bool Invert(UiMatrix &inverse) const
    {
      const float determinant = a * d - b * c;
      if (determinant > -1e-12f && determinant < 1e-12f) { return false; }

      inverse.a = d / determinant;
      inverse.b = -b / determinant;
      inverse.c = -c / determinant;
      inverse.d = a / determinant;
      inverse.e = (c * f - d * e) / determinant;
      inverse.f = (b * e - a * f) / determinant;
      return true;
    }
  };

  /// The steps of a `transform` as one matrix, for an element of a size,
  /// around a point that is counted from the corner of the element. The
  /// steps are applied as CSS does: the one written last is done first.
  [[nodiscard]] UiMatrix ToMatrix(
    const std::vector<UiTransformStep> &steps,
    float width,
    float height,
    float origin_x,
    float origin_y);

  /// A variant of an image for screens of a density, as `image-set()` and
  /// `srcset` of the web have them.
  struct UiImageSource
  {
    std::string path;

    /// Pixels of the image for each unit of the file.
    float scale = 1.0f;
  };

  /// The variant whose density is nearest above what is asked for, or the
  /// densest there is. `sources` is not empty.
  [[nodiscard]] std::size_t ChooseImageSource(const std::vector<UiImageSource> &sources, float scale);

  /// A value a shader of an element is given, by its name.
  struct UiShaderValue
  {
    std::string name;

    /// Up to four numbers: one for a number, four for a color.
    float numbers[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    std::size_t count = 1;

    /// The name of the value of the game the first number follows, or
    /// empty.
    std::string bound_to;
  };
} // neon

#endif //UI_PAINT_HPP
