#include "utf8.hpp"

#include <string>

#include <gtest/gtest.h>

namespace
{
  using neon::DecodeUtf8;
  using neon::EncodeUtf8;

  constexpr char32_t replacement = neon::Replacement_Character;

  TEST(Utf8, ReadsNothingFromNothing)
  {
    EXPECT_TRUE(DecodeUtf8("").empty());
  }

  TEST(Utf8, ReadsAscii)
  {
    EXPECT_EQ(DecodeUtf8("Health: 75"), U"Health: 75");
  }

  TEST(Utf8, ReadsCharactersOfTwoBytes)
  {
    // é, ü, and ÿ, which is the last of Latin-1
    EXPECT_EQ(DecodeUtf8("\xC3\xA9\xC3\xBC\xC3\xBF"), U"éüÿ");
  }

  TEST(Utf8, ReadsCharactersOfThreeBytes)
  {
    // the euro sign
    EXPECT_EQ(DecodeUtf8("\xE2\x82\xAC"), U"€");
  }

  TEST(Utf8, ReadsCharactersOfFourBytes)
  {
    // a grinning face, U+1F600
    EXPECT_EQ(DecodeUtf8("\xF0\x9F\x98\x80"), U"\U0001F600");
  }

  TEST(Utf8, ReadsTheFirstAndTheLastCharacterOfEveryLength)
  {
    EXPECT_EQ(DecodeUtf8(std::string("\x00", 1)), std::u32string(1, U'\0'));
    EXPECT_EQ(DecodeUtf8("\x7F"), U"\u007F");
    EXPECT_EQ(DecodeUtf8("\xC2\x80"), U"\u0080");
    EXPECT_EQ(DecodeUtf8("\xDF\xBF"), U"߿");
    EXPECT_EQ(DecodeUtf8("\xE0\xA0\x80"), U"ࠀ");
    EXPECT_EQ(DecodeUtf8("\xEF\xBF\xBF"), U"￿");
    EXPECT_EQ(DecodeUtf8("\xF0\x90\x80\x80"), U"\U00010000");
    EXPECT_EQ(DecodeUtf8("\xF4\x8F\xBF\xBF"), U"\U0010FFFF");
  }

  TEST(Utf8, ReadsMixedText)
  {
    EXPECT_EQ(DecodeUtf8("a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80z"), U"aé€\U0001F600z");
  }

  TEST(Utf8, ReplacesAByteThatContinuesNothing)
  {
    EXPECT_EQ(DecodeUtf8("a\x80z"), (std::u32string{U'a', replacement, U'z'}));
  }

  TEST(Utf8, ReplacesABeginningWithoutWhatFollows)
  {
    EXPECT_EQ(DecodeUtf8("a\xC3z"), (std::u32string{U'a', replacement, U'z'}));
  }

  TEST(Utf8, ReplacesEveryByteOfACharacterThatIsCutOff)
  {
    EXPECT_EQ(DecodeUtf8("\xE2\x82"), (std::u32string{replacement, replacement}));
    EXPECT_EQ(DecodeUtf8("\xF0\x9F\x98"), (std::u32string{replacement, replacement, replacement}));
    EXPECT_EQ(DecodeUtf8("\xC3"), (std::u32string{replacement}));
  }

  TEST(Utf8, ReplacesACharacterWrittenWithMoreBytesThanItNeeds)
  {
    // a slash as two bytes, which has been used to get past checks of paths
    EXPECT_EQ(DecodeUtf8("\xC0\xAF"), (std::u32string{replacement, replacement}));
    EXPECT_EQ(DecodeUtf8("\xE0\x80\xAF"), (std::u32string{replacement, replacement, replacement}));
  }

  TEST(Utf8, ReplacesHalvesOfSurrogatePairs)
  {
    EXPECT_EQ(DecodeUtf8("\xED\xA0\x80"), (std::u32string{replacement, replacement, replacement}));
  }

  TEST(Utf8, ReplacesWhatIsAboveTheLastCharacter)
  {
    EXPECT_EQ(
      DecodeUtf8("\xF4\x90\x80\x80"),
      (std::u32string{replacement, replacement, replacement, replacement}));
  }

  TEST(Utf8, ReplacesBytesThatNeverBeginACharacter)
  {
    EXPECT_EQ(DecodeUtf8("\xF8\xFF"), (std::u32string{replacement, replacement}));
  }

  TEST(Utf8, GoesOnReadingBehindWhatItReplaced)
  {
    EXPECT_EQ(DecodeUtf8("\xFF\xC3\xA9"), (std::u32string{replacement, U'é'}));
  }

  TEST(Utf8, WritesWhatItRead)
  {
    const std::string text = "a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80z";

    EXPECT_EQ(EncodeUtf8(DecodeUtf8(text)), text);
  }

  TEST(Utf8, WritesTheReplacementForWhatIsNoCharacter)
  {
    EXPECT_EQ(EncodeUtf8(std::u32string{0xD800}), "\xEF\xBF\xBD");
    EXPECT_EQ(EncodeUtf8(std::u32string{0x110000}), "\xEF\xBF\xBD");
  }
}
