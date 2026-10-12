#include "runtime-composition.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <utility>

#include <neon/world-system/ecs/scene-file/ui-view-format.hpp>
#include <neon/world-system/ecs/systems/audio/audio-playback.hpp>
#include <neon/world-system/ecs/systems/physics/physics-simulation.hpp>
#include <neon/world-system/ecs/systems/physics/rope-drawing.hpp>
#include <neon/world-system/ecs/systems/physics/scene-exiting.hpp>
#include <neon/world-system/ecs/systems/player/first-person-controller-system.hpp>
#include <neon/world-system/ecs/systems/scripting/extension-running.hpp>
#include <neon/world-system/ecs/systems/scripting/script-running.hpp>
#include <neon/world-system/ecs/systems/ui/ui-clock.hpp>
#include <neon/world-system/ecs/systems/ui/ui-surface-loading.hpp>
#include <neon/world-system/ecs/systems/ui/ui-surface-pointing.hpp>
#include <neon/world-system/ecs/systems/ui/ui-view-loading.hpp>
#include <neon/world-system/ecs/systems/world/entity-logging.hpp>
#include <neon/world-system/ecs/systems/world/ray-casting.hpp>

#include "input-map-loading.hpp"
#include "input-script-loading.hpp"
#include "project-reading.hpp"
#include "settings-layers.hpp"

namespace neon
{
  RuntimeComposition::RuntimeComposition(const ApplicationDescription &application, CompositionHooks *hooks)
    : _application(application),
      _hooks(hooks),
      _exit_code(EXIT_SUCCESS),
      _command_line(application.program, application.summary),
      _settings{.selected_api = RenderingApi::Vulkan} {}

  void RuntimeComposition::Say(const std::vector<std::string> &lines)
  {
    for (const auto &line : lines) { std::cerr << line << "\n"; }
  }

  bool RuntimeComposition::Fail()
  {
    _exit_code = EXIT_FAILURE;
    return false;
  }

  bool RuntimeComposition::ApplyCommandLine(const bool with_quality)
  {
    std::string error;
    if (_runtime_options.Apply(_command_line, _settings, error) &&
        (with_quality
           ? _display_options.Apply(_command_line, _settings, error)
           : _display_options.ApplyExceptTheQuality(_command_line, _settings, error)) &&
        _hooks->ApplyOptions(_command_line, _settings, error))
    {
      return true;
    }
    std::cerr << error << "\n\n" << _command_line.GetHelp();
    return Fail();
  }

  bool RuntimeComposition::ParseCommandLine(const int argc, char *argv[])
  {
    // Each set of options is owned by one application and registered by it:
    // the runtime owns the runtime set and the display set, which a player
    // may use, and the editor owns the editor set, which loads other content
    // and looks inside a game. A shipped game accepts no --scene (#310).
    // See docs/command-line.md.
    _runtime_options.Register(_command_line);
    _display_options.Register(_command_line);
    _hooks->RegisterOptions(_command_line);

    if (!_command_line.Parse(argc, argv))
    {
      std::cerr << _command_line.GetError() << "\n\n" << _command_line.GetHelp();
      return Fail();
    }

    if (_command_line.WantsHelp())
    {
      std::cout << _command_line.GetHelp();
      return false;
    }

    if (std::string error; !_hooks->ChooseProjectFolder(_command_line, _settings, error))
    {
      std::cerr << error << "\n\n" << _command_line.GetHelp();
      return Fail();
    }

    return ApplyCommandLine(false);
  }

  bool RuntimeComposition::OpenFileSystem()
  {
    _logging.emplace(_settings);
    _logging->Initialize();

    _file_system.emplace(_settings, _logging->CreateLogger("SDL2_FileSystem"));
    try
    {
      _file_system->Initialize();
    } catch (const std::exception &e)
    {
      // without the folder of the executable nothing else is found, and the
      // log file has no place yet
      std::cerr << e.what() << "\n";
      _file_system.reset();
      return Fail();
    }

    return true;
  }

