#ifndef MOCK_RENDER_2D_CONTEXT_HPP
#define MOCK_RENDER_2D_CONTEXT_HPP

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>

#include <neon/render/render-2d-context.hpp>

namespace neon::testing
{
  class MockRender2DContext : public Render2DContext
  {
  public:
    MOCK_METHOD(int, CreateTexture, (int width, int height, const std::vector<unsigned char> &pixels), (override));

    MOCK_METHOD(int, LoadTexture, (const std::string &path), (override));

    MOCK_METHOD(bool, GetTextureSize, (int texture, int &width, int &height), (override));

    MOCK_METHOD(void, DestroyTexture, (int texture), (override));

    MOCK_METHOD(void, DrawTriangles, (const Triangles2D &triangles), (override));

    MOCK_METHOD(const RenderResolution &, GetRenderResolution, (), (override));
  };

  /// A rectangle as a test sees it, put together again from the two
  /// triangles it was drawn as.
  struct RecordedQuad
  {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    float texture_left = 0.0f;
    float texture_top = 0.0f;
    float texture_right = 0.0f;
    float texture_bottom = 0.0f;

    Color color;
    bool textured = false;

    /// The texture of the call it was drawn with, and what that was cut
    /// off at.
    int texture = No_Texture;
    bool clipped = false;
    ClipRectangle clip;

    [[nodiscard]] float Width() const
    {
      return right - left;
    }

    [[nodiscard]] float Height() const
    {
      return bottom - top;
    }
  };

  /// A renderer that keeps what it is asked to draw, so that a test can
  /// look at it. Every texture it is asked for exists, unless the test says
  /// otherwise.
  class RecordingRenderer2D final : public Render2DContext
  {
    struct Texture
    {
      int width = 0;
      int height = 0;
      std::string path;

      // how it was asked to be kept, and the target it shows
      TextureOptions2D options{};
      int target = No_Render_Target;
    };

    std::map<int, Texture> _textures;
    int _next_texture = 0;
    RenderResolution _resolution{1920, 1080};

  public:
    /// The calls of the frame, in the order they were made in. A test
    /// clears them where a frame starts.
    std::vector<Triangles2D> batches;

    /// Paths that cannot be loaded.
    std::vector<std::string> missing;

    /// The size of an image that is loaded from a file.
    int image_width = 64;
    int image_height = 64;

    /// How often a texture was asked for, which includes what failed.
    std::size_t created = 0;
    std::size_t loaded = 0;
    std::size_t destroyed = 0;

    /// Makes CreateTexture() fail.
    bool refuses_pixels = false;

    void SetResolution(const int width, const int height)
    {
      // the members of a resolution cannot be written to
      std::destroy_at(&_resolution);
      std::construct_at(&_resolution, width, height);
    }

    /// Number of textures that exist.
    [[nodiscard]] std::size_t TextureCount() const
    {
      return _textures.size();
    }

    /// The texture that was loaded from a path, or No_Texture.
    [[nodiscard]] int TextureOf(const std::string &path) const
    {
      for (const auto &[id, texture] : _textures)
      {
        if (texture.path == path) { return id; }
      }
      return No_Texture;
    }

    /// Every rectangle of the frame, in the order they were drawn in. It
    /// expects what the user interface of the engine draws: four corners
    /// for each rectangle, from the left top one around to the left bottom
    /// one. Where a call was moved, the rectangle is where it ended up.
    [[nodiscard]] std::vector<RecordedQuad> Quads() const
    {
      std::vector<RecordedQuad> quads;

      for (const auto &batch : batches)
      {
        for (std::size_t first = 0; first + 3 < batch.vertices.size(); first += 4)
        {
          const Vertex2D &left_top = batch.vertices[first];
          const Vertex2D &right_bottom = batch.vertices[first + 2];

          RecordedQuad quad;
          quad.left = left_top.x + batch.translate_x;
          quad.top = left_top.y + batch.translate_y;
          quad.right = right_bottom.x + batch.translate_x;
          quad.bottom = right_bottom.y + batch.translate_y;
          quad.texture_left = left_top.u;
          quad.texture_top = left_top.v;
          quad.texture_right = right_bottom.u;
          quad.texture_bottom = right_bottom.v;
          quad.color = left_top.color;
          quad.textured = left_top.textured > 0.5f;
          quad.texture = batch.texture;
          quad.clipped = batch.clipped;
          quad.clip = batch.clip;

          quads.push_back(quad);
        }
      }

      return quads;
    }

    int CreateTexture(const int width, const int height, const std::vector<unsigned char> &pixels) override
    {
      created++;

      if (refuses_pixels || width <= 0 || height <= 0 ||
          pixels.size() != static_cast<std::size_t>(width) * height * 4)
      {
        return No_Texture;
      }

      _textures[_next_texture] = {width, height, ""};
      return _next_texture++;
    }

    int LoadTexture(const std::string &path) override
    {
      loaded++;

      if (std::ranges::find(missing, path) != missing.end()) { return No_Texture; }

      _textures[_next_texture] = {image_width, image_height, path};
      return _next_texture++;
    }

    bool GetTextureSize(const int texture, int &width, int &height) override
    {
      const auto found = _textures.find(texture);
      if (found == _textures.end()) { return false; }

      width = found->second.width;
      height = found->second.height;
      return true;
    }

    void DestroyTexture(const int texture) override
    {
      destroyed += _textures.erase(texture);
    }

    void DrawTriangles(const Triangles2D &triangles) override
    {
      // what is drawn into a target is kept with the target, and the
      // frame holds what is drawn into the frame
      if (const auto target = targets.find(current_target); target != targets.end())
      {
        target->second.batches.push_back(triangles);
        return;
      }

      batches.push_back(triangles);
    }

