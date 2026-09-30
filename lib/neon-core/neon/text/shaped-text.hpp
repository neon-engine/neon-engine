#ifndef SHAPED_TEXT_HPP
#define SHAPED_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "font-rasterizer.hpp"
#include "glyph-atlas.hpp"
#include "text-case.hpp"
#include "text-layout.hpp"
#include "text-shaper.hpp"

namespace neon
{
  /// `white-space` of CSS.
  enum class WhiteSpace
  {
    /// Spaces and line feeds are joined into one space, and lines are
    /// broken where they do not fit.
    Normal = 0,

    /// As `normal`, on one line.
    NoWrap,

    /// As it is written, and broken where the text says so alone.
    Pre,

    /// As it is written, and broken where a line does not fit as well.
    PreWrap,

    /// Spaces are joined, line feeds are kept, and lines are broken where
    /// they do not fit.
    PreLine
  };

  /// `text-overflow` of CSS.
  enum class TextOverflow
  {
    Clip = 0,

    /// What does not fit on a line that is not broken is left out, and an
    /// ellipsis is drawn in its place.
    Ellipsis
  };

  /// A font at the size a text is drawn at.
  struct TextFont
  {
    FontRasterizer *rasterizer = nullptr;
    int font = -1;

    /// What shapes the text, or nullptr. Without one, every character is
    /// the glyph its font maps it to, and the text runs from left to
    /// right.
    TextShaper *shaper = nullptr;
    int shaper_font = -1;

    GlyphAtlas *atlas = nullptr;

    /// The size of the text in pixels.
    float pixel_size = 0.0f;

    /// Pixels that are drawn for each pixel of the atlas. 1 but for glyphs
    /// that are kept as distances, which are kept at one size and drawn at
    /// every size.
    float scale = 1.0f;

    /// Whether glyphs are placed at quarters of a pixel. Otherwise at
    /// whole pixels.
    bool places_at_parts = false;
  };

  struct ShapingStyle
  {
    TextDirection direction = TextDirection::LeftToRight;

    /// Added behind every character, and behind every space, in pixels.
    float letter_spacing = 0.0f;
    float word_spacing = 0.0f;

    WhiteSpace white_space = WhiteSpace::PreWrap;
    TextTransform transform = TextTransform::None;

    bool operator==(const ShapingStyle &other) const = default;
  };

  /// A part of a text that has one font, one script, and one direction.
  struct ShapedRun
  {
    /// The characters it stands for: from `first` up to `end`.
    std::size_t first = 0;
    std::size_t end = 0;

    /// Which of the fonts of the text.
    std::size_t font = 0;

    TextDirection direction = TextDirection::LeftToRight;

    /// Its glyphs, in the order they are drawn in from left to right.
    std::size_t first_glyph = 0;
    std::size_t glyph_count = 0;
  };

  /// A text whose glyphs are known, and not yet where they go. It depends
  /// on the text, the fonts, and the style, and not on the room there is,
  /// so it is made once and placed as often as the room changes.
  struct ShapedText
  {
    /// The characters as they are shown: spaces joined and letters raised
    /// as the style says. The glyphs refer to places in here.
    std::u32string characters;

    std::vector<ShapedRun> runs;
    std::vector<ShapedGlyph> glyphs;

    /// How far each character moves the pen, with the spacing of the
    /// style. 0 for a character that shares its glyph with the one before.
    std::vector<float> advances;

    TextDirection direction = TextDirection::LeftToRight;

    /// Whether lines are broken where they do not fit, which the
    /// `white_space` of the style says.
    bool breaks_lines = true;
  };

  struct PlacingOptions
  {
    /// Lines are broken so that they are no wider than this. Not a number
    /// breaks lines only where the text says so.
    float max_width = std::numeric_limits<float>::quiet_NaN();

    /// The width the lines are aligned in, and cut off at. Not a number
    /// stands for the width of the longest line.
    float box_width = std::numeric_limits<float>::quiet_NaN();

    TextAlign align = TextAlign::Left;

    /// From one baseline to the next, in pixels. 0 and below stand for
    /// what the font asks for.
    float line_height = 0.0f;

    TextOverflow overflow = TextOverflow::Clip;
  };

  /// A glyph where it is drawn.
  struct PlacedShapedGlyph
  {
    unsigned int glyph = 0;

    /// Which of its pictures, by quarters of a pixel.
    std::uint8_t variant = 0;

    /// Which of the fonts of the text.
    std::uint8_t font = 0;

    /// The point the pictures of the glyph are placed against: where the
    /// pen is, on the baseline. Counted from the left top corner of the
    /// text. A picture is drawn `left` to the right of it and `top` above.
    float origin_x = 0.0f;
    float origin_y = 0.0f;

    /// The picture of the glyph as it is, or nullptr for a glyph that
    /// draws nothing.
    const CachedGlyph *picture = nullptr;

    std::uint32_t cluster = 0;
  };

  struct PlacedLine
  {
    /// Where the line starts and how wide it is, and where its baseline
    /// lies below the top of the text.
    float left = 0.0f;
    float width = 0.0f;
    float baseline = 0.0f;

    std::size_t first_glyph = 0;
    std::size_t glyph_count = 0;
  };

  struct PlacedShapedText
  {
    /// The width of the longest line, and the height of all lines.
    float width = 0.0f;
    float height = 0.0f;

    std::vector<PlacedLine> lines;
    std::vector<PlacedShapedGlyph> glyphs;
  };

  /// Turns a text into glyphs.
  ///
  /// `fonts` are tried in their order for every character, and the first
  /// that has it is drawn with. A character that none has is drawn as the
  /// replacement character, or as a question mark.
  ///
  /// A text is split where its font, its script, or its direction changes,
  /// and each part is shaped on its own. Parts that run the other way than
  /// the text are placed in the order they are written in. Text that mixes
  /// directions within a sentence is not put in order the way the Unicode
  /// Bidirectional Algorithm asks for.
  [[nodiscard]] ShapedText ShapeText(
    std::u32string_view text,
    const std::vector<TextFont> &fonts,
    const ShapingStyle &style);

  /// Breaks a text into lines and places its glyphs, in pixels.
  ///
  /// A line ends at a line feed, and at a space when the next word does not
  /// fit. A word that is wider than a line is not broken, but for Chinese,
  /// Japanese, and Korean, which are broken between any two characters.
  /// Rows of glyphs are at whole pixels. From side to side a glyph is at a
  /// quarter of a pixel when its font can draw it there.
  [[nodiscard]] PlacedShapedText PlaceShapedText(
    const ShapedText &text,
    const std::vector<TextFont> &fonts,
    const PlacingOptions &options);

  /// The height of a line in pixels, a whole number.
  [[nodiscard]] float LineHeightOf(const FontMetrics &metrics, float line_height);
} // neon

#endif //SHAPED_TEXT_HPP
