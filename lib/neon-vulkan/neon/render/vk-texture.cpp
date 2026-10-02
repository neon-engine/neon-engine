#include "vk-texture.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

// kept private to this file, so that another library can carry its own copy
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace neon
{
  VK_Texture::VK_Texture(
    const std::string &texture_path,
    FileSystemContext *file_system_context,
    VK_Device *device,
    const std::shared_ptr<Logger> &logger)
  {
    _texture_path = texture_path;
    _file_system_context = file_system_context;
    _device = device;
    _logger = logger;
  }

  bool VK_Texture::Initialize()
  {
    return Initialize(VK_TextureOptions{});
  }

  bool VK_Texture::InitializeWithPixels(
    const unsigned char *pixels,
    const uint32_t width,
    const uint32_t height,
    const VK_TextureOptions &options)
  {
    if (_initialized)
    {
      _logger->Warn("Texture {} was already initialized", _texture_path);
      return true;
    }

    if (pixels == nullptr || width == 0 || height == 0)
    {
      _logger->Error("Texture {} has no pixels", _texture_path);
      return false;
    }

    return Upload(pixels, width, height, options);
  }

  bool VK_Texture::Initialize(const VK_TextureOptions &options)
  {
    if (_initialized)
    {
      _logger->Warn("Texture {} was already initialized", _texture_path);
      return true;
    }

    _logger->Info("Initializing texture from {}", _texture_path);

    std::vector<unsigned char> file_contents;
    if (!_file_system_context->ReadBytes(_texture_path, file_contents))
    {
      _logger->Error("Failed to read texture {}", _texture_path);
      return false;
    }

    return InitializeWithFile(file_contents, options);
  }

  bool VK_Texture::InitializeWithFile(const std::vector<unsigned char> &file_contents, const VK_TextureOptions &options)
  {
    if (_initialized)
    {
      _logger->Warn("Texture {} was already initialized", _texture_path);
      return true;
    }

    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);
    unsigned char *pixels = stbi_load_from_memory(
      file_contents.data(),
      static_cast<int>(file_contents.size()),
      &width,
      &height,
      &channels,
      STBI_rgb_alpha);

    if (pixels == nullptr)
    {
      _logger->Error("Failed to load texture {}", _texture_path);
      return false;
    }

    const bool uploaded = Upload(pixels, static_cast<uint32_t>(width), static_cast<uint32_t>(height), options);
    stbi_image_free(pixels);
    return uploaded;
  }

  bool VK_Texture::InitializeWithColor(
    const unsigned char red,
    const unsigned char green,
    const unsigned char blue,
    const unsigned char alpha)
  {
    const unsigned char pixel[4] = {red, green, blue, alpha};
    return Upload(pixel, 1, 1);
  }

  bool VK_Texture::Upload(
    const unsigned char *pixels,
    const uint32_t width,
    const uint32_t height,
    const VK_TextureOptions &options)
  {
    const VkDevice device = _device->Device();
    const VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4;
    const VkFormat texture_format = FormatFor(options.is_color);

    // Smaller copies of the image are made for when it is seen from afar,
    // each half the size of the one before. That needs the graphics card to
    // be able to scale this format.
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(_device->PhysicalDevice(), texture_format, &format_properties);
    const bool can_scale =
      (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) &&
      (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) &&
      (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

    const uint32_t mip_levels = can_scale && options.mip_levels
      ? static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1
      : 1;

    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    if (!_device->CreateBuffer(
      size,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      staging,
      staging_memory))
    {
      return false;
    }

    const auto release_staging = [&]
    {
      vkDestroyBuffer(device, staging, nullptr);
      vkFreeMemory(device, staging_memory, nullptr);
    };

    void *mapped = nullptr;
    if (vkMapMemory(device, staging_memory, 0, size, 0, &mapped) != VK_SUCCESS)
    {
      release_staging();
      return false;
    }
    std::memcpy(mapped, pixels, size);

    if (options.premultiply_alpha)
    {
      auto *bytes = static_cast<unsigned char *>(mapped);
      for (VkDeviceSize i = 0; i < size; i += 4)
      {
        const unsigned int alpha = bytes[i + 3];
        for (int channel = 0; channel < 3; channel++)
        {
          // rounded to the nearest, so that white at full alpha stays white
          bytes[i + channel] = static_cast<unsigned char>((bytes[i + channel] * alpha + 127) / 255);
        }
      }
    }

    vkUnmapMemory(device, staging_memory);

    if (!_device->CreateImage(
          width,
          height,
          mip_levels,
          texture_format,
          VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
          _image,
          _memory))
    {
      release_staging();
      return false;
    }

    const VkCommandBuffer commands = _device->BeginCommands();
    constexpr VkImageAspectFlags color = VK_IMAGE_ASPECT_COLOR_BIT;

    VK_Device::TransitionImage(
      commands, _image, color, 0, mip_levels,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region{};
    region.imageSubresource = {color, 0, 0, 1};
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(commands, staging, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // each level is scaled down from the one above it
    auto level_width = static_cast<int32_t>(width);
    auto level_height = static_cast<int32_t>(height);

    for (uint32_t level = 1; level < mip_levels; level++)
    {
      VK_Device::TransitionImage(
        commands, _image, color, level - 1, 1,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

      const int32_t next_width = std::max(level_width / 2, 1);
      const int32_t next_height = std::max(level_height / 2, 1);

      VkImageBlit blit{};
      blit.srcSubresource = {color, level - 1, 0, 1};
      blit.srcOffsets[1] = {level_width, level_height, 1};
      blit.dstSubresource = {color, level, 0, 1};
      blit.dstOffsets[1] = {next_width, next_height, 1};

      vkCmdBlitImage(
        commands,
        _image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blit, VK_FILTER_LINEAR);

      VK_Device::TransitionImage(
        commands, _image, color, level - 1, 1,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

      level_width = next_width;
      level_height = next_height;
    }

    VK_Device::TransitionImage(
      commands, _image, color, mip_levels - 1, 1,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    const bool uploaded = _device->EndCommands(commands);
    release_staging();

    if (!uploaded || !_device->CreateImageView(_image, texture_format, color, mip_levels, _view))
    {
      _logger->Error("Could not upload texture {}", _texture_path);
      CleanUp();
      return false;
    }

    // read smoothly, and from the side without blurring, as the surface
    // of a model is seen
    _sampling = options.repeat ? VK_Sampling::AnisotropicRepeat : VK_Sampling::AnisotropicClamp;
    _width = width;
    _height = height;
    _initialized = true;
    return true;
  }

  bool VK_Texture::InitializeWithLevels(const std::vector<ImagePixels> &levels, const VK_TextureOptions &options)
  {
    if (_initialized)
    {
      _logger->Warn("Texture {} was already initialized", _texture_path);
      return true;
    }

    if (levels.empty() || levels.front().IsEmpty())
    {
      _logger->Error("Texture {} has no pixels", _texture_path);
      return false;
    }

    const VkDevice device = _device->Device();
    const auto width = static_cast<uint32_t>(levels.front().width);
    const auto height = static_cast<uint32_t>(levels.front().height);
    const auto mip_levels = static_cast<uint32_t>(levels.size());
    const VkFormat texture_format = FormatFor(options.is_color);

    VkDeviceSize size = 0;
    for (const auto &level : levels) { size += level.pixels.size(); }

    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    if (!_device->CreateBuffer(
      size,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      staging,
      staging_memory))
    {
      return false;
    }

    const auto release_staging = [&]
    {
      vkDestroyBuffer(device, staging, nullptr);
      vkFreeMemory(device, staging_memory, nullptr);
    };

    void *mapped = nullptr;
    if (vkMapMemory(device, staging_memory, 0, size, 0, &mapped) != VK_SUCCESS)
    {
      release_staging();
      return false;
    }

    VkDeviceSize offset = 0;
    for (const auto &level : levels)
    {
      std::memcpy(static_cast<unsigned char *>(mapped) + offset, level.pixels.data(), level.pixels.size());
      offset += level.pixels.size();
    }
    vkUnmapMemory(device, staging_memory);

    if (!_device->CreateImage(
          width,
          height,
          mip_levels,
          texture_format,
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
          _image,
          _memory))
    {
      release_staging();
      return false;
    }

    const VkCommandBuffer commands = _device->BeginCommands();
    constexpr VkImageAspectFlags color = VK_IMAGE_ASPECT_COLOR_BIT;

    VK_Device::TransitionImage(
      commands, _image, color, 0, mip_levels,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    offset = 0;
    for (uint32_t level = 0; level < mip_levels; level++)
    {
      VkBufferImageCopy region{};
      region.bufferOffset = offset;
      region.imageSubresource = {color, level, 0, 1};
      region.imageExtent = {
        static_cast<uint32_t>(levels[level].width), static_cast<uint32_t>(levels[level].height), 1};

      vkCmdCopyBufferToImage(commands, staging, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
      offset += levels[level].pixels.size();
    }

    VK_Device::TransitionImage(
      commands, _image, color, 0, mip_levels,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    const bool uploaded = _device->EndCommands(commands);
    release_staging();

    if (!uploaded || !_device->CreateImageView(_image, texture_format, color, mip_levels, _view))
    {
      _logger->Error("Could not upload texture {}", _texture_path);
      CleanUp();
      return false;
    }

    // read smoothly and straight on, as an image of a user interface is
    // seen
    _sampling = options.repeat ? VK_Sampling::LinearRepeat : VK_Sampling::LinearClamp;
    _width = width;
    _height = height;
    _mip_levels = mip_levels;
    _initialized = true;
    return true;
  }

  bool VK_Texture::Update(
    const unsigned char *pixels,
    const uint32_t x,
    const uint32_t y,
    const uint32_t width,
    const uint32_t height,
    const bool premultiply_alpha)
  {
    if (!_initialized || _is_borrowed || _mip_levels != 1 || pixels == nullptr) { return false; }
    if (width == 0 || height == 0 || x + width > _width || y + height > _height) { return false; }

    const VkDevice device = _device->Device();
    const VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4;

    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    if (!_device->CreateBuffer(
      size,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      staging,
      staging_memory))
    {
      return false;
    }

    void *mapped = nullptr;
    if (vkMapMemory(device, staging_memory, 0, size, 0, &mapped) != VK_SUCCESS)
    {
      vkDestroyBuffer(device, staging, nullptr);
      vkFreeMemory(device, staging_memory, nullptr);
      return false;
    }

    std::memcpy(mapped, pixels, size);

    if (premultiply_alpha)
    {
      auto *bytes = static_cast<unsigned char *>(mapped);
      for (VkDeviceSize i = 0; i < size; i += 4)
      {
        const unsigned int alpha = bytes[i + 3];
        for (int channel = 0; channel < 3; channel++)
        {
          bytes[i + channel] = static_cast<unsigned char>((bytes[i + channel] * alpha + 127) / 255);
        }
      }
    }

    vkUnmapMemory(device, staging_memory);

    // Run at once, and not with the commands of the frame. What the frame
    // has drawn with the texture so far reads it when the frame is
    // finished, and finds what is written here.
    const VkCommandBuffer commands = _device->BeginCommands();
    constexpr VkImageAspectFlags color = VK_IMAGE_ASPECT_COLOR_BIT;

    VK_Device::TransitionImage(
      commands, _image, color, 0, 1,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region{};
    region.imageSubresource = {color, 0, 0, 1};
    region.imageOffset = {static_cast<int32_t>(x), static_cast<int32_t>(y), 0};
    region.imageExtent = {width, height, 1};
    vkCmdCopyBufferToImage(commands, staging, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VK_Device::TransitionImage(
      commands, _image, color, 0, 1,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    const bool updated = _device->EndCommands(commands);

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, staging_memory, nullptr);
    return updated;
  }

  VK_Texture VK_Texture::Borrowed(
    const VkImageView view,
    const VK_Sampling sampling,
    const uint32_t width,
    const uint32_t height)
  {
    VK_Texture texture;
    texture._view = view;
    texture._sampling = sampling;
    texture._width = width;
    texture._height = height;
    texture._is_borrowed = true;
    texture._initialized = true;
    return texture;
  }

  VkFormat VK_Texture::FormatFor(const bool is_color)
  {
    return is_color ? kColor_Format : kData_Format;
  }

  void VK_Texture::CleanUp()
  {
    if (_is_borrowed)
    {
      _view = VK_NULL_HANDLE;
      _initialized = false;
      return;
    }

    if (_device == nullptr) { return; }
    const VkDevice device = _device->Device();

    if (_view != VK_NULL_HANDLE) { vkDestroyImageView(device, _view, nullptr); }
    if (_image != VK_NULL_HANDLE) { vkDestroyImage(device, _image, nullptr); }
    if (_memory != VK_NULL_HANDLE) { vkFreeMemory(device, _memory, nullptr); }

    _view = VK_NULL_HANDLE;
    _image = VK_NULL_HANDLE;
    _memory = VK_NULL_HANDLE;
    _width = 0;
    _height = 0;
    _initialized = false;
  }
} // neon
