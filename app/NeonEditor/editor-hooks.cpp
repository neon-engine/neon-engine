#include "editor-hooks.hpp"

#include <memory>

#include <neon/editor/camera/editor-camera-submission.hpp>
#include <neon/editor/scripts/editor-script-loading.hpp>
#include <neon/editor/toolchains/toolchain-file.hpp>
#include <neon/editor/toolchains/toolchain-files.hpp>

namespace neon
{
  // Helpers of EditorHooks: whether the tools are drawn in a run.
  namespace
  {
    /// The tools are drawn in a window, and without one only when asked for:
    /// the checks of the scene want the frame as a run of the game would
    /// save it.
    bool draws_tools(const SettingsConfig &settings)
    {
      return !settings.headless_renderer || settings.editor_tools || !settings.editor_script_path.empty();
    }
  }

  void EditorHooks::RegisterOptions(CommandLine &command_line)
  {
    command_line.SetArgument("FOLDER", "The folder of the project to open, which holds its assets and extensions");
    _editor_options.Register(command_line);
  }

  bool EditorHooks::ApplyOptions(const CommandLineContext &command_line, SettingsConfig &settings, std::string &error)
  {
    return _editor_options.Apply(command_line, settings, error);
  }

  bool EditorHooks::ChooseProjectFolder(const CommandLine &command_line, SettingsConfig &settings, std::string &error)
  {
    if (command_line.GetArgument().empty())
    {
      error = "NeonEditor needs the folder of a project to open, the one that holds its assets folder: "
              "NeonEditor FOLDER, such as the sandbox of the engine's build, bin/<build>/<platform>/sandbox, "
              "see docs/editor.md";
      return false;
    }

    settings.project_directory = command_line.GetArgument();
    _project_directory = settings.project_directory;
    return true;
  }

  bool EditorHooks::PlaceApplicationFolder(FileSystem &file_system, DocumentFormat &format, LoggingContext &logging)
  {
    if (!file_system.PlaceEditorDirectory(std::string(EditorFolder::organization), std::string(EditorFolder::application)))
    {
      return false;
    }

    // a file with a mistake is said and refused, and the editor goes on
    // with its defaults, see EditorFolder
    _folder.emplace(&file_system, &format, logging.CreateLogger("EditorFolder"));
    _folder->ReadSettings();
    _folder->GetSettings().last_project = _project_directory;
    return true;
  }

  std::string EditorHooks::AdviseWithoutAProject(const SettingsConfig &settings) const
  {
    return "NeonEditor opens the folder of a project, which holds assets/project.yml: " + settings.project_directory +
           " has none. See docs/projects.md";
  }

  void EditorHooks::AdjustSettings(
    SettingsConfig &settings,
    const Project &project,
    const CommandLineContext &command_line,
    Logger &logger)
  {
    // the editor's window is its own, not the game's; the tools drawn
    // without a window stand for it, so that a script sees its size
    if (draws_tools(settings) && _folder.has_value())
    {
      const EditorSettings &own = _folder->GetSettings();
      if (!command_line.IsSet("window-mode"))
      {
        if (settings.window_mode != own.window_mode)
        {
          logger.Info("The editor's window is shown as the editor's own settings say, not as the project's");
        }
        settings.window_mode = own.window_mode;
      }
      if (!command_line.IsSet("window-size"))
      {
        settings.width = own.window_width;
        settings.height = own.window_height;
      }
    }

    if (settings.title.empty()) { settings.title = "NeonEditor - " + project.name; }
  }

  Runtime &EditorHooks::MakeRuntime(const RuntimeSystems &systems)
  {
    return _editor.emplace(
      *systems.settings,
      systems.window,
      systems.input,
      systems.render,
      systems.render_pipeline,
      systems.logging,
      systems.world,
      systems.logger);
  }

  bool EditorHooks::AddSystems(const RuntimeSystems &systems)
  {
    const SettingsConfig &settings = *systems.settings;

    const bool with_tools = draws_tools(settings);

    // The pointer is the editor's: free and shown, for the tools. A game the
    // editor plays takes it then (#569). Without the tools the editor runs
    // as the runtime does, and the pointer is nothing.
    systems.world->SetCapturesPointer(!with_tools);

    if (!with_tools) { return true; }

    if (settings.headless_renderer)
    {
      _tool_ui.emplace(systems.window, systems.vk_render, systems.logging->CreateLogger("ImGui_ToolUiSystem"));
    } else
    {
      _tool_ui.emplace(
        systems.sdl2_window,
        systems.sdl2_input,
        systems.vk_render,
        systems.logging->CreateLogger("ImGui_ToolUiSystem"));
    }
    _editor->SetToolUi(&*_tool_ui);
    if (_folder.has_value()) { _editor->SetEditorFolder(&*_folder); }
    systems.world->AddSystemAfterPlacing(
      std::make_unique<EditorCameraSubmission>(&_editor->GetCamera(), systems.render_pipeline));

    // the toolchain files of this machine say where VS Code is; a file with
    // a mistake has said so in the log, which the log window shows
    _processes.emplace(systems.logging->CreateLogger("OS_ProcessRunner"));
    _native_files.emplace(systems.logging->CreateLogger("OS_NativeFiles"));
    const ToolchainFile file(systems.file_system, &_yaml, systems.logger);
    const ToolchainFiles files(&file, systems.file_system, systems.logger);
    files.Read(_toolchains);
    _external_editor.emplace(
      &_toolchains,
      &*_processes,
      &*_native_files,
      systems.library_loader->GetPlatform(),
      systems.logger);
    _editor->SetExternalEditor(&*_external_editor);

    // the script that drives the tools, read now so that one that cannot
    // be read ends the start-up with the reason, not after a frame
    const EditorScriptLoading loading(systems.file_system, systems.document_format);
    EditorScript script;
    std::vector<std::string> errors;
    if (!loading.Load(settings, script, errors))
    {
      for (const std::string &error : errors) { systems.logger->Error(error); }
      return false;
    }
    if (!script.IsEmpty()) { _editor->SetScript(script); }
    return true;
  }
} // neon
