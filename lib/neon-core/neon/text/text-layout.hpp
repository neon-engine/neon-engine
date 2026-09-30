#ifndef TEXT_LAYOUT_HPP
#define TEXT_LAYOUT_HPP

#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include "font-atlas.hpp"

namespace neon
{
  /// `text-align` of CSS.
  enum class TextAlign
  {
    Left = 0,
    Center,
    Right
  };

  struct TextOptions
  {
    /// Lines are broken at spaces so that they are no wider than this. Not
    /// a number breaks lines only where the text says so.
    float max_width = std::numeric_limits<float>::quiet_NaN();

    /// The width the lines are aligned in. Not a number stands for the
    /// width of the longest line.
    float box_width = std::numeric_limits<float>::quiet_NaN();

    TextAlign align = TextAlign::Left;

    /// From one baseline to the next, in pixels. 0 and below stand for
    /// what the font asks for.
    float line_height = 0.0f;
  };

  /// A character where it is drawn. `x` and `y` are the left top corner of
  /// its picture, counted from the left top corner of the text, in whole
  /// pixels.
  struct PlacedGlyph
  {
    const AtlasGlyph *glyph = nullptr;
    float x = 0.0f;
    float y = 0.0f;
  };

  struct PlacedText
  {
    /// The width of the longest line, and the height of all lines.
    float width = 0.0f;
    float height = 0.0f;
    std::size_t lines = 0;

    /// Only the characters that draw something.
    std::vector<PlacedGlyph> glyphs;
  };

  /// Places the characters of a text, in pixels.
  ///
  /// A text runs from left to right. A line ends at `\n`, and at a space
  /// when the next word does not fit. A word that is wider than a line is
  /// not broken. Every character is placed at whole pixels, which is what
  /// keeps it as sharp as it was drawn into the atlas.
  ///
  /// Characters are not moved closer together in pairs, which is kerning,
  /// and are not joined or reordered, which is shaping.
  [[nodiscard]] PlacedText PlaceText(
    const FontAtlas &atlas,
    const std::u32string &text,
    const TextOptions &options);

  /// The height of a line in pixels, a whole number.
  [[nodiscard]] float LineHeightOf(const FontAtlas &atlas, const TextOptions &options);
} // neon

#endif //TEXT_LAYOUT_HPP
