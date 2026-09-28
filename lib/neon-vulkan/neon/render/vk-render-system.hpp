#ifndef VK_RENDER_SYSTEM_HPP
#define VK_RENDER_SYSTEM_HPP

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <neon/application/settings-config.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/render-system.hpp>

#include "vk-device.hpp"
#include "vk-material.hpp"
#include "vk-model.hpp"
#include "vk-shader.hpp"
#include "vk-shader-data.hpp"

namespace neon
{
  /// Renders with Vulkan.
  ///
  /// Every frame is drawn into an image the renderer owns. With a window, the
  /// finished image is then copied to the screen. Without one it is left in
  /// place, where CaptureFrame() can read it. Drawing is the same in both
  /// cases.
  // ReSharper disable once CppInconsistentNaming
  class VK_RenderSystem final : public RenderSystem
  {
    struct PipelineEntry
    {
      VK_Shader shader;
      VkPipeline pipeline = VK_NULL_HANDLE;
    };

    /// A buffer of shader data that stays mapped, and is filled from the
    /// start again in every frame.
    struct FrameBuffer
    {
      VkBuffer buffer = VK_NULL_HANDLE;
      VkDeviceMemory memory = VK_NULL_HANDLE;
      unsigned char *mapped = nullptr;
      VkDeviceSize entry_size = 0;
      uint32_t capacity = 0;
      uint32_t used = 0;
    };

    static constexpr uint32_t kMax_Render_Objects = 4096;
    // cameras and sets of lights that can differ within one frame
    static constexpr uint32_t kMax_Scenes_Per_Frame = 8;

    VK_Device _device;
    std::optional<RenderResolution> _render_resolution;
    VkExtent2D _extent{};

    // the image every frame is drawn into
    VkFormat _depth_format = VK_FORMAT_UNDEFINED;
    VkImage _color_image = VK_NULL_HANDLE;
    VkDeviceMemory _color_memory = VK_NULL_HANDLE;
    VkImageView _color_view = VK_NULL_HANDLE;
    VkImage _depth_image = VK_NULL_HANDLE;
    VkDeviceMemory _depth_memory = VK_NULL_HANDLE;
    VkImageView _depth_view = VK_NULL_HANDLE;
    VkRenderPass _render_pass = VK_NULL_HANDLE;
    VkFramebuffer _framebuffer = VK_NULL_HANDLE;

    // the window, when there is one
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    VkExtent2D _swapchain_extent{};
    std::vector<VkImage> _swapchain_images;
    VkSemaphore _image_available = VK_NULL_HANDLE;
    VkSemaphore _render_finished = VK_NULL_HANDLE;

    VkCommandBuffer _commands = VK_NULL_HANDLE;
    VkFence _frame_done = VK_NULL_HANDLE;
    bool _frame_open = false;
    bool _frame_finished = false;

    VkDescriptorSetLayout _descriptor_layout = VK_NULL_HANDLE;
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;
    std::map<std::string, PipelineEntry> _pipelines;

    FrameBuffer _scene_buffer;
    FrameBuffer _object_buffer;
    VK_SceneData _last_scene;
    bool _has_last_scene = false;
    uint32_t _last_scene_offset = 0;
    bool _warned_about_lights = false;
    bool _warned_about_capacity = false;

    VK_Texture _white_texture;

    DataBuffer<VK_Model> _model_refs;
    DataBuffer<VK_Material> _material_refs;

    bool CreateRenderTarget();
    bool CreateSwapchain();
    bool CreateDescriptors();
    bool CreateFrameBuffer(FrameBuffer &buffer, VkDeviceSize entry_size, uint32_t capacity) const;
    void DestroyFrameBuffer(FrameBuffer &buffer) const;
    bool GetPipeline(const std::string &shader_path, VkPipeline &pipeline);
    bool CreateDescriptorSet(VK_Material &material) const;
    void CopyToWindow(VkCommandBuffer commands, uint32_t image_index) const;

    [[nodiscard]] VK_SceneData BuildSceneData(
      const glm::mat4 &view,
      const glm::mat4 &projection,
      const std::vector<LightSource> &lights);

  public:
    explicit VK_RenderSystem(
      WindowContext *window_context,
      FileSystemContext *file_system_context,
      const SettingsConfig &settings_config,
      const std::shared_ptr<Logger> &logger)
      : RenderSystem(window_context, file_system_context, settings_config, kMax_Render_Objects, logger),
        _model_refs(kMax_Render_Objects),
        _material_refs(kMax_Render_Objects) {}

    void Initialize() override;

    void CleanUp() override;

    void PrepareFrame() override;

    void FinishFrame() override;

    bool CaptureFrame(const std::string &path) override;

    const RenderResolution &GetRenderResolution() override;

    int CreateRenderObject(const RenderInfo &render_info) override;

    void DrawRenderObject(
      int render_object_id,
      const Transform &transform,
      const glm::mat4 &view,
      const glm::mat4 &projection,
      const std::vector<LightSource> &lights) override;

    void DestroyRenderObject(int render_object_id) override;
  };
} // neon

#endif //VK_RENDER_SYSTEM_HPP
