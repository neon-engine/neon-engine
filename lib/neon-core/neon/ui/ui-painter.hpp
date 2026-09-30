#ifndef UI_PAINTER_HPP
#define UI_PAINTER_HPP

#include <cstddef>
#include <vector>

#include <neon/common/color.hpp>
#include <neon/layout/layout-style.hpp>
#include <neon/render/render-2d-context.hpp>
#include <neon/text/text-layout.hpp>

#include "ui-paint.hpp"
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

      /// How the texture is read, as Vertex2D says. Below 0 stands for
      /// what `textured` says.
      float mode = -1.0f;

      /// A colour for each corner, from the left top one around to the
      /// left bottom one, in place of `color`.
      bool has_corner_colors = false;
      Color corners[4];

      /// The shape the rectangle is a part of, or nullptr, and the box of
      /// the shape.
      const Shape2D *shape = nullptr;
      UiRectangle shape_box;
    };

    /// What a call is drawn with next to its texture. Rectangles share a
    /// call while it stays the same.
    struct State
    {
      TextureFilter2D filter = TextureFilter2D::Smooth;
      int material = No_Material;
      std::vector<MaterialValue2D> material_values;
      UiRectangle material_box;
    };

    Render2DContext *_renderer;
    Triangles2D _batch;
    std::vector<ClipRectangle> _clips;
    int _frame_width = 0;
    int _frame_height = 0;
    std::size_t _draw_calls = 0;
    std::size_t _quads = 0;

    std::vector<UiMatrix> _transforms;

    // For each clip with round corners the number of clips there were
    // when it was added, so that it ends with the clip it came with.
    std::vector<std::pair<std::size_t, RoundedClip2D>> _rounded_clips;

    State _state;
    float _time = 0.0f;

    void Add(const Quad &quad, int texture);

    void SetClip();

    /// Starts a new call when what it is drawn with has changed.
    void SetState();

    /// The place of a shape among those of the call, which it joins when
    /// it is not the one that was added last.
    [[nodiscard]] float PlaceOf(const Shape2D &shape);

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

    /// Seconds since the user interface was started, which shaders that
    /// move are told.
    void SetTime(float seconds);

    /// From now on everything is moved by the matrix, and by those that
    /// were pushed before it.
    void PushTransform(const UiMatrix &matrix);

    void PopTransform();

    /// What a point is moved by now.
    [[nodiscard]] const UiMatrix &GetTransform() const;

    /// As PushClip(), and nothing is drawn outside the round corners
    /// either. One such clip applies at a time, the one that was pushed
    /// last. Under a transform that turns, the corners are not cut off.
    void PushRoundedClip(const UiRectangle &rectangle, const float radii[4]);

    void PopRoundedClip();

    /// How textures are read from now on.
    void SetFilter(TextureFilter2D filter);

    /// From now on everything is drawn with the shader of a material,
    /// which is told the box of the element it is for. No_Material draws
    /// with the shader of the renderer again.
    void SetMaterial(int material, const std::vector<MaterialValue2D> &values, const UiRectangle &box);

    /// A shape over its box. `rectangle` is what is drawn, which is larger
    /// than the box for a shadow. With an image, the shape is filled with
    /// the part of the image.
    void FillShape(
      const UiRectangle &rectangle,
      const UiRectangle &box,
      const Shape2D &shape,
      const Color &color);

    void FillShape(
      const UiRectangle &rectangle,
      const UiRectangle &box,
      const Shape2D &shape,
      const UiImage &image,
      const UiRectangle &part,
      const Color &tint);

    /// A rectangle with a colour for each corner, from the left top one
    /// around to the left bottom one.
    void FillRectangle(const UiRectangle &rectangle, const Color corners[4]);

    /// A glyph from a page of an atlas. `mode` is 1 for a bitmap and 2 for
    /// distances. `shape` says what is done to a glyph of distances, or is
    /// nullptr.
    void DrawGlyph(
      int texture,
      const UiRectangle &place,
      const UiRectangle &part,
      const Color corners[4],
      float mode,
      const Shape2D *shape,
      const UiRectangle &shape_box);
  };

  /// The shape of a box with round corners, in pixels.
  [[nodiscard]] Shape2D BoxShape(const UiRectangle &box, const float radii[4], ShapeKind2D kind);

  /// Puts a gradient into a shape.
  void SetGradient(Shape2D &shape, const UiGradient &gradient, float opacity);

  /// The distance of a point to the outline of a box with round corners,
  /// below 0 inside. `x` and `y` are counted from the middle of the box.
  /// It is what the shader works out for every pixel, and what says
  /// whether a pointer is on an element.
  [[nodiscard]] float DistanceToRoundedBox(
    float x,
    float y,
    float half_width,
    float half_height,
    const float radii[4]);
} // neon

#endif //UI_PAINTER_HPP
