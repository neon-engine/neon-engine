#include "runtime-options.hpp"

#include <algorithm>
#include <charconv>
#include <string_view>

#include <neon/filesystem/file-system.hpp>

namespace neon
{
  // Helpers of RuntimeOptions, for this file alone.
  namespace
  {
    const std::string scene = "scene";
    const std::string ui = "ui";
    const std::string renderer = "renderer";
    const std::string vulkan_version = "vulkan-version";
    const std::string frames = "frames";
    const std::string screenshot = "screenshot";
    const std::string screenshot_at = "screenshot-at";
    const std::string headless_renderer = "headless-renderer";
    const std::string headless = "headless";
    const std::string time_step = "time-step";
    const std::string output_dir = "output-dir";

    const std::string vulkan = "vulkan";

    const std::string development = "Development";

    /// Reads a list of frame numbers such as `1,30,60`. Returns false when a
    /// part of it is not a whole number above zero. The frames come out in
    /// rising order, each of them once.
    bool read_frames(const std::string_view list, std::vector<std::size_t> &frames)
    {
      std::size_t start = 0;
      while (start <= list.size())
      {
        std::size_t end = list.find(',', start);
        if (end == std::string_view::npos) { end = list.size(); }

        const std::string_view part = list.substr(start, end - start);
        start = end + 1;

        std::size_t frame = 0;
        const char *last = part.data() + part.size();
        const auto [stopped_at, error] = std::from_chars(part.data(), last, frame);
        if (part.empty() || error != std::errc{} || stopped_at != last || frame == 0) { return false; }

        frames.push_back(frame);
      }

      std::ranges::sort(frames);
      const auto repeated = std::ranges::unique(frames);
      frames.erase(repeated.begin(), repeated.end());
      return true;
    }

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
      .name = scene,
      .value_name = "PATH",
      .description = "Scene to start with, for example assets://scenes/demo.scene.yml"
    });

    command_line.Add({
      .name = ui,
      .value_name = "PATH",
      .description = "User interface to show on top, for example assets://ui/hud.ui.yml"
    });

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

    command_line.Add({
      .name = frames,
      .value_name = "N",
      .description = "Stop after N frames",
      .group = development
    });

    command_line.Add({
      .name = screenshot,
      .value_name = "PATH",
      .description = "Save the last frame as a PNG image, for example output://frame.png. "
                     "Needs --frames or --screenshot-at",
      .group = development
    });

    command_line.Add({
      .name = screenshot_at,
      .value_name = "N[,N...]",
      .description = "Save these frames instead of the last one, counted from 1. Each file gets "
                     "its frame in its name, as in frame-0030.png. Needs --screenshot",
      .group = development
    });

    command_line.Add({
      .name = output_dir,
      .value_name = "DIR",
      .description = "Folder of this machine that output:// stands for. Created when missing",
      .group = development
    });

    command_line.Add({
      .name = time_step,
      .value_name = "SECONDS",
      .description = "Advance the game by this much time in every frame, for example 0.016667, "
                     "so that a run gives the same frames every time",
      .group = development
    });

    command_line.Add({
      .name = headless_renderer,
      .description = "Render without a window, for screenshots and checks on a machine with no display",
      .group = development
    });

    // declared so that it is understood and refused with a pointer to the
    // right option, instead of being an unknown one. Applying it is for the
    // dedicated server, #144
    command_line.Add({
      .name = headless,
      .description = "Run as a dedicated server. Not available yet, see --headless-renderer",
      .group = development
    });
  }

  bool RuntimeOptions::Apply(
    const CommandLineContext &command_line,
    SettingsConfig &settings,
    std::string &error)
  {
    if (command_line.IsSet(scene))
    {
      settings.scene_path = command_line.GetValue(scene);
    }

    if (command_line.IsSet(ui))
    {
      settings.ui_path = command_line.GetValue(ui);
    }

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

    if (command_line.IsSet(headless))
    {
      error = "'--" + headless + "' is for a dedicated server, which does not exist yet (#144). "
              "To render without a window, use '--" + headless_renderer + "'";
      return false;
    }

    settings.headless_renderer = command_line.IsSet(headless_renderer);

    if (command_line.IsSet(output_dir))
    {
      settings.output_directory = command_line.GetValue(output_dir);
    }

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

    if (command_line.IsSet(time_step))
    {
      double seconds = 0.0;
      if (!command_line.GetNumber(time_step, seconds) || seconds <= 0.0)
      {
        error = "Option '--" + time_step + "' needs a number of seconds above zero, such as 0.016667";
        return false;
      }
      settings.time_step = seconds;
    }

    if (command_line.IsSet(screenshot_at))
    {
      if (!command_line.IsSet(screenshot))
      {
        error = "Option '--" + screenshot_at + "' needs '--" + screenshot + "'";
        return false;
      }

      // read afresh, so that applying the same command line twice, as the
      // application does around its settings files, gives the same frames
      settings.screenshot_frames.clear();
      if (!read_frames(command_line.GetValue(screenshot_at), settings.screenshot_frames))
      {
        error = "Option '--" + screenshot_at + "' needs whole numbers above zero, separated by commas";
        return false;
      }

      const std::size_t highest = settings.screenshot_frames.back();
      if (settings.max_frames == 0)
      {
        // the run is as long as the screenshots need it to be
        settings.max_frames = highest;
      } else if (highest > settings.max_frames)
      {
        error = "Frame " + std::to_string(highest) + " of '--" + screenshot_at + "' is never reached, '--" +
                frames + "' stops after " + std::to_string(settings.max_frames);
        return false;
      }
    }

    if (command_line.IsSet(screenshot))
    {
      if (settings.max_frames == 0)
      {
        error = "Option '--" + screenshot + "' needs '--" + frames + "' or '--" + screenshot_at + "'";
        return false;
      }
      settings.screenshot_path = command_line.GetValue(screenshot);

      // said here, before anything is rendered, and not only when the frame
      // is written
      if (settings.screenshot_path.starts_with(FileSystem::output_scheme) && settings.output_directory.empty())
      {
        error = "'" + settings.screenshot_path + "' needs '--" + output_dir + "', which says where " +
                std::string(FileSystem::output_scheme) + " is";
        return false;
      }
    }

    return true;
  }
} // neon
