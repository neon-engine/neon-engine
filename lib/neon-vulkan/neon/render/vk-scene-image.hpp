#ifndef VK_SCENE_IMAGE_HPP
#define VK_SCENE_IMAGE_HPP

#include "vk-device.hpp"

namespace neon
{
  /// The image a scene is lit in, with its depth. It holds linear light in
  /// floating point numbers, so that see-through surfaces blend as light
  /// does, and light brighter than white is kept until it is resolved into
  /// the colours of the image that is shown.
  // ReSharper disable once CppInconsistentNaming
  class VK_SceneImage
  {
    VK_Device *_device = nullptr;
    VkExtent2D _extent{};

    VkImage _color_image = VK_NULL_HANDLE;
    VkDeviceMemory _color_memory = VK_NULL_HANDLE;
    VkImageView _color_view = VK_NULL_HANDLE;
    VkImage _depth_image = VK_NULL_HANDLE;
    VkDeviceMemory _depth_memory = VK_NULL_HANDLE;
    VkImageView _depth_view = VK_NULL_HANDLE;
    VkFramebuffer _framebuffer = VK_NULL_HANDLE;

  public:
    /// Half floats: linear light with room above white, at half the memory
    /// of whole ones.
    static constexpr VkFormat kFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    /// Creates the images for the render pass of the scene, which has the
    /// format above and `depth_format`.
    bool Initialize(
      VK_Device *device,
      uint32_t width,
      uint32_t height,
      VkRenderPass scene_pass,
      VkFormat depth_format);

    void CleanUp();

    [[nodiscard]] bool IsReady() const { return _framebuffer != VK_NULL_HANDLE; }
    [[nodiscard]] VkExtent2D Extent() const { return _extent; }
    [[nodiscard]] VkFramebuffer Framebuffer() const { return _framebuffer; }

    /// What the resolve step reads.
    [[nodiscard]] VkImageView View() const { return _color_view; }
  };
} // neon

#endif //VK_SCENE_IMAGE_HPP
