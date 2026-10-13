#ifndef RUNTIME_COMPOSITION_HPP
#define RUNTIME_COMPOSITION_HPP

#include <memory>
#include <optional>
#include <string>

#include <neon/audio/ma-audio-system.hpp>
#include <neon/command-line/command-line.hpp>
#include <neon/command-line/display-options.hpp>
#include <neon/command-line/runtime-options.hpp>
#include <neon/data/ryml-document-format.hpp>
#include <neon/extension/extension-host.hpp>
#include <neon/extension/sdl2-library-loader.hpp>
#include <neon/filesystem/sdl2-file-system.hpp>
#include <neon/image/luna-vector-image-rasterizer.hpp>
#include <neon/image/stb-image-decoder.hpp>
#include <neon/input/headless-input-system.hpp>
#include <neon/input/sdl2-clipboard.hpp>
#include <neon/input/sdl2-input-system.hpp>
#include <neon/layout/flex-layout-engine.hpp>
#include <neon/logging/logging-system.hpp>
#include <neon/physics/jolt-physics-system.hpp>
#include <neon/project/project.hpp>
#include <neon/random/os-entropy.hpp>
#include <neon/render/forward-render-pipeline.hpp>
#include <neon/render/vk-render-system.hpp>
#include <neon/runtime/runtime.hpp>
#include <neon/runtime/settings-config.hpp>
#include <neon/scripting/lua-script-system.hpp>
#include <neon/settings/player-settings.hpp>
#include <neon/settings/settings-store.hpp>
#include <neon/text/ft-font-rasterizer.hpp>
#include <neon/text/hb-text-shaper.hpp>
#include <neon/ui/tree-ui-system.hpp>
#include <neon/window/headless-window-system.hpp>
#include <neon/window/sdl2-window-system.hpp>
#include <neon/world-system/ecs/entity-world.hpp>
#include <neon/world-system/ecs/scene-file/scene-file.hpp>
#include <neon/world-system/flecs-entity-store.hpp>

#include "application-description.hpp"
#include "composition-hooks.hpp"
#include "runtime-systems.hpp"

namespace neon
{
  /// The start-up NeonRuntime and NeonEditor share, as the steps an
  /// application's main() calls in order, each a method named for what it
  /// does. It is the one place that names the backends: the applications
  /// name this library and their own objects. Where an application differs
  /// it is asked through CompositionHooks, see there.
  ///
  /// Every step returns whether the start-up goes on. A step that fails has
  /// said why on the console, or in the log once it has its file, and the
  /// exit code is 1, so that a script learns of it and nobody sees a black
  /// screen; `--help` ends the start-up the same way with exit code 0.
  /// CleanUp() is called whatever happened and returns the exit code: it
  /// takes down what was made, in the reverse of the order it came up.
  ///
  ///   if (composition.ParseCommandLine(argc, argv) &&
  ///       composition.OpenFileSystem() &&
  ///       composition.ReadProject() &&
  ///       composition.PlaceUserFolder() &&
  ///       composition.ReadSettings() &&
  ///       composition.StartExtensions() &&
  ///       composition.CreateSystems())
  ///   {
  ///     composition.Run();
  ///   }
  ///   return composition.CleanUp();
  class RuntimeComposition final
  {
    ApplicationDescription _application;
    CompositionHooks *_hooks;
    int _exit_code;

    // Everything the start-up makes, in the order it is made, which is the
    // reverse of the order it is destroyed in. What needs the settings or
    // another system to be made waits in an optional until its step.
    CommandLine _command_line;
    RuntimeOptions _runtime_options;
    DisplayOptions _display_options;
    SettingsConfig _settings;

    std::optional<LoggingSystem> _logging;
    std::optional<SDL2_FileSystem> _file_system;
    RYML_DocumentFormat _yaml;
    Project _project;
    std::string _project_file;

    std::optional<SettingsStore> _settings_store;

    std::optional<SDL2_LibraryLoader> _library_loader;
    std::optional<ExtensionHost> _extension_host;

    std::optional<MA_AudioSystem> _audio_system;

    // only the systems that were asked for are made, and the rest of the
    // engine sees them through the pointers below
    std::optional<SDL2_WindowSystem> _sdl2_window_system;
    std::optional<Headless_WindowSystem> _headless_window_system;
    std::optional<SDL2_InputSystem> _sdl2_input_system;
    std::optional<Headless_InputSystem> _headless_input_system;
    std::optional<VK_RenderSystem> _vk_render_system;
    WindowSystem *_window_system = nullptr;
    InputSystem *_input_system = nullptr;
    RenderSystem *_render_system = nullptr;
    Render2DContext *_render_2d_context = nullptr;

