#include "display-options.hpp"

#include <format>

#include <cmath>
#include <neon/render/anisotropy.hpp>
#include <neon/render/graphics-presets.hpp>
#include <neon/render/shadow-filter.hpp>
#include <neon/render/shadow-map-size.hpp>
#include <neon/render/target-quality.hpp>
#include <neon/render/texture-scale.hpp>
#include <neon/window/frame-limit.hpp>
#include <charconv>
#include <string_view>

namespace neon
{
  // Helpers of DisplayOptions, for this file alone.
  namespace
  {
    const std::string window_size = "window-size";
    const std::string window_mode = "window-mode";
    const std::string vsync = "vsync";
    const std::string max_fps = "max-fps";
    const std::string quality = "quality";
    const std::string anisotropy = "anisotropy";
    const std::string texture_scale = "texture-scale";
    const std::string target_scale = "target-scale";
    const std::string target_mipmaps = "target-mipmaps";
    const std::string shadows = "shadows";
    const std::string shadow_map_size = "shadow-map-size";
    const std::string shadow_filter = "shadow-filter";
    const std::string shadow_distance = "shadow-distance";
    const std::string shadow_cascades = "shadow-cascades";
    const std::string ui_scale = "ui-scale";

    const std::string windowed = "windowed";
    const std::string borderless = "borderless";
    const std::string fullscreen = "fullscreen";

