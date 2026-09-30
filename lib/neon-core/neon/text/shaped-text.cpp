#include "shaped-text.hpp"

#include <algorithm>
#include <cmath>

#include "utf8.hpp"

namespace neon
{
  namespace
  {
    constexpr float epsilon = 0.01f;

    // scripts whose letters are joined, and are not spaced apart
    constexpr std::uint32_t joined_scripts[] = {
      0x41726162, // Arab
      0x53797263, // Syrc
      0x4D6F6E67, // Mong
      0x4E6B6F6F, // Nkoo
      0x4D616E64, // Mand
      0x41646C6D, // Adlm
    };

    bool IsSpace(const char32_t character)
    {
      return character == U' ';
    }

    bool IsDigit(const char32_t character)
    {
      return (character >= U'0' && character <= U'9') ||
             (character >= 0x0660 && character <= 0x0669) ||
             (character >= 0x06F0 && character <= 0x06F9);
    }

    /// Characters of Chinese, Japanese, and Korean, between which a line
    /// may end.
    bool IsIdeograph(const char32_t character)
    {
      return (character >= 0x2E80 && character <= 0x2FDF) ||
             (character >= 0x3040 && character <= 0x30FF) ||
             (character >= 0x3400 && character <= 0x4DBF) ||
             (character >= 0x4E00 && character <= 0x9FFF) ||
             (character >= 0xAC00 && character <= 0xD7AF) ||
             (character >= 0xF900 && character <= 0xFAFF) ||
             (character >= 0x20000 && character <= 0x3FFFF);
    }

    /// What is never the first on a line: the marks that close a sentence
    /// or a bracket.
    bool ClosesSomething(const char32_t character)
    {
      return character == 0x3001 || character == 0x3002 || character == 0xFF0C || character == 0xFF0E ||
             character == 0xFF01 || character == 0xFF1F || character == 0x300D || character == 0x300F ||
             character == 0xFF09 || character == 0x3009 || character == 0x300B;
    }

    bool Breaks(const WhiteSpace white_space)
    {
      return white_space == WhiteSpace::Normal || white_space == WhiteSpace::PreWrap ||
             white_space == WhiteSpace::PreLine;
    }

    /// The characters as they are shown.
    std::u32string Prepare(const std::u32string_view text, const ShapingStyle &style)
    {
      const bool joins_spaces = style.white_space == WhiteSpace::Normal ||
                                style.white_space == WhiteSpace::NoWrap ||
                                style.white_space == WhiteSpace::PreLine;
      const bool keeps_lines = style.white_space != WhiteSpace::Normal &&
                               style.white_space != WhiteSpace::NoWrap;

      std::u32string prepared;
      prepared.reserve(text.size());

      for (char32_t character : text)
      {
        // A carriage return comes with the line feeds of Windows. A soft
        // hyphen says where a word may be broken, which is not done.
        if (character == U'\r' || character == 0xAD) { continue; }

        // a tab is a space, since nothing says where its stops are
        if (character == U'\t') { character = U' '; }
        if (character == U'\n' && !keeps_lines) { character = U' '; }

        if (joins_spaces && character == U' ')
        {
          // none at the start of a line, and one between two words
          if (prepared.empty() || prepared.back() == U' ' || prepared.back() == U'\n') { continue; }
        }

        if (joins_spaces && character == U'\n')
        {
          while (!prepared.empty() && prepared.back() == U' ') { prepared.pop_back(); }
        }

        prepared += character;
      }

      if (joins_spaces)
      {
        while (!prepared.empty() && prepared.back() == U' ') { prepared.pop_back(); }
      }

      return style.transform == TextTransform::None ? prepared : TransformCase(prepared, style.transform);
    }

    struct Character
    {
      std::size_t font = 0;
      std::uint32_t script = 0;
      TextDirection direction = TextDirection::LeftToRight;

      // stands in for what no font has
      char32_t drawn = 0;
    };

    bool HasGlyph(const TextFont &font, const char32_t character)
    {
      return font.rasterizer != nullptr && font.font >= 0 && font.rasterizer->HasGlyph(font.font, character);
    }
  }

  float LineHeightOf(const FontMetrics &metrics, const float line_height)
  {
    if (line_height > 0.0f) { return std::round(line_height); }

    return std::ceil(metrics.ascent + metrics.descent + metrics.line_gap);
  }

