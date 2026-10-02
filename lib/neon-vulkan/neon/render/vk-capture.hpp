#ifndef VK_CAPTURE_HPP
#define VK_CAPTURE_HPP

#include <memory>
#include <string>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

#include "vk-device.hpp"

namespace neon
{
  /// Reads a finished frame back from the graphics card, and writes it as
  /// a PNG file. The bytes of the frame are sRGB already, and go into the
  /// file as they are.
  // ReSharper disable once CppInconsistentNaming
  class VK_Capture
  {
    VK_Device *_device = nullptr;
    FileSystemContext *_file_system_context = nullptr;
    std::shared_ptr<Logger> _logger;

  public:
    VK_Capture() = default;

    VK_Capture(VK_Device *device, FileSystemContext *file_system_context, const std::shared_ptr<Logger> &logger);

    /// Writes `frame`, an image of four bytes a pixel that is left ready to
    /// be copied from, to the virtual path given. Waits for the copy.
    [[nodiscard]] bool Write(VkImage frame, VkExtent2D extent, const std::string &path) const;
  };
} // neon

#endif //VK_CAPTURE_HPP
