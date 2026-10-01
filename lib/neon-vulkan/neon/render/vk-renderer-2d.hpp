#ifndef VK_RENDERER_2D_HPP
#define VK_RENDERER_2D_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <neon/common/data-buffer.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/render/render-2d-context.hpp>

#include "vk-device.hpp"
#include "vk-shader.hpp"
#include "vk-shader-values.hpp"
#include "vk-texture.hpp"

namespace neon
{
  /// Draws triangles in pixels, for the render system. It is what the
  /// render system implements Render2DContext with.
  ///
  /// Nothing is created until the first texture or the first triangle asks
  /// for it, so a game without a user interface pays nothing. What could
  /// not be created is not tried again.
  // ReSharper disable once CppInconsistentNaming
  class VK_Renderer2D
  {
    struct Texture
    {
      VK_Texture texture;
      VkDescriptorSet descriptor_set = VK_NULL_HANDLE;

      // the same image read pixel by pixel, made when it is first asked
      // for
      VkDescriptorSet pixelated_set = VK_NULL_HANDLE;
    };

    /// A shader of an element.
    struct Material
    {
      std::string path;
      VkShaderModule fragment = VK_NULL_HANDLE;
      VkPipeline pipeline = VK_NULL_HANDLE;
      VK_ShaderValues values;

      // names that were handed over and that the shader does not declare,
      // each reported once
      std::vector<std::string> unknown;
    };

    /// A buffer that stays mapped, and is filled from the start again in
    /// every frame.
    struct FrameBuffer
    {
      VkBuffer buffer = VK_NULL_HANDLE;
      VkDeviceMemory memory = VK_NULL_HANDLE;
      unsigned char *mapped = nullptr;
    };

    enum class State
    {
      NotCreated = 0,
      Ready,
      Failed
    };

    VK_Device *_device = nullptr;
    FileSystemContext *_file_system_context = nullptr;
    std::shared_ptr<Logger> _logger;
    VkRenderPass _render_pass = VK_NULL_HANDLE;
    VkExtent2D _extent{};

    State _state = State::NotCreated;
    VK_Shader _shader;
    VkDescriptorSetLayout _descriptor_layout = VK_NULL_HANDLE;
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;

    // the corners and the triangles of a frame, filled from the start
    // again in every frame
    VkBuffer _vertex_buffer = VK_NULL_HANDLE;
    VkDeviceMemory _vertex_memory = VK_NULL_HANDLE;
    Vertex2D *_vertices = nullptr;
    VkBuffer _index_buffer = VK_NULL_HANDLE;
    VkDeviceMemory _index_memory = VK_NULL_HANDLE;
    uint32_t *_indices = nullptr;
    uint32_t _used_vertices = 0;
    uint32_t _used_indices = 0;
    bool _warned_about_capacity = false;
    bool _warned_about_shape = false;

    DataBuffer<Texture> _textures;
    int _white_texture = No_Texture;

    VkDescriptorSetLayout _shapes_layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _values_layout = VK_NULL_HANDLE;
    VkDescriptorSet _shapes_set = VK_NULL_HANDLE;
    VkDescriptorSet _values_set = VK_NULL_HANDLE;
    VkSampler _pixelated_sampler = VK_NULL_HANDLE;

    FrameBuffer _shape_buffer;
    FrameBuffer _values_buffer;
    uint32_t _used_shapes = 0;
    uint32_t _used_values = 0;
    VkDeviceSize _values_entry_size = 0;
    bool _warned_about_shapes = false;
    bool _warned_about_values = false;

    // Shaders of elements by their path. One that could not be made is
    // kept as well, without a pipeline, so that it is tried and reported
    // once.
    std::vector<Material> _materials;

    // what is drawn to, which is the frame unless a render target was
    // begun
    VkCommandBuffer _target_commands = VK_NULL_HANDLE;
    VkExtent2D _target_extent{};
    bool _has_target = false;

    bool CreatePipelineFor(VkShaderModule fragment, const std::string &path, VkPipeline &pipeline) const;

