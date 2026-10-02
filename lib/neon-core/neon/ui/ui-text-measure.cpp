#include "ui-text-measure.hpp"

#include <algorithm>
#include <cmath>

#include <neon/text/utf8.hpp>

#include "ui-text-editor.hpp"

namespace neon
{
  // Helpers of UiTextMeasure, for this file alone.
  namespace
  {
    constexpr float epsilon = 0.01f;

    /// A character of the text with the bytes it takes, and how far it
    /// moves the pen.
    struct Placed
    {
      std::size_t offset = 0;
      std::size_t length = 0;
      char32_t character = 0;
      float advance = 0.0f;
    };

    bool IsSpace(const char32_t character)
    {
      return character == U' ' || character == U'\t';
    }

    bool IsLeftOut(const char32_t character)
    {
      return character == U'\r' || character == 0xAD;
    }

    float AdvanceOf(const FontAtlas &atlas, const char32_t character)
    {
      const auto *glyph = atlas.Find(character == U'\t' ? U' ' : character);
      return glyph != nullptr ? glyph->advance : 0.0f;
    }

    /// The characters of a text, each with the bytes it is made of.
    std::vector<Placed> CharactersOf(const std::string &text, const FontAtlas &atlas)
    {
      std::vector<Placed> characters;

      for (std::size_t at = 0; at < text.size();)
      {
        // one byte at a time, as DecodeUtf8() reads the text
        const std::size_t next = std::min(text.size(), UiTextBoundaries::Next(text, at));

        // what a caret takes for one may be drawn as several: an e and
        // an accent on top of it are two characters of the font
        const std::u32string decoded = DecodeUtf8(std::string_view(text).substr(at, next - at));

        std::size_t inner = at;
        for (std::size_t i = 0; i < decoded.size(); i++)
        {
          // the bytes of each, so that a place inside is found
          std::size_t bytes = 1;
          const auto first = static_cast<unsigned char>(text[inner]);
          if (first >= 0xF0) { bytes = 4; }
          else if (first >= 0xE0) { bytes = 3; }
          else if (first >= 0xC0) { bytes = 2; }
          bytes = std::min(bytes, next - inner);

          const char32_t character = decoded[i];
          characters.push_back({inner, bytes, character, IsLeftOut(character) ? 0.0f : AdvanceOf(atlas, character)});

          inner += bytes;
        }

        // what the decoding did not account for is given to the last
        if (!characters.empty() && inner < next) { characters.back().length += next - inner; }

        at = next;
      }

      return characters;
    }

    struct LaidOutLine
    {
      // places in the characters
      std::size_t first = 0;
      std::size_t end = 0;
      float width = 0.0f;
    };

    /// The lines, as PlaceText() breaks them.
    std::vector<LaidOutLine> BreakLines(
      const std::vector<Placed> &characters,
      const TextOptions &options,
      const bool breaks_long_words)
    {
      std::vector<LaidOutLine> lines;
      const bool breaks = !std::isnan(options.max_width);

      LaidOutLine line;
      std::size_t last_space = characters.size();
      float width_at_space = 0.0f;

      for (std::size_t i = 0; i < characters.size(); i++)
      {
        const Placed &placed = characters[i];

        if (IsLeftOut(placed.character)) { continue; }

        if (placed.character == U'\n')
        {
          line.end = i;
          lines.push_back(line);
          line = {i + 1, i + 1, 0.0f};
          last_space = characters.size();
          continue;
        }

        if (breaks && !IsSpace(placed.character) && last_space != characters.size() &&
            line.width + placed.advance > options.max_width + epsilon)
        {
          float carried = 0.0f;
          for (std::size_t k = last_space + 1; k < i; k++) { carried += characters[k].advance; }

          lines.push_back({line.first, last_space, width_at_space});
          line = {last_space + 1, last_space + 1, carried};
          last_space = characters.size();
        }

        // a word too wide for a line is broken where it no longer fits
        if (breaks && breaks_long_words && !IsSpace(placed.character) && i > line.first &&
            line.width + placed.advance > options.max_width + epsilon)
        {
          lines.push_back({line.first, i, line.width});
          line = {i, i, 0.0f};
          last_space = characters.size();
        }

        if (IsSpace(placed.character))
        {
          last_space = i;
          width_at_space = line.width;
        }

        line.width += placed.advance;
      }

      line.end = characters.size();
      lines.push_back(line);
      return lines;
    }

    float StartOf(const LaidOutLine &line, const TextOptions &options, const float text_width)
    {
      const float box_width = std::isnan(options.box_width) ? text_width : options.box_width;

      if (options.align == TextAlign::Center) { return std::round((box_width - line.width) / 2.0f); }
      if (options.align == TextAlign::Right) { return std::round(box_width - line.width); }
      return 0.0f;
    }

