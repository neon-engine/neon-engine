#ifndef VK_SAMPLING_HPP
#define VK_SAMPLING_HPP

namespace neon
{
  /// How a texture is read. The render system makes one sampler for each
  /// way, shared by every texture read that way, and a texture says which
  /// one it is read through.
  ///
  /// The shaders declare a `texture2D` and a `sampler` apart, and put them
  /// together where they read, so that every target of the shaders binds
  /// what the source says. See shaders.md.
  // ReSharper disable once CppInconsistentNaming
  enum class VK_Sampling
  {
    /// Smooth, with its smaller copies, and seen from the side without
    /// blurring, starting again past its edge: the texture of a model.
    AnisotropicRepeat,

    /// As AnisotropicRepeat, but its edge is drawn on past it: what a
    /// render target was drawn to, and an image of a user interface that
    /// is read from a file.
    AnisotropicClamp,

    /// Smooth, with its smaller copies, starting again past its edge: an
    /// image of a user interface that was made with its smaller copies,
    /// and repeats.
    LinearRepeat,

    /// As LinearRepeat, but its edge is drawn on past it.
    LinearClamp,

    /// Pixel by pixel, its edge drawn on past it: an image drawn without
    /// smoothing, and the scene image, which the resolve reads whole
    /// pixels of.
    NearestClamp,

    /// Compared, not read: the shadow map, where a shader asks whether a
    /// depth is in front of what the map holds and gets a 1 or a 0, blended
    /// between the four texels around the place. Past its edge it is all
    /// lit, which the border white says.
    ShadowCompare,
  };

  /// How many ways there are.
  constexpr int kSampling_Count = 6;
} // neon

#endif //VK_SAMPLING_HPP
