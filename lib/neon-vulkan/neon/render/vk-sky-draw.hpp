#ifndef VK_SKY_DRAW_HPP
#define VK_SKY_DRAW_HPP

#include <volk.h>

#include "vk-sky-values.hpp"

namespace neon
{
  /// The sky of a canvas, kept until its opaque models are drawn: what it
  /// is drawn with, the images it reads, and what its shader is told.
  // ReSharper disable once CppInconsistentNaming
  struct VK_SkyDraw
  {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
    VK_SkyValues values;
  };
} // neon

#endif //VK_SKY_DRAW_HPP
