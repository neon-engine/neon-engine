#ifndef VK_SURFACE_FORMAT_HPP
#define VK_SURFACE_FORMAT_HPP

#include <vector>
#include <volk.h>

namespace neon
{
  /// Whether a format of eight bits a channel turns linear light into sRGB
  /// when it is written.
  [[nodiscard]] bool IsSrgbFormat(VkFormat format);

  /// The format of the window to show frames in, from those it offers. The
  /// frame already holds sRGB colors as bytes, and is copied as it is, so
  /// plain bytes come first: a format that converts to sRGB would convert
  /// them a second time. Whatever is offered first comes next.
  ///
  /// `formats` must not be empty.
  [[nodiscard]] VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
} // neon

#endif //VK_SURFACE_FORMAT_HPP
