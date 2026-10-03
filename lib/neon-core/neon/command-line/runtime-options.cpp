#include "runtime-options.hpp"

#include <string_view>

namespace neon
{
  // Helpers of RuntimeOptions, for this file alone.
  namespace
  {
    const std::string renderer = "renderer";
    const std::string vulkan_version = "vulkan-version";

    const std::string vulkan = "vulkan";

    /// Reads a version of Vulkan, such as `1.3`. Returns false when it is
    /// not a version, or not of Vulkan 1.
    bool read_vulkan_version(const std::string_view text, ApiVersion &version)
    {
      ApiVersion read;
      if (!ApiVersion::Parse(text, read) || read.major != 1) { return false; }

      version = read;
      return true;
    }
  }

  void RuntimeOptions::Register(CommandLine &command_line)
  {
    command_line.Add({
      .name = renderer,
      .value_name = "NAME",
      .description = "Renderer to draw with",
      .allowed_values = {vulkan},
      .default_value = vulkan
    });

    command_line.Add({
      .name = vulkan_version,
      .value_name = "1.N",
      .description = "Highest version of Vulkan to render with, for example 1.2",
      .default_value = SettingsConfig::default_vulkan_version.ToString()
    });
  }

  bool RuntimeOptions::Apply(
    const CommandLineContext &command_line,
    SettingsConfig &settings,
    std::string &error)
  {
    // the parser only lets the accepted values through
    if (command_line.GetValue(renderer) == vulkan)
    {
      settings.selected_api = RenderingApi::Vulkan;
    }

    if (command_line.IsSet(vulkan_version) &&
        !read_vulkan_version(command_line.GetValue(vulkan_version), settings.vulkan_version))
    {
      error = "Option '--" + vulkan_version + "' needs a version of Vulkan 1, such as 1.3";
      return false;
    }

    return true;
  }
} // neon
