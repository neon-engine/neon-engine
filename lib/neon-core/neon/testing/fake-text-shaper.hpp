#ifndef FAKE_TEXT_SHAPER_HPP
#define FAKE_TEXT_SHAPER_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

#include <neon/text/text-shaper.hpp>

namespace neon::testing
{
  /// A shaper whose rules are few and known, so that a test can say what a
  /// text has to become without a real font.
  ///
  /// Every character is the glyph of its own number, and moves the pen by
  /// half the size, as the font of the tests does. An `A` in front of a `V`
  /// moves it a tenth of the size less. An `f` in front of an `i` is joined
  /// with it into the glyph kLigature, which moves the pen as one character
  /// does. Text that runs from right to left comes out last character
  /// first.
  ///
  /// Hebrew and Arabic run from right to left. Spaces, digits, and
  /// punctuation have no script and no direction.
  class FakeTextShaper final : public TextShaper
  {
    int _fonts = 0;
    std::set<int> _unloaded;

  public:
    static constexpr unsigned int kLigature = 0xFB01;

    /// How often a text was shaped.
    std::size_t shaped = 0;

    /// Makes Shape() fail.
    bool refuses = false;

    [[nodiscard]] static bool IsRightToLeft(const char32_t character)
    {
      return character >= 0x0590 && character <= 0x08FF;
    }

    [[nodiscard]] static bool HasNoScript(const char32_t character)
    {
      return character < U'A' || (character > U'Z' && character < U'a') ||
             (character > U'z' && character < 0xC0) ||
             (character >= 0x0300 && character <= 0x036F);
    }

    int LoadFont(const std::vector<unsigned char> &file) override
    {
      return file.empty() ? -1 : _fonts++;
    }

    void UnloadFont(const int font) override
    {
      _unloaded.insert(font);
    }

    bool Shape(
      const int font,
      const float pixel_size,
      const std::u32string_view text,
      const std::size_t first,
      const std::size_t count,
      const ShapingOptions &options,
      std::vector<ShapedGlyph> &glyphs) override
    {
      glyphs.clear();

      if (refuses || font < 0 || font >= _fonts || _unloaded.contains(font)) { return false; }
      if (first > text.size() || count > text.size() - first) { return false; }

      shaped++;

      for (std::size_t i = first; i < first + count; i++)
      {
        ShapedGlyph glyph;
        glyph.glyph = static_cast<unsigned int>(text[i]);
        glyph.cluster = static_cast<std::uint32_t>(i);
        glyph.advance = pixel_size * 0.5f;

        const bool has_next = i + 1 < first + count;

        if (options.ligatures && has_next && text[i] == U'f' && text[i + 1] == U'i')
        {
          glyph.glyph = kLigature;
          glyphs.push_back(glyph);
          i++;
          continue;
        }

        if (options.kerning && has_next && text[i] == U'A' && text[i + 1] == U'V')
        {
          glyph.advance -= pixel_size * 0.1f;
        }

        glyphs.push_back(glyph);
      }

      if (options.direction == TextDirection::RightToLeft) { std::ranges::reverse(glyphs); }
      return true;
    }

    [[nodiscard]] CharacterDirection GetDirection(const char32_t character) override
    {
      if (HasNoScript(character)) { return CharacterDirection::Neutral; }

      return IsRightToLeft(character) ? CharacterDirection::RightToLeft : CharacterDirection::LeftToRight;
    }

    [[nodiscard]] std::uint32_t GetScript(const char32_t character) override
    {
      if (HasNoScript(character)) { return 0; }
      if (character >= 0x0590 && character <= 0x05FF) { return 3; }
      if (character >= 0x0600 && character <= 0x08FF) { return 0x41726162; } // Arab
      if (character >= 0x4E00 && character <= 0x9FFF) { return 4; }
      return 1;
    }
  };
} // neon::testing

#endif //FAKE_TEXT_SHAPER_HPP
