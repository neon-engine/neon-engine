#ifndef TEXT_CASE_HPP
#define TEXT_CASE_HPP

#include <string>
#include <string_view>

namespace neon
{
  /// `text-transform` of CSS.
  enum class TextTransform
  {
    None = 0,
    Uppercase,
    Lowercase,
    Capitalize
  };

  /// The capital letter of a letter, or the letter itself when it has
  /// none. It knows Latin with its accents, Greek, Cyrillic, and Armenian,
  /// and does not depend on how the machine is set up.
  [[nodiscard]] char32_t ToUpperCase(char32_t character);

  [[nodiscard]] char32_t ToLowerCase(char32_t character);

  /// Whether a character is a letter or a digit, which is what a word is
  /// made of.
  [[nodiscard]] bool IsWordCharacter(char32_t character);

  /// A text in capital letters, in small letters, or with a capital at the
  /// start of every word. A sharp s becomes two capitals, which makes the
  /// text longer.
  [[nodiscard]] std::u32string TransformCase(std::u32string_view text, TextTransform transform);
} // neon

#endif //TEXT_CASE_HPP
