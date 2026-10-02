#include "ft-font-rasterizer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BITMAP_H
#include FT_GLYPH_H
#include FT_MODULE_H
#include FT_OUTLINE_H
#include FT_STROKER_H

namespace neon
{
  // Helpers of FT_FontRasterizer, for this file alone.
  namespace
  {
    // FreeType counts in parts of 64 of a pixel
    constexpr float parts = 64.0f;

    FT_F26Dot6 ToParts(const float pixels)
    {
      return static_cast<FT_F26Dot6>(std::lround(pixels * parts));
    }
  }

  struct FT_FontRasterizer::Library
  {
    FT_Library library = nullptr;
  };

  struct FT_FontRasterizer::Font
  {
    // FreeType reads from the bytes of the file for as long as the font is
    // used
    std::vector<unsigned char> file;
    FT_Face face = nullptr;
  };

  FT_FontRasterizer::FT_FontRasterizer()
  {
    _library = std::make_unique<Library>();

    if (FT_Init_FreeType(&_library->library) != 0)
    {
      _library->library = nullptr;
      return;
    }

    FT_Int spread = kDistance_Range;
    FT_Property_Set(_library->library, "sdf", "spread", &spread);
    FT_Property_Set(_library->library, "bsdf", "spread", &spread);
  }

  FT_FontRasterizer::~FT_FontRasterizer()
  {
    for (const auto &font : _fonts)
    {
      if (font != nullptr && font->face != nullptr) { FT_Done_Face(font->face); }
    }
    _fonts.clear();

    if (_library->library != nullptr) { FT_Done_FreeType(_library->library); }
  }

  FT_FontRasterizer::Font *FT_FontRasterizer::Find(const int font) const
  {
    if (font < 0 || static_cast<std::size_t>(font) >= _fonts.size()) { return nullptr; }

    return _fonts[font].get();
  }

