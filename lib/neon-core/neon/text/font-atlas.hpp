#ifndef FONT_ATLAS_HPP
#define FONT_ATLAS_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "font-rasterizer.hpp"

namespace neon
{
  /// Characters from `first` to `last`, both included.
  struct CharacterRange
  {
    char32_t first = 0;
    char32_t last = 0;
  };

  /// Where a character is in an atlas, and how it is placed when drawn.
  struct AtlasGlyph
  {
    /// The picture in the atlas, in pixels. Empty for a character that
    /// draws nothing.
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    /// From the pen to the left edge, and from the baseline up to the top
    /// edge of the picture.
    int left = 0;
    int top = 0;

    float advance = 0.0f;
  };

  /// The characters of one font at one size, drawn once into one image.
  /// Text is then drawn by copying from that image, which one draw call can
  /// do for a whole text.
  ///
  /// The image is white with the character in the alpha channel, four bytes
  /// for each pixel. The pictures are kept a pixel apart, so that one does
  /// not show at the edge of another.
  class FontAtlas
  {
    int _width = 0;
    int _height = 0;
    float _pixel_size = 0.0f;
    FontMetrics _metrics;
    std::vector<unsigned char> _pixels;
    std::unordered_map<char32_t, AtlasGlyph> _glyphs;

  public:
    /// The largest image that is tried, along each side.
    static constexpr int max_size = 4096;

    /// The characters that are drawn into an atlas: Basic Latin, Latin-1
    /// Supplement, Latin Extended-A, the punctuation that is common in
    /// text, the euro sign, and the replacement character.
    [[nodiscard]] static const std::vector<CharacterRange> &DefaultCharacters();

    /// Draws every character of the ranges that the font has. Returns false
    /// and says why when the font cannot be used, or when the characters do
    /// not fit into the largest image.
    bool Build(
      FontRasterizer &rasterizer,
      int font,
      float pixel_size,
      const std::vector<CharacterRange> &characters,
      std::string &error);

    /// The character, or what stands in for one the atlas does not have:
    /// the replacement character, then a question mark. nullptr when the
    /// atlas has none of them.
    [[nodiscard]] const AtlasGlyph *Find(char32_t character) const;

    /// Whether the atlas has the character itself.
    [[nodiscard]] bool Has(char32_t character) const;

    [[nodiscard]] int GetWidth() const;

    [[nodiscard]] int GetHeight() const;

    [[nodiscard]] float GetPixelSize() const;

    [[nodiscard]] const FontMetrics &GetMetrics() const;

    /// Red, green, blue, and alpha for each pixel, row after row from the
    /// top.
    [[nodiscard]] const std::vector<unsigned char> &GetPixels() const;
  };
} // neon

#endif //FONT_ATLAS_HPP
