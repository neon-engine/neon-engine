#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include <neon/command-line/command-line.hpp>
#include <neon/audio/ma-audio-system.hpp>
#include <neon/command-line/runtime-options.hpp>
#include <neon/data/ryml-document-format.hpp>
#include <neon/filesystem/sdl2-file-system.hpp>
#include <neon/input/headless-input-system.hpp>
#include <neon/input/sdl2-input-system.hpp>
#include <neon/physics/jolt-physics-system.hpp>
#include <neon/render/forward-render-pipeline.hpp>
#include <neon/render/vk-render-system.hpp>
#include <neon/window/headless-window-system.hpp>
#include <neon/window/sdl2-window-system.hpp>
#include <neon/world-system/ecs/entity-world.hpp>
#include <neon/world-system/ecs/scene-file/scene-file.hpp>
#include <neon/world-system/ecs/systems/audio-playback.hpp>
#include <neon/world-system/ecs/systems/physics-simulation.hpp>
#include <neon/world-system/flecs-entity-store.hpp>

#include "neon-runtime.hpp"

// SDL2 provides the real platform entry point (WinMain on Windows) through
// SDL2main and renames main to SDL_main behind this include. It requires the
// full argc/argv signature.
#include <SDL_main.h>

int main(const int argc, char *argv[])
{
  // What this application accepts on its command line. The editor will add a
  // set of its own next to the one every runtime has.
  neon::CommandLine command_line("NeonRuntime", "Runs a Neon Engine project.");
  neon::RuntimeOptions runtime_options;
  runtime_options.Register(command_line);

  if (!command_line.Parse(argc, argv))
  {
    std::cerr << command_line.GetError() << "\n\n" << command_line.GetHelp();
    return EXIT_FAILURE;
  }

  if (command_line.WantsHelp())
  {
    std::cout << command_line.GetHelp();
    return EXIT_SUCCESS;
  }

  auto settings_config = SettingsConfig{
    .width = 1920,
    .height = 1080,
    .selected_api = RenderingApi::Vulkan,
    .window_mode = WindowMode::Borderless
  };

  if (std::string error; !runtime_options.Apply(command_line, settings_config, error))
  {
    std::cerr << error << "\n\n" << command_line.GetHelp();
    return EXIT_FAILURE;
  }

  neon::LoggingSystem logging_system(settings_config);

  logging_system.Initialize();

  // everything that loads files depends on the file system, so it comes up
  // before the other systems are created
  neon::SDL2_FileSystem file_system(settings_config, logging_system.CreateLogger("SDL2_FileSystem"));
  file_system.Initialize();

  // Without a window there is no one to hear anything. Sounds are still read
  // and mixed, so that a run without a window finds a sound that is broken.
  if (settings_config.headless) { settings_config.audio_output = AudioOutput::None; }

  neon::MA_AudioSystem audio_system(
    settings_config,
    &file_system,
    logging_system.CreateLogger("MA_AudioSystem"));
  audio_system.Initialize();

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

  if (settings_config.headless)
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

  switch (settings_config.selected_api)
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

  neon::Flecs_EntityStore entity_store(logging_system.CreateLogger("Flecs_EntityStore"));
  neon::RYML_DocumentFormat yaml;

  neon::SceneFile scene(
    &file_system,
    &yaml,
    settings_config.scene_path,
    logging_system.CreateLogger("SceneFile"));

  neon::EntityWorld world(
    &entity_store,
    &scene,
    &render_pipeline,
    input_system,
    window_system,
    logging_system.CreateLogger("EntityWorld"));

  world.AddSystem(std::make_unique<neon::AudioPlayback>(&audio_system));
  // The physics. The world steps at a fixed rate, and the system that is
  // added here takes a step of the physics in each. Systems of a game are
  // added before it, so that what they ask for in a step is part of it.
  neon::Jolt_PhysicsSystem physics_system(settings_config, logging_system.CreateLogger("Jolt_PhysicsSystem"));
  physics_system.Initialize();

  world.GetFixedClock().SetStepsPerSecond(settings_config.steps_per_second);
  world.GetFixedClock().SetMostStepsPerFrame(settings_config.most_steps_per_frame);
  world.AddSystem(std::make_unique<neon::PhysicsSimulation>(
    &physics_system,
    &file_system,
    logging_system.CreateLogger("PhysicsSimulation")));

  const auto app_logger = logging_system.CreateLogger("NeonRuntime");

  NeonRuntime app(
    settings_config,
    window_system,
    input_system,
    render_system,
    &render_pipeline,
    &logging_system,
    &world,
    app_logger);

  // a script that starts the runtime learns from the exit code whether the
  // run did what it was asked to
  bool failed = false;

  try
  {
    app.Run();
    failed = app.HasFailed();
  } catch (const std::exception &e)
  {
    app_logger->Critical(e.what());
    failed = true;
  }

  // CleanUp is safe to call more than once. Doing it here guarantees the
  // systems shut down before the file system they depend on.
  app.CleanUp();
  audio_system.CleanUp();
  physics_system.CleanUp();
  file_system.CleanUp();

  return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
