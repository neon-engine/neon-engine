#ifndef RENDER_2D_CONTEXT_HPP
#define RENDER_2D_CONTEXT_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <neon/common/color.hpp>

#include "render-context.hpp"

namespace neon
{
  /// Stands for no texture.
  constexpr int No_Texture = -1;

  /// A corner of a triangle that is drawn in two dimensions.
  struct Vertex2D
  {
    /// In pixels of the frame, counted from its left top corner, to the
    /// right and down.
    float x = 0.0f;
    float y = 0.0f;

    /// Where in the texture the corner is, in parts of its size: 0 is the
    /// left and the top, 1 the right and the bottom.
    float u = 0.0f;
    float v = 0.0f;

    /// Multiplied with the texture. Its alpha says how much of what is
    /// behind shows through, and is not multiplied into its colours.
    Color color;

    /// How much of the texture is read: 1 draws the texture times the
    /// colour, 0 the colour alone. Plain shapes can then be drawn in one
    /// call with shapes that have a texture, such as a panel with its text.
    /// What hands over triangles with a texture and knows nothing of this
    /// leaves it at 1.
    float textured = 1.0f;
  };

  /// A part of the frame, in pixels from its left top corner.
  struct ClipRectangle
  {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool operator==(const ClipRectangle &other) const = default;
  };

  /// Triangles that are drawn with one draw call.
  struct Triangles2D
  {
    std::vector<Vertex2D> vertices;

    /// Three for each triangle, each the place of a corner in `vertices`.
    /// Triangles are drawn in this order, the last on top, and from both
    /// sides.
    std::vector<std::uint32_t> indices;

    /// The texture the corners refer to, or No_Texture. Without one, a
    /// corner reads white where it would read the texture.
    int texture = No_Texture;

    /// Added to the place of every corner, in pixels. Something that is
    /// moved is drawn again without being built again.
    float translate_x = 0.0f;
    float translate_y = 0.0f;

    /// Whether nothing is drawn outside `clip`.
    bool clipped = false;
    ClipRectangle clip;
  };

  /// What a renderer offers for drawing in two dimensions: triangles in
  /// pixels, on top of what was drawn before, blended by their alpha, and
  /// without regard to depth.
  ///
  /// It is all a user interface needs of a renderer, whichever user
  /// interface that is. The one of the engine builds its rectangles and
  /// its text from triangles, in neon-core. A library that brings a
  /// language of its own, such as RmlUi, hands the application triangles
  /// and textures to draw, which is what this takes. A renderer that
  /// implements these few functions draws every user interface.
  ///
  /// It stands next to RenderContext, which draws the models of a scene.
  /// Nothing in it names a graphics API.
  class Render2DContext
  {
  protected:
    ~Render2DContext() = default;

  public:
    /// Makes a texture from pixels in memory, such as the characters of a
    /// font. `pixels` holds red, green, blue, and alpha for each pixel, a
    /// byte each, row after row from the top. Alpha is not multiplied into
    /// the colours. Returns what the texture is known as, or No_Texture.
    virtual int CreateTexture(int width, int height, const std::vector<unsigned char> &pixels) = 0;

    /// Makes a texture from an image file at a virtual path, read through
    /// the file system. Returns what the texture is known as, or
    /// No_Texture.
    virtual int LoadTexture(const std::string &path) = 0;

    /// The size of a texture in pixels. Returns false for one that does not
    /// exist.
    virtual bool GetTextureSize(int texture, int &width, int &height) = 0;

    /// Releases a texture. What it was known as may be given out again.
    virtual void DestroyTexture(int texture) = 0;

    /// Draws triangles into the frame that is being drawn, on top of what
    /// is there. Call it between PrepareFrame() and FinishFrame() of the
    /// render system.
    virtual void DrawTriangles(const Triangles2D &triangles) = 0;

    /// The size of the frame in pixels.
    virtual const RenderResolution &GetRenderResolution() = 0;
  };
} // neon

#endif //RENDER_2D_CONTEXT_HPP
