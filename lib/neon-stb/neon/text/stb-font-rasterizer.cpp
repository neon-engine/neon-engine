#include "stb-font-rasterizer.hpp"

#include <cstddef>

// kept private to this file, so that another library can carry its own copy
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace neon
{
  struct STB_FontRasterizer::Font
  {
    // stb_truetype reads from the bytes of the file for as long as the font
    // is used
    std::vector<unsigned char> file;
    stbtt_fontinfo info{};
  };

  STB_FontRasterizer::STB_FontRasterizer() = default;

  STB_FontRasterizer::~STB_FontRasterizer() = default;

  const STB_FontRasterizer::Font *STB_FontRasterizer::Find(const int font) const
  {
    if (font < 0 || static_cast<std::size_t>(font) >= _fonts.size()) { return nullptr; }

    return _fonts[font].get();
  }

  int STB_FontRasterizer::LoadFont(const std::vector<unsigned char> &file)
  {
    // the table of contents of a font is 12 bytes, and anything shorter is
    // read past its end
    if (file.size() < 12) { return -1; }

    auto font = std::make_unique<Font>();
    font->file = file;

    const int offset = stbtt_GetFontOffsetForIndex(font->file.data(), 0);
    if (offset < 0 || stbtt_InitFont(&font->info, font->file.data(), offset) == 0) { return -1; }

    for (std::size_t i = 0; i < _fonts.size(); i++)
    {
      if (_fonts[i] == nullptr)
      {
        _fonts[i] = std::move(font);
        return static_cast<int>(i);
      }
    }

    _fonts.push_back(std::move(font));
    return static_cast<int>(_fonts.size()) - 1;
  }

  void STB_FontRasterizer::UnloadFont(const int font)
  {
    if (Find(font) != nullptr) { _fonts[font].reset(); }
  }

  bool STB_FontRasterizer::GetMetrics(const int font, const float pixel_size, FontMetrics &metrics)
  {
    const Font *found = Find(font);
    if (found == nullptr || pixel_size <= 0.0f) { return false; }

    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    stbtt_GetFontVMetrics(&found->info, &ascent, &descent, &line_gap);

    const float scale = stbtt_ScaleForMappingEmToPixels(&found->info, pixel_size);
    metrics.ascent = static_cast<float>(ascent) * scale;
    // a font counts downwards below zero
    metrics.descent = static_cast<float>(-descent) * scale;
    metrics.line_gap = static_cast<float>(line_gap) * scale;
    return true;
  }

  bool STB_FontRasterizer::HasGlyph(const int font, const char32_t character)
  {
    const Font *found = Find(font);
    return found != nullptr && stbtt_FindGlyphIndex(&found->info, static_cast<int>(character)) != 0;
  }

  bool STB_FontRasterizer::Rasterize(
    const int font,
    const float pixel_size,
    const char32_t character,
    GlyphBitmap &glyph)
  {
    const Font *found = Find(font);
    if (found == nullptr || pixel_size <= 0.0f) { return false; }

    const int index = stbtt_FindGlyphIndex(&found->info, static_cast<int>(character));
    if (index == 0) { return false; }

    const float scale = stbtt_ScaleForMappingEmToPixels(&found->info, pixel_size);

    int advance = 0;
    int bearing = 0;
    stbtt_GetGlyphHMetrics(&found->info, index, &advance, &bearing);

    // the box counts rows downwards from the baseline, so its top is below
    // zero
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    stbtt_GetGlyphBitmapBox(&found->info, index, scale, scale, &left, &top, &right, &bottom);

    glyph = GlyphBitmap{};
    glyph.advance = static_cast<float>(advance) * scale;

    const int width = right - left;
    const int height = bottom - top;
    if (width <= 0 || height <= 0) { return true; }

    glyph.width = width;
    glyph.height = height;
    glyph.left = left;
    glyph.top = -top;
    glyph.coverage.assign(static_cast<std::size_t>(width) * height, 0);

    stbtt_MakeGlyphBitmap(&found->info, glyph.coverage.data(), width, height, width, scale, scale, index);
    return true;
  }
} // neon
