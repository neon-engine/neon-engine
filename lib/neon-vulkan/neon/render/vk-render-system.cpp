#include "vk-render-system.hpp"

namespace neon
{
  Vk_RenderSystem::Vk_RenderSystem(
    WindowContext *window_context,
    const SettingsConfig &settings_config,
    const std::shared_ptr<Logger> &logger): RenderSystem(
    window_context,
    settings_config,
    4096,
    logger) {}

  int Vk_RenderSystem::CreateRenderObject(const RenderInfo &render_info)
  {
    return 0;
  }

  void Vk_RenderSystem::DrawRenderObject(
    int render_object_id,
    const Transform &transform,
    const glm::mat4 &view,
    const glm::mat4 &projection,
    const std::vector<LightSource> &lights) {}

  void Vk_RenderSystem::DestroyRenderObject(int render_object_id) {}

  void Vk_RenderSystem::Initialize() {}
  void Vk_RenderSystem::CleanUp() {}
  void Vk_RenderSystem::PrepareFrame() {}
} // neon
