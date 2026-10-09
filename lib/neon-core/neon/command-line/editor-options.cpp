#include "editor-options.hpp"

#include <algorithm>
#include <charconv>
#include <string_view>

#include <neon/filesystem/file-system.hpp>

namespace neon
{
  // Helpers of EditorOptions, for this file alone.
  namespace
  {
    const std::string scene = "scene";
    const std::string ui = "ui";
    const std::string frames = "frames";
    const std::string screenshot = "screenshot";
    const std::string screenshot_at = "screenshot-at";
    const std::string headless_renderer = "headless-renderer";
    const std::string headless = "headless";
    const std::string time_step = "time-step";
    const std::string output_dir = "output-dir";
    const std::string spawn = "spawn";
    const std::string log_entity = "log-entity";
    const std::string jit = "jit";
    const std::string tonemapper = "tonemapper";
    const std::string exposure = "exposure";

    const std::string none = "none";
    const std::string aces = "aces";
    const std::string agx = "agx";
    const std::string on = "on";
    const std::string off = "off";
    const std::string render_scale = "render-scale";
    const std::string input = "input";
    const std::string input_script = "input-script";

    const std::string editor = "Editor";

    /// Reads the name of a tonemapper, as the setting `rendering.tonemapper`
    /// names them. Returns false for a name that is none of them.
    bool read_tonemapper(const std::string &name, Tonemapper &curve)
    {
      if (name == none) { curve = Tonemapper::None; return true; }
      if (name == aces) { curve = Tonemapper::Aces; return true; }
      if (name == agx) { curve = Tonemapper::Agx; return true; }
      return false;
    }

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

    /// Reads a list of paths of entities such as `player,crates/upper`.
    /// Returns false when a part of it is empty, or starts or ends with a
    /// slash, which names no entity. The paths come out in the order they
    /// were given, each of them once.
    bool read_entities(const std::string_view list, std::vector<std::string> &paths)
    {
      std::size_t start = 0;
      while (start <= list.size())
      {
        std::size_t end = list.find(',', start);
        if (end == std::string_view::npos) { end = list.size(); }

        const std::string part(list.substr(start, end - start));
        start = end + 1;

        const bool names_none = part.empty() || part.starts_with('/') || part.ends_with('/') ||
                                part.find("//") != std::string::npos;
        if (names_none) { return false; }
        if (std::ranges::find(paths, part) == paths.end()) { paths.push_back(part); }
      }
      return true;
    }
  }

