#ifndef FAKE_FONT_RASTERIZER_HPP
#define FAKE_FONT_RASTERIZER_HPP

#include <cstddef>
#include <set>
#include <vector>

#include <neon/text/font-rasterizer.hpp>

namespace neon::testing
{
  /// A font whose characters are all the same box, so that a test can say
  /// where a text has to end up without knowing a real font.
  ///
  /// At size 20 every character moves the pen by 10. Its picture is 8 wide
  /// and 14 high, starts 1 right of the pen, and stands on the baseline.
  /// A space has no picture. The letters reach 16 above the baseline and 4
  /// below, which makes a line 20 high.
  class FakeFontRasterizer final : public FontRasterizer
  {
    int _fonts = 0;
    std::set<int> _unloaded;

  public:
    /// Characters the font does not have.
    std::set<char32_t> missing;

    /// How often a picture was asked for.
    std::size_t rasterized = 0;

    /// Bytes that are not a font, for a test of what happens then.
    static std::vector<unsigned char> NotAFont()
    {
      return {};
    }

    static std::vector<unsigned char> AFont()
    {
      return {'f', 'o', 'n', 't'};
    }

    [[nodiscard]] bool IsLoaded(const int font) const
    {
      return font >= 0 && font < _fonts && !_unloaded.contains(font);
    }

    int LoadFont(const std::vector<unsigned char> &file) override
    {
      return file.empty() ? -1 : _fonts++;
    }

    void UnloadFont(const int font) override
    {
      _unloaded.insert(font);
    }

    bool GetMetrics(const int font, const float pixel_size, FontMetrics &metrics) override
    {
      if (!IsLoaded(font)) { return false; }

      metrics.ascent = pixel_size * 0.8f;
      metrics.descent = pixel_size * 0.2f;
      metrics.line_gap = 0.0f;
      return true;
    }

    bool HasGlyph(const int font, const char32_t character) override
    {
      return IsLoaded(font) && character >= 0x20 && !missing.contains(character);
    }

    bool Rasterize(const int font, const float pixel_size, const char32_t character, GlyphBitmap &glyph) override
    {
      if (!HasGlyph(font, character)) { return false; }

      rasterized++;
      glyph = GlyphBitmap{};
      glyph.advance = pixel_size * 0.5f;

      if (character == U' ' || character == 0xA0) { return true; }

      glyph.width = static_cast<int>(pixel_size * 0.4f);
      glyph.height = static_cast<int>(pixel_size * 0.7f);
      glyph.left = 1;
      glyph.top = glyph.height;
      glyph.coverage.assign(static_cast<std::size_t>(glyph.width) * glyph.height, 255);
      return true;
    }
  };
} // neon::testing

#endif //FAKE_FONT_RASTERIZER_HPP