  ShapedText ShapeText(
    const std::u32string_view text,
    const std::vector<TextFont> &fonts,
    const ShapingStyle &style)
  {
    ShapedText shaped;
    shaped.direction = style.direction;
    shaped.breaks_lines = Breaks(style.white_space);
    shaped.characters = Prepare(text, style);

    if (fonts.empty() || shaped.characters.empty()) { return shaped; }

    std::u32string &characters = shaped.characters;
    const std::size_t count = characters.size();

    TextShaper *shaper = nullptr;
    for (const auto &font : fonts)
    {
      if (font.shaper != nullptr && font.shaper_font >= 0) { shaper = font.shaper; }
    }

    // what no font has is replaced first, so that it is shaped as what is
    // drawn
    for (char32_t &character : characters)
    {
      if (character == U'\n') { continue; }

      const bool is_known = std::ranges::any_of(fonts, [character](const TextFont &font)
      {
        return HasGlyph(font, character);
      });
      if (is_known) { continue; }

      for (const char32_t candidate : {Replacement_Character, static_cast<char32_t>(U'?')})
      {
        if (std::ranges::any_of(fonts, [candidate](const TextFont &font) { return HasGlyph(font, candidate); }))
        {
          character = candidate;
          break;
        }
      }
    }

    // The way every character runs. One that has no way of its own runs as
    // what is on both sides of it, and as the text where those differ.
    std::vector<int> ways(count, -1);
    for (std::size_t i = 0; i < count; i++)
    {
      const char32_t character = characters[i];

      if (IsDigit(character))
      {
        // numbers are written from left to right in every script
        ways[i] = 0;
      } else if (shaper != nullptr)
      {
        switch (shaper->GetDirection(character))
        {
          case CharacterDirection::LeftToRight:
            ways[i] = 0;
            break;
          case CharacterDirection::RightToLeft:
            ways[i] = 1;
            break;
          default:
            break;
        }
      } else if (character != U' ' && character != U'\n')
      {
        ways[i] = 0;
      }
    }

    const int way_of_text = style.direction == TextDirection::RightToLeft ? 1 : 0;

    for (std::size_t i = 0; i < count;)
    {
      if (ways[i] >= 0)
      {
        i++;
        continue;
      }

      std::size_t end = i;
      while (end < count && ways[end] < 0 && characters[end] != U'\n') { end++; }

      const bool starts_line = i == 0 || characters[i - 1] == U'\n';
      const bool ends_line = end >= count || characters[end] == U'\n';

      const int before = starts_line ? way_of_text : ways[i - 1];
      const int after = ends_line ? way_of_text : ways[end];
      const int way = before == after ? before : way_of_text;

      for (std::size_t k = i; k < end; k++) { ways[k] = way; }

      // a line feed belongs to no part
      i = end < count && characters[end] == U'\n' ? end + 1 : std::max(end, i + 1);
    }

    std::vector<Character> described(count);
    for (std::size_t i = 0; i < count; i++)
    {
      Character &each = described[i];
      const char32_t character = characters[i];

      each.direction = ways[i] == 1 ? TextDirection::RightToLeft : TextDirection::LeftToRight;
      each.script = shaper != nullptr ? shaper->GetScript(character) : 0;

      const bool belongs_to_no_script = each.script == 0 && (shaper != nullptr || character == U' ');
      const bool follows = i > 0 && characters[i - 1] != U'\n' &&
                           described[i - 1].direction == each.direction;

      // What belongs to no script, as a space, is shaped with what is
      // before it, and drawn with its font where that has the character,
      // so that a space does not split a text in two.
      if (belongs_to_no_script && follows)
      {
        each.script = described[i - 1].script;

        if (HasGlyph(fonts[described[i - 1].font], character))
        {
          each.font = described[i - 1].font;
          continue;
        }
      }

      for (std::size_t font = 0; font < fonts.size(); font++)
      {
        if (HasGlyph(fonts[font], character))
        {
          each.font = font;
          break;
        }
      }
    }

    // a script that is not known by now is that of what follows, as of a
    // space at the start of a line
    for (std::size_t i = count; i > 0; i--)
    {
      if (described[i - 1].script == 0 && i < count && characters[i] != U'\n' &&
          described[i].direction == described[i - 1].direction)
      {
        described[i - 1].script = described[i].script;

        if (HasGlyph(fonts[described[i].font], characters[i - 1])) { described[i - 1].font = described[i].font; }
      }
    }

    shaped.advances.assign(count, 0.0f);
    std::vector<ShapedGlyph> glyphs;

    for (std::size_t first = 0; first < count;)
    {
      if (characters[first] == U'\n')
      {
        first++;
        continue;
      }

      std::size_t end = first + 1;
      while (end < count && characters[end] != U'\n' &&
             described[end].font == described[first].font &&
             described[end].script == described[first].script &&
             described[end].direction == described[first].direction)
      {
        end++;
      }

      const TextFont &font = fonts[described[first].font];
      const bool is_joined = std::ranges::find(joined_scripts, described[first].script) != std::end(joined_scripts);
      const float letter_spacing = is_joined ? 0.0f : style.letter_spacing;

      ShapedRun run;
      run.first = first;
      run.end = end;
      run.font = described[first].font;
      run.direction = described[first].direction;
      run.first_glyph = shaped.glyphs.size();

      glyphs.clear();
      bool is_shaped = false;

      if (font.shaper != nullptr && font.shaper_font >= 0)
      {
        ShapingOptions options;
        options.direction = run.direction;

        // letters that are spaced apart are not joined, as CSS says
        options.ligatures = letter_spacing == 0.0f;

        is_shaped = font.shaper->Shape(
          font.shaper_font, font.pixel_size, characters, first, end - first, options, glyphs);
      }

      if (!is_shaped)
      {
        // every character as the glyph its font maps it to
        glyphs.clear();
        run.direction = TextDirection::LeftToRight;

        for (std::size_t i = first; i < end; i++)
        {
          ShapedGlyph glyph;
          glyph.cluster = static_cast<std::uint32_t>(i);

          if (font.rasterizer == nullptr || !font.rasterizer->GetGlyph(font.font, characters[i], glyph.glyph))
          {
            continue;
          }

          if (font.atlas != nullptr)
          {
            if (const CachedGlyph *picture = font.atlas->Find(glyph.glyph); picture != nullptr)
            {
              glyph.advance = picture->advance * font.scale;
            }
          }

          glyphs.push_back(glyph);
        }
      }

      // The spacing goes behind the glyph that is drawn last of those of
      // a character.
      for (std::size_t i = 0; i < glyphs.size(); i++)
      {
        const std::uint32_t cluster = glyphs[i].cluster;
        const bool is_last = i + 1 >= glyphs.size() || glyphs[i + 1].cluster != cluster;

        if (is_last && cluster < count)
        {
          if (letter_spacing != 0.0f) { glyphs[i].advance += letter_spacing; }
          if (style.word_spacing != 0.0f && (characters[cluster] == U' ' || characters[cluster] == 0xA0))
          {
            glyphs[i].advance += style.word_spacing;
          }
        }

        if (cluster < count) { shaped.advances[cluster] += glyphs[i].advance; }
      }

      run.glyph_count = glyphs.size();
      shaped.glyphs.insert(shaped.glyphs.end(), glyphs.begin(), glyphs.end());
      shaped.runs.push_back(run);

      first = end;
    }

    return shaped;
  }