  bool RuntimeComposition::ReadProject()
  {
    const ProjectReading reading(&*_file_system, &_yaml, &*_logging, _application.program);

    bool none = false;
    std::vector<std::string> errors;
    if (!reading.Read(_project, _project_file, none, errors))
    {
      Say(errors);
      // what to start instead is said once, after the error
      if (none) { std::cerr << _hooks->AdviseWithoutAProject(_settings) << "\n"; }
      return Fail();
    }

    TakeSettingsOfTheProject();
    return true;
  }

  void RuntimeComposition::TakeSettingsOfTheProject()
  {
    // the file the settings of the game were declared in, for the messages
    // about them
    _settings.game_file = _project_file;

    _settings.organization = _project.organization;
    _settings.application = _project.name;
    // the settings of the game, which the settings files set under `game`
    _settings.game_settings = _project.settings;

    // the presets the settings and --quality choose from are the project's
    _settings.graphics_presets = _project.graphics_presets;

    // the entry scene of the project, unless the editor's option named
    // another; the runtime has no option for one
    if (_settings.scene_path.empty()) { _settings.scene_path = _project.entry_scene; }
  }

  bool RuntimeComposition::PlaceUserFolder()
  {
    if (!_file_system->PlaceUserDirectory(_settings.organization, _settings.application))
    {
      std::cerr << "The folder of the user cannot be placed, see the log\n";
      return Fail();
    }

    // The log file belongs under user://, and only the file system knows
    // where that is. Until now everything went to the console, and was held
    // back for the file, which gets it first.
    _file_system->PlaceLogFile(_settings.logpath, *_logging);
    return true;
  }

  bool RuntimeComposition::ReadSettings()
  {
    const SettingsLayers layers(&*_file_system, &_yaml, &*_logging, _application.program);
    if (std::vector<std::string> errors; !layers.ReadFiles(_settings, errors))
    {
      Say(errors);
      return Fail();
    }

    // the command line wins over the files
    if (!ApplyCommandLine(true)) { return false; }
    if (!_display_options.GetWarnings().empty())
    {
      const auto logger = _logging->CreateLogger(_application.program);
      for (const auto &warning : _display_options.GetWarnings()) { logger->Warn("{}", warning); }
    }

    _hooks->AdjustSettings(_settings, _project, _command_line, *_logging->CreateLogger(_application.program));

    DeclareGameSettings();
    return true;
  }

  void RuntimeComposition::DeclareGameSettings()
  {
    // The menu changes them, the scripts read them, and a change is heard
    // by whoever subscribed. The files were checked above, so nothing is
    // refused here.
    const auto logger = _logging->CreateLogger("SettingsStore");
    _settings_store.emplace(logger);
    for (const auto &declaration : _settings.game_settings) { (void) _settings_store->Declare(declaration); }
    if (const std::size_t declared = _settings.game_settings.size(); declared > 0)
    {
      logger->Info("The game declares {} settings of its own", declared);
    }
  }

  bool RuntimeComposition::StartExtensions()
  {
    // Every folder of extensions://: next to the runtime, or in the folder
    // of the project the editor opens. Nothing lists them, and without the
    // folder there are none.
    _library_loader.emplace(&*_file_system);
    _extension_host.emplace(
      &*_file_system,
      &_yaml,
      &*_library_loader,
      &*_logging,
      _logging->CreateLogger("ExtensionHost"));
    _extension_host->Initialize();
    return true;
  }

  bool RuntimeComposition::CreateSystems()
  {
    // the window is named after the project unless the settings, or the
    // application, say otherwise
    if (_settings.title.empty()) { _settings.title = _project.name; }

    CreateAudio();
    if (!CreateWindowAndInput()) { return false; }
    if (!CreateRenderer()) { return false; }
    CreateUserInterface();
    CreateWorld();
    CreateRuntime();
    return true;
  }

  void RuntimeComposition::CreateAudio()
  {
    // Without a window there is no one to hear anything. Sounds are still
    // read and mixed, so that a run without a window finds a sound that is
    // broken.
    if (_settings.headless_renderer) { _settings.audio_output = AudioOutput::None; }

    _audio_system.emplace(_settings, &*_file_system, _logging->CreateLogger("MA_AudioSystem"));
    _audio_system->Initialize();
  }

