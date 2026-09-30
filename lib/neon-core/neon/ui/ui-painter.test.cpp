#include "ui-painter.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/fake-font-rasterizer.hpp>
#include <neon/testing/mock-render-2d-context.hpp>
#include <neon/text/utf8.hpp>

// How rectangles are collected into draw calls, against a mock of the
// renderer.

namespace
{
  using neon::ClipRectangle;
  using neon::Color;
  using neon::No_Texture;
  using neon::Triangles2D;
  using neon::UiFont;
  using neon::UiImage;
  using neon::UiPainter;
  using neon::UiRectangle;
  using neon::testing::FakeFontRasterizer;
  using neon::testing::MockRender2DContext;
  using ::testing::_;
  using ::testing::AllOf;
  using ::testing::Field;
  using ::testing::InSequence;
  using ::testing::SizeIs;
  using ::testing::StrictMock;

  constexpr Color red{1.0f, 0.0f, 0.0f, 1.0f};
  constexpr Color white{1.0f, 1.0f, 1.0f, 1.0f};
  constexpr UiRectangle whole{0.0f, 0.0f, 1.0f, 1.0f};

  /// Triangles of that many rectangles, with that texture.
  ::testing::Matcher<const Triangles2D &> Rectangles(const std::size_t count, const int texture)
  {
    return AllOf(
      Field("vertices", &Triangles2D::vertices, SizeIs(count * 4)),
      Field("indices", &Triangles2D::indices, SizeIs(count * 6)),
      Field("texture", &Triangles2D::texture, texture));
  }

  class UiPainterTest : public ::testing::Test
  {
  protected:
    StrictMock<MockRender2DContext> _renderer;
    UiPainter _painter{&_renderer};

    UiImage _heart{3, 64, 64};
    UiImage _star{4, 64, 64};

    void SetUp() override
    {
      _painter.Begin(1920, 1080);
    }

    static UiRectangle At(const float left, const float top)
    {
      return {left, top, left + 10.0f, top + 10.0f};
    }
  };

  TEST_F(UiPainterTest, HandsNothingOverForAFrameWithoutAnything)
  {
    _painter.End();

    EXPECT_EQ(_painter.GetDrawCalls(), 0u);
    EXPECT_EQ(_painter.GetQuads(), 0u);
  }

  TEST_F(UiPainterTest, HandsNothingOverUntilTheFrameEnds)
  {
    _painter.FillRectangle(At(0, 0), red);
    _painter.FillRectangle(At(20, 0), red);

    EXPECT_EQ(_painter.GetDrawCalls(), 0u);

    EXPECT_CALL(_renderer, DrawTriangles(Rectangles(2, No_Texture)));
    _painter.End();

    EXPECT_EQ(_painter.GetDrawCalls(), 1u);
    EXPECT_EQ(_painter.GetQuads(), 2u);
  }

  TEST_F(UiPainterTest, DrawsRectanglesOfOneTextureInOneCall)
  {
    for (int i = 0; i < 100; i++) { _painter.DrawImage(_heart, At(static_cast<float>(i) * 10, 0), whole, white); }

    EXPECT_CALL(_renderer, DrawTriangles(Rectangles(100, 3)));
    _painter.End();

    EXPECT_EQ(_painter.GetDrawCalls(), 1u);
  }

  TEST_F(UiPainterTest, StartsANewCallForAnotherTexture)
  {
    InSequence in_order;
    EXPECT_CALL(_renderer, DrawTriangles(Rectangles(2, 3)));
    EXPECT_CALL(_renderer, DrawTriangles(Rectangles(1, 4)));
    EXPECT_CALL(_renderer, DrawTriangles(Rectangles(1, 3)));

    _painter.DrawImage(_heart, At(0, 0), whole, white);
    _painter.DrawImage(_heart, At(20, 0), whole, white);
    _painter.DrawImage(_star, At(40, 0), whole, white);
    _painter.DrawImage(_heart, At(60, 0), whole, white);
    _painter.End();

    EXPECT_EQ(_painter.GetDrawCalls(), 3u);
  }

  TEST_F(UiPainterTest, ARectangleWithoutATextureJoinsTheCallItFollows)
  {
    EXPECT_CALL(_renderer, DrawTriangles(Rectangles(4, 3)));

    _painter.DrawImage(_heart, At(0, 0), whole, white);
    _painter.FillRectangle(At(20, 0), red);
    _painter.FillRectangle(At(40, 0), red);
    _painter.DrawImage(_heart, At(60, 0), whole, white);
    _painter.End();
  }

  TEST_F(UiPainterTest, ARectangleWithoutATextureJoinsTheCallThatFollowsIt)
  {
    EXPECT_CALL(_renderer, DrawTriangles(Rectangles(3, 4)));

    _painter.FillRectangle(At(0, 0), red);
    _painter.FillRectangle(At(20, 0), red);
    _painter.DrawImage(_star, At(40, 0), whole, white);
    _painter.End();
  }

