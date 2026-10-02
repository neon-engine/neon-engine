#ifndef VK_RENDER_TARGET_HPP
#define VK_RENDER_TARGET_HPP

#include <memory>
#include <string>
#include <neon/logging/logger.hpp>

#include "vk-device.hpp"

namespace neon
{
  /// An image that is drawn to in place of the frame, and read as a texture
  /// afterwards: by a user interface that shows it, and by a model in the
  /// world that carries it on its surface.
  ///
  /// It has the format of the frame, so that everything that draws into
  /// the frame draws into it as well, through a canvas of its own: models
  /// of a scene into a scene image, which is resolved into the target, and
  /// what is drawn in two dimensions on top. It has smaller copies, which are made again whenever it was drawn to, since a surface
  /// in the world is seen from afar and from the side.
  ///
  /// Between two frames the image is ready to be read by a shader.
  // ReSharper disable once CppInconsistentNaming
  class VK_RenderTarget
  {
    std::string _name;
    VK_Device *_device = nullptr;
    std::shared_ptr<Logger> _logger;

    VkExtent2D _extent{};
    uint32_t _mip_levels = 1;

    VkImage _color_image = VK_NULL_HANDLE;
    VkDeviceMemory _color_memory = VK_NULL_HANDLE;

    // What is drawn to, which is the image without its smaller copies, and
    // what is read, which is the image with them. The image holds sRGB
    // colours as bytes. A model reads them as linear light, and what is
    // drawn in two dimensions reads them as the bytes they are.
    VkImageView _attachment_view = VK_NULL_HANDLE;
    VkImageView _view = VK_NULL_HANDLE;
    VkImageView _bytes_view = VK_NULL_HANDLE;

    VkFramebuffer _framebuffer = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;

  public:
    /// The largest target, along each side.
    static constexpr int kMax_Size = 8192;

    /// What the texture of a model is called that shows a target:
    /// `surface://` and the name of the target.
    static constexpr const char *kScheme = "surface://";

    VK_RenderTarget() = default;

    VK_RenderTarget(const std::string &name, VK_Device *device, const std::shared_ptr<Logger> &logger);

    /// Creates the image for a render pass that writes it, of the format
    /// given, which holds sRGB colours as bytes. The image is black and
    /// see-through until it is drawn to.
    bool Initialize(
      uint32_t width,
      uint32_t height,
      VkRenderPass render_pass,
      VkFormat color_format);

    void CleanUp();

    /// Makes the smaller copies from what was drawn, and leaves the image
    /// ready to be read. Call it behind the render pass that drew to the
    /// target, which leaves the image ready to be copied from.
    void Finish(VkCommandBuffer commands) const;

    /// The format a model reads a target of `format` through, so that its
    /// sRGB colours are turned into linear light: the sRGB format of the
    /// same bytes.
    [[nodiscard]] static VkFormat SampledFormatOf(VkFormat format);

    /// How many smaller copies an image of a size has, itself included.
    [[nodiscard]] static uint32_t LevelsFor(uint32_t width, uint32_t height);

    /// The name of the target a texture of a model asks for, or empty for
    /// a texture that is a file.
    [[nodiscard]] static std::string NameOf(const std::string &texture_path);

    [[nodiscard]] const std::string &Name() const { return _name; }
    [[nodiscard]] VkExtent2D Extent() const { return _extent; }
    [[nodiscard]] VkFramebuffer Framebuffer() const { return _framebuffer; }
    /// What a model reads, as linear light.
    [[nodiscard]] VkImageView View() const { return _view; }

    /// What is drawn in two dimensions reads, as the bytes they are.
    [[nodiscard]] VkImageView BytesView() const { return _bytes_view; }
    [[nodiscard]] VkSampler Sampler() const { return _sampler; }
  };
} // neon

#endif //VK_RENDER_TARGET_HPP
