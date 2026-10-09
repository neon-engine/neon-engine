#ifndef TEXT_SHAPER_HPP
#define TEXT_SHAPER_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace neon
{
  /// `direction` of CSS: the way a text runs.
  enum class TextDirection
  {
    LeftToRight = 0,
    RightToLeft
  };

  /// What a character says about the way the text around it runs.
  enum class CharacterDirection
  {
    /// It has none of its own, as a space and a full stop. It runs the way
    /// the text before it does.
    Neutral = 0,
    LeftToRight,
    RightToLeft
  };

  /// A glyph where shaping put it.
  struct ShapedGlyph
  {
    /// The number of the glyph in its font.
    unsigned int glyph = 0;

    /// How far the pen moves on, in pixels.
    float advance = 0.0f;

    /// Where the glyph is drawn, counted from the pen: to the right, and
    /// up.
    float offset_x = 0.0f;
    float offset_y = 0.0f;

    /// The place in the text of the first character the glyph stands for.
    /// Several glyphs may share one, and a ligature stands for several
    /// characters.
    std::uint32_t cluster = 0;
  };

  struct ShapingOptions
  {
    TextDirection direction = TextDirection::LeftToRight;

    /// Whether characters are moved closer together in pairs.
    bool kerning = true;

    /// Whether characters are joined into one glyph where the font has
    /// one, as `fi`. Switched off where letters are spaced apart.
    bool ligatures = true;
  };

  /// Turns the characters of a text into the glyphs of a font, and says
  /// where each one goes: pairs are moved together, letters are joined into
  /// ligatures, and the letters of a script such as Arabic take the form
  /// their neighbors ask for.
  ///
  /// It is given the bytes of a font and reads no files itself. A text that
  /// is handed over runs in one direction and is drawn with one font.
  /// Splitting a text into such runs is the work of what calls this.
  class TextShaper
  {
  protected:
    ~TextShaper() = default;

  public:
    /// Returns what the font is known as from now on, or -1 when the bytes
    /// are not a font.
    virtual int LoadFont(const std::vector<unsigned char> &file) = 0;

    virtual void UnloadFont(int font) = 0;

    /// Shapes `text` from `first` on, `count` characters long. The
    /// characters around that part are looked at, since the form of a
    /// letter depends on its neighbors.
    ///
    /// The glyphs come in the order they are drawn in, from left to right,
    /// whichever way the text runs.
    virtual bool Shape(
      int font,
      float pixel_size,
      std::u32string_view text,
      std::size_t first,
      std::size_t count,
      const ShapingOptions &options,
      std::vector<ShapedGlyph> &glyphs) = 0;

    /// The way the script of a character runs.
    [[nodiscard]] virtual CharacterDirection GetDirection(char32_t character) = 0;

    /// What the script of a character is known as, so that characters of
    /// one script are shaped together. 0 for a character that belongs to
    /// the script before it, as a space and a mark that combines.
    [[nodiscard]] virtual std::uint32_t GetScript(char32_t character) = 0;
  };
} // neon

#endif //TEXT_SHAPER_HPP