  int FT_FontRasterizer::LoadFont(const std::vector<unsigned char> &file)
  {
    if (_library->library == nullptr || file.empty()) { return -1; }

    auto font = std::make_unique<Font>();
    font->file = file;

    if (FT_New_Memory_Face(
          _library->library,
          font->file.data(),
          static_cast<FT_Long>(font->file.size()),
          0,
          &font->face) != 0)
    {
      return -1;
    }

    // a font that cannot be drawn at any size, such as one of bitmaps alone
    if (!FT_IS_SCALABLE(font->face))
    {
      FT_Done_Face(font->face);
      return -1;
    }

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

  void FT_FontRasterizer::UnloadFont(const int font)
  {
    const Font *found = Find(font);
    if (found == nullptr) { return; }

    FT_Done_Face(found->face);
    _fonts[font].reset();
  }

  bool FT_FontRasterizer::SetSize(const Font &font, const float pixel_size) const
  {
    // at 72 dots for each inch a point is a pixel, and the size is the
    // height of the square a letter is designed in
    return pixel_size > 0.0f && FT_Set_Char_Size(font.face, 0, ToParts(pixel_size), 72, 72) == 0;
  }

  bool FT_FontRasterizer::GetMetrics(const int font, const float pixel_size, FontMetrics &metrics)
  {
    const Font *found = Find(font);
    if (found == nullptr || pixel_size <= 0.0f || found->face->units_per_EM == 0) { return false; }

    // From the measures of the design, which are not rounded to pixels.
    const FT_Face face = found->face;
    const float scale = pixel_size / static_cast<float>(face->units_per_EM);

    metrics.ascent = static_cast<float>(face->ascender) * scale;
    // a font counts downwards below zero
    metrics.descent = static_cast<float>(-face->descender) * scale;
    metrics.line_gap = static_cast<float>(face->height - (face->ascender - face->descender)) * scale;
    return true;
  }

  bool FT_FontRasterizer::HasGlyph(const int font, const char32_t character)
  {
    const Font *found = Find(font);
    return found != nullptr && FT_Get_Char_Index(found->face, character) != 0;
  }

  bool FT_FontRasterizer::GetGlyph(const int font, const char32_t character, unsigned int &glyph)
  {
    const Font *found = Find(font);
    if (found == nullptr) { return false; }

    const FT_UInt index = FT_Get_Char_Index(found->face, character);
    if (index == 0) { return false; }

    glyph = index;
    return true;
  }

  bool FT_FontRasterizer::Rasterize(
    const int font,
    const float pixel_size,
    const char32_t character,
    GlyphBitmap &glyph)
  {
    unsigned int index = 0;
    return GetGlyph(font, character, index) && RasterizeGlyph(font, pixel_size, index, {}, glyph);
  }

  bool FT_FontRasterizer::GetUnderline(
    const int font,
    const float pixel_size,
    float &position,
    float &thickness)
  {
    const Font *found = Find(font);
    if (found == nullptr || pixel_size <= 0.0f || found->face->units_per_EM == 0) { return false; }

    const FT_Face face = found->face;
    if (face->underline_thickness <= 0) { return false; }

    const float scale = pixel_size / static_cast<float>(face->units_per_EM);

    // a font says where the middle of the line is, counted upwards
    thickness = static_cast<float>(face->underline_thickness) * scale;
    position = static_cast<float>(-face->underline_position) * scale - thickness / 2.0f;
    return true;
  }

  bool FT_FontRasterizer::RasterizeGlyph(
    const int font,
    const float pixel_size,
    const unsigned int glyph,
    const GlyphOptions &options,
    GlyphBitmap &bitmap)
  {
    const Font *found = Find(font);
    if (found == nullptr || !SetSize(*found, pixel_size)) { return false; }

    const FT_Face face = found->face;
    if (glyph >= static_cast<unsigned int>(face->num_glyphs)) { return false; }

    const bool is_distance_field = options.rendering == GlyphRendering::DistanceField;
    const bool is_changed = options.slant != 0.0f || options.embolden != 0.0f || options.stroke > 0.0f;

    // Hints fit an outline to the pixels it was meant for. An outline that
    // is changed afterwards, or drawn at other sizes, is better left as it
    // was designed.
    FT_Int32 flags = is_distance_field || is_changed
      ? FT_LOAD_NO_HINTING
      : FT_LOAD_TARGET_LIGHT;
    flags |= FT_LOAD_NO_BITMAP;
    if (FT_HAS_COLOR(face) && !is_distance_field && !is_changed) { flags |= FT_LOAD_COLOR; }

    if (FT_Load_Glyph(face, glyph, flags) != 0) { return false; }

    const FT_GlyphSlot slot = face->glyph;

    bitmap = GlyphBitmap{};
    // as the design has it, and not rounded to a pixel
    bitmap.advance = static_cast<float>(slot->linearHoriAdvance) / 65536.0f;

    FT_Bitmap *drawn = nullptr;
    FT_Glyph stroked = nullptr;
    int left = 0;
    int top = 0;

    if (slot->format == FT_GLYPH_FORMAT_OUTLINE)
    {
      FT_Outline &outline = slot->outline;

      if (options.embolden != 0.0f) { FT_Outline_Embolden(&outline, ToParts(options.embolden)); }

      if (options.slant != 0.0f)
      {
        const FT_Matrix lean{
          0x10000, static_cast<FT_Fixed>(std::lround(options.slant * 65536.0f)),
          0, 0x10000
        };
        FT_Outline_Transform(&outline, &lean);
      }

      const float offset = std::clamp(options.offset_x, 0.0f, 1.0f);
      if (offset > 0.0f) { FT_Outline_Translate(&outline, ToParts(offset), 0); }

      if (options.stroke > 0.0f)
      {
        FT_Stroker stroker = nullptr;
        if (FT_Stroker_New(_library->library, &stroker) != 0) { return false; }

        // half of the line lies on each side of the outline
        FT_Stroker_Set(
          stroker,
          ToParts(options.stroke / 2.0f),
          FT_STROKER_LINECAP_ROUND,
          FT_STROKER_LINEJOIN_ROUND,
          0);

        bool is_stroked = FT_Get_Glyph(slot, &stroked) == 0;
        if (is_stroked) { is_stroked = FT_Glyph_Stroke(&stroked, stroker, 1) == 0; }
        if (is_stroked)
        {
          is_stroked = FT_Glyph_To_Bitmap(
            &stroked,
            is_distance_field ? FT_RENDER_MODE_SDF : FT_RENDER_MODE_NORMAL,
            nullptr,
            1) == 0;
        }

        FT_Stroker_Done(stroker);

        if (!is_stroked)
        {
          if (stroked != nullptr) { FT_Done_Glyph(stroked); }
          return false;
        }

        const auto as_bitmap = reinterpret_cast<FT_BitmapGlyph>(stroked);
        drawn = &as_bitmap->bitmap;
        left = as_bitmap->left;
        top = as_bitmap->top;
      }
    }

    if (drawn == nullptr)
    {
      if (slot->format != FT_GLYPH_FORMAT_BITMAP &&
          FT_Render_Glyph(slot, is_distance_field ? FT_RENDER_MODE_SDF : FT_RENDER_MODE_NORMAL) != 0)
      {
        return false;
      }

      drawn = &slot->bitmap;
      left = slot->bitmap_left;
      top = slot->bitmap_top;
    }

    const auto width = static_cast<int>(drawn->width);
    const auto height = static_cast<int>(drawn->rows);

    if (width > 0 && height > 0 && drawn->buffer != nullptr)
    {
      bitmap.width = width;
      bitmap.height = height;
      bitmap.left = left;
      bitmap.top = top;
      bitmap.is_distance_field = is_distance_field;
      bitmap.distance_range = is_distance_field ? static_cast<float>(kDistance_Range) : 0.0f;
      bitmap.coverage.assign(static_cast<std::size_t>(width) * height, 0);

      const bool has_colors = drawn->pixel_mode == FT_PIXEL_MODE_BGRA;
      if (has_colors) { bitmap.colors.assign(static_cast<std::size_t>(width) * height * 4, 0); }

      for (int row = 0; row < height; row++)
      {
        const unsigned char *from = drawn->buffer + static_cast<std::ptrdiff_t>(row) * drawn->pitch;

        for (int column = 0; column < width; column++)
        {
          const std::size_t to = static_cast<std::size_t>(row) * width + column;

          switch (drawn->pixel_mode)
          {
            case FT_PIXEL_MODE_GRAY:
              bitmap.coverage[to] = from[column];
              break;
            case FT_PIXEL_MODE_MONO:
              bitmap.coverage[to] = (from[column / 8] & (0x80 >> (column % 8))) != 0 ? 255 : 0;
              break;
            case FT_PIXEL_MODE_BGRA:
            {
              // FreeType multiplies alpha into the colours, and the engine
              // does so itself when it keeps a texture
              const unsigned char *pixel = from + static_cast<std::ptrdiff_t>(column) * 4;
              const unsigned int alpha = pixel[3];
              const auto straight = [alpha](const unsigned char value)
              {
                return alpha == 0
                  ? static_cast<unsigned char>(0)
                  : static_cast<unsigned char>(std::min(255u, (value * 255u + alpha / 2) / alpha));
              };

              bitmap.colors[to * 4 + 0] = straight(pixel[2]);
              bitmap.colors[to * 4 + 1] = straight(pixel[1]);
              bitmap.colors[to * 4 + 2] = straight(pixel[0]);
              bitmap.colors[to * 4 + 3] = pixel[3];
              bitmap.coverage[to] = pixel[3];
              break;
            }
            default:
              break;
          }
        }
      }
    }

    if (stroked != nullptr) { FT_Done_Glyph(stroked); }
    return true;
  }

  bool FT_FontRasterizer::PlacesAtPartsOfAPixel() const
  {
    return true;
  }
} // neon
