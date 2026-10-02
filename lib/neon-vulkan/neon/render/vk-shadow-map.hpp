#ifndef VK_SHADOW_MAP_HPP
#define VK_SHADOW_MAP_HPP

#include <memory>
#include <neon/logging/logger.hpp>

#include "vk-device.hpp"

namespace neon
{
  /// The shadow map of the direction light: the depth of the scene as the
  /// light sees it, drawn in a pass of its own before the scene, and
  /// compared against by the lit shaders to tell what the light reaches.
  /// It owns its image, its view, the render pass that writes it, and
  /// the framebuffer; the sampler it is compared through is one of
  /// VK_Samplers.
  ///
  /// Between two passes the map is ready to be read by a shader, from the
  /// moment it is made: a frame in which nothing casts reads it as it was
  /// left, which is all lit until a pass has drawn into it.
  // ReSharper disable once CppInconsistentNaming
  class VK_ShadowMap
  {
    VK_Device *_device = nullptr;
    std::shared_ptr<Logger> _logger;

    VkImage _image = VK_NULL_HANDLE;
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    VkImageView _view = VK_NULL_HANDLE;
    VkRenderPass _render_pass = VK_NULL_HANDLE;
    VkFramebuffer _framebuffer = VK_NULL_HANDLE;

    bool CreateRenderPass();

    /// Leaves the map all lit, and in the layout a shader reads it in.
    bool ClearToLit() const;

  public:
    /// Depth alone, in whole floats: the map is compared, not shown.
    static constexpr VkFormat kFormat = VK_FORMAT_D32_SFLOAT;

    /// The map is square, this many texels along each side.
    static constexpr uint32_t kSize = 2048;

    /// The layout the map is in between two passes, which is what the
    /// descriptor of every material says.
    static constexpr VkImageLayout kRead_Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    /// Makes the image, its render pass, and its framebuffer, and leaves
    /// the map all lit. Returns false when the graphics card refuses.
    bool Initialize(VK_Device *device, const std::shared_ptr<Logger> &logger);

    void CleanUp();

    [[nodiscard]] bool IsReady() const { return _framebuffer != VK_NULL_HANDLE; }

    /// Begins the pass that draws the map: clears it to the far depth and
    /// sets the viewport to the whole map. Every caster is drawn after it.
    void Begin(VkCommandBuffer commands) const;

    /// Ends the pass. The map is then ready to be read by the shaders of
    /// what is drawn after the commands.
    void End(VkCommandBuffer commands) const;

    /// What the pipelines of the pass are made for.
    [[nodiscard]] VkRenderPass RenderPass() const { return _render_pass; }

    /// What the lit shaders compare against.
    [[nodiscard]] VkImageView View() const { return _view; }
  };
} // neon

#endif //VK_SHADOW_MAP_HPP
