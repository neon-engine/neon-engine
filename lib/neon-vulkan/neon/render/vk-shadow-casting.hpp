#ifndef VK_SHADOW_CASTING_HPP
#define VK_SHADOW_CASTING_HPP

#include <vector>
#include <volk.h>

namespace neon
{
  /// Which draws of an object go into the shadow map. An object is drawn
  /// into the scene once for each material of its model, but the pass
  /// writes depth alone: when every material of the object casts, and with
  /// the same pipeline of the pass, the first of its draws casts every mesh
  /// of the model and the others cast nothing. When they differ, one
  /// see-through or one double-sided among them, each casts its own meshes.
  ///
  /// It is kept apart from the graphics card so that it can be checked.
  // ReSharper disable once CppInconsistentNaming
  struct VK_ShadowCasting
  {
    /// What is known of one material of an object: whether it casts, the
    /// pipeline of the pass it casts with, and which material of the model
    /// its meshes use.
    struct Caster
    {
      bool casts = false;
      VkPipeline pipeline = VK_NULL_HANDLE;
      int model_material = -1;
    };

    /// What a draw of the object does in the pass: whether it casts, and
    /// the meshes of which material of the model, every mesh when below 0.
    struct Cast
    {
      bool casts = false;
      int model_material = -1;
    };

    /// One answer for each material of the object, in their order.
    [[nodiscard]] static std::vector<Cast> Plan(const std::vector<Caster> &casters);
  };
} // neon

#endif //VK_SHADOW_CASTING_HPP