  bool RuntimeComposition::CreateWindowAndInput()
  {
    if (_settings.headless_renderer)
    {
      _window_system = &_headless_window_system.emplace(_settings, _logging->CreateLogger("Headless_WindowSystem"));
      _input_system = &_headless_input_system.emplace(_settings, _logging->CreateLogger("Headless_InputSystem"));
    } else
    {
      _window_system = &_sdl2_window_system.emplace(_settings, _logging->CreateLogger("SDL2_WindowSystem"));
      _input_system = &_sdl2_input_system.emplace(_settings, _window_system, _logging->CreateLogger("SDL2_InputSystem"));
    }

    const InputMapLoading input_map(&*_file_system, &_yaml, &*_logging, _application.in_prose);
    std::vector<std::string> remarks;
    std::vector<std::string> errors;
    const bool loaded = input_map.Load(_project.input, *_input_system, remarks, errors);
    Say(remarks);
    if (!loaded)
    {
      Say(errors);
      return Fail();
    }

    // the player's switch for the gyro, over what the map says. A settings
    // menu will turn it later through the same call
    if (_settings.gyro_enabled.has_value()) { _input_system->SetSensorEnabled("gyro", *_settings.gyro_enabled); }

    if (_settings.headless_renderer)
    {
      const InputScriptLoading input_script(&*_file_system);
      if (!input_script.Load(_settings, *_headless_input_system, errors))
      {
        Say(errors);
        return Fail();
      }
    }

    return true;
  }

  bool RuntimeComposition::CreateRenderer()
  {
    switch (_settings.selected_api)
    {
      case RenderingApi::Vulkan:
      {
        _render_system = &_vk_render_system.emplace(
          _window_system,
          &*_file_system,
          _settings,
          _logging->CreateLogger("Vulkan_RenderSystem"));
        _render_2d_context = &*_vk_render_system;
        break;
      }
      default:
      {
        std::cerr << "The renderer that was asked for is not part of this build\n";
        return Fail();
      }
    }

    _render_pipeline.emplace(_render_system, _settings.max_light_sources, _logging->CreateLogger("Forward_RenderPipeline"));
    return true;
  }

  void RuntimeComposition::CreateUserInterface()
  {
    _entity_store.emplace(_logging->CreateLogger("Flecs_EntityStore"));

    // Its text is drawn with Inter unless a file names another font. Glyphs
    // are drawn with FreeType and text is shaped with HarfBuzz.
    // STB_FontRasterizer of neon-stb stands in for the first where neither
    // is wanted, and draws text that is not shaped.
    _font_rasterizer.emplace();
    _text_shaper.emplace();

    _ui_system.emplace(
      _render_2d_context,
      &*_font_rasterizer,
      &_layout_engine,
      _input_system,
      &*_file_system,
      &_yaml,
      UiSettings{
        .fonts = {
          {"sans-serif", 400, "engine://fonts/inter/Inter-Regular.ttf"},
          {"sans-serif", 700, "engine://fonts/inter/Inter-Bold.ttf"}
        },
        .start_path = _settings.ui_path
      },
      _logging->CreateLogger("Tree_UiSystem"));

    _ui_system->SetTextShaper(&*_text_shaper);

    // Images are read with stb_image, and those that are made of shapes are
    // drawn with LunaSVG at the size they have on the screen.
    _vector_image_rasterizer.emplace();
    _ui_system->SetImageDecoder(&_image_decoder);
    _ui_system->SetVectorImageRasterizer(&*_vector_image_rasterizer);

    // The user interface follows the window for its size and density, and
    // scales by what the player asked for on top. Text is cut and pasted
    // through the clipboard of the platform where there is one.
    _ui_system->SetWindow(_window_system);
    if (!_settings.headless_renderer) { _ui_system->SetClipboard(&_clipboard); }
    _ui_system->SetUserScale(static_cast<float>(_settings.ui_scale));
  }

