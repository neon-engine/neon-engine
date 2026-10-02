#ifndef MODEL_FIT_HPP
#define MODEL_FIT_HPP

namespace neon
{
  /// How a model from a file is sized when it is drawn, and when a collider
  /// is made from it. In the order a scene file names the choices.
  enum class ModelFit
  {
    /// At its own size and around its own origin, as the file says. What
    /// glTF means: a piece of a kit is in metres and stands on its origin.
    None,

    /// Moved so that its middle lies at the origin, and scaled so that its
    /// longest side is 1. For a model that is not in metres, which the
    /// `scale` of the Transform then sizes.
    Unit
  };
} // neon

#endif //MODEL_FIT_HPP