  TEST_F(UiPainterTest, SaysForEveryCornerWhetherItReadsTheTexture)
  {
    Triangles2D drawn;
    EXPECT_CALL(_renderer, DrawTriangles(_)).WillOnce(::testing::SaveArg<0>(&drawn));

    _painter.FillRectangle(At(0, 0), red);
    _painter.DrawImage(_heart, At(20, 0), {0.25f, 0.5f, 0.75f, 1.0f}, white);
    _painter.End();

    ASSERT_EQ(drawn.vertices.size(), 8u);

    for (std::size_t i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ(drawn.vertices[i].textured, 0);
      EXPECT_FLOAT_EQ(drawn.vertices[i].color.g, 0);
    }

    for (std::size_t i = 4; i < 8; i++) { EXPECT_FLOAT_EQ(drawn.vertices[i].textured, 1); }

    // the part of the texture, from the left top corner around
    EXPECT_FLOAT_EQ(drawn.vertices[4].u, 0.25f);
    EXPECT_FLOAT_EQ(drawn.vertices[4].v, 0.5f);
    EXPECT_FLOAT_EQ(drawn.vertices[5].u, 0.75f);
    EXPECT_FLOAT_EQ(drawn.vertices[5].v, 0.5f);
    EXPECT_FLOAT_EQ(drawn.vertices[6].u, 0.75f);
    EXPECT_FLOAT_EQ(drawn.vertices[6].v, 1.0f);
    EXPECT_FLOAT_EQ(drawn.vertices[7].u, 0.25f);
    EXPECT_FLOAT_EQ(drawn.vertices[7].v, 1.0f);
  }