    std::optional<Forward_RenderPipeline> _render_pipeline;
    std::optional<Flecs_EntityStore> _entity_store;

    std::optional<FT_FontRasterizer> _font_rasterizer;
    std::optional<HB_TextShaper> _text_shaper;
    Flex_LayoutEngine _layout_engine;
    std::optional<Tree_UiSystem> _ui_system;
    STB_ImageDecoder _image_decoder;
    std::optional<LUNA_VectorImageRasterizer> _vector_image_rasterizer;
    SDL2_Clipboard _clipboard;

    std::optional<SceneFile> _scene;
    std::optional<EntityWorld> _world;
    std::optional<Jolt_PhysicsSystem> _physics_system;
    std::optional<Lua_ScriptSystem> _script_system;
    std::optional<OS_Entropy> _entropy;

    std::shared_ptr<Logger> _logger;
    Runtime *_runtime = nullptr;
    std::optional<PlayerSettings> _player_settings;

    /// Applies the command line to the settings: the runtime's sets, then
    /// the application's. It is applied twice, before the files, since the
    /// file system needs what it says to come up, and after them, so that
    /// it wins; `--quality` waits for the second pass, since it may name a
    /// preset the project defines. False once the mistake and the help text
    /// are on the console.
    bool ApplyCommandLine(bool with_quality);

    /// Takes the settings the project decides over: who makes it and what
    /// it is called, which place `user://`, the settings and presets it
    /// declares, and the scene it starts with.
    void TakeSettingsOfTheProject();

    /// Declares the settings the game declared for itself in the store,
    /// with what the player's file set them to.
    void DeclareGameSettings();

    /// The sounds, read and mixed with or without a window.
    void CreateAudio();

    /// The window and the input, or what stands in for them without a
    /// window, with the input map of the project and the script of a run
    /// without a window.
    bool CreateWindowAndInput();

    /// The renderer the settings ask for, and the pipeline on it.
    bool CreateRenderer();

    /// The user interface of the game: menus, and what is shown during play.
    void CreateUserInterface();

    /// The scene, the world that reads it, and every system of the world in
    /// the order they run: sounds, the user interface, the physics, the
    /// extensions, the scripts, the player, the simulation, and the checks.
    void CreateWorld();

    /// The Runtime of the application, on everything above, and what the
    /// application adds after it.
    /// False when the application's hooks refuse, see
    /// CompositionHooks::AddSystems().
    bool CreateRuntime();

    /// Says each line on the console, where it goes until the log has its
    /// file, and where a mistake in the command line is answered.
    static void Say(const std::vector<std::string> &lines);

    /// The pointers the hooks are handed.
    [[nodiscard]] RuntimeSystems Systems();

    /// Marks the start-up as failed. Returns false, so that a step ends
    /// with `return Fail();`.
    bool Fail();

  public:
    RuntimeComposition(const ApplicationDescription &application, CompositionHooks *hooks);

    /// Parses the command line with the option sets the runtime owns and
    /// those the hooks register, answers `--help`, lets the hooks choose the
    /// project folder, and applies the command line a first time.
    bool ParseCommandLine(int argc, char *argv[]);

    /// Brings up the log, to the console for now, and the file system, which
    /// everything that loads files depends on.
    bool OpenFileSystem();

    /// Reads the project behind `assets://`: `game.yml` of a built game
    /// first, `project.yml` without it, see ProjectReading. Its problems go
    /// to the console, since the log file has no place yet.
    bool ReadProject();

    /// Places `user://` by who makes the project and what it is called, the
    /// log file under it, which gets everything that was held back, and the
    /// folder the hooks place for the application itself.
    bool PlaceUserFolder();

    /// Reads the files of the settings, see SettingsLayers, applies the
    /// command line over them, gives the hooks their last word, and
    /// declares the settings of the game.
    bool ReadSettings();

    /// Starts every extension of `extensions://`, once the log has its file
    /// and before any other system comes up, so that what one brings is
    /// there for all of them.
    bool StartExtensions();

    /// Creates the systems in the order both applications use, the world and
    /// what runs in it, and the Runtime the hooks make.
    bool CreateSystems();

    /// Runs the Runtime until its window closes. Whatever the run was asked
    /// to do and could not shows in the exit code.
    void Run();

    /// Takes everything down, in the reverse of the order it came up, and
    /// returns the exit code of the process. Safe after a step failed.
    int CleanUp();
  };
} // neon

#endif //RUNTIME_COMPOSITION_HPP
