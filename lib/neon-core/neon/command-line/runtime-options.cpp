#include "runtime-options.hpp"

namespace neon
{
  namespace
  {
    const std::string renderer = "renderer";
    const std::string frames = "frames";
    const std::string screenshot = "screenshot";
    const std::string headless = "headless";

    const std::string vulkan = "vulkan";

    const std::string development = "Development";
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
      .name = frames,
      .value_name = "N",
      .description = "Stop after N frames",
      .group = development
    });

    command_line.Add({
      .name = screenshot,
      .value_name = "PATH",
      .description = "Save the last frame as a PNG image before stopping, "
                     "for example user://screenshots/frame.png. Needs --frames",
      .group = development
    });

    command_line.Add({
      .name = headless,
      .description = "Run without a window",
      .group = development
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

    settings.headless = command_line.IsSet(headless);

    if (command_line.IsSet(frames))
    {
      long count = 0;
      if (!command_line.GetInteger(frames, count) || count <= 0)
      {
        error = "Option '--" + frames + "' needs a whole number above zero";
        return false;
      }
      settings.max_frames = static_cast<std::size_t>(count);
    }

    if (command_line.IsSet(screenshot))
    {
      if (settings.max_frames == 0)
      {
        error = "Option '--" + screenshot + "' needs '--" + frames + "'";
        return false;
      }
      settings.screenshot_path = command_line.GetValue(screenshot);
    }

    return true;
  }
} // neon
