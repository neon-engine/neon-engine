#ifndef VK_TEXTURE_HPP
#define VK_TEXTURE_HPP

#include <string>
#include <neon/filesystem/file-system-context.hpp>

#include "vk-device.hpp"

namespace neon
{
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

    bool Upload(const unsigned char *pixels, uint32_t width, uint32_t height);

  public:
    VK_Texture() = default;

    VK_Texture(
      const std::string &texture_path,
      FileSystemContext *file_system_context,
      VK_Device *device,
      const std::shared_ptr<Logger> &logger);

    /// Loads the image the texture was created with.
    bool Initialize();

    /// Makes the texture a single colour. Used where a material names no
    /// texture, so that shaders always have something to read.
    bool InitializeWithColor(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);

    void CleanUp();

    [[nodiscard]] VkImageView View() const { return _view; }
    [[nodiscard]] VkSampler Sampler() const { return _sampler; }
  };
} // neon

#endif //VK_TEXTURE_HPP
