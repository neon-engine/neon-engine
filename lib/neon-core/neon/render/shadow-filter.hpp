#ifndef SHADOW_FILTER_HPP
#define SHADOW_FILTER_HPP

namespace neon
{
  /// How the shadow map of the direction light is compared against at a
  /// point: how many comparisons are averaged along the edge of a shadow.
  /// See docs/vulkan-renderer.md.
  enum class ShadowFilter
  {
    /// One comparison, which the sampler blends across the four texels
    /// around the place. The edge of a shadow is about a texel wide.
    None = 0,

    /// Percentage-closer filtering: nine comparisons one texel apart,
    /// averaged, which soften the edge over about three texels.
    Pcf
  };
} // neon

#endif //SHADOW_FILTER_HPP
