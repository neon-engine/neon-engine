#ifndef UI_DRAW_CACHE_HPP
#define UI_DRAW_CACHE_HPP

#include <cstddef>
#include <vector>

#include <neon/render/render-2d-context.hpp>

namespace neon
{
  /// Stands between what draws a user interface and the renderer, and
  /// keeps what was drawn. A frame in which nothing changed is drawn from
  /// what was kept, without anything being built again.
  ///
  /// A renderer has to be handed its triangles in every frame, since a
  /// frame starts empty. What is saved is the work of making them: going
  /// through the elements, placing the characters of every text, and
  /// putting the rectangles together.
  class UiDrawCache final : public Render2DContext
  {
    Render2DContext *_renderer;
    std::vector<Triangles2D> _kept;
    bool _is_keeping = false;

  public:
    explicit UiDrawCache(Render2DContext *renderer)
    {
      _renderer = renderer;
    }

    /// Forgets what was kept, and keeps what is drawn from now on.
    void Begin()
    {
      _kept.clear();
      _is_keeping = true;
    }

    void End()
    {
      _is_keeping = false;
    }

    void Clear()
    {
      _kept.clear();
    }

    /// Draws what was kept. Returns the number of draw calls.
    std::size_t Replay() const
    {
      for (const auto &triangles : _kept) { _renderer->DrawTriangles(triangles); }
      return _kept.size();
    }

    [[nodiscard]] std::size_t GetDrawCalls() const
    {
      return _kept.size();
    }

    int CreateTexture(const int width, const int height, const std::vector<unsigned char> &pixels) override
    {
      return _renderer->CreateTexture(width, height, pixels);
    }

    int LoadTexture(const std::string &path) override
    {
      return _renderer->LoadTexture(path);
    }

    bool GetTextureSize(const int texture, int &width, int &height) override
    {
      return _renderer->GetTextureSize(texture, width, height);
    }

    void DestroyTexture(const int texture) override
    {
      _renderer->DestroyTexture(texture);
    }

    void DrawTriangles(const Triangles2D &triangles) override
    {
      if (_is_keeping) { _kept.push_back(triangles); }
      _renderer->DrawTriangles(triangles);
    }

    const RenderResolution &GetRenderResolution() override
    {
      return _renderer->GetRenderResolution();
    }
  };
} // neon

#endif //UI_DRAW_CACHE_HPP
