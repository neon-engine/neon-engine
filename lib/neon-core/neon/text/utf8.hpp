#ifndef UTF8_HPP
#define UTF8_HPP

#include <string>
#include <string_view>

namespace neon
{
  /// What stands in for bytes that are not UTF-8, U+FFFD.
  constexpr char32_t Replacement_Character = 0xFFFD;

  /// Turns UTF-8 into characters. Text in files and in code is UTF-8, and a
  /// font is asked for characters.
  ///
  /// Bytes that are not valid UTF-8 become the replacement character, one
  /// for each byte that could not be read, and reading goes on behind them.
  /// That includes a character written with more bytes than it needs, the
  /// halves of a surrogate pair, and anything above U+10FFFF.
  [[nodiscard]] std::u32string DecodeUtf8(std::string_view text);

  /// Turns characters into UTF-8. A character that is none, such as half of
  /// a surrogate pair, is written as the replacement character.
  [[nodiscard]] std::string EncodeUtf8(std::u32string_view characters);
} // neon

#endif //UTF8_HPP