    const RenderResolution &GetRenderResolution() override
    {
      return _resolution;
    }

    /// How often a part of a texture was replaced.
    std::size_t updated = 0;

    /// Makes UpdateTexture() fail, as of a renderer that cannot do it.
    bool refuses_updates = false;

    bool UpdateTexture(
      const int texture,
      const int x,
      const int y,
      const int width,
      const int height,
      const std::vector<unsigned char> &pixels) override
    {
      const auto found = _textures.find(texture);
      if (refuses_updates || found == _textures.end()) { return false; }

      if (x < 0 || y < 0 || width <= 0 || height <= 0 ||
          x + width > found->second.width || y + height > found->second.height ||
          pixels.size() != static_cast<std::size_t>(width) * height * 4)
      {
        return false;
      }

      updated++;
      return true;
    }

    /// A render target as a test sees it.
    struct Target
    {
      std::string name;
      int width = 0;
      int height = 0;
      int texture = No_Texture;

      /// How often it was drawn to, what it was cleared to the last time,
      /// and what was drawn into it the last time.
      std::size_t begun = 0;
      Color clear;
      std::vector<Triangles2D> batches;
    };

    /// Shaders that cannot be used, and names of targets that cannot be
    /// made.
    std::vector<std::string> broken_shaders;
    std::vector<std::string> refused_targets;

    /// Makes BeginRenderTarget() fail, as of a frame that is not drawn.
    bool refuses_drawing_into_targets = false;

    /// The shaders that were asked for, in the order they were asked for.
    /// A material is known by its place in here.
    std::vector<std::string> materials;
    std::size_t materials_destroyed = 0;

    std::map<int, Target> targets;
    std::size_t targets_created = 0;
    std::size_t targets_destroyed = 0;
    int current_target = No_Render_Target;

    /// How the texture was asked to be kept.
    [[nodiscard]] TextureOptions2D OptionsOf(const int texture) const
    {
      const auto found = _textures.find(texture);
      return found != _textures.end() ? found->second.options : TextureOptions2D{};
    }

    /// The target of a name, or nullptr.
    [[nodiscard]] const Target *TargetOf(const std::string &name) const
    {
      for (const auto &[id, target] : targets)
      {
        if (target.name == name) { return &target; }
      }
      return nullptr;
    }

    /// Every rectangle of a call, as Quads() hands them over for the
    /// frame.
    [[nodiscard]] static std::vector<RecordedQuad> QuadsOf(const std::vector<Triangles2D> &calls)
    {
      RecordingRenderer2D renderer;
      renderer.batches = calls;
      return renderer.Quads();
    }

    int CreateTextureWith(
      const int width,
      const int height,
      const std::vector<unsigned char> &pixels,
      const TextureOptions2D &options) override
    {
      const int texture = CreateTexture(width, height, pixels);
      if (texture != No_Texture) { _textures[texture].options = options; }
      return texture;
    }

    int CreateMaterial(const std::string &shader_path) override
    {
      materials.push_back(shader_path);

      if (std::ranges::find(broken_shaders, shader_path) != broken_shaders.end()) { return No_Material; }
      return static_cast<int>(materials.size()) - 1;
    }

    void DestroyMaterial(const int material) override
    {
      if (material >= 0 && static_cast<std::size_t>(material) < materials.size()) { materials_destroyed++; }
    }

    int CreateRenderTarget(const std::string &name, const int width, const int height) override
    {
      targets_created++;

      if (name.empty() || width <= 0 || height <= 0 || TargetOf(name) != nullptr ||
          std::ranges::find(refused_targets, name) != refused_targets.end())
      {
        return No_Render_Target;
      }

      const int id = static_cast<int>(targets_created);

      Target target;
      target.name = name;
      target.width = width;
      target.height = height;
      target.texture = _next_texture++;
      _textures[target.texture] = {width, height, "", {}, id};

      targets[id] = target;
      return id;
    }

    void DestroyRenderTarget(const int target) override
    {
      const auto found = targets.find(target);
      if (found == targets.end()) { return; }

      _textures.erase(found->second.texture);
      targets.erase(found);
      targets_destroyed++;

      if (current_target == target) { current_target = No_Render_Target; }
    }

    bool BeginRenderTarget(const int target, const Color &clear) override
    {
      const auto found = targets.find(target);
      if (refuses_drawing_into_targets || found == targets.end() || current_target != No_Render_Target)
      {
        return false;
      }

      found->second.begun++;
      found->second.clear = clear;
      found->second.batches.clear();
      current_target = target;
      return true;
    }

    void EndRenderTarget() override
    {
      current_target = No_Render_Target;
    }

    int GetRenderTargetTexture(const int target) override
    {
      const auto found = targets.find(target);
      return found != targets.end() ? found->second.texture : No_Texture;
    }

    bool GetRenderTargetSize(const int target, int &width, int &height) override
    {
      const auto found = targets.find(target);
      if (found == targets.end()) { return false; }

      width = found->second.width;
      height = found->second.height;
      return true;
    }

    int FindRenderTarget(const std::string &name) override
    {
      for (const auto &[id, target] : targets)
      {
        if (target.name == name) { return id; }
      }
      return No_Render_Target;
    }

    /// The images a test made known under their names, as something with
    /// SetImage would.
    std::map<std::string, std::shared_ptr<const ImagePixels>> images;

    std::shared_ptr<const ImagePixels> FindImage(const std::string &name) override
    {
      const auto found = images.find(name);
      return found != images.end() ? found->second : nullptr;
    }
  };
} // neon::testing

#endif //MOCK_RENDER_2D_CONTEXT_HPP
