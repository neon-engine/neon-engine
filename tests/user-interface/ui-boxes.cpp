#include "ui-fixture.hpp"

#include <cmath>

// What a box is painted with beyond one color: round corners, borders of
// several colors, shadows, gradients, and what moves and turns it. Each is
// a shape the renderer works out for every pixel, which a test sees as the
// numbers that are handed over. What the pixels look like is looked at in
// the frames the runtime writes.

namespace
{
  using neon::No_Texture;
  using neon::Shape2D;
  using neon::ShapeKind2D;
  using neon::Triangles2D;
  using neon::Vertex2D;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;
  using ::testing::ElementsAre;

  class UiBoxTest : public UiTest
  {
  protected:
    /// A box of 200 by 100 at 100, 100 with the properties that are given.
    void ShowBox(const std::string &properties, const std::string &children = "")
    {
      std::string element =
        "- type: panel\n"
        "  name: box\n"
        "  position: absolute\n"
        "  left: 100\n"
        "  top: 100\n"
        "  box_sizing: border-box\n"
        "  width: 200\n"
        "  height: 100\n" + Indented(properties, "  ");

      if (!children.empty()) { element += "  children:\n" + Indented(children, "    "); }

      ASSERT_GE(ShowUnderRoot(element), 0) << _logger->Messages(LogLevel::Error);
      Frame();
    }

    /// A button of 200 by 100 at 100, 100.
    void ShowButton(const std::string &properties)
    {
      ASSERT_GE(ShowUnderRoot(
        "- type: button\n"
        "  name: start\n"
        "  position: absolute\n"
        "  left: 100\n"
        "  top: 100\n"
        "  box_sizing: border-box\n"
        "  width: 200\n"
        "  height: 100\n"
        "  padding: 0\n" + Indented(properties, "  ")), 0) << _logger->Messages(LogLevel::Error);
      Frame();
    }

    /// Every shape of the frame, in the order they are drawn in.
    [[nodiscard]] std::vector<Shape2D> Shapes() const
    {
      std::vector<Shape2D> shapes;

      for (const auto &batch : _renderer.batches)
      {
        // in the order of the rectangles that refer to them
        for (std::size_t first = 0; first + 3 < batch.vertices.size(); first += 4)
        {
          const float place = batch.vertices[first].shape;
          if (place < 0.0f) { continue; }

          shapes.push_back(batch.shapes.at(static_cast<std::size_t>(place)));
        }
      }

      return shapes;
    }

    static ShapeKind2D KindOf(const Shape2D &shape)
    {
      return static_cast<ShapeKind2D>(static_cast<int>(shape.box[2]));
    }

    static void ExpectRadii(const Shape2D &shape, const float a, const float b, const float c, const float d)
    {
      EXPECT_FLOAT_EQ(shape.radii[0], a) << "left top";
      EXPECT_FLOAT_EQ(shape.radii[1], b) << "right top";
      EXPECT_FLOAT_EQ(shape.radii[2], c) << "right bottom";
      EXPECT_FLOAT_EQ(shape.radii[3], d) << "left bottom";
    }

    static void ExpectColor(const float *color, const float red, const float green, const float blue, const float alpha)
    {
      EXPECT_NEAR(color[0], red, 0.002f);
      EXPECT_NEAR(color[1], green, 0.002f);
      EXPECT_NEAR(color[2], blue, 0.002f);
      EXPECT_NEAR(color[3], alpha, 0.002f);
    }

    [[nodiscard]] bool Clicks(const double x, const double y)
    {
      ClickAt(x, y);
      return _ui->WasClicked("start");
    }

    [[nodiscard]] std::vector<std::string> ProblemsOf(const std::string &properties)
    {
      _logger->Clear();
      EXPECT_EQ(ShowUnderRoot("- type: panel\n  name: box\n" + Indented(properties, "  ")), -1);

      auto errors = Errors();
      if (!errors.empty()) { errors.pop_back(); }
      return errors;
    }
  };

  // round corners

  TEST_F(UiBoxTest, ABoxWithoutAnyOfItIsDrawnAsRectanglesAlone)
  {
    ShowBox("background_color: \"#ff0000\"\nborder: \"2px solid #ffffff\"\n");

    ASSERT_EQ(_renderer.batches.size(), 1u);
    EXPECT_TRUE(_renderer.batches[0].shapes.empty());
    EXPECT_EQ(_renderer.Quads().size(), 5u);

    for (const Vertex2D &vertex : _renderer.batches[0].vertices) { EXPECT_LT(vertex.shape, 0.0f); }
  }

