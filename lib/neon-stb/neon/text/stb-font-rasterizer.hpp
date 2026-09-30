#ifndef STB_FONT_RASTERIZER_HPP
#define STB_FONT_RASTERIZER_HPP

#include <memory>
#include <vector>

#include <neon/text/font-rasterizer.hpp>

namespace neon
{
  /// Draws the characters of TrueType and OpenType fonts, with
  /// stb_truetype.
  ///
  /// A character is drawn from its outline, smoothed at its edges, and
  /// without the hints a font may carry for small sizes. Of a file that
  /// holds several fonts, the first is used.
  ///
  /// stb_truetype does not check a file for damage. A font has to come from
  /// where the assets of the game come from, and not from a player.
  // ReSharper disable once CppInconsistentNaming
  class STB_FontRasterizer final : public FontRasterizer
  {
    struct Font;

    // a font is known by its place in here, which is empty once it is
    // unloaded
    std::vector<std::unique_ptr<Font>> _fonts;

    [[nodiscard]] const Font *Find(int font) const;

  public:
    STB_FontRasterizer();

    ~STB_FontRasterizer();

    int LoadFont(const std::vector<unsigned char> &file) override;

    void UnloadFont(int font) override;

    bool GetMetrics(int font, float pixel_size, FontMetrics &metrics) override;

    bool HasGlyph(int font, char32_t character) override;

    bool Rasterize(int font, float pixel_size, char32_t character, GlyphBitmap &glyph) override;

    bool GetGlyph(int font, char32_t character, unsigned int &glyph) override;

    /// Draws a glyph where it is, or moved by a part of a pixel. Nothing
    /// else of the options can be done with stb_truetype: a glyph that is
    /// asked for as distances, bolder, leaning, or as its outline is
    /// refused.
    bool RasterizeGlyph(
      int font,
      float pixel_size,
      unsigned int glyph,
      const GlyphOptions &options,
      GlyphBitmap &bitmap) override;

    [[nodiscard]] bool PlacesAtPartsOfAPixel() const override;
  };
} // neon

#endif //STB_FONT_RASTERIZER_HPP
