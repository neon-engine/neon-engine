#include "text-case.hpp"

#include <string>

#include <gtest/gtest.h>

namespace
{
  using neon::TextTransform;
  using neon::ToLowerCase;
  using neon::ToUpperCase;
  using neon::TransformCase;

  TEST(TextCaseTest, RaisesAndLowersTheLettersOfEnglish)
  {
    EXPECT_EQ(ToUpperCase(U'a'), U'A');
    EXPECT_EQ(ToUpperCase(U'z'), U'Z');
    EXPECT_EQ(ToLowerCase(U'A'), U'a');
    EXPECT_EQ(ToLowerCase(U'Z'), U'z');
  }

  TEST(TextCaseTest, LeavesAloneWhatHasNoOtherCase)
  {
    for (const char32_t character : {U'1', U' ', U'.', U'@', U'[', U'`', U'{', static_cast<char32_t>(0x4E2D)})
    {
      EXPECT_EQ(ToUpperCase(character), character);
      EXPECT_EQ(ToLowerCase(character), character);
    }

    EXPECT_EQ(ToUpperCase(U'A'), U'A');
    EXPECT_EQ(ToLowerCase(U'a'), U'a');
  }

  TEST(TextCaseTest, KnowsTheLettersWithAccents)
  {
    EXPECT_EQ(ToUpperCase(0x00E9), 0x00C9u); // e with acute
    EXPECT_EQ(ToLowerCase(0x00C9), 0x00E9u);
    EXPECT_EQ(ToUpperCase(0x00FC), 0x00DCu); // u with diaeresis
    EXPECT_EQ(ToUpperCase(0x00FF), 0x0178u); // y with diaeresis
    EXPECT_EQ(ToLowerCase(0x0178), 0x00FFu);

    // the signs between the letters of Latin-1 stay what they are
    EXPECT_EQ(ToUpperCase(0x00F7), 0x00F7u);
    EXPECT_EQ(ToLowerCase(0x00D7), 0x00D7u);
  }

  TEST(TextCaseTest, KnowsLettersThatComeInPairs)
  {
    EXPECT_EQ(ToUpperCase(0x0101), 0x0100u); // a with macron
    EXPECT_EQ(ToLowerCase(0x0100), 0x0101u);
    EXPECT_EQ(ToUpperCase(0x0100), 0x0100u);

    // where the capital is at the odd place
    EXPECT_EQ(ToUpperCase(0x013A), 0x0139u); // l with acute
    EXPECT_EQ(ToLowerCase(0x0139), 0x013Au);
    EXPECT_EQ(ToUpperCase(0x017E), 0x017Du); // z with caron
  }

  TEST(TextCaseTest, KnowsGreekAndCyrillic)
  {
    EXPECT_EQ(ToUpperCase(0x03B1), 0x0391u); // alpha
    EXPECT_EQ(ToLowerCase(0x03A9), 0x03C9u); // omega
    EXPECT_EQ(ToUpperCase(0x03C2), 0x03A3u); // the sigma at the end of a word
    EXPECT_EQ(ToLowerCase(0x03A3), 0x03C3u);

    EXPECT_EQ(ToUpperCase(0x0436), 0x0416u); // zhe
    EXPECT_EQ(ToLowerCase(0x042F), 0x044Fu); // ya
    EXPECT_EQ(ToUpperCase(0x0451), 0x0401u); // io
  }

  TEST(TextCaseTest, TransformsAText)
  {
    EXPECT_EQ(TransformCase(U"Neon engine 2", TextTransform::None), U"Neon engine 2");
    EXPECT_EQ(TransformCase(U"Neon engine 2", TextTransform::Uppercase), U"NEON ENGINE 2");
    EXPECT_EQ(TransformCase(U"Neon ENGINE 2", TextTransform::Lowercase), U"neon engine 2");
    EXPECT_EQ(TransformCase(U"neon engine, the-best", TextTransform::Capitalize), U"Neon Engine, The-Best");
  }

  TEST(TextCaseTest, CapitalizesTheStartOfAWordAlone)
  {
    EXPECT_EQ(TransformCase(U"don't STOP", TextTransform::Capitalize), U"Don't STOP");
    EXPECT_EQ(TransformCase(U"  two  spaces", TextTransform::Capitalize), U"  Two  Spaces");
    EXPECT_EQ(TransformCase(U"3d view", TextTransform::Capitalize), U"3d View");
  }

  TEST(TextCaseTest, ASharpSBecomesTwoCapitals)
  {
    const std::u32string street = U"stra\u00DFe";

    EXPECT_EQ(TransformCase(street, TextTransform::Uppercase), U"STRASSE");
    EXPECT_EQ(TransformCase(street, TextTransform::Lowercase), street);
  }

  TEST(TextCaseTest, AnEmptyTextStaysEmpty)
  {
    EXPECT_TRUE(TransformCase(U"", TextTransform::Uppercase).empty());
  }
}
