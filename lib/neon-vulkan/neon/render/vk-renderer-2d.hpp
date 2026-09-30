#ifndef VK_RENDERER_2D_HPP
#define VK_RENDERER_2D_HPP

#include <memory>
#include <string>
#include <vector>
#include <neon/common/data-buffer.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/render/render-2d-context.hpp>

#include "vk-device.hpp"
#include "vk-shader.hpp"
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
  };
} // neon

#endif //VK_RENDERER_2D_HPP