  void RuntimeComposition::CreateWorld()
  {
    _scene.emplace(&*_file_system, &_yaml, _settings.scene_path, _logging->CreateLogger("SceneFile"));

    // a scene names the user interface it comes with as a component, and
    // the user interfaces that are shown on surfaces in the world
    _scene->GetComponentFormats().Add(UiViewFormat());
    _scene->GetComponentFormats().Add(UiSurfaceFormat());

    // the world reads the input less what the user interface has used
    _world.emplace(
      &*_entity_store,
      &*_scene,
      &*_render_pipeline,
      _ui_system->GetGameInput(),
      _window_system,
      _logging->CreateLogger("EntityWorld"));

    // sounds are heard where their entities are drawn, so that a sound is
    // not heard from where it was a frame ago, or from the origin in the
    // first frame, before anything was placed
    _world->AddSystemAfterPlacing(
      std::make_unique<AudioPlayback>(&*_audio_system, _logging->CreateLogger("AudioPlayback")));
    _world->AddSystem(std::make_unique<UiViewLoading>(&*_ui_system, _logging->CreateLogger("UiViewLoading")));
    _world->AddSystem(std::make_unique<UiClock>(&*_ui_system));
    _world->AddSystem(std::make_unique<UiSurfaceLoading>(&*_ui_system, _logging->CreateLogger("UiSurfaceLoading")));

    // The physics. The world steps at a fixed rate, and the system that is
    // added below takes a step of the physics in each. Systems of a game are
    // added before it, so that what they ask for in a step is part of it.
    _physics_system.emplace(_settings, _logging->CreateLogger("Jolt_PhysicsSystem"));
    _physics_system->Initialize();

    // the rays of the entities that cast one, through the physics, and then
    // what reads where they hit: the pointer on the screens in the world
    _world->AddSystem(std::make_unique<RayCasting>(&*_physics_system));
    _world->AddSystem(std::make_unique<UiSurfacePointing>(&*_ui_system, _ui_system->GetGameInput()));

    // What the extensions bring to the world. They register their components
    // before the scripts are read, so that a script finds them.
    _extension_host->SetInput(_ui_system->GetGameInput());
    _extension_host->SetWorld(&*_world);
    _extension_host->SetPhysics(&*_physics_system);
    _extension_host->SetUi(&*_ui_system);
    _extension_host->SetRender(_render_system);
    _extension_host->SetAudio(&*_audio_system);
    // an extension that asks the application to close tells the window, as
    // the pause menu does
    _extension_host->SetWindow(_window_system);
    _world->AddSystem(std::make_unique<ExtensionRunning>(&*_extension_host, &_scene->GetComponentFormats()));

    // The game's own code: scripts in Lua anywhere under assets://, which
    // declare components and systems as the engine's code does. They read
    // the input the user interface left, and may ask for another scene.
    _script_system.emplace(&*_file_system, _logging->CreateLogger("Lua_ScriptSystem"));
    _script_system->SetJit(_settings.script_jit);
    _script_system->Initialize();
    _script_system->SetInput(_ui_system->GetGameInput());
    _script_system->SetUi(&*_ui_system);
    // the settings of the game, which a script reads and hears of
    _script_system->SetSettings(&*_settings_store);
    // the volumes of the groups, which a menu of the game sets
    _script_system->SetAudio(&*_audio_system);
    _script_system->SetWorld(&*_world);
    auto script_running = std::make_unique<ScriptRunning>(
      &*_script_system,
      &*_physics_system,
      &_scene->GetComponentFormats(),
      "assets://",
      _logging->CreateLogger("ScriptRunning"));
    // an element of the user interface calls a function of the scripts
    script_running->SetUi(&*_ui_system);
    // an extension that needs scripts to run brings them in its own folder
    for (const auto &folder : _extension_host->GetScriptFolders()) { script_running->AddFolder(folder); }
    _world->AddSystem(std::move(script_running));

    // the player, driven by what the user interface left of the input. It
    // sets the velocity of its body, which the physics moves by
    _world->AddSystem(std::make_unique<FirstPersonControllerSystem>(_ui_system->GetGameInput()));

    _world->GetFixedClock().SetStepsPerSecond(_settings.steps_per_second);
    _world->GetFixedClock().SetMostStepsPerFrame(_settings.most_steps_per_frame);
    _world->AddSystem(std::make_unique<PhysicsSimulation>(
      &*_physics_system,
      &*_file_system,
      _logging->CreateLogger("PhysicsSimulation")));

    // ropes, drawn between their ends where the physics has just placed them
    _world->AddSystem(std::make_unique<RopeDrawing>(_logging->CreateLogger("RopeDrawing")));

    // the way out of a level, once the physics has said what entered it
    _world->AddSystem(std::make_unique<SceneExiting>(&*_world));

    // where the entities the command line names are, once in every frame,
    // for checks that read the log in place of the pixels. Where they are
    // drawn, so after placing
    if (!_settings.logged_entities.empty())
    {
      _world->AddSystemAfterPlacing(std::make_unique<EntityLogging>(
        _settings.logged_entities,
        &*_physics_system,
        _logging->CreateLogger("EntityLogging")));
    }
  }

