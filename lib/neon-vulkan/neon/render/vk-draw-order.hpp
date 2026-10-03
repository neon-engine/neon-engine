#ifndef VK_DRAW_ORDER_HPP
#define VK_DRAW_ORDER_HPP

#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include <volk.h>

namespace neon
{
  /// A see-through model, kept until the opaque ones are drawn: what binds
  /// and draws it, and how far it is from the camera.
  // ReSharper disable once CppInconsistentNaming
  struct VK_SeeThroughDraw
  {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
    uint32_t scene_offset = 0;
    // where the object's data is in the buffer of the frame, which the
    // shaders read by gl_InstanceIndex
    uint32_t object_index = 0;
    int model_id = -1;

    // the material of the model whose meshes are drawn, every mesh when
    // below 0
    int material = -1;
    float distance = 0.0f;
  };

  /// The order see-through models are drawn in. It is kept apart from the
  /// renderer so that it can be checked without a graphics card.
  // ReSharper disable once CppInconsistentNaming
  struct VK_DrawOrder
  {
    /// How far a point lies in front of the camera, along the direction
    /// it looks in. What is behind it is below 0.
    [[nodiscard]] static float DistanceOf(const glm::mat4 &view, const glm::vec3 &position);

    /// Puts the farthest first, so that each is blended over what lies
    /// behind it. Models at the same distance keep the order they were
    /// drawn in.
    static void BackToFront(std::vector<VK_SeeThroughDraw> &draws);
  };
} // neon

#endif //VK_DRAW_ORDER_HPP
