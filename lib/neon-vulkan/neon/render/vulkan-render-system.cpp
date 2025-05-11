#include "vulkan-render-system.hpp"

namespace neon
{
  VulkanRenderSystem::VulkanRenderSystem(
    WindowContext *window_context,
    const SettingsConfig &settings_config,
    const std::shared_ptr<Logger> &logger): RenderSystem(
    window_context,
    settings_config,
    4096,
    logger) {}

  int VulkanRenderSystem::CreateRenderObject(const RenderInfo &render_info)
  {
    return 0;
  }

  void VulkanRenderSystem::DrawRenderObject(
    int render_object_id,
    const Transform &transform,
    const glm::mat4 &view,
    const glm::mat4 &projection,
    const std::vector<LightSource> &lights) {}

  void VulkanRenderSystem::DestroyRenderObject(int render_object_id) {}

  void VulkanRenderSystem::Initialize() {}
  void VulkanRenderSystem::CleanUp() {}
  void VulkanRenderSystem::PrepareFrame() {}
} // neon
