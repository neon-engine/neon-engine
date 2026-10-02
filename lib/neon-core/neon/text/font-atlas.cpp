#include "font-atlas.hpp"

#include <algorithm>
#include <cstddef>
#include <format>

#include "utf8.hpp"

namespace neon
{
  // Helpers of CharacterRange, for this file alone.
  namespace
  {
    // kept free around every picture
    constexpr int spacing = 1;

    struct Drawn
    {
      char32_t character = 0;
      GlyphBitmap bitmap;
    };

    /// Places the pictures in rows, the tallest first. Returns false when
    /// they do not fit into a square of the given size.
    bool Pack(
      const std::vector<Drawn> &drawn,
      const std::vector<std::size_t> &order,
      const int size,
      std::vector<AtlasGlyph> &placed)
    {
      int x = spacing;
      int y = spacing;
      int row_height = 0;

      for (const std::size_t index : order)
      {
        const GlyphBitmap &bitmap = drawn[index].bitmap;
        if (bitmap.coverage.empty()) { continue; }

        if (bitmap.width + 2 * spacing > size) { return false; }

        if (x + bitmap.width + spacing > size)
        {
          x = spacing;
          y += row_height + spacing;
          row_height = 0;
        }

        if (y + bitmap.height + spacing > size) { return false; }

        placed[index].x = x;
        placed[index].y = y;

        x += bitmap.width + spacing;
        row_height = std::max(row_height, bitmap.height);
      }

      return true;
    }
  }

  const std::vector<CharacterRange> &FontAtlas::DefaultCharacters()
  {
    static const std::vector<CharacterRange> characters = {
      {0x0020, 0x007E}, // Basic Latin
      {0x00A0, 0x00FF}, // Latin-1 Supplement
      {0x0100, 0x017F}, // Latin Extended-A
      {0x2013, 0x2014}, // dashes
      {0x2018, 0x201E}, // quotation marks
      {0x2022, 0x2022}, // bullet
      {0x2026, 0x2026}, // ellipsis
      {0x20AC, 0x20AC}, // euro sign
      {0x2122, 0x2122}, // trade mark sign
      {Replacement_Character, Replacement_Character}
    };
    return characters;
  }

  bool FontAtlas::Build(
    FontRasterizer &rasterizer,
    const int font,
    const float pixel_size,
    const std::vector<CharacterRange> &characters,
    std::string &error)
  {
    _width = 0;
    _height = 0;
    _pixels.clear();
    _glyphs.clear();
    _pixel_size = pixel_size;

    if (pixel_size <= 0.0f)
    {
      error = std::format("a font of size {} cannot be drawn", pixel_size);
      return false;
    }

    if (!rasterizer.GetMetrics(font, pixel_size, _metrics))
    {
      error = "the font cannot be used";
      return false;
    }

    std::vector<Drawn> drawn;
    long long area = 0;

    for (const auto &[first, last] : characters)
    {
      for (char32_t character = first; character <= last; character++)
      {
        if (!rasterizer.HasGlyph(font, character)) { continue; }

        Drawn glyph;
        glyph.character = character;
        if (!rasterizer.Rasterize(font, pixel_size, character, glyph.bitmap)) { continue; }

        area += static_cast<long long>(glyph.bitmap.width + spacing) * (glyph.bitmap.height + spacing);
        drawn.push_back(std::move(glyph));
      }
    }

    if (drawn.empty())
    {
      error = "the font has none of the characters that are drawn";
      return false;
    }

    std::vector<std::size_t> order(drawn.size());
    for (std::size_t i = 0; i < order.size(); i++) { order[i] = i; }

    std::ranges::stable_sort(order, [&drawn](const std::size_t a, const std::size_t b)
    {
      return drawn[a].bitmap.height > drawn[b].bitmap.height;
    });

    // the smallest power of two the pictures fit into
    std::vector<AtlasGlyph> placed(drawn.size());
    int size = 64;
    while (size <= max_size &&
           (static_cast<long long>(size) * size < area || !Pack(drawn, order, size, placed)))
    {
      size *= 2;
    }

    if (size > max_size)
    {
      error = std::format(
        "the characters of a font of size {} do not fit into an image of {} by {}",
        pixel_size, max_size, max_size);
      return false;
    }

    _width = size;
    _height = size;

    // white everywhere, so that the colour next to a character is the
    // colour of the character when the image is scaled
    _pixels.assign(static_cast<std::size_t>(size) * size * 4, 255);
    for (std::size_t i = 3; i < _pixels.size(); i += 4) { _pixels[i] = 0; }

    for (std::size_t i = 0; i < drawn.size(); i++)
    {
      const GlyphBitmap &bitmap = drawn[i].bitmap;
      AtlasGlyph &glyph = placed[i];

      glyph.left = bitmap.left;
      glyph.top = bitmap.top;
      glyph.advance = bitmap.advance;

      if (!bitmap.coverage.empty())
      {
        glyph.width = bitmap.width;
        glyph.height = bitmap.height;

        for (int row = 0; row < bitmap.height; row++)
        {
          for (int column = 0; column < bitmap.width; column++)
          {
            const std::size_t to =
              (static_cast<std::size_t>(glyph.y + row) * size + static_cast<std::size_t>(glyph.x + column)) * 4;
            _pixels[to + 3] = bitmap.coverage[static_cast<std::size_t>(row) * bitmap.width + column];
          }
        }
      }

      _glyphs[drawn[i].character] = glyph;
    }

    return true;
  }

  const AtlasGlyph *FontAtlas::Find(const char32_t character) const
  {
    for (const char32_t candidate : {character, Replacement_Character, static_cast<char32_t>(U'?')})
    {
      if (const auto found = _glyphs.find(candidate); found != _glyphs.end()) { return &found->second; }
    }
    return nullptr;
  }

  bool FontAtlas::Has(const char32_t character) const
  {
    return _glyphs.contains(character);
  }

  int FontAtlas::GetWidth() const
  {
    return _width;
  }

  int FontAtlas::GetHeight() const
  {
    return _height;
  }

  float FontAtlas::GetPixelSize() const
  {
    return _pixel_size;
  }

  const FontMetrics &FontAtlas::GetMetrics() const
  {
    return _metrics;
  }

  const std::vector<unsigned char> &FontAtlas::GetPixels() const
  {
    return _pixels;
  }
} // neon
