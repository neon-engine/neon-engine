#ifndef VK_SHADOW_BATCH_HPP
#define VK_SHADOW_BATCH_HPP

#include <cstdint>
#include <volk.h>

namespace neon
{
  /// Draws into the shadow map that are one call: the same pipeline of the
  /// pass, the same model, the same meshes of it. Which material an object
  /// is drawn with does not come into it, since the pass writes depth
  /// alone. Its objects are side by side in the order of the pass, from
  /// `first_instance` on, see VK_DrawQueue::ShadowOrder().
  // ReSharper disable once CppInconsistentNaming
  struct VK_ShadowBatch
  {
    VkPipeline pipeline = VK_NULL_HANDLE;

    // any set serves, since the pass reads the scene and the objects alone,
    // which every set binds the same
    VkDescriptorSet set = VK_NULL_HANDLE;
    uint32_t scene_offset = 0;
    int model_id = -1;

    // the material of the model whose meshes are drawn, every mesh when
    // below 0
    int model_material = -1;
    uint32_t first_instance = 0;
    uint32_t instances = 0;
  };
} // neon

#endif //VK_SHADOW_BATCH_HPP
