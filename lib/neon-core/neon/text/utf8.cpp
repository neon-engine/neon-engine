#include "utf8.hpp"

#include <cstddef>

namespace neon
{
  // Helpers of utf8.cpp, for this file alone.
  namespace
  {
    bool IsContinuation(const unsigned char byte)
    {
      return (byte & 0xC0) == 0x80;
    }

    bool IsCharacter(const char32_t character)
    {
      return character <= 0x10FFFF && (character < 0xD800 || character > 0xDFFF);
    }
  }

  std::u32string DecodeUtf8(const std::string_view text)
  {
    std::u32string characters;
    characters.reserve(text.size());

    std::size_t i = 0;
    while (i < text.size())
    {
      const auto first = static_cast<unsigned char>(text[i]);

      if (first < 0x80)
      {
        characters.push_back(first);
        i++;
        continue;
      }

      // how many bytes follow, and the least a character of that length
      // stands for
      std::size_t following = 0;
      char32_t character = 0;
      char32_t least = 0;

      if ((first & 0xE0) == 0xC0)
      {
        following = 1;
        character = first & 0x1F;
        least = 0x80;
      } else if ((first & 0xF0) == 0xE0)
      {
        following = 2;
        character = first & 0x0F;
        least = 0x800;
      } else if ((first & 0xF8) == 0xF0)
      {
        following = 3;
        character = first & 0x07;
        least = 0x10000;
      } else
      {
        characters.push_back(Replacement_Character);
        i++;
        continue;
      }

      bool complete = i + following < text.size();
      for (std::size_t k = 1; complete && k <= following; k++)
      {
        const auto byte = static_cast<unsigned char>(text[i + k]);
        if (!IsContinuation(byte))
        {
          complete = false;
          break;
        }
        character = (character << 6) | (byte & 0x3F);
      }

      if (!complete || character < least || !IsCharacter(character))
      {
        characters.push_back(Replacement_Character);
        i++;
        continue;
      }

      characters.push_back(character);
      i += following + 1;
    }

    return characters;
  }

  std::string EncodeUtf8(const std::u32string_view characters)
  {
    std::string text;
    text.reserve(characters.size());

    for (char32_t character : characters)
    {
      if (!IsCharacter(character)) { character = Replacement_Character; }

      if (character < 0x80)
      {
        text.push_back(static_cast<char>(character));
      } else if (character < 0x800)
      {
        text.push_back(static_cast<char>(0xC0 | (character >> 6)));
        text.push_back(static_cast<char>(0x80 | (character & 0x3F)));
      } else if (character < 0x10000)
      {
        text.push_back(static_cast<char>(0xE0 | (character >> 12)));
        text.push_back(static_cast<char>(0x80 | ((character >> 6) & 0x3F)));
        text.push_back(static_cast<char>(0x80 | (character & 0x3F)));
      } else
      {
        text.push_back(static_cast<char>(0xF0 | (character >> 18)));
        text.push_back(static_cast<char>(0x80 | ((character >> 12) & 0x3F)));
        text.push_back(static_cast<char>(0x80 | ((character >> 6) & 0x3F)));
        text.push_back(static_cast<char>(0x80 | (character & 0x3F)));
      }
    }

    return text;
  }
} // neon
