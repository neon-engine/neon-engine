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

    /// Red, green, blue, and alpha for each pixel of a glyph that brings
    /// colors of its own, such as an emoji. Alpha is not multiplied into
    /// the colors. Empty for every other glyph, which has the color of
    /// the text.
    std::vector<unsigned char> colors;

    /// Whether `coverage` holds distances in place of how much is covered:
    /// 128 on the outline, more inside, less outside.
    bool is_distance_field = false;

    /// How many pixels of the picture the way from 128 to 0 or to 255 is.
    float distance_range = 0.0f;
  };

  /// How the picture of a glyph is made.
  enum class GlyphRendering
  {
    /// How much of each pixel the glyph covers. The sharpest at the size it
    /// is made for.
    Bitmap = 0,

    /// The distance of each pixel to the outline. It is drawn at any size
    /// and with an outline or a glow by a shader, without being made again.
    DistanceField
  };

  /// What is asked of the picture of a glyph.
  struct GlyphOptions
  {
    /// The part of a pixel the glyph is moved to the right by before it is
    /// drawn, from 0 to 1. Text is then spaced as evenly as its font means
    /// it to be, and not to the nearest pixel.
    float offset_x = 0.0f;

    GlyphRendering rendering = GlyphRendering::Bitmap;

    /// Pixels the outline is made thicker by, for a bold that a family
    /// does not have.
    float embolden = 0.0f;

    /// How far the glyph leans to the right for each pixel it rises, for
    /// an italic that a family does not have.
    float slant = 0.0f;

    /// Above 0, the line around the glyph is drawn in place of the glyph,
    /// this many pixels wide.
    float stroke = 0.0f;
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

    /// The glyph a font draws a character with, as the font numbers its
    /// glyphs. Returns false when it has none.
    ///
    /// Shaping hands over glyphs and not characters: a ligature and the
    /// form a letter takes between two others have no character of their
    /// own. An implementation that knows nothing of the numbers of a font
    /// is left as it is, and numbers a glyph by its character.
    virtual bool GetGlyph(const int font, const char32_t character, unsigned int &glyph)
    {
      if (!HasGlyph(font, character)) { return false; }

      glyph = static_cast<unsigned int>(character);
      return true;
    }

    /// Draws a glyph by its number. Returns false when it cannot be drawn
    /// the way the options ask for.
    virtual bool RasterizeGlyph(
      const int font,
      const float pixel_size,
      const unsigned int glyph,
      const GlyphOptions &options,
      GlyphBitmap &bitmap)
    {
      if (options.rendering != GlyphRendering::Bitmap || options.stroke > 0.0f) { return false; }

      return Rasterize(font, pixel_size, static_cast<char32_t>(glyph), bitmap);
    }

    /// Where the line under a text goes and how thick it is, in pixels.
    /// `position` is counted from the baseline down. Returns false when the
    /// font does not say, and the line is then placed by its size.
    virtual bool GetUnderline(const int font, const float pixel_size, float &position, float &thickness)
    {
      return false;
    }

    /// Whether RasterizeGlyph() moves a glyph by the part of a pixel it is
    /// asked to. Text is placed at whole pixels otherwise.
    [[nodiscard]] virtual bool PlacesAtPartsOfAPixel() const
    {
      return false;
    }
  };
} // neon

#endif //FONT_RASTERIZER_HPP
