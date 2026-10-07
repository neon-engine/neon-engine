#ifndef RENDER_TARGET_OPTIONS_HPP
#define RENDER_TARGET_OPTIONS_HPP

namespace neon
{
  /// What a render target is made with beyond its name and size, see
  /// RenderContext::CreateRenderTarget().
  struct RenderTargetOptions
  {
    /// The most levels of smaller copies the target has, which are made
    /// again whenever it is drawn into: 1 for none, 0 for as many as its
    /// size allows. The setting rendering.target_mipmaps lowers it, never
    /// raises it.
    int mipmaps = 0;

    /// Whether the target is made at the scale of rendering.target_scale
    /// in place of the size asked for, as what a camera draws into is. An
    /// image of a user interface keeps its size, so that its text stays
    /// sharp.
    bool scales = false;
  };
} // neon

#endif //RENDER_TARGET_OPTIONS_HPP