    float WidthOf(const std::vector<LaidOutLine> &lines)
    {
      float width = 0.0f;
      for (const auto &line : lines) { width = std::max(width, line.width); }
      return std::ceil(width - epsilon);
    }
  }

  const Atlas_UiTextMeasure &Atlas_UiTextMeasure::Get()
  {
    static const Atlas_UiTextMeasure measure;
    return measure;
  }

  std::vector<UiTextMeasure::Line> Atlas_UiTextMeasure::LinesOf(const Request &request) const
  {
    std::vector<Line> lines;
    if (request.font == nullptr) { return lines; }

    const auto characters = CharactersOf(request.text, request.font->atlas);
    const auto laid_out = BreakLines(characters, request.options, request.breaks_long_words);
    const float line_height = LineHeightOf(request.font->atlas, request.options);

    for (std::size_t number = 0; number < laid_out.size(); number++)
    {
      const LaidOutLine &each = laid_out[number];

      Line line;
      line.start = each.first < characters.size() ? characters[each.first].offset : request.text.size();
      line.end = each.end < characters.size() ? characters[each.end].offset : request.text.size();
      line.top = line_height * static_cast<float>(number);
      line.height = line_height;
      line.width = each.width;
      lines.push_back(line);
    }

    return lines;
  }

  UiTextMeasure::Caret Atlas_UiTextMeasure::CaretAt(const Request &request, const std::size_t offset) const
  {
    Caret caret;
    if (request.font == nullptr) { return caret; }

    const auto characters = CharactersOf(request.text, request.font->atlas);
    const auto lines = BreakLines(characters, request.options, request.breaks_long_words);
    const float line_height = LineHeightOf(request.font->atlas, request.options);
    const float text_width = WidthOf(lines);

    caret.height = line_height;

    // the line the place is in: the one that holds the character at the
    // place, or the one that ends there
    std::size_t number = 0;
    for (std::size_t i = 0; i < lines.size(); i++)
    {
      const LaidOutLine &line = lines[i];
      const std::size_t start = line.first < characters.size() ? characters[line.first].offset : request.text.size();
      const std::size_t end = line.end < characters.size() ? characters[line.end].offset : request.text.size();

      if (offset < start) { break; }

      number = i;

      // at the end of a line that was broken, the caret is at the start of
      // the next
      if (offset < end || (offset == end && i + 1 == lines.size())) { break; }
      if (offset == end && line.end < characters.size() && characters[line.end].character == U'\n') { break; }
    }

    const LaidOutLine &line = lines[number];
    float pen = 0.0f;

    for (std::size_t i = line.first; i < line.end; i++)
    {
      if (characters[i].offset >= offset) { break; }
      pen += characters[i].advance;
    }

    caret.line = number;
    caret.x = StartOf(line, request.options, text_width) + std::round(pen);
    caret.y = line_height * static_cast<float>(number);
    return caret;
  }

  std::size_t Atlas_UiTextMeasure::OffsetAt(const Request &request, const float x, const float y) const
  {
    if (request.font == nullptr) { return 0; }

    const auto characters = CharactersOf(request.text, request.font->atlas);
    const auto lines = BreakLines(characters, request.options, request.breaks_long_words);
    const float line_height = LineHeightOf(request.font->atlas, request.options);
    const float text_width = WidthOf(lines);

    // the line that is pointed at, or the nearest
    const auto number = static_cast<std::size_t>(std::clamp(
      std::floor(y / std::max(1.0f, line_height)), 0.0f, static_cast<float>(lines.size() - 1)));
    const LaidOutLine &line = lines[number];

    float pen = StartOf(line, request.options, text_width);
    std::size_t nearest = line.first < characters.size() ? characters[line.first].offset : request.text.size();

    for (std::size_t i = line.first; i < line.end; i++)
    {
      const Placed &placed = characters[i];

      // nearer to the end of the character than to its start
      if (x < pen + placed.advance / 2.0f) { return placed.offset; }

      pen += placed.advance;
      nearest = placed.offset + placed.length;
    }

    // behind the last character of the line, in front of what ends it
    return UiTextBoundaries::Snap(request.text, std::min(nearest, request.text.size()));
  }

  LayoutSize Atlas_UiTextMeasure::SizeOf(const Request &request) const
  {
    if (request.font == nullptr) { return {}; }

    const auto characters = CharactersOf(request.text, request.font->atlas);
    const auto lines = BreakLines(characters, request.options, request.breaks_long_words);
    const float line_height = LineHeightOf(request.font->atlas, request.options);

    return {WidthOf(lines), line_height * static_cast<float>(lines.size())};
  }
} // neon
