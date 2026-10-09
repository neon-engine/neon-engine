#ifndef RENDER_2D_CONTEXT_HPP
#define RENDER_2D_CONTEXT_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <neon/common/color.hpp>
#include <neon/image/image-pixels.hpp>

#include "render-context.hpp"

namespace neon
{
  /// Stands for no texture.
  constexpr int No_Texture = -1;

  /// Stands for the shader of the renderer itself.
  constexpr int No_Material = -1;

  /// Stands for the frame, which is what is drawn to when nothing else is
  /// said.
  constexpr int No_Render_Target = -1;

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
    /// behind shows through, and is not multiplied into its colors.
    Color color;

    /// How much of the texture is read: 1 draws the texture times the
    /// color, 0 the color alone. Plain shapes can then be drawn in one
    /// call with shapes that have a texture, such as a panel with its text.
    /// What hands over triangles with a texture and knows nothing of this
    /// leaves it at 1.
    ///
    /// 2 reads the alpha of the texture as the distance to an outline, as
    /// of a glyph that is kept as distances: 0.5 on the outline, more
    /// inside.
    float textured = 1.0f;

    /// Which of the shapes of the call the corner belongs to, or below 0
    /// for none. The four corners of a rectangle name the same shape.
    float shape = -1.0f;

    /// Where the corner is in its shape, in pixels from the middle of the
    /// box of the shape, to the right and down.
    float local_x = 0.0f;
    float local_y = 0.0f;
  };

  /// What a shape is, which says how the other numbers are read.
  enum class ShapeKind2D
  {
    /// A box with round corners, filled with the color and the texture of
    /// its corners, or with a gradient.
    Fill = 0,

    /// The border of such a box, each side in a color of its own.
    Border,

    /// The shadow such a box casts around itself.
    Shadow,

    /// The shadow that falls into such a box.
    InsetShadow,

    /// Glyphs that are kept as distances, with a line around them or out
    /// of focus.
    Text
  };

  /// A shape that is worked out for every pixel from the distance to its
  /// outline, so that it is sharp at every size and needs no texture. The
  /// edge is smoothed over one pixel.
  ///
  /// Every part is four numbers, which is how a shader reads them.
  struct Shape2D
  {
    /// Half the width and half the height of the box in pixels, the kind,
    /// and nothing.
    float box[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    /// The radius of the corners in pixels: left top, right top, right
    /// bottom, left bottom.
    float radii[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    /// Of a border, its widths: top, right, bottom, left. Of a shadow,
    /// how far it is moved to the right and down, the deviation of its
    /// blur, and how much larger than the box it is. Of text, the width
    /// of the line around a glyph, how far it is out of focus, and the
    /// pixels of the texture its distances reach over.
    float widths[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    /// 0 for none, 1 for a linear and 2 for a radial gradient; the angle
    /// in radians, clockwise from the top; the number of colors; nothing.
    float gradient[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    /// Of a border, the colors of its sides: top, right, bottom, left. Of
    /// text, the color of the line around a glyph. Alpha is not
    /// multiplied into the colors.
    float colors[4][4] = {};

    /// Where the colors of the gradient lie, from 0 to 1, and the
    /// colors.
    float stop_positions[8] = {};
    float stop_colors[8][4] = {};
  };

  /// A value a shader of an element is given, by the name the shader
  /// declares it under.
  struct MaterialValue2D
  {
    std::string name;

    /// One number, or as many as `count` says: four for a color.
    float numbers[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    int count = 1;
  };

  /// A box with round corners that nothing is drawn outside of.
  struct RoundedClip2D
  {
    /// The middle of the box in pixels of what is drawn to, and half its
    /// size.
    float center_x = 0.0f;
    float center_y = 0.0f;
    float half_width = 0.0f;
    float half_height = 0.0f;

    /// Left top, right top, right bottom, left bottom.
    float radii[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    bool operator==(const RoundedClip2D &other) const = default;
  };

  /// How a texture is read where it is drawn at another size than it has.
  enum class TextureFilter2D
  {
    /// Blended from the pixels around, and from its smaller copies when it
    /// has them.
    Smooth = 0,

    /// The nearest pixel, which keeps art that is drawn pixel by pixel as
    /// it is.
    Pixelated
  };

  /// How a texture is kept.
  struct TextureOptions2D
  {
    /// Smaller copies, each half the size of the one before, for an image
    /// that is drawn smaller than it is. They are made with alpha
    /// multiplied into the colors, so that a pixel that is see-through
    /// does not darken its neighbors.
    bool has_smaller_copies = false;

    /// Whether the image starts again past its edge.
    bool repeats = false;
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

    /// The shapes the corners refer to.
    std::vector<Shape2D> shapes;

    /// Whether nothing is drawn outside `rounded_clip` either.
    bool has_rounded_clip = false;
    RoundedClip2D rounded_clip;

    TextureFilter2D filter = TextureFilter2D::Smooth;

    /// The shader the triangles are drawn with, as CreateMaterial() made
    /// it, or No_Material, and what it is given.
    int material = No_Material;
    std::vector<MaterialValue2D> material_values;

    /// The box the shader of a material is told about: its left top
    /// corner and its size, in pixels.
    float material_box[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    /// Seconds since the user interface was started, for shaders that
    /// move.
    float time = 0.0f;
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
    /// the colors. Returns what the texture is known as, or No_Texture.
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

    // What follows can be left as it is by a renderer. What asks for it
    // is told that it is not there, and does without.

    /// Replaces a part of a texture that CreateTexture() made, such as the
    /// glyphs a font has drawn since. `pixels` holds the part alone, row
    /// after row. What was drawn with the texture in this frame shows what
    /// it holds when the frame is finished. Returns false when the texture
    /// cannot be written to.
    virtual bool UpdateTexture(
      const int texture,
      const int x,
      const int y,
      const int width,
      const int height,
      const std::vector<unsigned char> &pixels)
    {
      return false;
    }

    /// As CreateTexture(), with a say in how the texture is kept.
    virtual int CreateTextureWith(
      const int width,
      const int height,
      const std::vector<unsigned char> &pixels,
      const TextureOptions2D &options)
    {
      return CreateTexture(width, height, pixels);
    }

    /// Makes a shader ready that elements are drawn with. `shader_path` is
    /// a virtual path without an extension, such as
    /// `engine://shaders/ui/shine`. Returns what the material is known as,
    /// or No_Material when the shader cannot be used, which is said once.
    virtual int CreateMaterial(const std::string &shader_path)
    {
      return No_Material;
    }

    virtual void DestroyMaterial(const int material) {}

    /// Makes an image that is drawn to in place of the frame, and that is
    /// then drawn with as a texture: in two dimensions with what
    /// GetRenderTargetTexture() returns, and on a model that names
    /// `surface://` and the name as one of its textures.
    ///
    /// Returns what the target is known as, or No_Render_Target when it
    /// cannot be made or the name is taken.
    virtual int CreateRenderTarget(const std::string &name, const int width, const int height)
    {
      return No_Render_Target;
    }

    /// Releases a target. Models that show it show plain white from then
    /// on.
    virtual void DestroyRenderTarget(const int target) {}

    /// From now on DrawTriangles() draws into the target, which is cleared
    /// to `clear` first. Returns false when there is no such target, or
    /// when one is being drawn to already. A target is drawn to once in a
    /// frame, and is finished before the frame that shows it.
    virtual bool BeginRenderTarget(const int target, const Color &clear)
    {
      return false;
    }

    /// From now on DrawTriangles() draws into the frame again.
    virtual void EndRenderTarget() {}

    /// The texture a target is drawn with in two dimensions, or
    /// No_Texture.
    virtual int GetRenderTargetTexture(const int target)
    {
      return No_Texture;
    }

    /// The size of a target in pixels.
    virtual bool GetRenderTargetSize(const int target, int &width, int &height)
    {
      return false;
    }

    /// The target of a name, or No_Render_Target.
    virtual int FindRenderTarget(const std::string &name)
    {
      return No_Render_Target;
    }

    /// The pixels that were made known under a name with
    /// RenderContext::SetImage(), which a user interface shows as the image
    /// `image://<name>`, see TextureSource. Nothing for a name that was not
    /// set, and for a renderer that keeps no images.
    virtual std::shared_ptr<const ImagePixels> FindImage(const std::string &name)
    {
      return nullptr;
    }
  };
} // neon

#endif //RENDER_2D_CONTEXT_HPP