  void EditorOptions::Register(CommandLine &command_line)
  {
    command_line.Add({
      .name = scene,
      .value_name = "PATH",
      .description = "Scene to start with in place of the entry scene of the project, "
                     "for example assets://scenes/demo.scene.yml",
      .group = editor
    });

    command_line.Add({
      .name = ui,
      .value_name = "PATH",
      .description = "User interface to show on top, for example assets://ui/hud.ui.yml",
      .group = editor
    });

    command_line.Add({
      .name = frames,
      .value_name = "N",
      .description = "Stop after N frames",
      .group = editor
    });

    command_line.Add({
      .name = screenshot,
      .value_name = "PATH",
      .description = "Save the last frame as a PNG image, for example output://frame.png. "
                     "Needs --frames or --screenshot-at",
      .group = editor
    });

    command_line.Add({
      .name = screenshot_at,
      .value_name = "N[,N...]",
      .description = "Save these frames instead of the last one, counted from 1. Each file gets "
                     "its frame in its name, as in frame-0030.png. Needs --screenshot",
      .group = editor
    });

    command_line.Add({
      .name = output_dir,
      .value_name = "DIR",
      .description = "Folder of this machine that output:// stands for. Created when missing",
      .group = editor
    });

    command_line.Add({
      .name = time_step,
      .value_name = "SECONDS",
      .description = "Advance the game by this much time in every frame, for example 0.016667, "
                     "so that a run gives the same frames every time",
      .group = editor
    });

    command_line.Add({
      .name = headless_renderer,
      .description = "Render without a window, for screenshots and checks on a machine with no display",
      .group = editor
    });

    command_line.Add({
      .name = render_scale,
      .value_name = "NUMBER",
      .description = "Pixels that are drawn for each point, for example 2 for what a display of high "
                     "density shows. Needs --headless-renderer, a window takes the density of its display",
      .group = editor
    });

    command_line.Add({
      .name = input,
      .value_name = "SCRIPT",
      .description = "Input in place of devices, for example \"1: pointer 640 360; 2: click\". "
                     "Needs --headless-renderer",
      .group = editor
    });

    command_line.Add({
      .name = input_script,
      .value_name = "PATH",
      .description = "The same from a file, for example assets://input/menu.input. Needs --headless-renderer",
      .group = editor
    });

    command_line.Add({
      .name = spawn,
      .value_name = "PATH",
      .description = "Spawn this prefab at the top of the world once the scene is read, as a script would, "
                     "for example assets://prefabs/target.prefab.yml",
      .group = editor
    });

    command_line.Add({
      .name = log_entity,
      .value_name = "NAME[,...]",
      .description = "Log where these entities are in every frame, each by its path as a scene names it, for "
                     "example player,crates/upper",
      .group = editor
    });

    command_line.Add({
      .name = tonemapper,
      .value_name = "NAME",
      .description = "Curve for light brighter than white: none, aces, or agx, over rendering.tonemapper of "
                     "the settings",
      .group = editor
    });

    command_line.Add({
      .name = exposure,
      .value_name = "NUMBER",
      .description = "How bright the scene is taken to be, for example 2 for twice the light, over "
                     "rendering.exposure of the settings",
      .group = editor
    });

    command_line.Add({
      .name = jit,
      .value_name = "MODE",
      .description = "Compile the scripts as they run, or run them in LuaJIT's interpreter. Over scripting.jit "
                     "of the settings, for comparing the two",
      .group = editor,
      .allowed_values = {on, off}
    });

    // declared so that it is understood and refused with a pointer to the
    // right option, instead of being an unknown one. Applying it is for the
    // dedicated server, #144
    command_line.Add({
      .name = headless,
      .description = "Run as a dedicated server. Not available yet, see --headless-renderer",
      .group = editor
    });
  }

  bool EditorOptions::Apply(
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

    if (command_line.IsSet(spawn))
    {
      settings.spawn_path = command_line.GetValue(spawn);
    }

    if (command_line.IsSet(log_entity))
    {
      // read afresh, so that applying the same command line twice, as the
      // application does around its settings files, logs each of them once
      settings.logged_entities.clear();
      if (!read_entities(command_line.GetValue(log_entity), settings.logged_entities))
      {
        error = "Option '--" + log_entity + "' needs paths of entities such as player or crates/upper, "
                "separated by commas";
        return false;
      }
    }

    if (command_line.IsSet(tonemapper) && !read_tonemapper(command_line.GetValue(tonemapper), settings.tonemapper))
    {
      error = "Option '--" + tonemapper + "' needs none, aces, or agx";
      return false;
    }

    if (command_line.IsSet(exposure))
    {
      double amount = 0.0;
      if (!command_line.GetNumber(exposure, amount) || amount <= 0.0)
      {
        error = "Option '--" + exposure + "' needs a number above zero, such as 2";
        return false;
      }
      settings.exposure = amount;
    }

    // the parser only lets on and off through
    if (command_line.IsSet(jit))
    {
      settings.script_jit = command_line.GetValue(jit) == on;
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

    if (command_line.IsSet(render_scale))
    {
      double scale = 0.0;
      if (!command_line.GetNumber(render_scale, scale) || scale < 0.25 || scale > 8.0)
      {
        error = "Option '--" + render_scale + "' needs a number from 0.25 to 8, such as 2";
        return false;
      }

      if (!settings.headless_renderer)
      {
        error = "Option '--" + render_scale + "' needs '--" + headless_renderer +
                "'. A window takes the density of its display";
        return false;
      }

      settings.render_scale = scale;
    }

    if (command_line.IsSet(input) && command_line.IsSet(input_script))
    {
      error = "Option '--" + input + "' and '--" + input_script + "' cannot be given together";
      return false;
    }

    for (const auto &name : {input, input_script})
    {
      if (command_line.IsSet(name) && !settings.headless_renderer)
      {
        error = "Option '--" + name + "' needs '--" + headless_renderer +
                "'. A window takes its input from devices";
        return false;
      }
    }

    if (command_line.IsSet(input)) { settings.input_script = command_line.GetValue(input); }
    if (command_line.IsSet(input_script)) { settings.input_script_path = command_line.GetValue(input_script); }

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
