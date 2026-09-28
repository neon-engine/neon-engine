#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include <neon/filesystem/sdl2-file-system.hpp>
#include <neon/input/headless-input-system.hpp>
#include <neon/input/sdl2-input-system.hpp>
#include <neon/render/forward-render-pipeline.hpp>
#include <neon/render/vk-render-system.hpp>
#include <neon/window/headless-window-system.hpp>
#include <neon/window/sdl2-window-system.hpp>

#include "neon-fps-application.hpp"

// SDL2 provides the real platform entry point (WinMain on Windows) through
// SDL2main and renames main to SDL_main behind this include. It requires the
// full argc/argv signature.
#include <SDL_main.h>

namespace
{
  struct Options
  {
    RenderingApi renderer = RenderingApi::Vulkan;
    bool headless = false;
    std::size_t frames = 0;
    std::string screenshot;
  };

  void print_usage()
  {
    std::cout <<
      "Usage: NeonRuntime [options]\n"
      "\n"
      "  --renderer vulkan         Renderer to draw with. Vulkan is the only one so far\n"
      "  --help                    Show this text\n"
      "\n"
      "For development:\n"
      "  --frames N                Stop after N frames\n"
      "  --screenshot PATH         Save the last frame as a PNG image before stopping,\n"
      "                            for example user://screenshots/frame.png. Needs --frames\n"
      "  --headless                Run without a window\n";
  }

  /// Returns false when the application should not start. `exit_code` says
  /// whether that is an error.
  bool parse_options(const int argc, char *argv[], Options &options, int &exit_code)
  {
    exit_code = EXIT_FAILURE;

    for (int i = 1; i < argc; i++)
    {
      const std::string_view argument = argv[i];
      const bool has_value = i + 1 < argc;

      if (argument == "--help")
      {
        print_usage();
        exit_code = EXIT_SUCCESS;
        return false;
      }

      if (argument == "--headless")
      {
        options.headless = true;
      } else if (argument == "--renderer" && has_value)
      {
        const std::string_view value = argv[++i];
        if (value == "vulkan")
        {
          options.renderer = RenderingApi::Vulkan;
        } else
        {
          std::cerr << "Unknown renderer '" << value << "'\n\n";
          print_usage();
          return false;
        }
      } else if (argument == "--frames" && has_value)
      {
        const long frames = std::strtol(argv[++i], nullptr, 10);
        if (frames <= 0)
        {
          std::cerr << "--frames needs a number above zero\n\n";
          print_usage();
          return false;
        }
        options.frames = static_cast<std::size_t>(frames);
      } else if (argument == "--screenshot" && has_value)
      {
        options.screenshot = argv[++i];
      } else
      {
        std::cerr << "Unknown or incomplete option '" << argument << "'\n\n";
        print_usage();
        return false;
      }
    }

    if (!options.screenshot.empty() && options.frames == 0)
    {
      std::cerr << "--screenshot needs --frames\n";
      return false;
    }

    return true;
  }
}

int main(const int argc, char *argv[])
{
  Options options;
  if (int exit_code; !parse_options(argc, argv, options, exit_code))
  {
    return exit_code;
  }

  auto settings_config = SettingsConfig{
    .width = 1920,
    .height = 1080,
    .selected_api = options.renderer,
    .window_mode = WindowMode::Borderless
  };
  settings_config.max_frames = options.frames;
  settings_config.screenshot_path = options.screenshot;

  neon::LoggingSystem logging_system(settings_config);

  logging_system.Initialize();

  // everything that loads files depends on the file system, so it comes up
  // before the other systems are created
  neon::SDL2_FileSystem file_system(settings_config, logging_system.CreateLogger("SDL2_FileSystem"));
  file_system.Initialize();

  // Only the systems that were asked for are created. The rest of the engine
  // sees them through their interfaces and cannot tell them apart.
  std::optional<neon::SDL2_WindowSystem> sdl2_window_system;
  std::optional<neon::Headless_WindowSystem> headless_window_system;
  std::optional<neon::SDL2_InputSystem> sdl2_input_system;
  std::optional<neon::Headless_InputSystem> headless_input_system;
  std::optional<neon::VK_RenderSystem> vk_render_system;

  neon::WindowSystem *window_system;
  neon::InputSystem *input_system;
  neon::RenderSystem *render_system;

  if (options.headless)
  {
    window_system = &headless_window_system.emplace(
      settings_config,
      logging_system.CreateLogger("Headless_WindowSystem"));
    input_system = &headless_input_system.emplace(
      settings_config,
      logging_system.CreateLogger("Headless_InputSystem"));
  } else
  {
    window_system = &sdl2_window_system.emplace(
      settings_config,
      logging_system.CreateLogger("SDL2_WindowSystem"));
    input_system = &sdl2_input_system.emplace(
      settings_config,
      window_system,
      logging_system.CreateLogger("SDL2_InputSystem"));
  }

  switch (options.renderer)
  {
    case RenderingApi::Vulkan:
    {
      render_system = &vk_render_system.emplace(
        window_system,
        &file_system,
        settings_config,
        logging_system.CreateLogger("Vulkan_RenderSystem"));
      break;
    }
    default:
    {
      std::cerr << "The renderer that was asked for is not part of this build\n";
      return EXIT_FAILURE;
    }
  }

  neon::Forward_RenderPipeline render_pipeline(
    render_system,
    settings_config.max_light_sources,
    logging_system.CreateLogger("Forward_RenderPipeline"));

  neon::SceneManager scene_manager(
    &render_pipeline,
    input_system,
    window_system,
    logging_system.CreateLogger("SceneManager"));

  const auto app_logger = logging_system.CreateLogger("NeonFpsApplication");

  NeonFpsApplication app(
    settings_config,
    window_system,
    input_system,
    render_system,
    &render_pipeline,
    &logging_system,
    &scene_manager,
    app_logger);

  try
  {
    app.Run();
  } catch (const std::exception &e)
  {
    app_logger->Critical(e.what());
  }

  // CleanUp is safe to call more than once. Doing it here guarantees the
  // systems shut down before the file system they depend on.
  app.CleanUp();
  file_system.CleanUp();

  return 0;
}
