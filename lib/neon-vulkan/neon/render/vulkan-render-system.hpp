#ifndef VULKAN_RENDER_SYSTEM_HPP
#define VULKAN_RENDER_SYSTEM_HPP
#include "neon/render/render-system.hpp"

namespace neon
{
  class VulkanRenderSystem final : public RenderSystem
  {
  public:
    VulkanRenderSystem(WindowContext *window_context,
                       const SettingsConfig &settings_config,
                       int max_render_objects,
                       const std::shared_ptr<Logger> &logger);

    int CreateRenderObject(const RenderInfo &render_info) override;

    void DrawRenderObject(int render_object_id,
                          const Transform &transform,
                          const glm::mat4 &view,
                          const glm::mat4 &projection,
                          const std::vector<LightSource> &lights) override;

    void DestroyRenderObject(int render_object_id) override;

    const RenderResolution &GetRenderResolution() override;

    void Initialize() override;

    void CleanUp() override;

    void PrepareFrame() override;
  };
} // neon

#endif //VULKAN_RENDER_SYSTEM_HPP
