#ifndef FONT_RASTERIZER_HPP
#define FONT_RASTERIZER_HPP

#include <vector>

namespace neon
{
  /// The measures of a font at one size, in pixels.
  struct FontMetrics
  {
    /// From the baseline up to the top of the tallest letters.
    float ascent = 0.0f;

    /// From the baseline down to the bottom of the lowest letters. It is
    /// above zero.
    float descent = 0.0f;

    /// What the font asks to be left between two lines.
    float line_gap = 0.0f;
  };

  /// The picture of one character.
  struct GlyphBitmap
  {
    int width = 0;
    int height = 0;

    /// From the pen to the left edge of the picture. Below zero when the
    /// picture starts left of the pen.
    int left = 0;

    /// From the baseline up to the top edge of the picture.
    int top = 0;

    /// How far the pen moves on to the next character.
    float advance = 0.0f;

    /// How much of each pixel the character covers, from 0 to 255. One byte
    /// for each pixel, row after row from the top. Empty for a character
    /// that draws nothing, such as a space.
    std::vector<unsigned char> coverage;
  };

  /// Turns the characters of a font into pictures. An implementation knows
  /// one kind of font file, such as TrueType.
  ///
  /// It is given the bytes of a file and reads no files itself. Those go
  /// through the file system.
  class FontRasterizer
  {
  protected:
    ~FontRasterizer() = default;

  public:
    /// Returns what the font is known as from now on, or -1 when the bytes
    /// are not a font.
    virtual int LoadFont(const std::vector<unsigned char> &file) = 0;

    virtual void UnloadFont(int font) = 0;

    /// `pixel_size` is the size of the font as CSS means it: the height of
    /// the square a letter is designed in.
    virtual bool GetMetrics(int font, float pixel_size, FontMetrics &metrics) = 0;

    /// Whether the font has a picture for the character.
    virtual bool HasGlyph(int font, char32_t character) = 0;

    virtual bool Rasterize(int font, float pixel_size, char32_t character, GlyphBitmap &glyph) = 0;
  };
} // neon

#endif //FONT_RASTERIZER_HPP
