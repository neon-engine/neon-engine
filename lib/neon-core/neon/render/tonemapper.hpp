#ifndef TONEMAPPER_HPP
#define TONEMAPPER_HPP

namespace neon
{
  /// The curve the resolve step maps the light of a scene through before
  /// it is written as the colors of a screen. A scene is lit in linear
  /// light, which has no top: an emissive surface or a bright light makes
  /// values above 1, which a screen cannot show. The curve decides what
  /// becomes of them. See docs/vulkan-renderer.md.
  enum class Tonemapper
  {
    /// No curve. What is brighter than white is white, cut off flat.
    None = 0,

    /// The ACES filmic curve, as Krzysztof Narkowicz fits it: a gentle
    /// roll-off towards white that keeps a little contrast in what is
    /// bright. White itself comes out a little below white.
    Aces,

    /// AgX, in the minimal form of Benjamin Wrensch after Troy Sobotka: a
    /// log encoding and a sigmoid, which keeps colors from turning to
    /// yellow or white as they get bright.
    Agx
  };
} // neon

#endif //TONEMAPPER_HPP
