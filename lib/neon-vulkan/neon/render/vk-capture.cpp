#include "vk-capture.hpp"

#include <vector>

// kept private to this file, so that another library can carry its own copy
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace neon
{
  // Helpers of VK_Capture, for this file alone.
  namespace
  {
    void append_png_bytes(void *context, void *data, const int size)
    {
      auto *out = static_cast<std::vector<unsigned char> *>(context);
      const auto *bytes = static_cast<const unsigned char *>(data);
      out->insert(out->end(), bytes, bytes + size);
    }
  }

  VK_Capture::VK_Capture(
    VK_Device *device,
    FileSystemContext *file_system_context,
    const std::shared_ptr<Logger> &logger)
    : _device(device), _file_system_context(file_system_context), _logger(logger) {}

  bool VK_Capture::Write(const VkImage frame, const VkExtent2D extent, const std::string &path) const
  {
    const VkDevice device = _device->Device();
    const VkDeviceSize size = static_cast<VkDeviceSize>(extent.width) * extent.height * 4;

    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    if (!_device->CreateBuffer(
      size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      buffer,
      memory))
    {
      return false;
    }

    // a finished frame is left ready to be copied from
    const VkCommandBuffer commands = _device->BeginCommands();

    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {extent.width, extent.height, 1};
    vkCmdCopyImageToBuffer(commands, frame, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

    bool written = false;
    void *pixels = nullptr;

    if (_device->EndCommands(commands) && vkMapMemory(device, memory, 0, size, 0, &pixels) == VK_SUCCESS)
    {
      std::vector<unsigned char> png;
      const int encoded = stbi_write_png_to_func(
        append_png_bytes,
        &png,
        static_cast<int>(extent.width),
        static_cast<int>(extent.height),
        4,
        pixels,
        static_cast<int>(extent.width) * 4);

      vkUnmapMemory(device, memory);

      written = encoded != 0 && _file_system_context->WriteBytes(path, png);
    }

    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);

    if (written)
    {
      _logger->Info("Saved the frame to {}", path);
    } else
    {
      _logger->Error("Could not save the frame to {}", path);
    }
    return written;
  }
} // neon
