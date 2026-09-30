#ifndef HB_TEXT_SHAPER_HPP
#define HB_TEXT_SHAPER_HPP

#include <memory>
#include <vector>

#include <neon/text/text-shaper.hpp>

namespace neon
{
  /// Shapes text with HarfBuzz: kerning, ligatures, the joined forms of
  /// Arabic, the marks and the reordering of the scripts of India, and
  /// whatever else a font says in its OpenType tables.
  ///
  /// A font is read with the OpenType functions of HarfBuzz itself. Where
  /// a glyph goes is worked out from the design of the font and is not
  /// rounded to pixels.
  // ReSharper disable once CppInconsistentNaming
  class HB_TextShaper final : public TextShaper
  {
    struct Font;
    struct Buffer;

    // a font is known by its place in here, which is empty once it is
    // unloaded
    std::vector<std::unique_ptr<Font>> _fonts;

    // used again for every text, so that shaping allocates nothing once
    // it is as large as the longest text
    std::unique_ptr<Buffer> _buffer;

    [[nodiscard]] const Font *Find(int font) const;

  public:
    HB_TextShaper();

    ~HB_TextShaper();

    HB_TextShaper(const HB_TextShaper &) = delete;

    HB_TextShaper &operator=(const HB_TextShaper &) = delete;

    int LoadFont(const std::vector<unsigned char> &file) override;

    void UnloadFont(int font) override;

    bool Shape(
      int font,
      float pixel_size,
      std::u32string_view text,
      std::size_t first,
      std::size_t count,
      const ShapingOptions &options,
      std::vector<ShapedGlyph> &glyphs) override;

    [[nodiscard]] CharacterDirection GetDirection(char32_t character) override;

    [[nodiscard]] std::uint32_t GetScript(char32_t character) override;
  };
} // neon

#endif //HB_TEXT_SHAPER_HPP
