#include "ui-fixture.hpp"

#include <neon/testing/fake-text-shaper.hpp>

// The properties of text: how it is spaced, raised, underlined, shadowed,
// cut off, and which way it runs. The font of the tests moves the pen by
// half its size for every character. At the size 20 a character is 8 by 14,
// starts 1 right of the pen, and stands on a baseline 16 below the top of
// its line.

namespace
{
  using neon::No_Texture;
  using neon::Vertex2D;
  using neon::testing::FakeTextShaper;
  using neon::testing::LogLevel;
  using neon::testing::RecordedQuad;
  using neon::testing::UiTest;
  using ::testing::ElementsAre;

  class UiTextStyleTest : public UiTest
  {
  protected:
    FakeTextShaper _shaper;

    /// A label at 100, 100 with the size 20 and the properties given.
    void ShowLabel(const std::string &text, const std::string &properties = "")
    {
      const std::string element =
        "- type: label\n"
        "  name: label\n"
        "  text: \"" + text + "\"\n"
        "  position: absolute\n"
        "  left: 100\n"
        "  top: 100\n"
        "  font_size: 20\n" + Indented(properties, "  ");

      ASSERT_GE(ShowUnderRoot(element), 0) << _logger->Messages(LogLevel::Error);
      Frame();
    }

    [[nodiscard]] std::vector<float> LeftSides() const
    {
      std::vector<float> sides;
      for (const auto &quad : _renderer.Quads()) { sides.push_back(quad.left); }
      return sides;
    }

    static void ExpectQuad(
      const RecordedQuad &quad,
      const float left,
      const float top,
      const float width,
      const float height)
    {
      EXPECT_FLOAT_EQ(quad.left, left);
      EXPECT_FLOAT_EQ(quad.top, top);
      EXPECT_FLOAT_EQ(quad.Width(), width);
      EXPECT_FLOAT_EQ(quad.Height(), height);
    }

    static void ExpectColor(
      const neon::Color &color,
      const float red,
      const float green,
      const float blue,
      const float alpha)
    {
      EXPECT_NEAR(color.r, red, 0.002f);
      EXPECT_NEAR(color.g, green, 0.002f);
      EXPECT_NEAR(color.b, blue, 0.002f);
      EXPECT_NEAR(color.a, alpha, 0.002f);
    }

    [[nodiscard]] std::vector<std::string> ProblemsOf(const std::string &properties)
    {
      _logger->Clear();
      EXPECT_EQ(ShowUnderRoot("- type: label\n  name: label\n  text: a\n" + Indented(properties, "  ")), -1);

      auto errors = Errors();
      if (!errors.empty()) { errors.pop_back(); }
      return errors;
    }
  };

  // spacing

  TEST_F(UiTextStyleTest, SpacesLettersApart)
  {
    ShowLabel("abc", "letter_spacing: 4\n");

    EXPECT_EQ(LeftSides(), (std::vector<float>{101, 115, 129}));
    ExpectBox("label", 100, 100, 42, 20);
  }

  TEST_F(UiTextStyleTest, SpacingGrowsWithTheFrame)
  {
    _renderer.SetResolution(3840, 2160);
    ShowLabel("abc", "letter_spacing: 4\n");

    // at twice the size the pen moves by 20, and the spacing is 8
    EXPECT_EQ(LeftSides(), (std::vector<float>{201, 229, 257}));
  }

  TEST_F(UiTextStyleTest, SpacesWordsApart)
  {
    ShowLabel("a b", "word_spacing: 10px\n");

    EXPECT_EQ(LeftSides(), (std::vector<float>{101, 131}));
    ExpectBox("label", 100, 100, 40, 20);
  }

  TEST_F(UiTextStyleTest, NormalSpacingIsNone)
  {
    ShowLabel("abc", "letter_spacing: normal\nword_spacing: normal\n");

    EXPECT_EQ(LeftSides(), (std::vector<float>{101, 111, 121}));
  }

  // letters

