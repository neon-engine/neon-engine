#ifndef MODEL_KEY_HPP
#define MODEL_KEY_HPP

#include <compare>
#include <optional>
#include <string>

#include "model-fit.hpp"
#include "render-info.hpp"

namespace neon
{
  /// What a model is known by, so that the render objects that draw the
  /// same one share it: a model file by its path and fit, whatever its
  /// format (.obj, .fbx, .gltf, ...), and a mesh that was built by its
  /// RenderInfo::mesh_key, the values it was built from.
  struct ModelKey
  {
    /// The path of the file, or the key of the mesh.
    std::string source;

    /// How a file is sized. A mesh that was built has none.
    ModelFit fit = ModelFit::None;

    auto operator<=>(const ModelKey &) const = default;

    /// The key of what a render object draws. None for a mesh that was
    /// built without a key, which is the render object's own: it may
    /// change, as a rope's does, and is never shared.
    [[nodiscard]] static std::optional<ModelKey> Of(const RenderInfo &render_info)
    {
      if (render_info.mesh != nullptr)
      {
        if (render_info.mesh_key.empty()) { return std::nullopt; }
        return ModelKey{.source = render_info.mesh_key};
      }

      return ModelKey{.source = render_info.model_path, .fit = render_info.fit};
    }
  };
} // neon

#endif //MODEL_KEY_HPP