  TEST_F(UiBoxTest, ABoxWithRoundCornersIsOneShapeOverItsBorderBox)
  {
    ShowBox("background_color: \"#ff8000\"\nborder_radius: 16\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);

    EXPECT_FLOAT_EQ(quads[0].left, 100);
    EXPECT_FLOAT_EQ(quads[0].top, 100);
    EXPECT_FLOAT_EQ(quads[0].Width(), 200);
    EXPECT_FLOAT_EQ(quads[0].Height(), 100);
    EXPECT_FALSE(quads[0].textured);
    EXPECT_NEAR(quads[0].color.g, 0.502f, 0.002f);

    const auto shapes = Shapes();
    ASSERT_EQ(shapes.size(), 1u);

    EXPECT_EQ(KindOf(shapes[0]), ShapeKind2D::Fill);
    EXPECT_FLOAT_EQ(shapes[0].box[0], 100) << "half its width";
    EXPECT_FLOAT_EQ(shapes[0].box[1], 50) << "half its height";
    ExpectRadii(shapes[0], 16, 16, 16, 16);
    EXPECT_FLOAT_EQ(shapes[0].gradient[0], 0) << "no gradient";
  }

  TEST_F(UiBoxTest, EveryCornerOfAShapeSaysWhereItIsInTheBox)
  {
    ShowBox("background_color: \"#ff8000\"\nborder_radius: 16\n");

    const auto &vertices = _renderer.batches[0].vertices;
    ASSERT_EQ(vertices.size(), 4u);

    // counted from the middle of the box, to the right and down
    const float expected[4][2] = {{-100, -50}, {100, -50}, {100, 50}, {-100, 50}};

    for (std::size_t i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ(vertices[i].shape, 0);
      EXPECT_FLOAT_EQ(vertices[i].local_x, expected[i][0]);
      EXPECT_FLOAT_EQ(vertices[i].local_y, expected[i][1]);
    }
  }

  TEST_F(UiBoxTest, ReadsOneToFourRadiiAsCssDoes)
  {
    ShowBox("background_color: \"#ff8000\"\nborder_radius: \"10 20\"\n");
    ExpectRadii(Shapes().at(0), 10, 20, 10, 20);

    _ui->CleanUp();
    Create({});
    ShowBox("background_color: \"#ff8000\"\nborder_radius: [10, 20, 30]\n");
    ExpectRadii(Shapes().at(0), 10, 20, 30, 20);

    _ui->CleanUp();
    Create({});
    ShowBox("background_color: \"#ff8000\"\nborder_radius: \"10px 20px 30px 40px\"\n");
    ExpectRadii(Shapes().at(0), 10, 20, 30, 40);
  }

  TEST_F(UiBoxTest, ACornerOfItsOwnWinsOverTheRadiusOfAll)
  {
    ShowBox(
      "background_color: \"#ff8000\"\n"
      "border_top_left_radius: 4\n"
      "border_radius: 16\n"
      "border_bottom_right_radius: 0\n");

    ExpectRadii(Shapes().at(0), 4, 16, 0, 16);
  }

  TEST_F(UiBoxTest, ARadiusInPercentIsOfTheShorterSide)
  {
    ShowBox("background_color: \"#ff8000\"\nborder_radius: 50%\n");

    // half of 100, which makes the short sides half circles
    ExpectRadii(Shapes().at(0), 50, 50, 50, 50);
  }

  TEST_F(UiBoxTest, CornersThatDoNotFitNextToEachOtherAreMadeSmaller)
  {
    ShowBox("background_color: \"#ff8000\"\nborder_radius: \"80 80 20 20\"\n");

    // 80 and 20 are 100 along the sides, which fits. 80 and 80 are 160
    // along the top, which is 200 wide and fits as well. 80 and 20 along
    // a side of 100 fit exactly.
    ExpectRadii(Shapes().at(0), 80, 80, 20, 20);

    _ui->CleanUp();
    Create({});
    ShowBox("background_color: \"#ff8000\"\nborder_radius: 80\n");

    // 80 and 80 along a side of 100: all four by the same factor
    ExpectRadii(Shapes().at(0), 50, 50, 50, 50);
  }

  TEST_F(UiBoxTest, RoundCornersGrowWithTheFrame)
  {
    _renderer.SetResolution(3840, 2160);
    ShowBox("background_color: \"#ff8000\"\nborder_radius: 16\n");

    const auto shapes = Shapes();
    ASSERT_EQ(shapes.size(), 1u);
    EXPECT_FLOAT_EQ(shapes[0].box[0], 200);
    ExpectRadii(shapes[0], 32, 32, 32, 32);
  }

  // borders

  TEST_F(UiBoxTest, TheBorderOfARoundBoxFollowsItsCorners)
  {
    ShowBox("background_color: \"#202020\"\nborder: \"4px solid #ff0000\"\nborder_radius: 16\n");

    const auto shapes = Shapes();
    ASSERT_EQ(shapes.size(), 2u);

    // the background, and the border on top of it
    EXPECT_EQ(KindOf(shapes[0]), ShapeKind2D::Fill);
    EXPECT_EQ(KindOf(shapes[1]), ShapeKind2D::Border);

    ExpectRadii(shapes[1], 16, 16, 16, 16);
    for (const float width : shapes[1].widths) { EXPECT_FLOAT_EQ(width, 4); }
    for (const auto &color : shapes[1].colors) { ExpectColor(color, 1, 0, 0, 1); }

    EXPECT_EQ(_renderer.Quads().size(), 2u) << "one rectangle for all four sides";
    EXPECT_EQ(_renderer.batches.size(), 1u);
  }

  TEST_F(UiBoxTest, EverySideOfABorderHasItsWidthAndItsColor)
  {
    ShowBox(
      "border_width: \"2 4 6 8\"\n"
      "border_color: \"#ffffff\"\n"
      "border_top_color: \"#ff0000\"\n"
      "border_bottom_color: \"#0000ff\"\n");

    const auto shapes = Shapes();
    ASSERT_EQ(shapes.size(), 1u);
    EXPECT_EQ(KindOf(shapes[0]), ShapeKind2D::Border);

    // top, right, bottom, left
    EXPECT_FLOAT_EQ(shapes[0].widths[0], 2);
    EXPECT_FLOAT_EQ(shapes[0].widths[1], 4);
    EXPECT_FLOAT_EQ(shapes[0].widths[2], 6);
    EXPECT_FLOAT_EQ(shapes[0].widths[3], 8);

    ExpectColor(shapes[0].colors[0], 1, 0, 0, 1);
    ExpectColor(shapes[0].colors[1], 1, 1, 1, 1);
    ExpectColor(shapes[0].colors[2], 0, 0, 1, 1);
    ExpectColor(shapes[0].colors[3], 1, 1, 1, 1);

    ExpectRadii(shapes[0], 0, 0, 0, 0);
  }

  TEST_F(UiBoxTest, ASideCanBeWrittenInOneLine)
  {
    ShowBox("border: \"2px solid #ffffff\"\nborder_left: \"6px solid #00ff00\"\nborder_top: none\n");

    const auto shapes = Shapes();
    ASSERT_EQ(shapes.size(), 1u);

    EXPECT_FLOAT_EQ(shapes[0].widths[0], 0);
    EXPECT_FLOAT_EQ(shapes[0].widths[1], 2);
    EXPECT_FLOAT_EQ(shapes[0].widths[3], 6);
    ExpectColor(shapes[0].colors[3], 0, 1, 0, 1);
    ExpectColor(shapes[0].colors[1], 1, 1, 1, 1);

    // the width of a side is room the content does not have
    ExpectBox("box", 100, 100, 200, 100);
    EXPECT_FLOAT_EQ(Element("box").GetPaddingBox().left, 106);
    EXPECT_FLOAT_EQ(Element("box").GetPaddingBox().top, 100);
  }

  TEST_F(UiBoxTest, ABorderWithoutAWidthIsNotDrawn)
  {
    ShowBox("background_color: \"#202020\"\nborder_radius: 8\nborder_color: \"#ff0000\"\n");

    EXPECT_EQ(Shapes().size(), 1u);
  }

  TEST_F(UiBoxTest, OpacityFadesABorder)
  {
    ShowBox("border: \"4px solid #ff0000\"\nborder_radius: 8\nopacity: 0.5\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);

    // the colors of the sides are those of the file, and the corners of
    // the rectangle say how much of them is seen
    EXPECT_FLOAT_EQ(quads[0].color.a, 0.5f);
    ExpectColor(Shapes().at(0).colors[0], 1, 0, 0, 1);
  }

  // gradients

  TEST_F(UiBoxTest, FillsABoxWithAGradient)
  {
    ShowBox("background: \"linear-gradient(90deg, #ff0000, #00ff00 25%, #0000ff)\"\n");

    const auto shapes = Shapes();
    ASSERT_EQ(shapes.size(), 1u);

    EXPECT_EQ(KindOf(shapes[0]), ShapeKind2D::Fill);
    EXPECT_FLOAT_EQ(shapes[0].gradient[0], 1) << "linear";
    EXPECT_NEAR(shapes[0].gradient[1], 1.5708f, 0.001f) << "90 degrees";
    EXPECT_FLOAT_EQ(shapes[0].gradient[2], 3) << "three colors";

    EXPECT_FLOAT_EQ(shapes[0].stop_positions[0], 0);
    EXPECT_FLOAT_EQ(shapes[0].stop_positions[1], 0.25f);
    EXPECT_FLOAT_EQ(shapes[0].stop_positions[2], 1);

    ExpectColor(shapes[0].stop_colors[0], 1, 0, 0, 1);
    ExpectColor(shapes[0].stop_colors[1], 0, 1, 0, 1);
    ExpectColor(shapes[0].stop_colors[2], 0, 0, 1, 1);
  }

  TEST_F(UiBoxTest, AGradientIsWrittenAsABackgroundImageAsWell)
  {
    ShowBox("background_image: \"radial-gradient(#ffffff, #000000)\"\nborder_radius: 8\n");

    const auto shapes = Shapes();
    ASSERT_EQ(shapes.size(), 1u);
    EXPECT_FLOAT_EQ(shapes[0].gradient[0], 2) << "radial";
    ExpectRadii(shapes[0], 8, 8, 8, 8);

    EXPECT_EQ(_renderer.loaded + _renderer.created, 0u) << "and asks for no texture";
  }

  TEST_F(UiBoxTest, DrawsAGradientOverTheColorOfTheBackground)
  {
    ShowBox(
      "background_color: \"#ff0000\"\n"
      "background_image: \"linear-gradient(#ffffff80, #00000000)\"\n");

    const auto shapes = Shapes();
    ASSERT_EQ(shapes.size(), 2u);
    EXPECT_FLOAT_EQ(shapes[0].gradient[0], 0);
    EXPECT_FLOAT_EQ(shapes[1].gradient[0], 1);
  }

  TEST_F(UiBoxTest, TheBackgroundTakesAColorAsWell)
  {
    ShowBox("background: \"#00ff00\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);
    EXPECT_FLOAT_EQ(quads[0].color.g, 1);
    EXPECT_TRUE(_renderer.batches[0].shapes.empty());
  }

  // shadows

  TEST_F(UiBoxTest, DrawsTheShadowOfABoxBelowIt)
  {
    ShowBox("background_color: \"#202020\"\nborder_radius: 8\nbox_shadow: \"4px 6px 10px 2px #000000\"\n");

    const auto shapes = Shapes();
    ASSERT_EQ(shapes.size(), 2u);

    EXPECT_EQ(KindOf(shapes[0]), ShapeKind2D::Shadow);
    EXPECT_EQ(KindOf(shapes[1]), ShapeKind2D::Fill);

    // the box of the element, which the shadow is not drawn under
    EXPECT_FLOAT_EQ(shapes[0].box[0], 100);
    EXPECT_FLOAT_EQ(shapes[0].box[1], 50);
    ExpectRadii(shapes[0], 8, 8, 8, 8);

    EXPECT_FLOAT_EQ(shapes[0].widths[0], 4) << "to the right";
    EXPECT_FLOAT_EQ(shapes[0].widths[1], 6) << "down";
    EXPECT_FLOAT_EQ(shapes[0].widths[2], 5) << "the deviation, which is half the blur of CSS";
    EXPECT_FLOAT_EQ(shapes[0].widths[3], 2) << "how much larger";

    // as far as the shadow can be seen: three deviations and what it is
    // larger by, and further to where it is moved
    const auto quads = _renderer.Quads();
    const float reach = 2 + 15 + 1;
    EXPECT_FLOAT_EQ(quads[0].left, 100 - reach);
    EXPECT_FLOAT_EQ(quads[0].top, 100 - reach);
    EXPECT_FLOAT_EQ(quads[0].right, 300 + reach + 4);
    EXPECT_FLOAT_EQ(quads[0].bottom, 200 + reach + 6);

    EXPECT_FLOAT_EQ(quads[0].color.r, 0);
    EXPECT_FLOAT_EQ(quads[0].color.a, 1);

    // and where a corner is in the shape is counted from the middle of
    // the box, not from that of the rectangle
    EXPECT_FLOAT_EQ(_renderer.batches[0].vertices[0].local_x, -100 - reach);
    EXPECT_FLOAT_EQ(_renderer.batches[0].vertices[2].local_x, 100 + reach + 4);
  }

  TEST_F(UiBoxTest, ABoxWithoutRoundCornersHasAShadowAsWell)
  {
    ShowBox("background_color: \"#202020\"\nbox_shadow: \"0 0 8px #000000\"\n");

    const auto shapes = Shapes();
    ASSERT_EQ(shapes.size(), 2u);
    EXPECT_EQ(KindOf(shapes[0]), ShapeKind2D::Shadow);
    ExpectRadii(shapes[0], 0, 0, 0, 0);
  }

  TEST_F(UiBoxTest, DrawsTheShadowThatIsWrittenFirstOnTop)
  {
    ShowBox("box_shadow: \"0 0 4px #ff0000, 0 0 8px #0000ff\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 2u);

    EXPECT_FLOAT_EQ(quads[0].color.b, 1);
    EXPECT_FLOAT_EQ(quads[1].color.r, 1);
  }

  TEST_F(UiBoxTest, AShadowThatFallsIntoABoxLiesOverItsBackgroundAndInsideItsBorder)
  {
    ShowBox(
      "background_color: \"#202020\"\n"
      "border: \"4px solid #ffffff\"\n"
      "border_radius: 12\n"
      "box_shadow: \"inset 0 2px 6px #000000\"\n");

    const auto shapes = Shapes();
    ASSERT_EQ(shapes.size(), 3u);

    EXPECT_EQ(KindOf(shapes[0]), ShapeKind2D::Fill);
    EXPECT_EQ(KindOf(shapes[1]), ShapeKind2D::InsetShadow);
    EXPECT_EQ(KindOf(shapes[2]), ShapeKind2D::Border);

    // the padding box, whose corners are less round by the border
    EXPECT_FLOAT_EQ(shapes[1].box[0], 96);
    EXPECT_FLOAT_EQ(shapes[1].box[1], 46);
    ExpectRadii(shapes[1], 8, 8, 8, 8);
    EXPECT_FLOAT_EQ(shapes[1].widths[1], 2);
    EXPECT_FLOAT_EQ(shapes[1].widths[2], 3);

    const auto quads = _renderer.Quads();
    EXPECT_FLOAT_EQ(quads[1].left, 104);
    EXPECT_FLOAT_EQ(quads[1].Width(), 192);
  }

  TEST_F(UiBoxTest, AShadowWithoutAColorHasThatOfTheText)
  {
    ShowBox("color: \"#00ff00\"\nbox_shadow: \"0 0 4px\"\nopacity: 0.5\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);
    EXPECT_FLOAT_EQ(quads[0].color.g, 1);
    EXPECT_FLOAT_EQ(quads[0].color.a, 0.5f);
  }

  TEST_F(UiBoxTest, ShadowsCostNoTextureAndNoDrawCallOfTheirOwn)
  {
    ShowBox(
      "background: \"linear-gradient(#202020, #404040)\"\n"
      "border: \"2px solid #ffffff\"\n"
      "border_radius: 12\n"
      "box_shadow: \"0 4px 12px #000000, inset 0 0 4px #ffffff\"\n",
      "- type: label\n  text: ab\n");

    EXPECT_EQ(_renderer.batches.size(), 1u);
    EXPECT_EQ(_renderer.batches[0].shapes.size(), 4u);
    EXPECT_EQ(_renderer.created, 1u) << "the atlas of the font";
  }

  // what is cut off

  TEST_F(UiBoxTest, CutsOffAtRoundCorners)
  {
    ShowBox(
      "overflow: hidden\nborder_radius: 20\nborder_width: 4\n",
      "- type: panel\n  width: 400\n  height: 400\n  flex_shrink: 0\n  background_color: \"#ff0000\"\n");

    // the border is one call, and what is inside the box another
    ASSERT_EQ(_renderer.batches.size(), 2u);

    const Triangles2D &outside = _renderer.batches[0];
    const Triangles2D &inside = _renderer.batches[1];

    EXPECT_FALSE(outside.has_rounded_clip);
    ASSERT_TRUE(inside.has_rounded_clip);

    // the padding box, from 104 to 296 and from 104 to 196
    EXPECT_TRUE(inside.clipped);
    EXPECT_EQ(inside.clip, (neon::ClipRectangle{104, 104, 192, 92}));

    EXPECT_FLOAT_EQ(inside.rounded_clip.center_x, 200);
    EXPECT_FLOAT_EQ(inside.rounded_clip.center_y, 150);
    EXPECT_FLOAT_EQ(inside.rounded_clip.half_width, 96);
    EXPECT_FLOAT_EQ(inside.rounded_clip.half_height, 46);

    for (const float radius : inside.rounded_clip.radii) { EXPECT_FLOAT_EQ(radius, 16); }
  }

  TEST_F(UiBoxTest, ABoxWithoutRoundCornersCutsOffAsBefore)
  {
    ShowBox(
      "overflow: hidden\n",
      "- type: panel\n  width: 400\n  height: 400\n  flex_shrink: 0\n  background_color: \"#ff0000\"\n");

    ASSERT_EQ(_renderer.batches.size(), 1u);
    EXPECT_TRUE(_renderer.batches[0].clipped);
    EXPECT_FALSE(_renderer.batches[0].has_rounded_clip);
  }

  TEST_F(UiBoxTest, StopsCuttingOffAtRoundCornersBehindTheBox)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  width: 100\n"
      "  height: 100\n"
      "  overflow: hidden\n"
      "  border_radius: 20\n"
      "  children:\n"
      "    - type: panel\n"
      "      width: 50\n"
      "      height: 50\n"
      "      background_color: \"#ff0000\"\n"
      "- type: panel\n"
      "  width: 50\n"
      "  height: 50\n"
      "  background_color: \"#00ff00\"\n"), 0);
    Frame();

    ASSERT_EQ(_renderer.batches.size(), 2u);
    EXPECT_TRUE(_renderer.batches[0].has_rounded_clip);
    EXPECT_FALSE(_renderer.batches[1].has_rounded_clip);
    EXPECT_FALSE(_renderer.batches[1].clipped);
  }

  // the outline

  TEST_F(UiBoxTest, TheOutlineOfARoundBoxFollowsItsCorners)
  {
    ShowBox(
      "background_color: \"#202020\"\n"
      "border_radius: 10\n"
      "outline_width: 2\n"
      "outline_offset: 3\n"
      "outline_color: \"#ffd166\"\n");

    const auto shapes = Shapes();
    ASSERT_EQ(shapes.size(), 2u);

    const Shape2D &outline = shapes[1];
    EXPECT_EQ(KindOf(outline), ShapeKind2D::Border);

    // 5 further out on every side, and as much rounder
    EXPECT_FLOAT_EQ(outline.box[0], 105);
    EXPECT_FLOAT_EQ(outline.box[1], 55);
    ExpectRadii(outline, 15, 15, 15, 15);
    for (const float width : outline.widths) { EXPECT_FLOAT_EQ(width, 2); }
    ExpectColor(outline.colors[0], 1, 0.820f, 0.4f, 1);

    const auto quads = _renderer.Quads();
    EXPECT_FLOAT_EQ(quads[1].left, 95);
    EXPECT_FLOAT_EQ(quads[1].Width(), 210);
  }

  TEST_F(UiBoxTest, ACornerThatIsSquareStaysSquareInTheOutline)
  {
    ShowBox(
      "background_color: \"#202020\"\n"
      "border_radius: \"10 0 0 0\"\n"
      "outline_width: 2\n");

    ExpectRadii(Shapes().at(1), 12, 0, 0, 0);
  }

  // where the pointer is

  TEST_F(UiBoxTest, APointerInARoundCornerIsNotOnTheElement)
  {
    // a disc of 100 across, from 100 to 200
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n"
      "  name: start\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 100\n"
      "  box_sizing: border-box\n"
      "  width: 100\n"
      "  height: 100\n"
      "  padding: 0\n"
      "  border_radius: 50%\n"), 0);
    Frame();

    EXPECT_TRUE(Clicks(150, 150)) << "its middle";
    EXPECT_TRUE(Clicks(150, 101)) << "the top of the disc";
    EXPECT_TRUE(Clicks(198, 150)) << "its right";

    // inside the box, and outside the disc
    EXPECT_FALSE(Clicks(102, 102));
    EXPECT_FALSE(Clicks(198, 102));
    EXPECT_FALSE(Clicks(198, 198));
    EXPECT_FALSE(Clicks(102, 198));

    // on the disc, 45 from its middle towards a corner, and off it at 52
    EXPECT_TRUE(Clicks(150 - 31, 150 - 31));
    EXPECT_FALSE(Clicks(150 - 37, 150 - 37));

    EXPECT_FALSE(Clicks(99, 150));
    EXPECT_FALSE(Clicks(201, 150));
  }

