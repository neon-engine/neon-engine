#ifndef VK_RENDER_SYSTEM_HPP
#define VK_RENDER_SYSTEM_HPP

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <neon/runtime/settings-config.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/render-2d-context.hpp>
#include <neon/render/render-system.hpp>

#include "vk-device.hpp"
#include "vk-draw-order.hpp"
#include "vk-frame-stages.hpp"
#include "vk-material.hpp"
#include "vk-model.hpp"
#include "vk-render-target.hpp"
#include "vk-renderer-2d.hpp"
#include "vk-resolve.hpp"
#include "vk-scene-image.hpp"
#include "vk-shader.hpp"
#include "vk-shader-data.hpp"
#include "vk-swapchain-sizing.hpp"

namespace neon
{
  /// Renders with Vulkan.
  ///
  /// Every frame is drawn into an image the renderer owns. With a window, the
  /// finished image is then copied to the screen. Without one it is left in
  /// place, where CaptureFrame() can read it. Drawing is the same in both
  /// cases.
  ///
  /// The models of a scene are lit in a scene image of linear light. The
  /// resolve step turns that into the sRGB colours of the image that is
  /// shown, and what is drawn in two dimensions goes on top of it. A render
  /// target is drawn the same way.
  ///
  /// It draws the models of a scene as a RenderContext, and triangles in
  /// two dimensions on top of them as a Render2DContext.
  // ReSharper disable once CppInconsistentNaming
  class VK_RenderSystem final : public RenderSystem, public Render2DContext
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

    // The scene of every frame is lit in the scene image, which the resolve
    // step draws into the image that is shown.
    VkFormat _depth_format = VK_FORMAT_UNDEFINED;
    VkRenderPass _scene_pass = VK_NULL_HANDLE;
    VK_SceneImage _scene;
    VkDescriptorSet _scene_set = VK_NULL_HANDLE;
    VK_Resolve _resolve;

    // the image that is shown, which every frame is drawn into
    VkImage _color_image = VK_NULL_HANDLE;
    VkDeviceMemory _color_memory = VK_NULL_HANDLE;
    VkImageView _color_view = VK_NULL_HANDLE;
    VkRenderPass _frame_pass = VK_NULL_HANDLE;
    VkFramebuffer _framebuffer = VK_NULL_HANDLE;
    VK_FrameStage _frame_stage = VK_FrameStage::Nothing;

    // see-through models of the frame, drawn when its scene is finished
    std::vector<VK_SeeThroughDraw> _see_through;

    // the window, when there is one
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    VkExtent2D _swapchain_extent{};
    std::vector<VkImage> _swapchain_images;
    VkSemaphore _image_available = VK_NULL_HANDLE;
    VkSemaphore _render_finished = VK_NULL_HANDLE;

    // The size of the window the swapchain was made for, and whether Vulkan
    // said since that it no longer fits. Either makes it again.
    WindowSize _window_size{};
    bool _swapchain_stale = false;

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
    bool _warned_about_order = false;
    bool _warned_about_window_format = false;

    VK_Texture _white_texture;

    DataBuffer<VK_Model> _model_refs;
    DataBuffer<VK_Material> _material_refs;

    VK_Renderer2D _renderer_2d;

    /// A render target, and what it is known as where it is drawn with in
    /// two dimensions.
    struct Target
    {
      VK_RenderTarget target;
      int texture = No_Texture;

      // whether it was drawn to in the frame that is being drawn
      bool is_drawn = false;

      // what it is cleared to, and how far its drawing got in this frame
      Color clear;
      VK_FrameStage stage = VK_FrameStage::Nothing;
      std::vector<VK_SeeThroughDraw> see_through;

      // what the resolve step reads its scene image through, once it has
      // one
      VkDescriptorSet scene_set = VK_NULL_HANDLE;
    };

    static constexpr int kMax_Render_Targets = 64;

    DataBuffer<Target> _targets{kMax_Render_Targets};

