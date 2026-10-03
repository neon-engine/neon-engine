#ifndef VK_DRAW_HPP
#define VK_DRAW_HPP

#include <cstdint>
#include <volk.h>

#include "vk-shader-data.hpp"

namespace neon
{
  /// One model drawn with one material in a scene, kept until the scene
  /// ends, when every draw is put in the order that costs the least and
  /// those alike are drawn in one call. See VK_DrawQueue.
  // ReSharper disable once CppInconsistentNaming
  struct VK_Draw
  {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipeline shadow_pipeline = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
    uint32_t scene_offset = 0;
    int model_id = -1;
    int material_id = -1;

    // the material of the model whose meshes are drawn, every mesh when
    // below 0, which is what a mesh that was built has
    int model_material = -1;

    // how far in front of the camera, for drawing the nearest first
    float distance = 0.0f;
    bool see_through = false;
    bool casts_shadow = true;

    // what the shaders read of the object, written into the buffer of the
    // frame in the order the draws end up in
    VK_ObjectData data;
  };
} // neon

#endif //VK_DRAW_HPP
