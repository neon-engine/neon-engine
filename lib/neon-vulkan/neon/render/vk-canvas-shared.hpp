#ifndef VK_CANVAS_SHARED_HPP
#define VK_CANVAS_SHARED_HPP

#include <functional>
#include <memory>

#include <neon/logging/logger.hpp>

#include "vk-device.hpp"
#include "vk-model-cache.hpp"
#include "vk-resolve.hpp"

namespace neon
{
  class VK_Canvas;

  /// What every canvas draws with, which the renderer owns and the
  /// frame and the render targets share.
  // ReSharper disable once CppInconsistentNaming
  struct VK_CanvasShared
  {
    VK_Device *device = nullptr;
    VK_Resolve *resolve = nullptr;
    VkRenderPass scene_pass = VK_NULL_HANDLE;
    VkRenderPass frame_pass = VK_NULL_HANDLE;
    VkFormat depth_format = VK_FORMAT_UNDEFINED;

    // what the see-through models are drawn with
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    const VK_ModelCache *models = nullptr;

    // draws the opaque models the render system kept for the canvas,
    // when its scene is about to end
    std::function<void(VK_Canvas &)> draw_opaque;

    std::shared_ptr<Logger> logger;

    // said once, and not for every model
    bool warned_about_order = false;
  };
} // neon

#endif //VK_CANVAS_SHARED_HPP
