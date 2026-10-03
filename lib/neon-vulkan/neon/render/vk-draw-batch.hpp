#ifndef VK_DRAW_BATCH_HPP
#define VK_DRAW_BATCH_HPP

#include <cstdint>
#include <volk.h>

namespace neon
{
  /// Draws that are one call: the same pipeline, the same material, the
  /// same model, with their objects side by side in the buffer of the
  /// frame, from `first_instance` on. See VK_DrawQueue.
  // ReSharper disable once CppInconsistentNaming
  struct VK_DrawBatch
  {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipeline shadow_pipeline = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
    uint32_t scene_offset = 0;
    int model_id = -1;
    // the material of the model whose meshes are drawn, every mesh when
    // below 0
    int model_material = -1;
    uint32_t first_instance = 0;
    uint32_t instances = 0;
    bool casts_shadow = true;
  };
} // neon

#endif //VK_DRAW_BATCH_HPP
