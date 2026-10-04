#ifndef RENDER_OBJECT_REF_HPP
#define RENDER_OBJECT_REF_HPP

#include <vector>

namespace neon
{
  /// What a render object is made of: the model it draws, and a material
  /// of the renderer for each material of the model that a mesh uses, in
  /// the order the model lists them. One for a model with one material,
  /// and for a mesh that was built.
  struct RenderObjectRef
  {
    int model_id = -1;
    std::vector<int> material_ids;

    /// The materials that were made ready for what the object will show
    /// later, see RenderInfo::preload_paths. It holds them and draws with
    /// none of them.
    std::vector<int> preloaded_material_ids;
  };

} // neon

#endif //RENDER_OBJECT_REF_HPP
