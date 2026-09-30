#include "hb-text-shaper.hpp"

#include <cmath>
#include <cstddef>

#include <hb.h>
#include <hb-ot.h>

namespace neon
{
  namespace
  {
    // HarfBuzz counts in whole numbers, which are parts of 64 of a pixel
    // here
    constexpr float parts = 64.0f;

    bool BelongsToNoScript(const hb_script_t script)
    {
      return script == HB_SCRIPT_COMMON || script == HB_SCRIPT_INHERITED || script == HB_SCRIPT_UNKNOWN;
    }
  }

  struct HB_TextShaper::Font
  {
    // HarfBuzz reads from the bytes of the file for as long as the font is
    // used
    std::vector<unsigned char> file;
    hb_face_t *face = nullptr;
    hb_font_t *font = nullptr;

    ~Font()
    {
      if (font != nullptr) { hb_font_destroy(font); }
      if (face != nullptr) { hb_face_destroy(face); }
    }
  };

  struct HB_TextShaper::Buffer
  {
    hb_buffer_t *buffer = nullptr;
  };

  HB_TextShaper::HB_TextShaper()
  {
    _buffer = std::make_unique<Buffer>();
    _buffer->buffer = hb_buffer_create();
  }

  HB_TextShaper::~HB_TextShaper()
  {
    _fonts.clear();
    if (_buffer->buffer != nullptr) { hb_buffer_destroy(_buffer->buffer); }
  }

  const HB_TextShaper::Font *HB_TextShaper::Find(const int font) const
  {
    if (font < 0 || static_cast<std::size_t>(font) >= _fonts.size()) { return nullptr; }

    return _fonts[font].get();
  }

  int HB_TextShaper::LoadFont(const std::vector<unsigned char> &file)
  {
    if (file.empty()) { return -1; }

    auto font = std::make_unique<Font>();
    font->file = file;

    hb_blob_t *blob = hb_blob_create(
      reinterpret_cast<const char *>(font->file.data()),
      static_cast<unsigned int>(font->file.size()),
      HB_MEMORY_MODE_READONLY,
      nullptr,
      nullptr);

    font->face = hb_face_create(blob, 0);
    hb_blob_destroy(blob);

    // bytes that are not a font give a face without glyphs
    if (font->face == nullptr || hb_face_get_glyph_count(font->face) == 0) { return -1; }

    font->font = hb_font_create(font->face);
    if (font->font == nullptr) { return -1; }

    hb_ot_font_set_funcs(font->font);

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

  void HB_TextShaper::UnloadFont(const int font)
  {
    if (Find(font) != nullptr) { _fonts[font].reset(); }
  }

  bool HB_TextShaper::Shape(
    const int font,
    const float pixel_size,
    const std::u32string_view text,
    const std::size_t first,
    const std::size_t count,
    const ShapingOptions &options,
    std::vector<ShapedGlyph> &glyphs)
  {
    glyphs.clear();

    const Font *found = Find(font);
    if (found == nullptr || pixel_size <= 0.0f || _buffer->buffer == nullptr) { return false; }
    if (first > text.size() || count > text.size() - first) { return false; }
    if (count == 0) { return true; }

    const int scale = static_cast<int>(std::lround(pixel_size * parts));
    hb_font_set_scale(found->font, scale, scale);

    hb_buffer_t *buffer = _buffer->buffer;
    hb_buffer_reset(buffer);

    // a character is a number of 32 bits to both
    static_assert(sizeof(hb_codepoint_t) == sizeof(char32_t));
    hb_buffer_add_codepoints(
      buffer,
      reinterpret_cast<const hb_codepoint_t *>(text.data()),
      static_cast<int>(text.size()),
      static_cast<unsigned int>(first),
      static_cast<int>(count));

    hb_buffer_set_direction(
      buffer,
      options.direction == TextDirection::RightToLeft ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);

    // the script is that of the first character that has one
    hb_unicode_funcs_t *unicode = hb_unicode_funcs_get_default();
    for (std::size_t i = first; i < first + count; i++)
    {
      const hb_script_t script = hb_unicode_script(unicode, text[i]);
      if (!BelongsToNoScript(script))
      {
        hb_buffer_set_script(buffer, script);
        break;
      }
    }

    hb_buffer_guess_segment_properties(buffer);

    hb_feature_t features[8];
    unsigned int feature_count = 0;

    const auto switch_off = [&features, &feature_count](const hb_tag_t tag)
    {
      features[feature_count++] = {tag, 0, HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END};
    };

    if (!options.kerning) { switch_off(HB_TAG('k', 'e', 'r', 'n')); }
    if (!options.ligatures)
    {
      switch_off(HB_TAG('l', 'i', 'g', 'a'));
      switch_off(HB_TAG('c', 'l', 'i', 'g'));
      switch_off(HB_TAG('d', 'l', 'i', 'g'));

      // what a font joins by looking at the neighbours, as Inter does
      // with the two characters of an arrow
      switch_off(HB_TAG('c', 'a', 'l', 't'));
    }

    hb_shape(found->font, buffer, features, feature_count);

    unsigned int length = 0;
    const hb_glyph_info_t *infos = hb_buffer_get_glyph_infos(buffer, &length);
    const hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buffer, &length);
    if (infos == nullptr || positions == nullptr) { return length == 0; }

    glyphs.reserve(length);
    for (unsigned int i = 0; i < length; i++)
    {
      ShapedGlyph glyph;
      glyph.glyph = infos[i].codepoint;
      glyph.cluster = infos[i].cluster;
      glyph.advance = static_cast<float>(positions[i].x_advance) / parts;
      glyph.offset_x = static_cast<float>(positions[i].x_offset) / parts;
      glyph.offset_y = static_cast<float>(positions[i].y_offset) / parts;
      glyphs.push_back(glyph);
    }

    return true;
  }

  CharacterDirection HB_TextShaper::GetDirection(const char32_t character)
  {
    const hb_script_t script = hb_unicode_script(hb_unicode_funcs_get_default(), character);
    if (BelongsToNoScript(script)) { return CharacterDirection::Neutral; }

    return hb_script_get_horizontal_direction(script) == HB_DIRECTION_RTL
      ? CharacterDirection::RightToLeft
      : CharacterDirection::LeftToRight;
  }

  std::uint32_t HB_TextShaper::GetScript(const char32_t character)
  {
    const hb_script_t script = hb_unicode_script(hb_unicode_funcs_get_default(), character);
    return BelongsToNoScript(script) ? 0u : static_cast<std::uint32_t>(script);
  }
} // neon