    // The commands that draw into render targets. They are run in front of
    // those of the frame, so that a target is finished before what shows
    // it is drawn.
    VkCommandBuffer _target_commands = VK_NULL_HANDLE;
    bool _target_commands_open = false;
    int _current_target = No_Render_Target;

    // What waits for the frame to be finished: nothing that the commands
    // of a frame refer to may change or go while they are not run.
    std::vector<Target> _targets_to_release;
    bool _surfaces_changed = false;

    // names that were refused, each said once
    std::vector<std::string> _refused_targets;

    bool WriteDescriptorSet(const VK_Material &material, VkDescriptorSet set) const;

    [[nodiscard]] bool FindSurface(const std::string &name, VK_Texture &texture) const;

    /// Does what waited for the frame to be finished.
    void SettleRenderTargets();

    void ReleaseTarget(Target &target) const;

    /// Gets the frame, or the render target that is drawn to, to the stage
    /// that draws models of the scene. Returns false when they cannot be
    /// drawn any more.
    bool EnterScene();

    /// Gets it to the stage that draws on top of the scene, which resolves
    /// the scene on the way.
    void EnterOverlay();

    /// Finishes what the frame, or the render target, draws to.
    void LeaveCanvas();

    bool CreateRenderPasses();
    bool CreateFrameImages();
    void DestroyFrameImages();
    bool CreateSwapchain(VkExtent2D wanted);

    /// Makes the swapchain again when the window changed its size, and
    /// with it everything that has the size of the frame. Returns false
    /// when there is nothing to draw to, as while the window is minimized.
    bool FitWindow();
    bool CreateDescriptors();
    bool CreateFrameBuffer(FrameBuffer &buffer, VkDeviceSize entry_size, uint32_t capacity) const;
    void DestroyFrameBuffer(FrameBuffer &buffer) const;
    bool GetPipeline(
      const std::string &shader_path, AlphaMode alpha_mode, bool double_sided, bool mirrored, VkPipeline &pipeline);

    /// Draws the see-through models that were kept, from the farthest to
    /// the nearest, and forgets them.
    void DrawSeeThrough(VkCommandBuffer commands, std::vector<VK_SeeThroughDraw> &draws);
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

    [[nodiscard]] const RenderCapabilities &GetCapabilities() const override;

    const RenderResolution &GetRenderResolution() override;

    int CreateRenderObject(const RenderInfo &render_info) override;

    void DrawRenderObject(
      int render_object_id,
      const Transform &transform,
      const glm::mat4 &view,
      const glm::mat4 &projection,
      const std::vector<LightSource> &lights) override;

    void DestroyRenderObject(int render_object_id) override;

    int CreateTexture(int width, int height, const std::vector<unsigned char> &pixels) override;

    int LoadTexture(const std::string &path) override;

    bool GetTextureSize(int texture, int &width, int &height) override;

    void DestroyTexture(int texture) override;

    void DrawTriangles(const Triangles2D &triangles) override;

    bool UpdateTexture(
      int texture,
      int x,
      int y,
      int width,
      int height,
      const std::vector<unsigned char> &pixels) override;

    int CreateTextureWith(
      int width,
      int height,
      const std::vector<unsigned char> &pixels,
      const TextureOptions2D &options) override;

    int CreateMaterial(const std::string &shader_path) override;

    void DestroyMaterial(int material) override;

    // Render targets, for what is drawn in two dimensions and for the
    // models of a scene alike.

    int CreateRenderTarget(const std::string &name, int width, int height) override;

    void DestroyRenderTarget(int target) override;

    bool BeginRenderTarget(int target, const Color &clear) override;

    void EndRenderTarget() override;

    int GetRenderTargetTexture(int target) override;

    bool GetRenderTargetSize(int target, int &width, int &height) override;

    int FindRenderTarget(const std::string &name) override;
  };
} // neon

#endif //VK_RENDER_SYSTEM_HPP
