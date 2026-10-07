#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

#include <string>
#include <vector>
#include <neon/common/color.hpp>
#include <neon/common/data-buffer.hpp>
#include <neon/common/transform.hpp>
#include <neon/image/image-pixels.hpp>
#include <neon/logging/logger.hpp>

#include "light-source.hpp"
#include "render-info.hpp"
#include "render-object-ref.hpp"
#include "render-target-options.hpp"
#include "sky-info.hpp"
#include "texture-source.hpp"


namespace neon
{
  struct RenderResolution
  {
    RenderResolution(const int width, const int height)
      : width(width),
        height(height) {}

    const int width;
    const int height;
  };

  class RenderContext
  {
  protected:
    DataBuffer<RenderObjectRef> _render_object_buffer;
    std::shared_ptr<Logger> _logger;

    ~RenderContext() = default;

  public:
    explicit RenderContext(const int max_render_objects, const std::shared_ptr<Logger> &logger):
      _render_object_buffer(max_render_objects)
    {
      _logger = logger;
    }

    virtual int CreateRenderObject(const RenderInfo &render_info) = 0;

    virtual void DrawRenderObject(
      int render_object_id,
      const Transform &transform,
      const glm::mat4 &view,
      const glm::mat4 &projection,
      const std::vector<LightSource> &lights) = 0;

    virtual void DestroyRenderObject(int render_object_id) = 0;

    /// Draws a render object with what `render_info` says from now on: its
    /// textures, its shader, its material, and its model when it is drawn
    /// from a file. It keeps its id. A mesh that was built stays the one
    /// the object has, see UpdateRenderObjectMesh(). What cannot be made,
    /// a shader that is not there, is said, and the object is drawn as it
    /// was. A renderer that leaves this as it is draws it as it was first
    /// created.
    virtual void UpdateRenderObject(int render_object_id, const RenderInfo &render_info) {}

    /// Frees what nothing draws with any more. A material and its textures
    /// stay loaded once nothing shows them, since what showed them may show
    /// them again: a face that blinks, a level that is walked back through.
    /// They are freed when this is asked for, which the world does once a
    /// scene has taken the place of another, and a game that changes its
    /// levels itself does when it does; and when there is no room for
    /// another. A renderer that keeps nothing leaves this as it is.
    virtual void FreeUnused() {}

    /// Draws a render object that was created from a mesh with `mesh` from
    /// now on, since what built the mesh changed it. A render object from
    /// a model file is left as it is. A renderer that cannot change a mesh
    /// keeps drawing the first.
    virtual void UpdateRenderObjectMesh(int render_object_id, const MeshData &mesh) {}

    /// Makes pixels in memory known under a name, which a material then
    /// reads as the texture `image://<name>`, see TextureSource: red, green, blue, and alpha
    /// for each pixel, row after row from the top. It is how what reads a
    /// format of its own, an importer or an extension, hands over a texture
    /// that is in no file. The pixels are copied. A name that is set again
    /// with pixels of the same size shows them from the next frame on,
    /// wherever the image is drawn, without a material being made anew: it
    /// is how a picture that is worked out while the game runs is shown.
    /// Another size is refused for what was made already. Returns false when the pixels are not
    /// width by height of four bytes, or for a renderer that keeps no
    /// images.
    virtual bool SetImage(const std::string &name, const ImagePixels &pixels)
    {
      return false;
    }

    virtual const RenderResolution& GetRenderResolution() = 0;

    // What follows can be left as it is by a renderer, which then draws
    // into the frame alone. A camera that asks for a texture is told so.

    /// Makes an image that is drawn to in place of the frame, and that a
    /// model shows when it names `surface://` and the name as one of its
    /// textures. Returns what the target is known as, or -1.
    virtual int CreateRenderTarget(const std::string &name, const int width, const int height)
    {
      return -1;
    }

    /// As CreateRenderTarget(), with the levels of smaller copies it has
    /// and whether it is made at the scale the game sets, see
    /// RenderTargetOptions. A renderer that leaves this as it is makes the
    /// target as asked.
    virtual int CreateRenderTarget(
      const std::string &name,
      const int width,
      const int height,
      const RenderTargetOptions &options)
    {
      return CreateRenderTarget(name, width, height);
    }

    virtual void DestroyRenderTarget(const int target) {}

    /// The target of a name, or -1.
    virtual int FindRenderTarget(const std::string &name)
    {
      return -1;
    }

    /// The size of a target in pixels.
    virtual bool GetRenderTargetSize(const int target, int &width, int &height)
    {
      return false;
    }

