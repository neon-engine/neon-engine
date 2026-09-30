#ifndef UI_PAINTER_HPP
#define UI_PAINTER_HPP

#include <cstddef>
#include <vector>

#include <neon/common/color.hpp>
#include <neon/layout/layout-style.hpp>
#include <neon/render/render-2d-context.hpp>
#include <neon/text/text-layout.hpp>

#include "ui-resources.hpp"

namespace neon
{
  /// A rectangle in pixels of the frame.
  struct UiRectangle
  {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    [[nodiscard]] float Width() const
    {
      return right - left;
    }

    [[nodiscard]] float Height() const
    {
      return bottom - top;
    }

    [[nodiscard]] bool IsEmpty() const
    {
      return right <= left || bottom <= top;
    }

    [[nodiscard]] bool Contains(const float x, const float y) const
    {
      return x >= left && x < right && y >= top && y < bottom;
    }
  };

  /// Draws what a user interface is made of, with as few draw calls as the
  /// order of drawing allows.
  ///
  /// A renderer draws triangles. Every rectangle is built from two of them
  /// here, and collected into a batch. The batch is handed to the renderer
  /// when something cannot join it: a rectangle with another texture, or
  /// another place to be cut off at. A rectangle without a texture joins
  /// any batch, so a panel, its border, and its text are one draw call.
  class UiPainter
  {
    /// A rectangle before it is turned into triangles.
    struct Quad
    {
      UiRectangle place;
      UiRectangle part{0.0f, 0.0f, 1.0f, 1.0f};
      Color color;
      bool textured = false;
    };

    Render2DContext *_renderer;
    Triangles2D _batch;
    std::vector<ClipRectangle> _clips;
    int _frame_width = 0;
    int _frame_height = 0;
    std::size_t _draw_calls = 0;
    std::size_t _quads = 0;

    void Add(const Quad &quad, int texture);

    void SetClip();

  public:
    explicit UiPainter(Render2DContext *renderer);

    /// Starts a frame of the given size in pixels.
    void Begin(int frame_width, int frame_height);

    /// Hands what is left to the renderer.
    void End();

    /// From now on nothing is drawn outside the rectangle, nor outside
    /// what it was cut off at before.
    void PushClip(const UiRectangle &rectangle);

    void PopClip();

    void FillRectangle(const UiRectangle &rectangle, const Color &color);

    /// The four sides of a border that lies inside the rectangle.
    void FillBorder(const UiRectangle &rectangle, const LayoutEdges<float> &widths, const Color &color);

    /// The part of the image is in parts of its size, from 0 to 1.
    void DrawImage(
      const UiImage &image,
      const UiRectangle &rectangle,
      const UiRectangle &part,
      const Color &tint);

    /// Draws an image in nine parts. `slice` is how far the corners reach
    /// into the image, in pixels of the image, and `widths` how wide they
    /// are drawn. Corners that do not fit next to each other are made
    /// smaller, as CSS does.
    void DrawNineSlice(
      const UiImage &image,
      const UiRectangle &rectangle,
      const LayoutEdges<float> &slice,
      const LayoutEdges<float> &widths,
      const Color &tint);

    /// `left` and `top` are the corner of the text, in whole pixels.
    void DrawText(const UiFont &font, const PlacedText &text, float left, float top, const Color &color);

    /// Draw calls and rectangles of the frame so far.
    [[nodiscard]] std::size_t GetDrawCalls() const;

    [[nodiscard]] std::size_t GetQuads() const;
  };
} // neon

#endif //UI_PAINTER_HPP
