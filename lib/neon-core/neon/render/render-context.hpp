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

    /// Draws a render object that was created from a mesh with `mesh` from
    /// now on, since what built the mesh changed it. A render object from
    /// a model file is left as it is. A renderer that cannot change a mesh
    /// keeps drawing the first.
    virtual void UpdateRenderObjectMesh(int render_object_id, const MeshData &mesh) {}

    /// Makes pixels in memory known under a name, which a material then
    /// reads as the texture `image://<name>`, see TextureSource: red, green, blue, and alpha
    /// for each pixel, row after row from the top. It is how what reads a
    /// format of its own, an importer or an extension, hands over a texture
    /// that is in no file. The pixels are copied. A name is set once, before
    /// the first material that reads it is made; setting it again changes
    /// nothing that was made already. Returns false when the pixels are not
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
  };
} // neon

#endif //RENDER_CONTEXT_HPP