    /// From now on DrawRenderObject() draws into the target, which is
    /// cleared to `clear` first. A target is drawn to once in a frame.
    /// What shows the target itself is left out of what is drawn into it.
    virtual bool BeginRenderTarget(const int target, const Color &clear)
    {
      return false;
    }

    /// From now on DrawRenderObject() draws into the frame again.
    virtual void EndRenderTarget() {}

    /// Draws the sky of the scene as a camera sees it, behind every model
    /// drawn with that camera: into the render target that is drawn to,
    /// or the frame. Called once for every camera of a frame, at any time
    /// among its models. A renderer that leaves it as it is shows what the
    /// frame is cleared to.
    virtual void DrawSky(const SkyInfo &sky, const glm::mat4 &view, const glm::mat4 &projection) {}

    /// Says which effects the camera names that draws what is drawn next:
    /// into the render target that was begun, or else into the frame. An
    /// effect is a fragment shader a game brings that is run over the whole
    /// picture the camera drew, named as a shader is. Those of `effects`
    /// are run on the light of the scene, before the tonemapper, and those
    /// of `screen_effects` on the colours a screen is given, after it and
    /// before the user interface. Each list is run in its order. They hold
    /// for the frame. A renderer that leaves this as it is runs none.
    virtual void SetEffects(
      const std::vector<std::string> &effects,
      const std::vector<std::string> &screen_effects) {}

    /// Has a frame wait for the screen before it is shown, or not, while
    /// the application runs. With it no frame is torn and no more are drawn
    /// than the screen shows; without it frames are shown as soon as they
    /// are done, where the driver can. Returns false for a renderer that
    /// shows nothing on a screen.
    virtual bool SetVerticalSync(bool enabled) { return false; }

    /// Whether a frame waits for the screen.
    [[nodiscard]] virtual bool GetVerticalSync() { return false; }

    /// How many samples a texture is read with where its surface is seen
    /// from the side, while the application runs: one of
    /// Anisotropy::kLevels, 1 for none. Held to what the graphics card
    /// allows, which is logged. It holds from the next frame on. Returns
    /// false for a level that is not one of them, or a renderer that
    /// cannot change it.
    virtual bool SetAnisotropy(int level) { return false; }

    /// What a texture is read with from the side, as the graphics card
    /// allows it: 1 for none.
    [[nodiscard]] virtual int GetAnisotropy() { return 1; }

    /// The size textures read from files are kept at, while the application
    /// runs: one of TextureScale::kScales. Textures read from then on are
    /// read at it; those that are held stay as they are, so a scene that
    /// is loaded next has its textures at the new size. Returns false for a
    /// scale that is none of them, or a renderer that cannot change it.
    virtual bool SetTextureScale(double scale) { return false; }

    /// The size textures read from files are kept at.
    [[nodiscard]] virtual double GetTextureScale() { return 1.0; }

    /// The quality of render targets, while the application runs: the
    /// scale a camera's target is made at, one of TargetQuality::kScales,
    /// and the most levels of smaller copies a target has, 0 for as many as
    /// its size allows. Both hold for the targets made from then on, so a
    /// scene that is loaded next has its targets at them. Return false for
    /// a value that is none of them, or a renderer that cannot change it.
    virtual bool SetTargetScale(double scale) { return false; }

    [[nodiscard]] virtual double GetTargetScale() { return 1.0; }

    virtual bool SetTargetMipmaps(int mipmaps) { return false; }

    [[nodiscard]] virtual int GetTargetMipmaps() { return 0; }

    /// How many places there are for numbers of a game that every shader
    /// reads, see SetShaderNumbers().
    static constexpr int kShader_Number_Places = 8;

    /// Tells the shaders the time: the seconds the world has run, and how
    /// long its last frame took. Every shader reads them as `scene.time`.
    /// The time stands still while the world does.
    virtual void SetShaderTime(double seconds, double delta) {}

    /// Sets four numbers of a game that every shader reads, at one of
    /// kShader_Number_Places places, as `scene.numbers[place]`. They are for
    /// the shaders a game brings: how thick its fog is, the colour of it,
    /// whatever its own shaders are written to read. The shaders of the
    /// engine read none of them. They stay until they are set again. Returns
    /// false for a place that there is not, and for a renderer that keeps
    /// none.
    virtual bool SetShaderNumbers(int place, const glm::vec4 &numbers) { return false; }
  };
} // neon

#endif //RENDER_CONTEXT_HPP
