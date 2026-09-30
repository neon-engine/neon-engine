#ifndef FT_FONT_RASTERIZER_HPP
#define FT_FONT_RASTERIZER_HPP

#include <memory>
#include <vector>

#include <neon/text/font-rasterizer.hpp>

namespace neon
{
  /// Draws the glyphs of TrueType and OpenType fonts, with FreeType.
  ///
  /// A glyph is drawn with light hinting: its outline is fitted to the rows
  /// of pixels, so that the tops and the feet of letters are sharp, and is
  /// left alone from side to side, so that a glyph can be placed at a part
  /// of a pixel. A glyph that is slanted, made bolder, or drawn as
  /// distances is not hinted.
  ///
  /// Of a file that holds several fonts, the first is used. A font that is
  /// compressed, and glyphs that are kept as PNG images, are not read.
  // ReSharper disable once CppInconsistentNaming
  class FT_FontRasterizer final : public FontRasterizer
  {
    struct Library;
    struct Font;

    std::unique_ptr<Library> _library;

    // a font is known by its place in here, which is empty once it is
    // unloaded
    std::vector<std::unique_ptr<Font>> _fonts;

    [[nodiscard]] Font *Find(int font) const;

    [[nodiscard]] bool SetSize(const Font &font, float pixel_size) const;

  public:
    /// Pixels from the outline of a glyph to where its distances end.
    static constexpr int kDistance_Range = 8;

    FT_FontRasterizer();

    ~FT_FontRasterizer();

    FT_FontRasterizer(const FT_FontRasterizer &) = delete;

    FT_FontRasterizer &operator=(const FT_FontRasterizer &) = delete;

    int LoadFont(const std::vector<unsigned char> &file) override;

    void UnloadFont(int font) override;

    bool GetMetrics(int font, float pixel_size, FontMetrics &metrics) override;

    bool HasGlyph(int font, char32_t character) override;

    bool Rasterize(int font, float pixel_size, char32_t character, GlyphBitmap &glyph) override;

    bool GetGlyph(int font, char32_t character, unsigned int &glyph) override;

    bool RasterizeGlyph(
      int font,
      float pixel_size,
      unsigned int glyph,
      const GlyphOptions &options,
      GlyphBitmap &bitmap) override;

    bool GetUnderline(int font, float pixel_size, float &position, float &thickness) override;

    [[nodiscard]] bool PlacesAtPartsOfAPixel() const override;
  };
} // neon

#endif //FT_FONT_RASTERIZER_HPP
