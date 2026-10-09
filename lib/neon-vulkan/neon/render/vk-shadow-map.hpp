#ifndef VK_SHADOW_MAP_HPP
#define VK_SHADOW_MAP_HPP

#include <array>
#include <memory>
#include <neon/logging/logger.hpp>

#include "vk-device.hpp"
#include "vk-shader-data.hpp"

namespace neon
{
  /// The shadow map of the direction light: the depth of the scene as the
  /// light sees it, drawn in a pass of its own before the scene, and
  /// compared against by the lit shaders to tell what the light reaches.
  /// It is one image of as many layers as there are cascades at most, a
  /// layer a cascade, each drawn in a pass of its own and read together
  /// as an array. It owns its image, its views, the render pass that
  /// writes a layer, and a framebuffer a layer; the sampler it is
  /// compared through is one of VK_Samplers.
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
    uint32_t _size = 0;

    // a layer is drawn through a view and a framebuffer of its own
    std::array<VkImageView, kMax_Shadow_Cascades> _layer_views{};
    std::array<VkFramebuffer, kMax_Shadow_Cascades> _framebuffers{};

    bool CreateRenderPass();

    /// Makes the image of `size` texels a side, its views, and a
    /// framebuffer a layer, for the render pass there is, and leaves the
    /// map all lit.
    bool CreateImage(uint32_t size);

    /// Destroys the image, its views, and its framebuffers, and keeps the
    /// render pass, which the pipelines of the pass were made for.
    void DestroyImage();

    /// Leaves the map all lit, and in the layout a shader reads it in.
    bool ClearToLit() const;

  public:
    /// Depth alone, in whole floats: the map is compared, not shown.
    static constexpr VkFormat kFormat = VK_FORMAT_D32_SFLOAT;

    /// The layout the map is in between two passes, which is what the
    /// descriptor of every material says.
    static constexpr VkImageLayout kRead_Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    /// Makes the image of `size` texels along each side, every layer, its
    /// render pass, and its framebuffers, and leaves the map all lit.
    /// Returns false when the graphics card refuses.
    bool Initialize(VK_Device *device, uint32_t size, const std::shared_ptr<Logger> &logger);

    /// Makes the image again at another size, as the frame is made again
    /// when the window changes its size: the render pass stays, so the
    /// pipelines of the pass do, and every set that reads the map has to
    /// be written again with View(). Nothing may be reading the map. The
    /// map is left all lit. Returns false when the graphics card refuses,
    /// and the map is gone then.
    bool Resize(uint32_t size);

    void CleanUp();

    [[nodiscard]] bool IsReady() const { return _framebuffers[0] != VK_NULL_HANDLE; }

    /// The map is square, this many texels along each side, every layer.
    [[nodiscard]] uint32_t Size() const { return _size; }

    /// Begins the pass that draws one layer of the map, the cascade at
    /// `layer`: clears it to the far depth and sets the viewport to the
    /// whole map. Every caster is drawn after it.
    void Begin(VkCommandBuffer commands, uint32_t layer) const;

    /// Ends the pass. The map is then ready to be read by the shaders of
    /// what is drawn after the commands.
    void End(VkCommandBuffer commands) const;

    /// What the pipelines of the pass are made for.
    [[nodiscard]] VkRenderPass RenderPass() const { return _render_pass; }

    /// What the lit shaders compare against: every layer, as an array.
    [[nodiscard]] VkImageView View() const { return _view; }
  };
} // neon

#endif //VK_SHADOW_MAP_HPP