  RuntimeSystems RuntimeComposition::Systems()
  {
    return RuntimeSystems{
      .settings = &_settings,
      .window = _window_system,
      .input = _input_system,
      .render = _render_system,
      .render_pipeline = &*_render_pipeline,
      .logging = &*_logging,
      .world = &*_world,
      .logger = _logger,
      .sdl2_window = _sdl2_window_system.has_value() ? &*_sdl2_window_system : nullptr,
      .sdl2_input = _sdl2_input_system.has_value() ? &*_sdl2_input_system : nullptr,
      .vk_render = _vk_render_system.has_value() ? &*_vk_render_system : nullptr
    };
  }

  void RuntimeComposition::CreateRuntime()
  {
    // The random numbers of the operating system, for the game. The engine
    // draws none itself, and no option or setting seeds them: a game that
    // is to repeat a run keeps the seed it took from here.
    _entropy.emplace(_logging->CreateLogger("OS_Entropy"));

    _logger = _logging->CreateLogger(_application.program);

    const RuntimeSystems systems = Systems();
    _runtime = &_hooks->MakeRuntime(systems);

    _runtime->SetUiSystem(&*_ui_system);
    _runtime->SetEntropy(&*_entropy);
    // the sounds that pause with the game are held while a menu is shown
    _runtime->SetAudio(&*_audio_system);

    // what the player chooses in the settings menu is kept for the next start
    _player_settings.emplace(&*_file_system, &_yaml, _logging->CreateLogger("PlayerSettings"));
    _runtime->SetPlayerSettings(&*_player_settings);
    // the rows of the settings menu named after the settings of the game
    _runtime->SetSettingsStore(&*_settings_store);

    _hooks->AddSystems(systems);
  }

  void RuntimeComposition::Run()
  {
    // a script that starts the application learns from the exit code whether
    // the run did what it was asked to
    bool failed = false;
    try
    {
      _runtime->Run();
      failed = _runtime->HasFailed();
    } catch (const std::exception &e)
    {
      _logger->Critical(e.what());
      failed = true;
    }

    if (failed) { Fail(); }
  }

  int RuntimeComposition::CleanUp()
  {
    // Every CleanUp is safe to call more than once. Doing it here guarantees
    // the systems shut down before the file system they depend on. The
    // extensions reach the store no more once the world is cleaned up, and
    // the host goes after everything that could hold what an extension
    // brought.
    if (_extension_host.has_value()) { _extension_host->LeaveWorld(); }
    if (_runtime != nullptr) { _runtime->CleanUp(); }
    if (_script_system.has_value()) { _script_system->CleanUp(); }
    if (_audio_system.has_value()) { _audio_system->CleanUp(); }
    if (_physics_system.has_value()) { _physics_system->CleanUp(); }
    if (_extension_host.has_value()) { _extension_host->CleanUp(); }
    if (_file_system.has_value()) { _file_system->CleanUp(); }

    return _exit_code;
  }
} // neon
