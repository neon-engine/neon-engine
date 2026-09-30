#ifndef VK_TEXTURE_HPP
#define VK_TEXTURE_HPP

#include <string>
#include <neon/filesystem/file-system-context.hpp>

#include "vk-device.hpp"

namespace neon
{
  /// How a texture is kept and read. What is left as it is suits the
  /// surface of a model.
  // ReSharper disable once CppInconsistentNaming
  struct VK_TextureOptions
  {
    /// Smaller copies for when the image is seen from afar.
    bool mip_levels = true;

    /// Whether the image starts again past its edge. Otherwise its edge is
    /// drawn on.
    bool repeat = true;

    /// Multiplies alpha into the colours before the image is kept, which
    /// is what blending by ONE and ONE_MINUS_SRC_ALPHA expects. A pixel
    /// that is see-through then adds no colour of its own to its
    /// neighbours when the image is scaled.
    bool premultiply_alpha = false;
  };

  // ReSharper disable once CppInconsistentNaming
  class VK_Texture
  {
    std::string _texture_path;
    VK_Device *_device = nullptr;
    FileSystemContext *_file_system_context = nullptr;
    std::shared_ptr<Logger> _logger;
    bool _initialized = false;

    VkImage _image = VK_NULL_HANDLE;
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    VkImageView _view = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;
    uint32_t _width = 0;
    uint32_t _height = 0;

    bool Upload(
      const unsigned char *pixels,
      uint32_t width,
      uint32_t height,
      const VK_TextureOptions &options = {});

  public:
    VK_Texture() = default;

    VK_Texture(
      const std::string &texture_path,
      FileSystemContext *file_system_context,
      VK_Device *device,
      const std::shared_ptr<Logger> &logger);

    /// Loads the image the texture was created with.
    bool Initialize();

    bool Initialize(const VK_TextureOptions &options);

    /// Makes the texture from pixels in memory: red, green, blue, and alpha
    /// for each, row after row from the top.
    bool InitializeWithPixels(
      const unsigned char *pixels,
      uint32_t width,
      uint32_t height,
      const VK_TextureOptions &options);

    /// Makes the texture a single colour. Used where a material names no
    /// texture, so that shaders always have something to read.
    bool InitializeWithColor(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);

    void CleanUp();

    [[nodiscard]] VkImageView View() const { return _view; }
    [[nodiscard]] VkSampler Sampler() const { return _sampler; }
    [[nodiscard]] uint32_t Width() const { return _width; }
    [[nodiscard]] uint32_t Height() const { return _height; }
  };
} // neon

#endif //VK_TEXTURE_HPP