    const std::string display = "Display";

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
                     "place of one that covers the display, unless --window-mode says otherwise",
      .group = display
    });

    // the values are checked in Apply, so that the usage column of the
    // help stays narrow enough to read
    command_line.Add({
      .name = window_mode,
      .value_name = "MODE",
      .description = "How the window is shown: windowed, borderless, or fullscreen, over window.mode of the settings",
      .group = display
    });

    command_line.Add({
      .name = vsync,
      .value_name = "on|off",
      .description = "Whether a frame waits for the screen before it is shown, over rendering.vsync of the settings",
      .group = display
    });

    command_line.Add({
      .name = max_fps,
      .value_name = "NUMBER",
      .description = "Most frames a second, from 30 to 300, or 0 for as many as can be drawn, over "
                     "rendering.max_fps of the settings",
      .group = display
    });

    command_line.Add({
      .name = quality,
      .value_name = "NAME",
      .description = "Preset of the graphics that cost frame time: low, medium, high, ultra, or custom for none, "
                     "over rendering.quality of the settings. The options below it change one of its values",
      .group = display
    });

    command_line.Add({
      .name = anisotropy,
      .value_name = "NUMBER",
      .description = "Samples a texture is read with where it is seen from the side: 1 for none, 2, 4, 8, or 16, "
                     "over rendering.anisotropy of the settings",
      .group = display
    });

    command_line.Add({
      .name = texture_scale,
      .value_name = "NUMBER",
      .description = "Size textures read from files are kept at: 1, 0.5, 0.25, or 0.125 of their size, over "
                     "rendering.texture_scale of the settings",
      .group = display
    });

    command_line.Add({
      .name = target_scale,
      .value_name = "NUMBER",
      .description = "Size what a camera draws into is made at: 1, 0.5, or 0.25 of what it asks for, over "
                     "rendering.target_scale of the settings",
      .group = display
    });

    command_line.Add({
      .name = target_mipmaps,
      .value_name = "NUMBER",
      .description = "Most levels of smaller copies a render target has: 0 for as many as its size allows, 1 for "
                     "none, up to 16, over rendering.target_mipmaps of the settings",
      .group = display
    });

    command_line.Add({
      .name = shadows,
      .value_name = "on|off",
      .description = "Whether the direction light casts shadows at all, over rendering.shadows of the settings; "
                     "off skips the shadow pass",
      .group = display
    });

    command_line.Add({
      .name = shadow_map_size,
      .value_name = "NUMBER",
      .description = "Texels the shadow map has along each side: 512, 1024, 2048, or 4096, over "
                     "rendering.shadow_map_size of the settings",
      .group = display
    });

    command_line.Add({
      .name = shadow_filter,
      .value_name = "NAME",
      .description = "How the shadow map is compared against: none for one comparison, pcf for nine averaged, "
                     "over rendering.shadow_filter of the settings",
      .group = display
    });

    command_line.Add({
      .name = shadow_distance,
      .value_name = "METERS",
      .description = "How far from the camera the shadows reach, above 0, over rendering.shadow_distance of "
                     "the settings",
      .group = display
    });

    command_line.Add({
      .name = shadow_cascades,
      .value_name = "NUMBER",
      .description = "How many cascades the shadow map has, 1 to 4, over rendering.shadow_cascades of the settings",
      .group = display
    });

    command_line.Add({
      .name = ui_scale,
      .value_name = "NUMBER",
      .description = "Makes the user interface larger or smaller, for example 1.5",
      .group = display
    });
  }

  bool DisplayOptions::Apply(
    const CommandLineContext &command_line,
    SettingsConfig &settings,
    std::string &error)
  {
    return Apply(command_line, settings, error, true);
  }

  bool DisplayOptions::ApplyExceptTheQuality(
    const CommandLineContext &command_line,
    SettingsConfig &settings,
    std::string &error)
  {
    return Apply(command_line, settings, error, false);
  }

  bool DisplayOptions::Apply(
    const CommandLineContext &command_line,
    SettingsConfig &settings,
    std::string &error,
    const bool with_quality)
  {
    _warnings.clear();

    /// Whether an option a preset decides may be taken: while a preset
    /// holds it is left out, and said once.
    const auto decided = [&](const std::string &option)
    {
      if (settings.quality == GraphicsPresets::kCustom) { return true; }
      _warnings.push_back(std::format(
        "Option '--{}' is set by the preset {}, and is left out unless '--{}' is custom",
        option, settings.quality, quality));
      return false;
    };

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

    if (command_line.IsSet(window_mode))
    {
      const std::string mode = command_line.GetValue(window_mode);
      if (mode == windowed)
      {
        settings.window_mode = WindowMode::Windowed;
      } else if (mode == borderless)
      {
        settings.window_mode = WindowMode::Borderless;
      } else if (mode == fullscreen)
      {
        settings.window_mode = WindowMode::Fullscreen;
      } else
      {
        error = "Option '--" + window_mode + "' needs " + windowed + ", " + borderless + ", or " + fullscreen;
        return false;
      }
    }

    if (command_line.IsSet(vsync))
    {
      const std::string wanted = command_line.GetValue(vsync);
      if (wanted != "on" && wanted != "off")
      {
        error = "Option '--" + vsync + "' needs on or off";
        return false;
      }
      settings.vertical_sync = wanted == "on";
    }

    if (command_line.IsSet(max_fps))
    {
      double most = -1.0;
      if (!command_line.GetNumber(max_fps, most) || most != std::floor(most) ||
          (most != 0.0 && (most < FrameLimit::kLeast || most > FrameLimit::kMost)))
      {
        error = "Option '--" + max_fps + "' needs 0 for no limit, or a number from 30 to 300";
        return false;
      }
      settings.max_fps = static_cast<int>(most);
    }

    // the preset first: its values hold over the options it decides, and
    // custom lets them apply
    if (with_quality && command_line.IsSet(quality))
    {
      const std::string name = command_line.GetValue(quality);
      if (!settings.graphics_presets.IsName(name))
      {
        error = "Option '--" + quality + "' needs " + settings.graphics_presets.NamesForAMessage();
        return false;
      }
      settings.quality = name;
      if (const GraphicsPreset *preset = settings.graphics_presets.Named(name); preset != nullptr)
      {
        preset->ApplyTo(settings);
      }
    }

    if (command_line.IsSet(anisotropy))
    {
      double level = 0.0;
      if (!command_line.GetNumber(anisotropy, level) || level != std::floor(level) ||
          !Anisotropy::IsLevel(static_cast<int>(level)))
      {
        error = "Option '--" + anisotropy + "' needs 1 for none, 2, 4, 8, or 16";
        return false;
      }
      if (decided(anisotropy)) { settings.anisotropy = static_cast<int>(level); }
    }

    if (command_line.IsSet(texture_scale))
    {
      double scale = 0.0;
      if (!command_line.GetNumber(texture_scale, scale) || !TextureScale::IsScale(scale))
      {
        error = "Option '--" + texture_scale + "' needs 1, 0.5, 0.25, or 0.125";
        return false;
      }
      if (decided(texture_scale)) { settings.texture_scale = scale; }
    }

    if (command_line.IsSet(target_scale))
    {
      double scale = 0.0;
      if (!command_line.GetNumber(target_scale, scale) || !TargetQuality::IsScale(scale))
      {
        error = "Option '--" + target_scale + "' needs 1, 0.5, or 0.25";
        return false;
      }
      if (decided(target_scale)) { settings.target_scale = scale; }
    }

    if (command_line.IsSet(target_mipmaps))
    {
      double mipmaps = -1.0;
      if (!command_line.GetNumber(target_mipmaps, mipmaps) || mipmaps != std::floor(mipmaps) ||
          !TargetQuality::IsMipmaps(static_cast<int>(mipmaps)))
      {
        error = "Option '--" + target_mipmaps + "' needs 0 for as many as the size allows, or 1 to 16";
        return false;
      }
      if (decided(target_mipmaps)) { settings.target_mipmaps = static_cast<int>(mipmaps); }
    }

    if (command_line.IsSet(shadows))
    {
      const std::string wanted = command_line.GetValue(shadows);
      if (wanted != "on" && wanted != "off")
      {
        error = "Option '--" + shadows + "' needs on or off";
        return false;
      }
      settings.shadows = wanted == "on";
    }

    if (command_line.IsSet(shadow_map_size))
    {
      double size = 0.0;
      if (!command_line.GetNumber(shadow_map_size, size) || size != std::floor(size) ||
          !ShadowMapSize::IsSize(static_cast<int>(size)))
      {
        error = "Option '--" + shadow_map_size + "' needs 512, 1024, 2048, or 4096";
        return false;
      }
      if (decided(shadow_map_size)) { settings.shadow_map_size = static_cast<int>(size); }
    }

    if (command_line.IsSet(shadow_filter))
    {
      const std::string wanted = command_line.GetValue(shadow_filter);
      if (wanted != "none" && wanted != "pcf")
      {
        error = "Option '--" + shadow_filter + "' needs none or pcf";
        return false;
      }
      if (decided(shadow_filter)) { settings.shadow_filter = wanted == "pcf" ? ShadowFilter::Pcf : ShadowFilter::None; }
    }

    if (command_line.IsSet(shadow_distance))
    {
      double distance = 0.0;
      if (!command_line.GetNumber(shadow_distance, distance) || !(distance > 0.0))
      {
        error = "Option '--" + shadow_distance + "' needs a number of meters above 0";
        return false;
      }
      if (decided(shadow_distance)) { settings.shadow_distance = distance; }
    }

    if (command_line.IsSet(shadow_cascades))
    {
      double count = 0.0;
      if (!command_line.GetNumber(shadow_cascades, count) || count != std::floor(count) || count < 1.0 || count > 4.0)
      {
        error = "Option '--" + shadow_cascades + "' needs a number from 1 to 4";
        return false;
      }
      if (decided(shadow_cascades)) { settings.shadow_cascades = static_cast<std::size_t>(count); }
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

    return true;
  }
} // neon