  namespace
  {
    struct Line
    {
      std::size_t first = 0;
      std::size_t end = 0;
      float width = 0.0f;
    };

    std::vector<Line> BreakIntoLines(const ShapedText &text, const PlacingOptions &options, const bool breaks)
    {
      const std::u32string &characters = text.characters;
      const std::size_t none = characters.size();

      std::vector<Line> lines;
      Line line;

      // where the line may end: in front of `next_start`, with what is in
      // front of `break_end` on it
      std::size_t break_end = none;
      std::size_t next_start = none;
      float width_at_break = 0.0f;

      for (std::size_t i = 0; i < characters.size(); i++)
      {
        const char32_t character = characters[i];

        if (character == U'\n')
        {
          line.end = i;
          lines.push_back(line);
          line = {i + 1, i + 1, 0.0f};
          break_end = none;
          continue;
        }

        const float advance = text.advances[i];

        // between two characters of Chinese, Japanese, or Korean
        if (breaks && i > line.first && !IsSpace(character) && !ClosesSomething(character) &&
            (IsIdeograph(character) || IsIdeograph(characters[i - 1])) && !IsSpace(characters[i - 1]))
        {
          break_end = i;
          next_start = i;
          width_at_break = line.width;
        }

        if (breaks && !IsSpace(character) && break_end != none &&
            line.width + advance > options.max_width + epsilon)
        {
          float carried = 0.0f;
          for (std::size_t k = next_start; k < i; k++) { carried += text.advances[k]; }

          lines.push_back({line.first, break_end, width_at_break});
          line = {next_start, next_start, carried};
          break_end = none;
        }

        if (IsSpace(character))
        {
          break_end = i;
          next_start = i + 1;
          width_at_break = line.width;
        }

        line.width += advance;
      }

      line.end = characters.size();
      lines.push_back(line);
      return lines;
    }