  TEST_F(UiBoxTest, APointerInACornerThatIsSquareIsOnTheElement)
  {
    ShowButton("border_radius: \"40 0 0 0\"\n");

    EXPECT_FALSE(Clicks(103, 103)) << "the round corner";
    EXPECT_TRUE(Clicks(297, 103)) << "a square one";
    EXPECT_TRUE(Clicks(297, 197));
    EXPECT_TRUE(Clicks(103, 197));
    EXPECT_TRUE(Clicks(140, 103)) << "where the round corner has ended";
  }

  TEST_F(UiBoxTest, ARoundCornerSetsTheStateOfAnElementAsWell)
  {
    ShowButton("border_radius: 50\n");

    PointAt(103, 103);
    Frame();
    EXPECT_FALSE(Element("start").GetStates().hover);

    PointAt(150, 150);
    Frame();
    EXPECT_TRUE(Element("start").GetStates().hover);
  }

  TEST_F(UiBoxTest, WhatIsCutOffAtARoundCornerCannotBePointedAt)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 100\n"
      "  width: 100\n"
      "  height: 100\n"
      "  overflow: hidden\n"
      "  border_radius: 50\n"
      "  children:\n"
      "    - type: button\n"
      "      name: start\n"
      "      width: 100\n"
      "      height: 100\n"
      "      flex_shrink: 0\n"
      "      padding: 0\n"), 0);
    Frame();

    EXPECT_TRUE(Clicks(150, 150));
    EXPECT_FALSE(Clicks(103, 103)) << "the button reaches there, and is cut off";
  }

  // what moves and turns an element

  TEST_F(UiBoxTest, DrawsAnElementWhereItIsMovedTo)
  {
    ShowBox("background_color: \"#ff0000\"\ntransform: \"translate(50px, -20px)\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);
    EXPECT_FLOAT_EQ(quads[0].left, 150);
    EXPECT_FLOAT_EQ(quads[0].top, 80);
    EXPECT_FLOAT_EQ(quads[0].Width(), 200);

    // where it is in the layout stays what it was
    ExpectBox("box", 100, 100, 200, 100);
  }

  TEST_F(UiBoxTest, MovesAnElementByPartsOfItsSize)
  {
    ShowBox("background_color: \"#ff0000\"\ntransform: \"translate(-50%, 100%)\"\n");

    const auto quads = _renderer.Quads();
    EXPECT_FLOAT_EQ(quads.at(0).left, 0);
    EXPECT_FLOAT_EQ(quads.at(0).top, 200);
  }

  TEST_F(UiBoxTest, TurnsAnElementAroundItsMiddle)
  {
    ShowBox("background_color: \"#ff0000\"\ntransform: \"rotate(90deg)\"\n");

    const auto &vertices = _renderer.batches.at(0).vertices;
    ASSERT_EQ(vertices.size(), 4u);

    // The middle is at 200, 150. The left top corner, 100 left of it and
    // 50 above, goes to 50 right of it and 100 above.
    EXPECT_NEAR(vertices[0].x, 250, 0.001f);
    EXPECT_NEAR(vertices[0].y, 50, 0.001f);
    EXPECT_NEAR(vertices[1].x, 250, 0.001f);
    EXPECT_NEAR(vertices[1].y, 250, 0.001f);
    EXPECT_NEAR(vertices[2].x, 150, 0.001f);
    EXPECT_NEAR(vertices[2].y, 250, 0.001f);
  }

  TEST_F(UiBoxTest, TurnsAnElementAroundTheOriginItIsGiven)
  {
    ShowBox("background_color: \"#ff0000\"\ntransform: \"rotate(90deg)\"\ntransform_origin: \"left top\"\n");

    const auto &vertices = _renderer.batches.at(0).vertices;

    EXPECT_NEAR(vertices[0].x, 100, 0.001f);
    EXPECT_NEAR(vertices[0].y, 100, 0.001f);
    EXPECT_NEAR(vertices[1].x, 100, 0.001f);
    EXPECT_NEAR(vertices[1].y, 300, 0.001f);
  }

  TEST_F(UiBoxTest, MakesAnElementLargerFromItsMiddle)
  {
    ShowBox("background_color: \"#ff0000\"\ntransform: \"scale(2, 0.5)\"\n");

    const auto quads = _renderer.Quads();
    EXPECT_FLOAT_EQ(quads.at(0).left, 0);
    EXPECT_FLOAT_EQ(quads.at(0).right, 400);
    EXPECT_FLOAT_EQ(quads.at(0).top, 125);
    EXPECT_FLOAT_EQ(quads.at(0).bottom, 175);
  }

  TEST_F(UiBoxTest, WhatIsInsideAnElementMovesWithIt)
  {
    ShowBox(
      "transform: \"translate(100px, 0)\"\n",
      "- type: panel\n"
      "  width: 20\n"
      "  height: 20\n"
      "  background_color: \"#00ff00\"\n"
      "  transform: \"translate(0, 30px)\"\n");

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);
    EXPECT_FLOAT_EQ(quads[0].left, 200);
    EXPECT_FLOAT_EQ(quads[0].top, 130);
  }

  TEST_F(UiBoxTest, AMoveGrowsWithTheFrame)
  {
    _renderer.SetResolution(3840, 2160);
    ShowBox("background_color: \"#ff0000\"\ntransform: \"translate(50px, 0)\"\n");

    EXPECT_FLOAT_EQ(_renderer.Quads().at(0).left, 300);
  }

  TEST_F(UiBoxTest, AShapeTurnsWithItsElement)
  {
    ShowBox("background_color: \"#ff0000\"\nborder_radius: 16\ntransform: \"rotate(90deg)\"\n");

    const auto &vertices = _renderer.batches.at(0).vertices;

    // where a corner is in its shape is what it was before the turn
    EXPECT_NEAR(vertices[0].x, 250, 0.001f);
    EXPECT_FLOAT_EQ(vertices[0].local_x, -100);
    EXPECT_FLOAT_EQ(vertices[0].local_y, -50);
  }

  TEST_F(UiBoxTest, ThePointerFindsAnElementWhereItIsMovedTo)
  {
    ShowButton("transform: \"translate(300px, 0)\"\n");

    // from 400 to 600 now
    EXPECT_TRUE(Clicks(500, 150));
    EXPECT_TRUE(Clicks(598, 150));
    EXPECT_FALSE(Clicks(150, 150)) << "where it would be without the move";
    EXPECT_FALSE(Clicks(399, 150));
  }

  TEST_F(UiBoxTest, ThePointerFindsAnElementThatIsTurned)
  {
    // 200 wide and 100 high, turned by 90 degrees around 200, 150: it
    // covers 150 to 250 from side to side and 50 to 250 from top to bottom
    ShowButton("transform: \"rotate(90deg)\"\n");

    EXPECT_TRUE(Clicks(200, 60));
    EXPECT_TRUE(Clicks(200, 240));
    EXPECT_TRUE(Clicks(155, 150));

    EXPECT_FALSE(Clicks(110, 150)) << "on the element before the turn, and beside it now";
    EXPECT_FALSE(Clicks(290, 150));
    EXPECT_FALSE(Clicks(200, 45));
  }

  TEST_F(UiBoxTest, ThePointerFindsAnElementThatIsMadeLarger)
  {
    ShowButton("transform: \"scale(0.5)\"\n");

    // 100 by 50 around 200, 150
    EXPECT_TRUE(Clicks(200, 150));
    EXPECT_TRUE(Clicks(152, 127));
    EXPECT_FALSE(Clicks(140, 150));
    EXPECT_FALSE(Clicks(200, 120));
  }

  TEST_F(UiBoxTest, ThePointerFindsWhatIsInsideAnElementThatIsMoved)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 100\n"
      "  width: 200\n"
      "  height: 100\n"
      "  transform: \"translate(0, 200px) rotate(180deg)\"\n"
      "  children:\n"
      "    - type: button\n"
      "      name: start\n"
      "      width: 50\n"
      "      height: 50\n"
      "      padding: 0\n"), 0);
    Frame();

    // The button is at the left top of its panel, from 100, 100 to
    // 150, 150. Turned half way around 200, 150 it is at the right
    // bottom, from 250, 150 to 300, 200, and then 200 further down.
    EXPECT_TRUE(Clicks(275, 375));
    EXPECT_TRUE(Clicks(252, 352));
    EXPECT_FALSE(Clicks(125, 125));
    EXPECT_FALSE(Clicks(125, 325));
  }

  TEST_F(UiBoxTest, AnElementThatIsFlattenedCannotBePointedAt)
  {
    ShowButton("transform: \"scale(0)\"\n");

    EXPECT_FALSE(Clicks(200, 150));
    EXPECT_TRUE(Errors().empty()) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(UiBoxTest, TheRoundCornersOfAnElementThatIsMovedMoveWithIt)
  {
    ShowButton("border_radius: 50\ntransform: \"translate(300px, 0)\"\n");

    EXPECT_FALSE(Clicks(403, 103));
    EXPECT_TRUE(Clicks(450, 150));
  }

  TEST_F(UiBoxTest, AStateCanMoveAnElement)
  {
    ShowButton("hover:\n  transform: \"scale(1.1)\"\n");

    PointAt(200, 150);
    Frame();
    Frame();

    const auto quads = _renderer.Quads();
    ASSERT_FALSE(quads.empty());
    EXPECT_FLOAT_EQ(quads[0].left, 90);
    EXPECT_FLOAT_EQ(quads[0].right, 310);
  }

  // what is wrong

  TEST_F(UiBoxTest, SaysWhatIsWrongWithAPropertyOfABox)
  {
    EXPECT_THAT(ProblemsOf("border_radius: round\n"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'border_radius' of panel 'box' is 'round', where one to four "
                  "values, each a number of pixels or a percentage such as 50%, and none below 0 was "
                  "expected"));

    EXPECT_THAT(ProblemsOf("border_radius: \"1 2 3 4 5\"\n"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'border_radius' of panel 'box' is '1 2 3 4 5', where one to four "
                  "values, each a number of pixels or a percentage such as 50%, and none below 0 was "
                  "expected"));

    EXPECT_THAT(ProblemsOf("border_top_left_radius: -4\n"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'border_top_left_radius' of panel 'box' is a number, where a "
                  "number of pixels or a percentage that is not below 0 was expected"));

    EXPECT_THAT(ProblemsOf("border_top_color: red\n"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'border_top_color' of panel 'box' is 'red', where a color such "
                  "as \"#ff8000\", \"#ff800080\", or rgb(255, 128, 0), or a list of 3 to 4 numbers was "
                  "expected"));

    EXPECT_THAT(ProblemsOf("border_left: \"2px dashed #fff\"\n"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'border_left' of panel 'box' is '2px dashed #fff', where a "
                  "width, solid or none, and a color, such as \"2px solid #ffffff\" was expected"));

    EXPECT_THAT(ProblemsOf("box_shadow: \"4px\"\n"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'box_shadow' of panel 'box' is '4px', where none, or shadows "
                  "such as \"0 4px 12px 0 rgba(0, 0, 0, 0.5)\": inset or not, to the right, down, a blur that "
                  "is not below 0, how much larger, and a color was expected"));

    EXPECT_THAT(ProblemsOf("background: \"conic-gradient(#f00, #00f)\"\n"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'background' of panel 'box' is 'conic-gradient(#f00, #00f)', "
                  "where none, a color, a gradient such as linear-gradient(90deg, #f00, #00f), or the "
                  "virtual path of an image was expected"));

    EXPECT_THAT(ProblemsOf("background_image: \"linear-gradient(#f00)\"\n"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'background_image' of panel 'box' is 'linear-gradient(#f00)', "
                  "where the virtual path of an image, none, or a gradient such as "
                  "linear-gradient(90deg, #f00, #00f) with 2 to 8 colors was expected"));

    EXPECT_THAT(ProblemsOf("transform: \"skew(10deg)\"\n"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'transform' of panel 'box' is 'skew(10deg)', where none, or "
                  "steps such as \"translate(10px, 50%) rotate(45deg) scale(1.5)\" was expected"));

    EXPECT_THAT(ProblemsOf("transform_origin: middle\n"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'transform_origin' of panel 'box' is 'middle', where one or two "
                  "of left, center, right, top, bottom, a number of pixels, or a percentage was expected"));
  }
}
