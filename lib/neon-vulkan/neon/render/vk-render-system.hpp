#ifndef VK_RENDER_SYSTEM_HPP
#define VK_RENDER_SYSTEM_HPP

#include <unordered_map>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <neon/runtime/settings-config.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/render-2d-context.hpp>
#include <neon/render/render-system.hpp>

#include "vk-canvas.hpp"
#include "vk-capture.hpp"
#include "vk-device.hpp"
#include "vk-draw-order.hpp"
#include "vk-draw-queue.hpp"
#include "vk-material.hpp"
#include "vk-material-cache.hpp"
#include "vk-model-cache.hpp"
#include "vk-pipelines.hpp"
#include "vk-render-target.hpp"
#include "vk-renderer-2d.hpp"
#include "vk-resolve.hpp"
#include "vk-samplers.hpp"
#include "vk-shader-data.hpp"
#include "vk-shadow-cascades.hpp"
#include "vk-shadow-map.hpp"
#include "vk-sky.hpp"
#include "vk-swapchain.hpp"
#include "vk-texture-cache.hpp"

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
  /// The sky of a scene is drawn into the scene image behind its models:
  /// after the opaque ones, wherever none of them is, and before the
  /// see-through ones.
  ///
  /// Before any of that, the shadow map of the direction light is drawn in
  /// a pass of its own, into commands that run before those of the
  /// frame and the render targets: the opaque models of the first scene
  /// of the frame whose light casts, as the light sees them.
  ///
  /// It draws the models of a scene as a RenderContext, and triangles in
  /// two dimensions on top of them as a Render2DContext.
  // ReSharper disable once CppInconsistentNaming
  class VK_RenderSystem final : public RenderSystem, public Render2DContext
  {
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

    /// How many materials one descriptor pool serves; another is made when
    /// one is full.
    static constexpr uint32_t kSets_Per_Pool = 256;
    // cameras and sets of lights that can differ within one frame
    static constexpr uint32_t kMax_Scenes_Per_Frame = 8;

    VK_Device _device;

    // what every texture is read through, one sampler for each way
    VK_Samplers _samplers;
    std::optional<RenderResolution> _render_resolution;
    VkExtent2D _extent{};

    // The scene of every frame is lit in a scene image, which the resolve
    // step draws into the image that is shown.
    VkFormat _depth_format = VK_FORMAT_UNDEFINED;
    VkRenderPass _scene_pass = VK_NULL_HANDLE;
    VK_Resolve _resolve;

    // what is seen behind the models of a scene, and the images of it
    VK_Sky _sky;

    // the image that is shown, which every frame is drawn into
    VkImage _color_image = VK_NULL_HANDLE;
    VkDeviceMemory _color_memory = VK_NULL_HANDLE;
    VkImageView _color_view = VK_NULL_HANDLE;
    VkRenderPass _frame_pass = VK_NULL_HANDLE;
    VkFramebuffer _framebuffer = VK_NULL_HANDLE;

    // where the frame is drawn, and what it and the render targets draw
    // with
    VK_CanvasShared _canvas_shared;
    VK_Canvas _frame;

    // reads a finished frame back for a screenshot
    VK_Capture _capture;

    // the window, when there is one
    VK_Swapchain _swapchain;

    VkCommandBuffer _commands = VK_NULL_HANDLE;
    VkFence _frame_done = VK_NULL_HANDLE;
    bool _frame_open = false;
    bool _frame_finished = false;

    // The shadow map of the direction light, and the commands that draw
    // it, which run before every other command of the frame. The map is
    // fitted once a frame, around the first camera that draws with a
    // light that casts, and holds what that camera's scene draws.
    VK_ShadowMap _shadow_map;
    VkCommandBuffer _shadow_commands = VK_NULL_HANDLE;
    bool _shadow_open = false;
    bool _shadow_fitted = false;
    VK_ShadowCascades _shadow_cascades;
    int _shadow_target = No_Render_Target;

    // The draws of the scene being drawn, kept until it ends and then
    // drawn in the order that costs the least, those alike as one call,
    // see VK_DrawQueue; and the canvas they are for.
    VK_DrawQueue _draws;
    VK_Canvas *_draws_canvas = nullptr;

    // what a frame cost in calls, said in the log now and then
    std::size_t _frame_objects = 0;
    std::size_t _frame_draws = 0;
    std::size_t _frame_pipeline_binds = 0;
    std::size_t _frame_set_binds = 0;
    std::size_t _frames_since_said = 0;

    VK_Pipelines _pipelines;
    // the pools the materials' sets are taken from, one more whenever the
    // last is full
    std::vector<VkDescriptorPool> _descriptor_pools;

    FrameBuffer _scene_buffer;
    FrameBuffer _object_buffer;
    VK_SceneData _last_scene;
    bool _has_last_scene = false;
    uint32_t _last_scene_offset = 0;
    // what _last_scene was built from, see SameScene()
    glm::mat4 _last_view{1.0f};
    glm::mat4 _last_projection{1.0f};
    std::vector<LightSource> _last_lights;
    bool _warned_about_lights = false;
    bool _warned_about_capacity = false;

    VK_Texture _white_texture;

    // What the render objects draw: each model and each texture is held
    // once, however many objects draw it, and a material for each material
    // of its model that a mesh uses, shared with the objects that draw with
    // the same one.
    VK_ModelCache _models;
    VK_TextureCache _textures;
    VK_MaterialCache _materials;

    // what was created since the last frame said so, and what the caches
    // had done by then
    std::size_t _objects_created = 0;
    std::size_t _material_makes_reported = 0;
    std::size_t _material_shares_reported = 0;
    std::size_t _model_loads_reported = 0;
    std::size_t _model_shares_reported = 0;

    /// Says in the log what the render objects created since the last
    /// frame loaded, and what they shared. Nothing is said when none was.
    void ReportCreated();

    VK_Renderer2D _renderer_2d;

    /// A render target, where it is drawn, and what it is known as where
    /// it is drawn with in two dimensions.
    struct Target
    {
      VK_RenderTarget target;
      VK_Canvas canvas;
      int texture = No_Texture;

      // whether it was drawn to in the frame that is being drawn
      bool is_drawn = false;
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

    // The images that were made at run time, by their names, kept as pixels
    // until a material reads one as a texture, and after, for the next
    // material that is made of it once the first was let go.
    std::unordered_map<std::string, std::shared_ptr<const ImagePixels>> _images;

    [[nodiscard]] bool FindSurface(const std::string &name, VK_Texture &texture) const;

    /// Does what waited for the frame to be finished.
    void SettleRenderTargets();

    void ReleaseTarget(Target &target) const;

    /// Where what is drawn goes: the render target that is drawn to, or
    /// the frame.
    [[nodiscard]] VK_Canvas &CurrentCanvas();

    bool CreateRenderPasses();
    bool CreateFrameImages();
    void DestroyFrameImages();

    /// Has the swapchain made again when the window changed its size, and
    /// with it everything that has the size of the frame. Returns false
    /// when there is nothing to draw to, as while the window is minimized.
    bool FitWindow();
    bool CreateDescriptors();
    bool CreateFrameBuffer(FrameBuffer &buffer, VkDeviceSize entry_size, uint32_t capacity) const;
    void DestroyFrameBuffer(FrameBuffer &buffer) const;

    /// Allocates the material's set from the last pool, making another pool
    /// when that one is full.
    bool CreateDescriptorSet(VK_Material &material);

    /// Adds a pool to take sets from.
    bool CreateDescriptorPool();

    /// The material a render object draws the meshes of one material of
    /// its model with: what the scene says, filled in with what the file
    /// says about that material, see docs/models.md. `material` is an
    /// index into the model's materials, or -1 for a mesh that was built;
    /// `first` says whether it is the first of the object, which is the
    /// one the scene's own textures replace. Shared with the render objects
    /// that draw with the same one already, made otherwise. Returns the id
    /// it is held under, or -1 when it cannot be made, which is logged.
    int AcquireObjectMaterial(const RenderInfo &render_info, const VK_Model &model, int material, bool first);

    /// Gives a material of a render object back, which frees it and its
    /// descriptor set when nothing else draws with it.
    void ReleaseObjectMaterial(int material_id);

    [[nodiscard]] VK_SceneData BuildSceneData(
      const glm::mat4 &view,
      const glm::mat4 &projection,
      const std::vector<LightSource> &lights);

    /// Begins the pass that draws the shadow map, for the scene that is
    /// being drawn. Returns false when it cannot be recorded.
    bool BeginShadowPass();

    /// Draws what was kept for the canvas: the opaque batches in their
    /// order, and into the shadow map when the scene casts, and hands the
    /// see-through draws to the canvas. The objects' data is written into
    /// the buffer of the frame in that order first.
    void FlushDraws(VK_Canvas &canvas);

    /// Whether `view`, `projection`, and `lights` are those the last scene
    /// data was built from, which saves building it for every object.
    [[nodiscard]] bool SameScene(
      const glm::mat4 &view,
      const glm::mat4 &projection,
      const std::vector<LightSource> &lights) const;

  public:
    explicit VK_RenderSystem(
      WindowContext *window_context,
      FileSystemContext *file_system_context,
      const SettingsConfig &settings_config,
      const std::shared_ptr<Logger> &logger)
      : RenderSystem(window_context, file_system_context, settings_config, static_cast<int>(settings_config.max_render_objects), logger),
        _models(static_cast<int>(settings_config.max_render_objects)),
        _materials(static_cast<int>(settings_config.max_render_objects)) {}

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

    void UpdateRenderObjectMesh(int render_object_id, const MeshData &mesh) override;

    bool SetImage(const std::string &name, const ImagePixels &pixels) override;

    void DrawSky(const SkyInfo &sky, const glm::mat4 &view, const glm::mat4 &projection) override;

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