  TEST_F(UiTextStyleTest, RaisesTheLettersOfAText)
  {
    ShowLabel("ab", "text_transform: uppercase\n");

    // the text of the element stays what the file says
    EXPECT_EQ(_renderer.Quads().size(), 2u);

    const auto lower = _renderer.Quads();
    _ui->CleanUp();
    Create({});
    ShowLabel("AB");

    // both draw the same two places of the atlas
    const auto upper = _renderer.Quads();
    ASSERT_EQ(upper.size(), 2u);
    EXPECT_FLOAT_EQ(lower[0].texture_left, upper[0].texture_left);
    EXPECT_FLOAT_EQ(lower[1].texture_left, upper[1].texture_left);
    EXPECT_FLOAT_EQ(lower[0].texture_top, upper[0].texture_top);
  }

  // lines under and through a text

  TEST_F(UiTextStyleTest, DrawsALineUnderAText)
  {
    ShowLabel("abc", "text_decoration: underline\ncolor: \"#ff8000\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 4u);

    // below the glyphs in the order of drawing, as wide as the text, and
    // below the baseline, which is at 116
    EXPECT_FALSE(quads[0].textured);
    EXPECT_FLOAT_EQ(quads[0].left, 100);
    EXPECT_FLOAT_EQ(quads[0].Width(), 30);
    EXPECT_GT(quads[0].top, 116);
    EXPECT_LE(quads[0].bottom, 120);
    EXPECT_GE(quads[0].Height(), 1);
    ExpectColor(quads[0].color, 1, 0.502f, 0, 1);

    EXPECT_TRUE(quads[1].textured);
  }

  TEST_F(UiTextStyleTest, DrawsALineThroughAText)
  {
    ShowLabel("abc", "text_decoration: line-through\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 4u);

    // above the glyphs in the order of drawing, and through the letters
    EXPECT_TRUE(quads[0].textured);
    EXPECT_FALSE(quads[3].textured);
    EXPECT_FLOAT_EQ(quads[3].Width(), 30);
    EXPECT_GT(quads[3].top, 104);
    EXPECT_LT(quads[3].bottom, 116);
  }

  TEST_F(UiTextStyleTest, GivesTheLinesAColourAndAThickness)
  {
    ShowLabel("abc", "text_decoration: \"underline line-through #00ff00 3px\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 5u);

    for (const std::size_t line : {std::size_t{0}, std::size_t{4}})
    {
      EXPECT_FALSE(quads[line].textured);
      EXPECT_FLOAT_EQ(quads[line].Height(), 3);
      ExpectColor(quads[line].color, 0, 1, 0, 1);
    }

    // the text keeps its colour
    ExpectColor(quads[1].color, 1, 1, 1, 1);
  }

  TEST_F(UiTextStyleTest, TheLongNamesOfTheLinesWinOverTheShortOne)
  {
    ShowLabel(
      "abc",
      "text_decoration: \"underline #00ff00 3px\"\n"
      "text_decoration_line: line-through\n"
      "text_decoration_color: \"#0000ff\"\n"
      "text_decoration_thickness: 2\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 4u);

    EXPECT_FALSE(quads[3].textured);
    EXPECT_FLOAT_EQ(quads[3].Height(), 2);
    ExpectColor(quads[3].color, 0, 0, 1, 1);
  }

  TEST_F(UiTextStyleTest, DrawsALineUnderEveryLineOfAText)
  {
    ShowLabel("ab\\ncdef", "text_decoration: underline\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 8u);

    EXPECT_FLOAT_EQ(quads[0].Width(), 20);
    EXPECT_FLOAT_EQ(quads[1].Width(), 40);
    EXPECT_FLOAT_EQ(quads[1].top, quads[0].top + 20);
  }

  // shadows

  TEST_F(UiTextStyleTest, DrawsTheShadowOfATextBelowIt)
  {
    ShowLabel("ab", "text_shadow: \"2px 3px #000000\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 4u);

    // the shadow first, moved and black, then the text
    ExpectQuad(quads[0], 103, 105, 8, 14);
    ExpectQuad(quads[1], 113, 105, 8, 14);
    ExpectColor(quads[0].color, 0, 0, 0, 1);

    ExpectQuad(quads[2], 101, 102, 8, 14);
    ExpectColor(quads[2].color, 1, 1, 1, 1);

    // a shadow that is sharp is the glyph itself, from the same place of
    // the atlas
    EXPECT_FLOAT_EQ(quads[0].texture_left, quads[2].texture_left);
    EXPECT_EQ(_renderer.batches.size(), 1u) << "and costs no draw call";
  }

  TEST_F(UiTextStyleTest, AShadowOutOfFocusIsLargerThanItsGlyph)
  {
    ShowLabel("a", "text_shadow: \"0 0 6px #000000\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 2u);

    const float margin = (quads[0].Width() - 8) / 2;
    EXPECT_GT(margin, 2);
    ExpectQuad(quads[0], 101 - margin, 102 - margin, 8 + 2 * margin, 14 + 2 * margin);

    // a picture of its own in the atlas
    EXPECT_NE(quads[0].texture_left, quads[1].texture_left);
    EXPECT_EQ(quads[0].texture, quads[1].texture);
  }

  TEST_F(UiTextStyleTest, DrawsTheShadowThatIsWrittenFirstOnTop)
  {
    ShowLabel("a", "text_shadow: \"1px 1px #ff0000, 4px 4px #0000ff\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 3u);

    ExpectColor(quads[0].color, 0, 0, 1, 1);
    EXPECT_FLOAT_EQ(quads[0].left, 105);
    ExpectColor(quads[1].color, 1, 0, 0, 1);
    EXPECT_FLOAT_EQ(quads[1].left, 102);
    ExpectColor(quads[2].color, 1, 1, 1, 1);
  }

  TEST_F(UiTextStyleTest, AShadowWithoutAColourHasThatOfTheText)
  {
    ShowLabel("a", "text_shadow: 2 2\ncolor: \"#00ff00\"\nopacity: 0.5\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 2u);
    ExpectColor(quads[0].color, 0, 1, 0, 0.5f);
  }

  TEST_F(UiTextStyleTest, TakesShadowsAsAList)
  {
    ShowLabel("a", "text_shadow:\n  - \"1px 1px #ff0000\"\n  - \"2px 2px #0000ff\"\n");

    EXPECT_EQ(_renderer.Quads().size(), 3u);
  }

  TEST_F(UiTextStyleTest, DrawsShadowsOnceIntoTheAtlas)
  {
    ShowLabel("ab", "text_shadow: \"0 0 6px #000000\"\n");

    const std::size_t after_the_first = _rasterizer.rasterized;
    const std::size_t created = _renderer.created;

    Frame();
    Frame();

    EXPECT_EQ(_rasterizer.rasterized, after_the_first);
    EXPECT_EQ(_renderer.created, created);
    EXPECT_EQ(_renderer.Quads().size(), 4u);
  }

  // the line around a glyph

  TEST_F(UiTextStyleTest, AFontThatCannotDrawTheLineAroundAGlyphDrawsTheTextWithoutIt)
  {
    ShowLabel("ab", "text_stroke_width: 2\ntext_stroke_color: \"#ff0000\"\n");

    Frame();
    Frame();

    EXPECT_EQ(_renderer.Quads().size(), 2u);
    EXPECT_TRUE(Errors().empty()) << _logger->Messages(LogLevel::Error);
  }

  // colours

  TEST_F(UiTextStyleTest, FillsATextWithAGradient)
  {
    ShowLabel("abcd", "color: \"linear-gradient(to right, #ff0000, #0000ff)\"\n");

    ASSERT_EQ(_renderer.batches.size(), 1u);
    const auto &vertices = _renderer.batches[0].vertices;
    ASSERT_EQ(vertices.size(), 16u);

    // The text is 40 wide, from 100 to 140. Every corner has the colour
    // of the gradient where it lies.
    for (const Vertex2D &vertex : vertices)
    {
      const float along = (vertex.x - 100) / 40;

      EXPECT_NEAR(vertex.color.r, 1 - along, 0.002f) << "at " << vertex.x;
      EXPECT_NEAR(vertex.color.b, along, 0.002f) << "at " << vertex.x;
      EXPECT_NEAR(vertex.color.g, 0, 0.002f);
      EXPECT_NEAR(vertex.color.a, 1, 0.002f);
    }

    // the left corners of the first glyph and the right ones of the last
    EXPECT_NEAR(vertices[0].color.r, 0.975f, 0.002f);
    EXPECT_NEAR(vertices[14].color.b, 0.975f, 0.002f);
  }

  TEST_F(UiTextStyleTest, AGradientFromTopToBottomIsTheSameForEveryGlyph)
  {
    ShowLabel("ab", "color: \"linear-gradient(#ff0000, #0000ff)\"\n");

    const auto &vertices = _renderer.batches[0].vertices;
    ASSERT_EQ(vertices.size(), 8u);

    for (std::size_t corner = 0; corner < 4; corner++)
    {
      EXPECT_FLOAT_EQ(vertices[corner].color.r, vertices[4 + corner].color.r);
    }

    // the top of a glyph is 2 below the top of a line of 20
    EXPECT_NEAR(vertices[0].color.b, 0.1f, 0.002f);
    EXPECT_NEAR(vertices[2].color.b, 0.8f, 0.002f);
  }

  TEST_F(UiTextStyleTest, TheLinesOfATextWithAGradientHaveItsFirstColour)
  {
    ShowLabel("ab", "color: \"linear-gradient(#ff0000, #0000ff)\"\ntext_decoration: underline\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 3u);
    ExpectColor(quads[0].color, 1, 0, 0, 1);
  }

  // white space and what does not fit

  TEST_F(UiTextStyleTest, KeepsATextOnOneLine)
  {
    ShowLabel("one two three", "white_space: nowrap\nwidth: 60\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 11u);

    for (const auto &quad : quads) { EXPECT_FLOAT_EQ(quad.top, 102); }
    ExpectBox("label", 100, 100, 60, 20);
  }

  TEST_F(UiTextStyleTest, BreaksATextThatIsNotKeptOnOneLine)
  {
    ShowLabel("one two three", "width: 70\n");

    ExpectBox("label", 100, 100, 70, 40);
  }

  TEST_F(UiTextStyleTest, JoinsSpaces)
  {
    ShowLabel("a    b", "white_space: normal\n");

    EXPECT_EQ(LeftSides(), (std::vector<float>{101, 121}));
    ExpectBox("label", 100, 100, 30, 20);
  }

  TEST_F(UiTextStyleTest, KeepsSpacesUnlessItIsToldOtherwise)
  {
    ShowLabel("a    b");

    EXPECT_EQ(LeftSides(), (std::vector<float>{101, 151}));
  }

  TEST_F(UiTextStyleTest, PutsAnEllipsisWhereATextIsCutOff)
  {
    ShowLabel("abcdefgh", "white_space: nowrap\ntext_overflow: ellipsis\nwidth: 55\noverflow: hidden\n");

    // four characters and the ellipsis
    EXPECT_EQ(LeftSides(), (std::vector<float>{101, 111, 121, 131, 141}));
    EXPECT_LE(_renderer.Quads().back().right, 155);
  }

  TEST_F(UiTextStyleTest, PutsNoEllipsisIntoATextThatIsBroken)
  {
    ShowLabel("one two three", "text_overflow: ellipsis\nwidth: 70\n");

    EXPECT_EQ(_renderer.Quads().size(), 11u);
  }

  // the style of a font

  TEST_F(UiTextStyleTest, AnItalicThatAFamilyDoesNotHaveIsDrawnIntoAnAtlasOfItsOwn)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n  text: a\n"
      "- type: label\n  text: a\n  font_style: italic\n"
      "- type: label\n  text: a\n  font_style: normal\n"), 0);
    Frame();

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 3u);

    EXPECT_NE(quads[0].texture, quads[1].texture);
    EXPECT_EQ(quads[0].texture, quads[2].texture);
  }

  TEST_F(UiTextStyleTest, TakesTheItalicAFileNames)
  {
    WriteAsset("fonts/italic.ttf", "a font");

    ASSERT_GE(Show(
      "fonts:\n"
      "  - family: sans-serif\n"
      "    src: assets://fonts/italic.ttf\n"
      "    style: italic\n"
      "root:\n"
      "  type: panel\n"
      "  children:\n"
      "    - type: label\n"
      "      text: a\n"
      "      font_style: italic\n"), 0) << _logger->Messages(LogLevel::Error);
    Frame();

    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Loaded the font assets://fonts/italic.ttf"));
    EXPECT_FALSE(_logger->Contains(LogLevel::Info, "Loaded the font assets://fonts/regular.ttf"));
  }

  TEST_F(UiTextStyleTest, DrawsWithTheFirstFamilyOfAListThatIsKnown)
  {
    ShowLabel("ab", "font_family: \"missing, 'sans-serif'\"\n");

    EXPECT_EQ(_renderer.Quads().size(), 2u);
    EXPECT_EQ(Errors().size(), 1u);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "The font family 'missing' is not known"));
  }

  TEST_F(UiTextStyleTest, DrawsWhatTheFirstFontDoesNotHaveWithTheNext)
  {
    WriteAsset("fonts/other.ttf", "a font");
    _rasterizer.missing.insert(U'x');

    // The font of the tests is one font under two names, so both are
    // missing the character. What is tested is that the second is asked.
    ASSERT_GE(Show(
      "fonts:\n"
      "  - family: other\n"
      "    src: assets://fonts/other.ttf\n"
      "root:\n"
      "  type: panel\n"
      "  children:\n"
      "    - type: label\n"
      "      text: ab\n"
      "      font_family: \"sans-serif, other\"\n"), 0) << _logger->Messages(LogLevel::Error);
    Frame();

    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Loaded the font assets://fonts/other.ttf"));
    EXPECT_EQ(_renderer.Quads().size(), 2u);
  }

  // from right to left

  TEST_F(UiTextStyleTest, ATextFromRightToLeftStartsAtTheRight)
  {
    _ui->SetTextShaper(&_shaper);

    ShowLabel("\u05D0\u05D1\u05D2", "direction: rtl\ntext_align: start\nwidth: 100\n");

    // three letters of 10: the first is the one furthest right
    EXPECT_EQ(LeftSides(), (std::vector<float>{171, 181, 191}));
  }

  TEST_F(UiTextStyleTest, TextIsAlignedLeftUnlessItIsToldOtherwise)
  {
    _ui->SetTextShaper(&_shaper);

    ShowLabel("\u05D0\u05D1\u05D2", "direction: rtl\nwidth: 100\n");

    EXPECT_EQ(LeftSides(), (std::vector<float>{101, 111, 121}));
  }

  TEST_F(UiTextStyleTest, ShapesATextOnceAndNotInEveryFrame)
  {
    _ui->SetTextShaper(&_shaper);

    ASSERT_GE(Show(
      "values:\n"
      "  health: 75\n"
      "  score: 1\n"
      "root:\n"
      "  type: panel\n"
      "  children:\n"
      "    - type: label\n"
      "      text: \"Health {health}\"\n"), 0);

    Frame();
    const std::size_t after_the_first = _shaper.shaped;
    EXPECT_GT(after_the_first, 0u);

    Frame();
    Frame();
    EXPECT_EQ(_shaper.shaped, after_the_first);

    // a value the text does not show changes nothing
    _ui->SetNumber("score", 2);
    Frame();
    EXPECT_EQ(_shaper.shaped, after_the_first);

    _ui->SetNumber("health", 50);
    Frame();
    EXPECT_GT(_shaper.shaped, after_the_first);
  }

  TEST_F(UiTextStyleTest, JoinsLettersWhereTheShaperSays)
  {
    _ui->SetTextShaper(&_shaper);

    ShowLabel("fin");

    // a ligature and an n
    EXPECT_EQ(_renderer.Quads().size(), 2u);
    ExpectBox("label", 100, 100, 20, 20);
  }

  // what is wrong

  TEST_F(UiTextStyleTest, SaysWhatIsWrongWithAPropertyOfText)
  {
    EXPECT_THAT(ProblemsOf("letter_spacing: wide\n"), ElementsAre(
                  "assets://ui/test.ui.yml:10: 'letter_spacing' of label 'label' is 'wide', where a number of "
                  "pixels was expected"));

    EXPECT_THAT(ProblemsOf("text_transform: shout\n"), ElementsAre(
                  "assets://ui/test.ui.yml:10: 'text_transform' of label 'label' is 'shout', where one of these "
                  "was expected: none, uppercase, lowercase, capitalize"));

    EXPECT_THAT(ProblemsOf("text_decoration: wavy\n"), ElementsAre(
                  "assets://ui/test.ui.yml:10: 'text_decoration' of label 'label' is 'wavy', where none, "
                  "underline, line-through, a colour, and a thickness, such as \"underline #ff8000 2px\" was "
                  "expected"));

    EXPECT_THAT(ProblemsOf("text_shadow: soft\n"), ElementsAre(
                  "assets://ui/test.ui.yml:10: 'text_shadow' of label 'label' is 'soft', where none, or shadows "
                  "such as \"0 2px 4px #000000\": to the right, down, a blur that is not below 0, and a colour "
                  "was expected"));

    EXPECT_THAT(ProblemsOf("white_space: break-spaces\n"), ElementsAre(
                  "assets://ui/test.ui.yml:10: 'white_space' of label 'label' is 'break-spaces', where one of "
                  "these was expected: normal, nowrap, pre, pre-wrap, pre-line"));

    EXPECT_THAT(ProblemsOf("text_overflow: fade\n"), ElementsAre(
                  "assets://ui/test.ui.yml:10: 'text_overflow' of label 'label' is 'fade', where one of these "
                  "was expected: clip, ellipsis"));

    EXPECT_THAT(ProblemsOf("font_style: oblique\n"), ElementsAre(
                  "assets://ui/test.ui.yml:10: 'font_style' of label 'label' is 'oblique', where one of these "
                  "was expected: normal, italic"));

    EXPECT_THAT(ProblemsOf("direction: up\n"), ElementsAre(
                  "assets://ui/test.ui.yml:10: 'direction' of label 'label' is 'up', where one of these was "
                  "expected: ltr, rtl"));

    EXPECT_THAT(ProblemsOf("text_stroke_width: -1\n"), ElementsAre(
                  "assets://ui/test.ui.yml:10: 'text_stroke_width' of label 'label' is a number, where a number "
                  "of pixels that is not below 0 was expected"));

    EXPECT_THAT(ProblemsOf("color: \"linear-gradient(#ff0000)\"\n"), ElementsAre(
                  "assets://ui/test.ui.yml:10: 'color' of label 'label' is 'linear-gradient(#ff0000)', where a "
                  "gradient such as linear-gradient(90deg, #f00, #00f) with 2 to 8 colours was expected"));
  }

  TEST_F(UiTextStyleTest, SaysWhatIsWrongWithTheStyleOfAFont)
  {
    _logger->Clear();
    EXPECT_EQ(Show(
      "fonts:\n"
      "  - family: title\n"
      "    src: assets://fonts/regular.ttf\n"
      "    style: oblique\n"
      "    rendering: vector\n"
      "root:\n"
      "  type: panel\n"), -1);

    auto errors = Errors();
    ASSERT_EQ(errors.size(), 3u);
    EXPECT_EQ(
      errors[0],
      "assets://ui/test.ui.yml:4: 'style' of font 1 is 'oblique', where one of these was expected: normal, "
      "italic");
    EXPECT_EQ(
      errors[1],
      "assets://ui/test.ui.yml:5: 'rendering' of font 1 is 'vector', where one of these was expected: bitmap, "
      "sdf");
  }
}
