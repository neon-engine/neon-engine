#include "text-layout.hpp"

#include <algorithm>
#include <cmath>

namespace neon
{
  namespace
  {
    constexpr float epsilon = 0.01f;

    struct Line
    {
      std::size_t first = 0;
      std::size_t end = 0;
      float width = 0.0f;
    };

    float AdvanceOf(const FontAtlas &atlas, const char32_t character)
    {
      // a tab is a space, since nothing says where its stops are
      const auto *glyph = atlas.Find(character == U'\t' ? U' ' : character);
      return glyph != nullptr ? glyph->advance : 0.0f;
    }

    bool IsSpace(const char32_t character)
    {
      return character == U' ' || character == U'\t';
    }

    /// A carriage return comes with the line feeds of Windows. A soft
    /// hyphen says where a word may be broken, which is not done.
    bool IsLeftOut(const char32_t character)
    {
      return character == U'\r' || character == 0xAD;
    }
  }

  float LineHeightOf(const FontAtlas &atlas, const TextOptions &options)
  {
    if (options.line_height > 0.0f) { return std::round(options.line_height); }

    const auto &[ascent, descent, line_gap] = atlas.GetMetrics();
    return std::ceil(ascent + descent + line_gap);
  }

  PlacedText PlaceText(const FontAtlas &atlas, const std::u32string &text, const TextOptions &options)
  {
    PlacedText placed;
    if (text.empty()) { return placed; }

    const bool breaks = !std::isnan(options.max_width);

    std::vector<Line> lines;
    Line line;
    std::size_t last_space = text.size();
    float width_at_space = 0.0f;

    for (std::size_t i = 0; i < text.size(); i++)
    {
      const char32_t character = text[i];

      if (IsLeftOut(character)) { continue; }

      if (character == U'\n')
      {
        line.end = i;
        lines.push_back(line);
        line = {i + 1, i + 1, 0.0f};
        last_space = text.size();
        continue;
      }

      const float advance = AdvanceOf(atlas, character);

      if (breaks && !IsSpace(character) && last_space != text.size() &&
          line.width + advance > options.max_width + epsilon)
      {
        // the line ends in front of the space, and the word goes on in the
        // next one
        float carried = 0.0f;
        for (std::size_t k = last_space + 1; k < i; k++)
        {
          if (!IsLeftOut(text[k])) { carried += AdvanceOf(atlas, text[k]); }
        }

        lines.push_back({line.first, last_space, width_at_space});
        line = {last_space + 1, last_space + 1, carried};
        last_space = text.size();
      }

      if (IsSpace(character))
      {
        last_space = i;
        width_at_space = line.width;
      }

      line.width += advance;
    }

    line.end = text.size();
    lines.push_back(line);

    const auto &metrics = atlas.GetMetrics();
    const float line_height = LineHeightOf(atlas, options);

    // what the line is higher than the letters is shared above and below
    const float baseline = std::round((line_height - (metrics.ascent + metrics.descent)) / 2.0f + metrics.ascent);

    for (const auto &each : lines) { placed.width = std::max(placed.width, each.width); }

    placed.width = std::ceil(placed.width - epsilon);
    placed.height = line_height * static_cast<float>(lines.size());
    placed.lines = lines.size();

    const float box_width = std::isnan(options.box_width) ? placed.width : options.box_width;

    for (std::size_t number = 0; number < lines.size(); number++)
    {
      const Line &each = lines[number];

      float start = 0.0f;
      if (options.align == TextAlign::Center) { start = std::round((box_width - each.width) / 2.0f); }
      if (options.align == TextAlign::Right) { start = std::round(box_width - each.width); }

      const float top = line_height * static_cast<float>(number) + baseline;
      float pen = 0.0f;

      for (std::size_t i = each.first; i < each.end; i++)
      {
        const char32_t character = text[i];
        if (IsLeftOut(character)) { continue; }

        const auto *glyph = atlas.Find(character == U'\t' ? U' ' : character);
        if (glyph == nullptr) { continue; }

        if (glyph->width > 0 && glyph->height > 0)
        {
          placed.glyphs.push_back({
            glyph,
            start + std::round(pen) + static_cast<float>(glyph->left),
            top - static_cast<float>(glyph->top)
          });
        }

        pen += glyph->advance;
      }
    }

    return placed;
  }
} // neon
