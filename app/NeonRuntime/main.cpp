#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include <neon/command-line/command-line.hpp>
#include <neon/audio/ma-audio-system.hpp>
#include <neon/command-line/display-options.hpp>
#include <neon/command-line/runtime-options.hpp>
#include <neon/data/ryml-document-format.hpp>
#include <neon/filesystem/sdl2-file-system.hpp>
#include <neon/input/headless-input-system.hpp>
#include <neon/input/input-map-file.hpp>
#include <neon/input/input-script.hpp>
#include <neon/input/sdl2-clipboard.hpp>
#include <neon/input/sdl2-input-system.hpp>
#include <neon/physics/jolt-physics-system.hpp>
#include <neon/project/project-file.hpp>
#include <neon/random/os-entropy.hpp>
#include <neon/settings/settings-file.hpp>
#include <neon/layout/flex-layout-engine.hpp>
#include <neon/render/forward-render-pipeline.hpp>
#include <neon/render/vk-render-system.hpp>
#include <neon/image/luna-vector-image-rasterizer.hpp>
#include <neon/image/stb-image-decoder.hpp>
#include <neon/text/ft-font-rasterizer.hpp>
#include <neon/text/hb-text-shaper.hpp>
#include <neon/ui/tree-ui-system.hpp>
#include <neon/window/headless-window-system.hpp>
#include <neon/window/sdl2-window-system.hpp>
#include <neon/world-system/ecs/entity-world.hpp>
#include <neon/world-system/ecs/scene-file/scene-file.hpp>
#include <neon/world-system/ecs/systems/audio-playback.hpp>
#include <neon/world-system/ecs/systems/geometry-building.hpp>
#include <neon/world-system/ecs/systems/physics-simulation.hpp>
#include <neon/world-system/ecs/scene-file/ui-view-format.hpp>
#include <neon/world-system/ecs/components/ui-sound-switch.hpp>
#include <neon/world-system/ecs/components/ui-volume.hpp>
#include <neon/world-system/ecs/systems/ui-audio.hpp>
#include <neon/world-system/ecs/systems/ui-clock.hpp>
#include <neon/world-system/ecs/systems/ui-surface-loading.hpp>
#include <neon/world-system/ecs/systems/ui-surface-pointing.hpp>
#include <neon/world-system/ecs/systems/ui-view-loading.hpp>
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

  neon::DisplayOptions display_options;
  display_options.Register(command_line);

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

  // The settings come in layers, each on top of the one before: these
  // defaults, the settings of the project, the settings of the player, and
  // the command line. The command line is applied here already, since the
  // file system needs what it says to come up, and again after the files,
  // so that it wins.
  auto settings_config = SettingsConfig{.selected_api = RenderingApi::Vulkan};

  const auto apply_command_line = [&]
  {
    std::string error;
    if (runtime_options.Apply(command_line, settings_config, error) &&
        display_options.Apply(command_line, settings_config, error))
    {
      return true;
    }
    std::cerr << error << "\n\n" << command_line.GetHelp();
    return false;
  };

  if (!apply_command_line()) { return EXIT_FAILURE; }

  neon::LoggingSystem logging_system(settings_config);

  logging_system.Initialize();

  // everything that loads files depends on the file system, so it comes up
  // before the other systems are created
  neon::SDL2_FileSystem file_system(settings_config, logging_system.CreateLogger("SDL2_FileSystem"));
  file_system.Initialize();

  neon::RYML_DocumentFormat yaml;

  // The project is the folder behind assets://. Its file says who makes it
  // and what it is called, which is what places user://, so it is read
  // before anything is written. Its problems go to the console, since the
  // log file has no place yet.
  neon::Project project;
  {
    const neon::ProjectFile project_file(&file_system, &yaml, logging_system.CreateLogger("ProjectFile"));
    if (std::vector<std::string> errors; !project_file.Read(project, errors))
    {
      for (const auto &error : errors) { std::cerr << error << "\n"; }
      file_system.CleanUp();
      return EXIT_FAILURE;
    }
  }

  settings_config.organization = project.organization;
  settings_config.application = project.name;

  // the command line may start another scene, which is for development: a
  // shipped runtime runs what its project says
  if (settings_config.scene_path.empty()) { settings_config.scene_path = project.entry_scene; }

  if (!file_system.PlaceUserDirectory(settings_config.organization, settings_config.application))
  {
    std::cerr << "The folder of the user cannot be placed, see the log\n";
    file_system.CleanUp();
    return EXIT_FAILURE;
  }

  // The log file belongs under user://, and only the file system knows where
  // that is. Until now everything went to the console, and was held back for
  // the file, which gets it first.
  file_system.PlaceLogFile(settings_config.logpath, logging_system);

  // The settings of the project, then what the player changed. A mistake in
  // the project's file stops the runtime, since the game would not be what
  // its author meant. A mistake in the player's file is said in the log and
  // the file is left out, since a player should not be locked out of the
  // game by a file a menu wrote.
  {
    const neon::SettingsFile settings_file(&file_system, &yaml, logging_system.CreateLogger("SettingsFile"));
    const auto settings_logger = logging_system.CreateLogger("NeonRuntime");

    bool found = false;
    std::vector<std::string> errors;
    if (!settings_file.Read(std::string(neon::SettingsFile::of_the_project), settings_config, found, errors))
    {
      for (const auto &error : errors) { std::cerr << error << "\n"; }
      file_system.CleanUp();
      return EXIT_FAILURE;
    }

    if (!settings_file.Read(std::string(neon::SettingsFile::of_the_player), settings_config, found, errors))
    {
      for (const auto &error : errors) { settings_logger->Warn("{}", error); }
      settings_logger->Warn("The settings of the player are left out until they are corrected");
    }
  }

  // the command line wins over the files
  if (!apply_command_line())
  {
    file_system.CleanUp();
    return EXIT_FAILURE;
  }

  // the window is named after the project unless the settings say otherwise
  if (settings_config.title.empty()) { settings_config.title = project.name; }

  // Without a window there is no one to hear anything. Sounds are still read
  // and mixed, so that a run without a window finds a sound that is broken.
  if (settings_config.headless_renderer) { settings_config.audio_output = AudioOutput::None; }

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
  neon::Render2DContext *render_2d_context;

  if (settings_config.headless_renderer)
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

  // The input map of the project: the actions the game reads, and what is
  // bound to each. Without one the engine's default applies. A mistake in
  // it stops the runtime, since the game could not be played as meant.
  if (!project.input.empty())
  {
    const neon::InputMapFile input_map_file(&file_system, &yaml, logging_system.CreateLogger("InputMapFile"));
    neon::InputMap input_map;
    if (std::vector<std::string> errors; !input_map_file.Read(project.input, input_map, errors))
    {
      for (const auto &error : errors) { std::cerr << error << "\n"; }
      file_system.CleanUp();
      return EXIT_FAILURE;
    }
    input_system->SetInputMap(input_map);
  }

  // the player's switch for the gyro, over what the map says. A settings
  // menu will turn it later through the same call
  if (settings_config.gyro_enabled.has_value())
  {
    input_system->SetSensorEnabled("gyro", *settings_config.gyro_enabled);
  }

  if (settings_config.headless_renderer)
  {
    // input that is written down, in place of the devices a run without a
    // window does not have. It names actions of the map, so it comes after
    std::string script_text = settings_config.input_script;
    std::string script_name = "--input";

    if (!settings_config.input_script_path.empty())
    {
      script_name = settings_config.input_script_path;
      if (!file_system.ReadText(settings_config.input_script_path, script_text))
      {
        std::cerr << "The script of input " << script_name << " cannot be read\n";
        file_system.CleanUp();
        return EXIT_FAILURE;
      }
    }

    if (!script_text.empty())
    {
      neon::InputScript script;
      std::vector<std::string> errors;
      if (!neon::InputScript::Parse(script_text, script_name, script, errors) ||
          !headless_input_system->SetScript(script, errors))
      {
        for (const auto &error : errors) { std::cerr << error << "\n"; }
        file_system.CleanUp();
        return EXIT_FAILURE;
      }
    }
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
      render_2d_context = &*vk_render_system;
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

  // The user interface: menus, and what is shown during play. Its text is
  // drawn with Inter unless a file names another font. Glyphs are drawn
  // with FreeType and text is shaped with HarfBuzz. STB_FontRasterizer of
  // neon-stb stands in for the first where neither is wanted, and draws
  // text that is not shaped.
  neon::FT_FontRasterizer font_rasterizer;
  neon::HB_TextShaper text_shaper;
  neon::Flex_LayoutEngine layout_engine;

  neon::Tree_UiSystem ui_system(
    render_2d_context,
    &font_rasterizer,
    &layout_engine,
    input_system,
    &file_system,
    &yaml,
    neon::UiSettings{
      .fonts = {
        {"sans-serif", 400, "assets://fonts/inter/Inter-Regular.ttf"},
        {"sans-serif", 700, "assets://fonts/inter/Inter-Bold.ttf"}
      },
      .start_path = settings_config.ui_path
    },
    logging_system.CreateLogger("Tree_UiSystem"));

  ui_system.SetTextShaper(&text_shaper);

  // Images are read with stb_image, and those that are made of shapes are
  // drawn with LunaSVG at the size they have on the screen.
  neon::STB_ImageDecoder image_decoder;
  neon::LUNA_VectorImageRasterizer vector_image_rasterizer;
  ui_system.SetImageDecoder(&image_decoder);
  ui_system.SetVectorImageRasterizer(&vector_image_rasterizer);
  // The user interface follows the window for its size and density, and
  // scales by what the player asked for on top. Text is cut and pasted
  // through the clipboard of the platform where there is one.
  neon::SDL2_Clipboard sdl2_clipboard;
  ui_system.SetWindow(window_system);
  if (!settings_config.headless_renderer) { ui_system.SetClipboard(&sdl2_clipboard); }
  ui_system.SetUserScale(static_cast<float>(settings_config.ui_scale));

  neon::SceneFile scene(
    &file_system,
    &yaml,
    settings_config.scene_path,
    logging_system.CreateLogger("SceneFile"));

  // a scene names the user interface it comes with as a component
  scene.GetComponentFormats().Add(neon::UiViewFormat());

  // and the user interfaces that are shown on surfaces in the world
  scene.GetComponentFormats().Add(neon::UiSurfaceFormat());

  // and what values of user interfaces do to what is heard
  scene.GetComponentFormats().Add(neon::ComponentFormat::Of<neon::UiVolume>());
  scene.GetComponentFormats().Add(neon::ComponentFormat::Of<neon::UiSoundSwitch>());

  // the world reads the input less what the user interface has used
  neon::EntityWorld world(
    &entity_store,
    &scene,
    &render_pipeline,
    ui_system.GetGameInput(),
    window_system,
    logging_system.CreateLogger("EntityWorld"));

  // sounds are heard where their entities are drawn, so that a sound is not
  // heard from where it was a frame ago, or from the origin in the first
  // frame, before anything was placed
  world.AddSystemAfterPlacing(
    std::make_unique<neon::AudioPlayback>(&audio_system, logging_system.CreateLogger("AudioPlayback")));
  // shapes built from recipes, drawn in place of models and collided with
  world.AddSystem(std::make_unique<neon::GeometryBuilding>(logging_system.CreateLogger("GeometryBuilding")));
  world.AddSystem(std::make_unique<neon::UiViewLoading>(&ui_system, logging_system.CreateLogger("UiViewLoading")));
  world.AddSystem(std::make_unique<neon::UiClock>(&ui_system));
  world.AddSystem(std::make_unique<neon::UiAudio>(&ui_system, &audio_system));
  world.AddSystem(std::make_unique<neon::UiSurfaceLoading>(&ui_system, logging_system.CreateLogger("UiSurfaceLoading")));
  world.AddSystem(std::make_unique<neon::UiSurfacePointing>(&ui_system, ui_system.GetGameInput()));

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

  // The random numbers of the operating system, for the game. The engine
  // draws none itself, and no option or setting seeds them: a game that is
  // to repeat a run keeps the seed it took from here.
  neon::OS_Entropy entropy(logging_system.CreateLogger("OS_Entropy"));

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

  app.SetUiSystem(&ui_system);
  app.SetEntropy(&entropy);

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
