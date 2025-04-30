#include "vulkan-render-system.hpp"

namespace neon
{
  VulkanRenderSystem::VulkanRenderSystem(WindowContext *window_context,
                                         const SettingsConfig &settings_config,
                                         int max_render_objects,
                                         const std::shared_ptr<Logger> &logger): RenderSystem(window_context, settings_config, max_render_objects, logger) {}

  int VulkanRenderSystem::CreateRenderObject(const RenderInfo &render_info)
  {
    return 0;
  }

  void VulkanRenderSystem::DrawRenderObject(int render_object_id,
                                            const Transform &transform,
                                            const glm::mat4 &view,
                                            const glm::mat4 &projection,
                                            const std::vector<LightSource> &lights) {}

  void VulkanRenderSystem::DestroyRenderObject(int render_object_id) {}
  const RenderResolution &VulkanRenderSystem::GetRenderResolution()
  {
    return RenderResolution(1920, 1080);
  }
  void VulkanRenderSystem::Initialize() {}
  void VulkanRenderSystem::CleanUp() {}
  void VulkanRenderSystem::PrepareFrame() {}
} // neon
