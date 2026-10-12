#include "editor-hooks.hpp"

#include <memory>

#include <neon/editor/camera/editor-camera-submission.hpp>
#include <neon/editor/scripts/editor-script-loading.hpp>

namespace neon
{
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
    if (!settings.headless_renderer)
    {
      if (settings.window_mode != WindowMode::Windowed)
      {
        logger.Info("The editor runs in a window, whatever the window mode asks");
        settings.window_mode = WindowMode::Windowed;
      }
      if (!command_line.IsSet("window-size"))
      {
        settings.width = 1600;
        settings.height = 1000;
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

    // Without a window the tools are drawn only when asked for: the checks
    // of the scene want the frame as a run of the game would save it
    const bool with_tools = !settings.headless_renderer || settings.editor_tools || !settings.editor_script_path.empty();

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
    systems.world->AddSystemAfterPlacing(
      std::make_unique<EditorCameraSubmission>(&_editor->GetCamera(), systems.render_pipeline));

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
