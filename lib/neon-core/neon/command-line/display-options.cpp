#include "display-options.hpp"

#include <charconv>
#include <string_view>

namespace neon
{
  namespace
  {
    const std::string window_size = "window-size";
    const std::string render_scale = "render-scale";
    const std::string ui_scale = "ui-scale";
    const std::string input = "input";
    const std::string input_script = "input-script";

    // declared by the options of the runtime
    const std::string headless_renderer = "headless-renderer";

    const std::string display = "Display";
    const std::string development = "Development";

    /// Reads a size such as `1280x720`.
    bool read_size(const std::string_view text, int &width, int &height)
    {
      const std::size_t middle = text.find_first_of("xX");
      if (middle == std::string_view::npos) { return false; }

      const std::string_view first = text.substr(0, middle);
      const std::string_view second = text.substr(middle + 1);

      int read_width = 0;
      int read_height = 0;

      const auto [width_end, width_error] = std::from_chars(first.data(), first.data() + first.size(), read_width);
      const auto [height_end, height_error] =
        std::from_chars(second.data(), second.data() + second.size(), read_height);

      if (first.empty() || second.empty() || width_error != std::errc{} || height_error != std::errc{} ||
          width_end != first.data() + first.size() || height_end != second.data() + second.size() ||
          read_width <= 0 || read_height <= 0)
      {
        return false;
      }

      width = read_width;
      height = read_height;
      return true;
    }
  }

  void DisplayOptions::Register(CommandLine &command_line)
  {
    command_line.Add({
      .name = window_size,
      .value_name = "WxH",
      .description = "Size of the window in points, for example 1280x720. Shows a window of that size in "
                     "place of one that covers the display",
      .group = display
    });

    command_line.Add({
      .name = render_scale,
      .value_name = "NUMBER",
      .description = "Pixels that are drawn for each point, for example 2 for what a display of high "
                     "density shows. Needs --headless-renderer, a window takes the density of its display",
      .group = display
    });

    command_line.Add({
      .name = ui_scale,
      .value_name = "NUMBER",
      .description = "Makes the user interface larger or smaller, for example 1.5",
      .group = display
    });

    command_line.Add({
      .name = input,
      .value_name = "SCRIPT",
      .description = "Input in place of devices, for example \"1: pointer 640 360; 2: click\". "
                     "Needs --headless-renderer",
      .group = development
    });

    command_line.Add({
      .name = input_script,
      .value_name = "PATH",
      .description = "The same from a file, for example assets://input/menu.input. Needs --headless-renderer",
      .group = development
    });
  }

  bool DisplayOptions::Apply(
    const CommandLineContext &command_line,
    SettingsConfig &settings,
    std::string &error)
  {
    if (command_line.IsSet(window_size))
    {
      if (!read_size(command_line.GetValue(window_size), settings.width, settings.height))
      {
        error = "Option '--" + window_size + "' needs a width and a height above zero, such as 1280x720";
        return false;
      }

      // a window that covers the display has the size of the display
      settings.window_mode = WindowMode::Windowed;
    }

    const bool is_headless_renderer = command_line.IsSet(headless_renderer);

    if (command_line.IsSet(render_scale))
    {
      double scale = 0.0;
      if (!command_line.GetNumber(render_scale, scale) || scale < 0.25 || scale > 8.0)
      {
        error = "Option '--" + render_scale + "' needs a number from 0.25 to 8, such as 2";
        return false;
      }

      if (!is_headless_renderer)
      {
        error = "Option '--" + render_scale + "' needs '--" + headless_renderer +
                "'. A window takes the density of its display";
        return false;
      }

      settings.render_scale = scale;
    }

    if (command_line.IsSet(ui_scale))
    {
      double scale = 0.0;
      if (!command_line.GetNumber(ui_scale, scale) || scale < 0.25 || scale > 8.0)
      {
        error = "Option '--" + ui_scale + "' needs a number from 0.25 to 8, such as 1.5";
        return false;
      }
      settings.ui_scale = scale;
    }

    if (command_line.IsSet(input) && command_line.IsSet(input_script))
    {
      error = "Option '--" + input + "' and '--" + input_script + "' cannot be given together";
      return false;
    }

    for (const auto &name : {input, input_script})
    {
      if (command_line.IsSet(name) && !is_headless_renderer)
      {
        error = "Option '--" + name + "' needs '--" + headless_renderer +
                "'. A window takes its input from devices";
        return false;
      }
    }

    if (command_line.IsSet(input)) { settings.input_script = command_line.GetValue(input); }
    if (command_line.IsSet(input_script)) { settings.input_script_path = command_line.GetValue(input_script); }

    return true;
  }
} // neon