  TEST_F(UiPainterTest, KeepsTheOrderTheRectanglesWereDrawnIn)
  {
    Triangles2D drawn;
    EXPECT_CALL(_renderer, DrawTriangles(_)).WillOnce(::testing::SaveArg<0>(&drawn));

    _painter.FillRectangle(At(0, 0), red);
    _painter.FillRectangle(At(20, 0), white);
    _painter.FillRectangle(At(40, 0), red);
    _painter.End();

    ASSERT_EQ(drawn.vertices.size(), 12u);
    EXPECT_FLOAT_EQ(drawn.vertices[0].x, 0);
    EXPECT_FLOAT_EQ(drawn.vertices[4].x, 20);
    EXPECT_FLOAT_EQ(drawn.vertices[8].x, 40);

    // every rectangle names corners of its own
    EXPECT_EQ(drawn.indices, (std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11}));
  }

  TEST_F(UiPainterTest, LeavesOutWhatCannotBeSeen)
  {
    _painter.FillRectangle({10, 10, 10, 50}, red);
    _painter.FillRectangle({10, 10, 5, 50}, red);
    _painter.FillRectangle({10, 10, 50, 10}, red);
    _painter.FillRectangle(At(0, 0), {1.0f, 0.0f, 0.0f, 0.0f});
    _painter.FillRectangle({-50, 0, 0, 10}, red);
    _painter.FillRectangle({1920, 0, 1930, 10}, red);
    _painter.FillRectangle({0, -10, 10, 0}, red);
    _painter.FillRectangle({0, 1080, 10, 1090}, red);
    _painter.DrawImage(UiImage{}, At(0, 0), whole, white);
    _painter.End();

    EXPECT_EQ(_painter.GetDrawCalls(), 0u);
    EXPECT_EQ(_painter.GetQuads(), 0u);
  }

  TEST_F(UiPainterTest, KeepsWhatIsPartlyInsideTheFrame)
  {
    EXPECT_CALL(_renderer, DrawTriangles(Rectangles(2, No_Texture)));

    _painter.FillRectangle({-5, -5, 5, 5}, red);
    _painter.FillRectangle({1915, 1075, 1925, 1085}, red);
    _painter.End();
  }

  // what is cut off

  TEST_F(UiPainterTest, StartsANewCallWhereWhatIsCutOffChanges)
  {
    InSequence in_order;
    EXPECT_CALL(_renderer, DrawTriangles(AllOf(
      Rectangles(1, No_Texture),
      Field("clipped", &Triangles2D::clipped, false))));
    EXPECT_CALL(_renderer, DrawTriangles(AllOf(
      Rectangles(2, No_Texture),
      Field("clipped", &Triangles2D::clipped, true),
      Field("clip", &Triangles2D::clip, ClipRectangle{100, 100, 200, 100}))));
    EXPECT_CALL(_renderer, DrawTriangles(AllOf(
      Rectangles(1, No_Texture),
      Field("clipped", &Triangles2D::clipped, false))));

    _painter.FillRectangle(At(0, 0), red);

    _painter.PushClip({100, 100, 300, 200});
    _painter.FillRectangle(At(100, 100), red);
    _painter.FillRectangle(At(120, 100), red);
    _painter.PopClip();

    _painter.FillRectangle(At(20, 0), red);
    _painter.End();

    EXPECT_EQ(_painter.GetDrawCalls(), 3u);
  }

  TEST_F(UiPainterTest, StartsNoCallForACutThatNothingIsDrawnIn)
  {
    EXPECT_CALL(_renderer, DrawTriangles(Rectangles(2, No_Texture)));

    _painter.FillRectangle(At(0, 0), red);
    _painter.PushClip({100, 100, 300, 200});
    _painter.PopClip();
    _painter.FillRectangle(At(20, 0), red);
    _painter.End();
  }

  TEST_F(UiPainterTest, CutsOffAtWhatAllTheCutsLeave)
  {
    EXPECT_CALL(_renderer, DrawTriangles(
      Field("clip", &Triangles2D::clip, ClipRectangle{150, 120, 150, 80})));

    _painter.PushClip({100, 100, 300, 200});
    _painter.PushClip({150, 120, 400, 300});
    _painter.FillRectangle(At(160, 130), red);
    _painter.PopClip();
    _painter.PopClip();
    _painter.End();
  }

  TEST_F(UiPainterTest, GoesBackToTheCutBefore)
  {
    InSequence in_order;
    EXPECT_CALL(_renderer, DrawTriangles(Field("clip", &Triangles2D::clip, ClipRectangle{150, 120, 150, 80})));
    EXPECT_CALL(_renderer, DrawTriangles(Field("clip", &Triangles2D::clip, ClipRectangle{100, 100, 200, 100})));

    _painter.PushClip({100, 100, 300, 200});
    _painter.PushClip({150, 120, 400, 300});
    _painter.FillRectangle(At(160, 130), red);
    _painter.PopClip();
    _painter.FillRectangle(At(110, 110), red);
    _painter.PopClip();
    _painter.End();
  }

  TEST_F(UiPainterTest, LeavesOutWhatIsCutOffAltogether)
  {
    _painter.PushClip({100, 100, 300, 200});
    _painter.FillRectangle(At(0, 0), red);
    _painter.FillRectangle(At(300, 100), red);
    _painter.PopClip();

    // cuts that leave nothing
    _painter.PushClip({100, 100, 200, 200});
    _painter.PushClip({300, 300, 400, 400});
    _painter.FillRectangle(At(150, 150), red);
    _painter.FillRectangle(At(350, 350), red);
    _painter.PopClip();
    _painter.PopClip();

    _painter.End();

    EXPECT_EQ(_painter.GetDrawCalls(), 0u);
  }

  TEST_F(UiPainterTest, APopWithoutAPushIsLeftAlone)
  {
    EXPECT_CALL(_renderer, DrawTriangles(Field("clipped", &Triangles2D::clipped, false)));

    _painter.PopClip();
    _painter.FillRectangle(At(0, 0), red);
    _painter.End();
  }

  // a frame after another

  TEST_F(UiPainterTest, StartsEveryFrameFromNothing)
  {
    EXPECT_CALL(_renderer, DrawTriangles(Rectangles(1, 3)));
    _painter.PushClip({0, 0, 100, 100});
    _painter.DrawImage(_heart, At(0, 0), whole, white);
    _painter.End();

    _painter.Begin(1280, 720);

    EXPECT_EQ(_painter.GetDrawCalls(), 0u);
    EXPECT_EQ(_painter.GetQuads(), 0u);

    EXPECT_CALL(_renderer, DrawTriangles(AllOf(
      Rectangles(1, No_Texture),
      Field("clipped", &Triangles2D::clipped, false))));
    _painter.FillRectangle(At(200, 200), red);
    _painter.End();
  }

  TEST_F(UiPainterTest, LeavesOutWhatLiesOutsideTheFrameOfThisFrame)
  {
    _painter.Begin(1280, 720);

    _painter.FillRectangle(At(1500, 100), red);
    _painter.End();

    EXPECT_EQ(_painter.GetDrawCalls(), 0u);
  }

  // text

  TEST_F(UiPainterTest, DrawsATextInOneCallWithItsBackground)
  {
    FakeFontRasterizer rasterizer;
    const int loaded = rasterizer.LoadFont(FakeFontRasterizer::AFont());

    UiFont font;
    std::string error;
    ASSERT_TRUE(font.atlas.Build(rasterizer, loaded, 20, neon::FontAtlas::DefaultCharacters(), error)) << error;
    font.texture = 7;

    const auto text = neon::PlaceText(font.atlas, neon::DecodeUtf8("Health: 75"), {});

    Triangles2D drawn;
    EXPECT_CALL(_renderer, DrawTriangles(Rectangles(10, 7))).WillOnce(::testing::SaveArg<0>(&drawn));

    _painter.FillRectangle({100, 100, 300, 130}, red);
    _painter.DrawText(font, text, 100, 100, white);
    _painter.End();

    // the first character, where the atlas has it
    const auto *glyph = font.atlas.Find(U'H');
    const auto size = static_cast<float>(font.atlas.GetWidth());

    EXPECT_FLOAT_EQ(drawn.vertices[4].x, 101);
    EXPECT_FLOAT_EQ(drawn.vertices[4].y, 102);
    EXPECT_FLOAT_EQ(drawn.vertices[6].x, 109);
    EXPECT_FLOAT_EQ(drawn.vertices[6].y, 116);
    EXPECT_FLOAT_EQ(drawn.vertices[4].u, static_cast<float>(glyph->x) / size);
    EXPECT_FLOAT_EQ(drawn.vertices[4].v, static_cast<float>(glyph->y) / size);
    EXPECT_FLOAT_EQ(drawn.vertices[6].u, static_cast<float>(glyph->x + glyph->width) / size);
    EXPECT_FLOAT_EQ(drawn.vertices[6].v, static_cast<float>(glyph->y + glyph->height) / size);
  }

  TEST_F(UiPainterTest, DrawsNoTextWithoutATexture)
  {
    FakeFontRasterizer rasterizer;
    const int loaded = rasterizer.LoadFont(FakeFontRasterizer::AFont());

    UiFont font;
    std::string error;
    ASSERT_TRUE(font.atlas.Build(rasterizer, loaded, 20, neon::FontAtlas::DefaultCharacters(), error)) << error;

    _painter.DrawText(font, neon::PlaceText(font.atlas, U"abc", {}), 0, 0, white);
    _painter.DrawText(UiFont{}, neon::PlaceText(font.atlas, U"abc", {}), 0, 0, white);
    _painter.End();

    EXPECT_EQ(_painter.GetDrawCalls(), 0u);
  }

  // a border, and an image in nine parts

  TEST_F(UiPainterTest, ABorderWithoutAWidthIsNotDrawn)
  {
    _painter.FillBorder({100, 100, 300, 200}, {0, 0, 0, 0}, red);
    _painter.End();

    EXPECT_EQ(_painter.GetQuads(), 0u);
  }

  TEST_F(UiPainterTest, ABorderThatIsWiderThanItsBoxFillsTheBox)
  {
    Triangles2D drawn;
    EXPECT_CALL(_renderer, DrawTriangles(_)).WillOnce(::testing::SaveArg<0>(&drawn));

    _painter.FillBorder({100, 100, 120, 120}, {50, 50, 50, 50}, red);
    _painter.End();

    // nothing sticks out of the box
    for (const auto &vertex : drawn.vertices)
    {
      EXPECT_GE(vertex.x, 100);
      EXPECT_LE(vertex.x, 120);
      EXPECT_GE(vertex.y, 100);
      EXPECT_LE(vertex.y, 120);
    }
  }

  TEST_F(UiPainterTest, AnImageInNinePartsWithoutCornersIsTheMiddleAlone)
  {
    EXPECT_CALL(_renderer, DrawTriangles(Rectangles(1, 3)));

    _painter.DrawNineSlice(_heart, {100, 100, 300, 200}, {0, 0, 0, 0}, {0, 0, 0, 0}, white);
    _painter.End();
  }

  TEST_F(UiPainterTest, AnImageInNinePartsNeedsAnImage)
  {
    _painter.DrawNineSlice(UiImage{}, {100, 100, 300, 200}, {8, 8, 8, 8}, {8, 8, 8, 8}, white);
    _painter.DrawNineSlice(UiImage{3, 0, 0}, {100, 100, 300, 200}, {8, 8, 8, 8}, {8, 8, 8, 8}, white);
    _painter.DrawNineSlice(_heart, {100, 100, 100, 200}, {8, 8, 8, 8}, {8, 8, 8, 8}, white);
    _painter.End();

    EXPECT_EQ(_painter.GetQuads(), 0u);
  }

  TEST_F(UiPainterTest, TheCornersOfAnImageInNinePartsStayInsideTheImage)
  {
    Triangles2D drawn;
    EXPECT_CALL(_renderer, DrawTriangles(_)).WillOnce(::testing::SaveArg<0>(&drawn));

    // further than the image is wide
    _painter.DrawNineSlice(_heart, {100, 100, 300, 200}, {50, 50, 50, 50}, {20, 20, 20, 20}, white);
    _painter.End();

    for (const auto &vertex : drawn.vertices)
    {
      EXPECT_GE(vertex.u, 0);
      EXPECT_LE(vertex.u, 1);
      EXPECT_GE(vertex.v, 0);
      EXPECT_LE(vertex.v, 1);
    }
  }
}
