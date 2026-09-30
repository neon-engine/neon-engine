#include "text-case.hpp"

namespace neon
{
  namespace
  {
    /// Letters that come in pairs next to each other, the capital first:
    /// from `first` to `last`, the capital at the even or at the odd place.
    struct Pairs
    {
      char32_t first;
      char32_t last;
      bool capital_is_even;
    };

    constexpr Pairs pairs[] = {
      {0x0100, 0x012F, true}, // Latin Extended-A
      {0x0132, 0x0137, true},
      {0x0139, 0x0148, false},
      {0x014A, 0x0177, true},
      {0x0179, 0x017E, false},
      {0x01CD, 0x01DC, false}, // Latin Extended-B, in parts
      {0x01DE, 0x01EF, true},
      {0x01F8, 0x021F, true},
      {0x0222, 0x0233, true},
      {0x0370, 0x0373, true}, // Greek, the old letters
      {0x03D8, 0x03EF, true},
      {0x0460, 0x0481, true}, // Cyrillic, the old letters
      {0x048A, 0x04BF, true},
      {0x04C1, 0x04CE, false},
      {0x04D0, 0x052F, true},
      {0x1E00, 0x1E95, true}, // Latin Extended Additional
      {0x1EA0, 0x1EFF, true},
    };

    /// Letters whose capital is a fixed distance in front of them: from
    /// `first` to `last` are the small ones.
    struct Shifted
    {
      char32_t first;
      char32_t last;
      char32_t distance;
    };

    constexpr Shifted shifted[] = {
      {U'a', U'z', 32},
      {0x00E0, 0x00F6, 32}, // Latin-1
      {0x00F8, 0x00FE, 32},
      {0x03B1, 0x03C1, 32}, // Greek
      {0x03C3, 0x03CB, 32},
      {0x03AD, 0x03AF, 37},
      {0x0430, 0x044F, 32}, // Cyrillic
      {0x0450, 0x045F, 80},
      {0x0561, 0x0586, 48}, // Armenian
    };

    /// What does not fit a rule, the small letter first.
    constexpr char32_t single[][2] = {
      {0x00FF, 0x0178}, // y with diaeresis
      {0x03AC, 0x0386}, // Greek letters with tonos
      {0x03CC, 0x038C},
      {0x03CD, 0x038E},
      {0x03CE, 0x038F},
      {0x03C2, 0x03A3}, // the sigma at the end of a word
      {0x04CF, 0x04C0},
    };
  }

  char32_t ToUpperCase(const char32_t character)
  {
    for (const auto &[first, last, distance] : shifted)
    {
      if (character >= first && character <= last) { return character - distance; }
    }

    for (const auto &[first, last, capital_is_even] : pairs)
    {
      if (character < first || character > last) { continue; }

      const bool is_even = (character & 1) == 0;
      return is_even == capital_is_even ? character : character - 1;
    }

    for (const auto &each : single)
    {
      if (character == each[0]) { return each[1]; }
    }

    return character;
  }

  char32_t ToLowerCase(const char32_t character)
  {
    for (const auto &[first, last, distance] : shifted)
    {
      if (character >= first - distance && character <= last - distance)
      {
        // the signs of multiplication and division sit among the letters
        // of Latin-1
        if (character == 0x00D7) { return character; }
        return character + distance;
      }
    }

    for (const auto &[first, last, capital_is_even] : pairs)
    {
      if (character < first || character > last) { continue; }

      const bool is_even = (character & 1) == 0;
      return is_even == capital_is_even ? character + 1 : character;
    }

    for (const auto &each : single)
    {
      // the sigma at the end of a word has no capital of its own
      if (character == each[1] && each[0] != 0x03C2) { return each[0]; }
    }

    return character;
  }

  bool IsWordCharacter(const char32_t character)
  {
    if (character >= U'0' && character <= U'9') { return true; }
    if (character == U'\'' || character == 0x2019) { return true; } // inside a word, as in don't
    if (character < 0x80)
    {
      return (character >= U'a' && character <= U'z') || (character >= U'A' && character <= U'Z');
    }

    // what is above Basic Latin and is no space and no punctuation
    if (character == 0x00A0 || character == 0x00D7 || character == 0x00F7) { return false; }
    if (character >= 0x00A1 && character <= 0x00BF) { return false; }
    if (character >= 0x2000 && character <= 0x206F) { return false; }
    if (character >= 0x3000 && character <= 0x303F) { return false; }
    return true;
  }

  std::u32string TransformCase(const std::u32string_view text, const TextTransform transform)
  {
    std::u32string transformed;
    transformed.reserve(text.size());

    bool is_in_word = false;

    for (const char32_t character : text)
    {
      const bool starts_word = !is_in_word && IsWordCharacter(character);
      is_in_word = IsWordCharacter(character);

      const bool is_raised =
        transform == TextTransform::Uppercase || (transform == TextTransform::Capitalize && starts_word);

      if (is_raised)
      {
        if (character == 0x00DF)
        {
          transformed += transform == TextTransform::Uppercase ? U"SS" : U"Ss";
        } else
        {
          transformed += ToUpperCase(character);
        }
      } else if (transform == TextTransform::Lowercase)
      {
        transformed += ToLowerCase(character);
      } else
      {
        transformed += character;
      }
    }

    return transformed;
  }
} // neon
