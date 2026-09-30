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
    unsigned int index = 0;
    return GetGlyph(font, character, index) && RasterizeGlyph(font, pixel_size, index, {}, glyph);
  }

  bool STB_FontRasterizer::GetGlyph(const int font, const char32_t character, unsigned int &glyph)
  {
    const Font *found = Find(font);
    if (found == nullptr) { return false; }

    const int index = stbtt_FindGlyphIndex(&found->info, static_cast<int>(character));
    if (index == 0) { return false; }

    glyph = static_cast<unsigned int>(index);
    return true;
  }

  bool STB_FontRasterizer::RasterizeGlyph(
    const int font,
    const float pixel_size,
    const unsigned int glyph,
    const GlyphOptions &options,
    GlyphBitmap &bitmap)
  {
    const Font *found = Find(font);
    if (found == nullptr || pixel_size <= 0.0f) { return false; }

    if (options.rendering != GlyphRendering::Bitmap || options.embolden != 0.0f ||
        options.slant != 0.0f || options.stroke > 0.0f)
    {
      return false;
    }

    if (glyph >= static_cast<unsigned int>(found->info.numGlyphs)) { return false; }

    const auto index = static_cast<int>(glyph);
    const float scale = stbtt_ScaleForMappingEmToPixels(&found->info, pixel_size);
    const float shift = options.offset_x < 0.0f ? 0.0f : (options.offset_x > 1.0f ? 1.0f : options.offset_x);

    int advance = 0;
    int bearing = 0;
    stbtt_GetGlyphHMetrics(&found->info, index, &advance, &bearing);

    // the box counts rows downwards from the baseline, so its top is below
    // zero
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    stbtt_GetGlyphBitmapBoxSubpixel(&found->info, index, scale, scale, shift, 0.0f, &left, &top, &right, &bottom);

    bitmap = GlyphBitmap{};
    bitmap.advance = static_cast<float>(advance) * scale;

    const int width = right - left;
    const int height = bottom - top;
    if (width <= 0 || height <= 0) { return true; }

    bitmap.width = width;
    bitmap.height = height;
    bitmap.left = left;
    bitmap.top = -top;
    bitmap.coverage.assign(static_cast<std::size_t>(width) * height, 0);

    stbtt_MakeGlyphBitmapSubpixel(
      &found->info, bitmap.coverage.data(), width, height, width, scale, scale, shift, 0.0f, index);
    return true;
  }

  bool STB_FontRasterizer::PlacesAtPartsOfAPixel() const
  {
    return true;
  }
} // neon
