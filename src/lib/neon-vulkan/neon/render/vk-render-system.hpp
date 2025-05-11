#ifndef VULKAN_RENDER_SYSTEM_HPP
#define VULKAN_RENDER_SYSTEM_HPP
#include <neon/render/render-system.hpp>

namespace neon
{
  class Vk_RenderSystem final : public RenderSystem
  {
  public:
    Vk_RenderSystem(WindowContext *window_context,
                       const SettingsConfig &settings_config,
                       const std::shared_ptr<Logger> &logger);

    int CreateRenderObject(const RenderInfo &render_info) override;

    void DrawRenderObject(int render_object_id,
                          const Transform &transform,
                          const glm::mat4 &view,
                          const glm::mat4 &projection,
                          const std::vector<LightSource> &lights) override;

    void DestroyRenderObject(int render_object_id) override;

    void Initialize() override;

    void CleanUp() override;

    void PrepareFrame() override;
  };
} // neon

#endif //VULKAN_RENDER_SYSTEM_HPP