    /// The pen of a glyph as a whole pixel and the picture that is nearest
    /// to what is left.
    void SplitPen(const float pen, const bool places_at_parts, float &whole, std::uint8_t &variant)
    {
      if (!places_at_parts)
      {
        whole = std::round(pen);
        variant = 0;
        return;
      }

      whole = std::floor(pen);
      auto quarter = static_cast<int>(std::lround((pen - whole) * static_cast<float>(GlyphAtlas::kVariants)));

      if (quarter >= GlyphAtlas::kVariants)
      {
        whole += 1.0f;
        quarter = 0;
      }

      variant = static_cast<std::uint8_t>(quarter);
    }
  }

  PlacedShapedText PlaceShapedText(
    const ShapedText &text,
    const std::vector<TextFont> &fonts,
    const PlacingOptions &options)
  {
    PlacedShapedText placed;
    if (text.characters.empty() || fonts.empty() || fonts.front().atlas == nullptr) { return placed; }

    const bool breaks = !std::isnan(options.max_width) && text.breaks_lines;
    const std::vector<Line> lines = BreakIntoLines(text, options, breaks);

    const FontMetrics &metrics = fonts.front().atlas->GetMetrics();
    const float scale = fonts.front().scale;
    const float ascent = metrics.ascent * scale;
    const float descent = metrics.descent * scale;

    float line_height = options.line_height > 0.0f
      ? std::round(options.line_height)
      : std::ceil((metrics.ascent + metrics.descent + metrics.line_gap) * scale);
    line_height = std::max(line_height, 1.0f);

    // what the line is higher than the letters is shared above and below
    const float baseline = std::round((line_height - (ascent + descent)) / 2.0f + ascent);

    for (const auto &line : lines) { placed.width = std::max(placed.width, line.width); }

    placed.width = std::ceil(placed.width - epsilon);
    placed.height = line_height * static_cast<float>(lines.size());

    const float box_width = std::isnan(options.box_width) ? placed.width : options.box_width;
    const bool runs_right_to_left = text.direction == TextDirection::RightToLeft;

    // what the ellipsis is drawn with
    const bool cuts_off = options.overflow == TextOverflow::Ellipsis && !std::isnan(options.box_width);

    std::vector<const ShapedRun *> runs_of_line;

    for (std::size_t number = 0; number < lines.size(); number++)
    {
      const Line &line = lines[number];

      PlacedLine placed_line;
      placed_line.first_glyph = placed.glyphs.size();
      placed_line.baseline = line_height * static_cast<float>(number) + baseline;

      float width = line.width;

      // how much of the line is drawn, from the start it is read from
      float limit = std::numeric_limits<float>::max();
      unsigned int ellipsis = 0;
      std::size_t ellipsis_font = 0;
      float ellipsis_advance = 0.0f;

      if (cuts_off && line.width > box_width + 0.5f)
      {
        for (std::size_t font = 0; font < fonts.size() && ellipsis == 0; font++)
        {
          if (fonts[font].rasterizer == nullptr || fonts[font].atlas == nullptr) { continue; }
          if (!fonts[font].rasterizer->GetGlyph(fonts[font].font, 0x2026, ellipsis)) { continue; }

          if (const CachedGlyph *picture = fonts[font].atlas->Find(ellipsis); picture != nullptr)
          {
            ellipsis_font = font;
            ellipsis_advance = picture->advance * fonts[font].scale;
          } else
          {
            ellipsis = 0;
          }
        }

        limit = std::max(0.0f, box_width - ellipsis_advance);

        // whole characters, from the start of the line
        float kept = 0.0f;
        for (std::size_t i = line.first; i < line.end; i++)
        {
          if (kept + text.advances[i] > limit + epsilon) { break; }
          kept += text.advances[i];
        }
        limit = kept;
        width = kept + ellipsis_advance;
      }

      const bool is_cut = limit < std::numeric_limits<float>::max();

      float start = 0.0f;
      switch (options.align)
      {
        case TextAlign::Center:
          start = std::round((box_width - width) / 2.0f);
          break;
        case TextAlign::Right:
          start = std::round(box_width - width);
          break;
        case TextAlign::Start:
          start = runs_right_to_left ? std::round(box_width - width) : 0.0f;
          break;
        case TextAlign::End:
          start = runs_right_to_left ? 0.0f : std::round(box_width - width);
          break;
        default:
          break;
      }

      placed_line.left = start;
      placed_line.width = width;

      // the parts of the line, in the order they are drawn in from the
      // left
      runs_of_line.clear();
      for (const auto &run : text.runs)
      {
        if (run.end > line.first && run.first < line.end) { runs_of_line.push_back(&run); }
      }
      if (runs_right_to_left) { std::ranges::reverse(runs_of_line); }

      // Which characters of the line are drawn when it is cut off: those
      // whose advances fit, counted from where the line is read from.
      std::size_t last_kept = line.end;
      if (is_cut)
      {
        float kept = 0.0f;
        last_kept = line.first;
        for (std::size_t i = line.first; i < line.end; i++)
        {
          if (kept + text.advances[i] > limit + epsilon) { break; }
          kept += text.advances[i];
          last_kept = i + 1;
        }
      }

      float pen = start;

      const auto place_ellipsis = [&]
      {
        if (ellipsis == 0) { return; }

        const TextFont &font = fonts[ellipsis_font];

        PlacedShapedGlyph glyph;
        glyph.glyph = ellipsis;
        glyph.font = static_cast<std::uint8_t>(ellipsis_font);
        glyph.cluster = static_cast<std::uint32_t>(last_kept);
        SplitPen(pen, font.places_at_parts && font.scale == 1.0f, glyph.origin_x, glyph.variant);
        glyph.origin_y = placed_line.baseline;
        glyph.picture = font.atlas->Find(ellipsis, glyph.variant);

        if (glyph.picture != nullptr && glyph.picture->width > 0) { placed.glyphs.push_back(glyph); }
        pen += ellipsis_advance;
      };

      // where a text runs from right to left, what is left out is at its
      // left, and so is the ellipsis
      if (is_cut && runs_right_to_left) { place_ellipsis(); }

      for (const ShapedRun *run : runs_of_line)
      {
        if (run->font >= fonts.size() || fonts[run->font].atlas == nullptr) { continue; }

        const TextFont &font = fonts[run->font];
        const bool at_parts = font.places_at_parts && font.scale == 1.0f;

        for (std::size_t i = 0; i < run->glyph_count; i++)
        {
          const ShapedGlyph &shaped = text.glyphs[run->first_glyph + i];

          if (shaped.cluster < line.first || shaped.cluster >= line.end) { continue; }
          if (shaped.cluster >= last_kept) { continue; }

          PlacedShapedGlyph glyph;
          glyph.glyph = shaped.glyph;
          glyph.font = static_cast<std::uint8_t>(run->font);
          glyph.cluster = shaped.cluster;

          SplitPen(pen + shaped.offset_x, at_parts, glyph.origin_x, glyph.variant);
          glyph.origin_y = at_parts || font.scale == 1.0f
            ? std::round(placed_line.baseline - shaped.offset_y)
            : placed_line.baseline - shaped.offset_y;

          if (!at_parts && font.scale != 1.0f) { glyph.origin_x = pen + shaped.offset_x; }

          glyph.picture = font.atlas->Find(shaped.glyph, glyph.variant);

          if (glyph.picture != nullptr && glyph.picture->width > 0 && glyph.picture->height > 0)
          {
            placed.glyphs.push_back(glyph);
          }

          pen += shaped.advance;
        }
      }

      if (is_cut && !runs_right_to_left) { place_ellipsis(); }

      placed_line.glyph_count = placed.glyphs.size() - placed_line.first_glyph;
      placed.lines.push_back(placed_line);
    }

    return placed;
  }
} // neon
