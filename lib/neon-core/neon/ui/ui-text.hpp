#ifndef UI_TEXT_HPP
#define UI_TEXT_HPP

#include <cstdint>
#include <string>

#include <neon/data/data-reader.hpp>
#include <neon/layout/layout-engine.hpp>

#include "ui-element.hpp"

namespace neon
{
  /// The text of an element that has one, such as a label and a button. It
  /// follows the values of the game, and is measured and drawn in pixels of
  /// the frame, so that it is as sharp as the font was drawn.
  class UiText
  {
    UiTemplate _template;
    std::string _text;
    std::u32string _characters;
    bool _has_text = false;

    // the values the text was made from
    std::uint64_t _revision = 0;
    bool _is_made = false;

    [[nodiscard]] static const UiFont *FontOf(const UiStyle &style, const UiFrame &frame);

    [[nodiscard]] static TextOptions OptionsOf(const UiStyle &style, const UiFrame &frame);

    /// The glyphs of the text, which are made again when the text, its
    /// fonts, or what shapes it has changed.
    struct Shaped
    {
      bool is_made = false;
      std::uint64_t made_from = 0;
      std::uint64_t fonts_revision = 0;

      std::string families;
      int weight = 0;
      bool is_italic = false;
      int pixel_size = 0;
      ShapingStyle style;

      const UiTextFonts *fonts = nullptr;
      ShapedText text;
    };

    /// Where the glyphs go in a room of one size. Kept so that a frame in
    /// which nothing changed places no glyph again.
    struct Placed
    {
      bool is_made = false;
      std::uint64_t shaped = 0;
      float max_width = 0.0f;
      float box_width = 0.0f;
      TextAlign align = TextAlign::Left;
      float line_height = 0.0f;
      TextOverflow overflow = TextOverflow::Clip;

      PlacedShapedText text;
    };

    mutable Shaped _shaped;
    mutable std::uint64_t _shaped_count = 0;
    mutable Placed _measured;
    mutable Placed _painted;
    std::uint64_t _text_count = 0;

    /// The glyphs of the text as the style asks for them, or nullptr when
    /// no font of the style can be used.
    [[nodiscard]] const Shaped *Shape(const UiStyle &style, const UiFrame &frame) const;

    [[nodiscard]] const PlacedShapedText &Place(
      Placed &placed,
      const Shaped &shaped,
      const PlacingOptions &options) const;

  public:
    /// Reads `text` of the element. Text that is left out is empty.
    void Read(const DataReader &reader);

    /// Whether `text` was written.
    [[nodiscard]] bool IsWritten() const;

    /// The text as it is shown now.
    [[nodiscard]] const std::string &Get() const;

    /// Puts the values of the game into their places, when one of them has
    /// changed.
    void Update(const UiFrame &frame);

    /// In units of the file.
    [[nodiscard]] LayoutSize Measure(const UiStyle &style, const UiFrame &frame, float available_width) const;

    /// `centered` places the text in the middle of the height of the box,
    /// and not at its top.
    void Paint(
      UiPainter &painter,
      const UiStyle &style,
      const UiFrame &frame,
      const UiRectangle &content_box,
      float opacity,
      bool centered) const;
  };
} // neon

#endif //UI_TEXT_HPP