    bool CreateFrameBuffer(FrameBuffer &buffer, VkDeviceSize size, VkBufferUsageFlags usage) const;

    void DestroyFrameBuffer(FrameBuffer &buffer) const;

    [[nodiscard]] VkDescriptorSet SetOf(int texture, TextureFilter2D filter);

    bool Create();

    bool CreatePipeline();

    bool CreateBuffers();

    /// Creates what drawing needs when that has not been tried yet.
    bool Prepare();

    int Keep(VK_Texture &texture);

  public:
    /// Corners that can be drawn in one frame, and the corners of
    /// triangles. They are enough for 32768 rectangles.
    static constexpr uint32_t kMax_Vertices = 131072;
    static constexpr uint32_t kMax_Indices = 196608;
    static constexpr int kMax_Textures = 256;

    /// Shapes that can be drawn in one frame, and calls that are drawn
    /// with a shader of an element.
    static constexpr uint32_t kMax_Shapes = 16384;
    static constexpr uint32_t kMax_Material_Draws = 1024;

    /// The bytes the values of a shader of an element may take.
    static constexpr uint32_t kMax_Values_Size = 256;

    /// What both halves of the shader are told about a call. It has to
    /// match ui-frame.glsl.
    struct Frame
    {
      float size[2];
      float translation[2];
      float clip_box[4];
      float clip_radii[4];
      float state[4];
      float element[4];
    };

    /// The shader, which the build compiles from `flat.vert` and
    /// `flat.frag`.
    static constexpr const char *kShader_Path = "assets://shaders/flat";

    VK_Renderer2D() : _textures(kMax_Textures) {}

    /// Remembers what it draws with. Creates nothing.
    void Initialize(
      VK_Device *device,
      FileSystemContext *file_system_context,
      VkRenderPass render_pass,
      VkExtent2D extent,
      const std::shared_ptr<Logger> &logger);

    void CleanUp();

    /// The frame was given another size, as when the window was resized.
    void Resize(VkExtent2D extent);

    /// Called once before anything is drawn in a frame.
    void PrepareFrame();

    int CreateTexture(int width, int height, const std::vector<unsigned char> &pixels);

    int LoadTexture(const std::string &path);

    bool GetTextureSize(int texture, int &width, int &height) const;

    void DestroyTexture(int texture);

    /// Draws into the commands of the frame, and leaves the viewport the
    /// way the models of the scene need it.
    void Draw(const Triangles2D &triangles);

    /// The part of the frame triangles are cut off at. Nothing of it lies
    /// outside the frame, which Vulkan does not allow.
    [[nodiscard]] static VkRect2D ScissorOf(const Triangles2D &triangles, VkExtent2D extent);

    /// Whether every triangle has three corners and every corner exists.
    [[nodiscard]] static bool IsWellFormed(const Triangles2D &triangles);

    bool UpdateTexture(int texture, int x, int y, int width, int height, const std::vector<unsigned char> &pixels);

    int CreateTextureWith(
      int width,
      int height,
      const std::vector<unsigned char> &pixels,
      const TextureOptions2D &options);

    /// Keeps a texture that reads an image of something else, such as a
    /// render target. Returns what it is known as, or No_Texture.
    int KeepBorrowed(VkImageView view, VkSampler sampler, uint32_t width, uint32_t height);

    int CreateMaterial(const std::string &shader_path);

    void DestroyMaterial(int material);

    /// From now on Draw() records into these commands, for an image of
    /// this size. `commands` that are VK_NULL_HANDLE stand for the frame.
    void SetTarget(VkCommandBuffer commands, VkExtent2D extent);

    /// What the values of a shader are handed over as: the bytes of its
    /// block, with every value that is named at its place. Names the
    /// shader does not declare are returned in `unknown`.
    static void FillValues(
      const VK_ShaderValues &declared,
      const std::vector<MaterialValue2D> &values,
      std::vector<unsigned char> &bytes,
      std::vector<std::string> &unknown);
  };
} // neon

#endif //VK_RENDERER_2D_HPP
