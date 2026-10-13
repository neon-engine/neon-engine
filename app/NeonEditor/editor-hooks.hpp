#ifndef EDITOR_HOOKS_HPP
#define EDITOR_HOOKS_HPP

#include <optional>
#include <string>

#include <neon/command-line/editor-options.hpp>
#include <neon/data/ryml-document-format.hpp>
#include <neon/editor/editor.hpp>
#include <neon/editor/settings/editor-folder.hpp>
#include <neon/editor/toolchains/toolchains.hpp>
#include <neon/editor/tools/external-editor.hpp>
#include <neon/filesystem/os-native-files.hpp>
#include <neon/process/os-process-runner.hpp>
#include <neon/runtime/composition-hooks.hpp>
#include <neon/tool-ui/imgui-tool-ui-system.hpp>

namespace neon
{
  /// NeonEditor's own choices on the shared start-up, see
  /// RuntimeComposition and docs/editor.md: the editor takes the folder of
  /// a project as its first argument, registers the editor's options next
  /// to the runtime's, insists on a window of its own, and with a window
  /// draws the tool interface over the scene, seen through its camera. It
  /// owns the Editor that is run and the tools it draws with.
  class EditorHooks final : public CompositionHooks
  {
    EditorOptions _editor_options;

    // the folder the command line named, as it was given, for the editor's
    // own settings
    std::string _project_directory;

    // the editor's own folder, placed once user:// is
    std::optional<EditorFolder> _folder;

    // the editor before the tools, since the tools are taken down first
    std::optional<Editor> _editor;
    std::optional<ImGui_ToolUiSystem> _tool_ui;

    // the processes and the files of the machine, which the toolchains and
    // the editor of the code are started and found through, and the
    // toolchains of this machine, read once when the tools are made
    std::optional<OS_ProcessRunner> _processes;
    std::optional<OS_NativeFiles> _native_files;
    RYML_DocumentFormat _yaml;
    Toolchains _toolchains;
    std::optional<ExternalEditor> _external_editor;

  public:
    void RegisterOptions(CommandLine &command_line) override;

    bool ApplyOptions(const CommandLineContext &command_line, SettingsConfig &settings, std::string &error) override;

    /// The folder the command line names, which holds the project's assets
    /// and extensions. Without one the editor says so and stops.
    bool ChooseProjectFolder(const CommandLine &command_line, SettingsConfig &settings, std::string &error) override;

    [[nodiscard]] std::string AdviseWithoutAProject(const SettingsConfig &settings) const override;

    /// Places `editor://`, the editor's own folder, by the editor's names,
    /// and reads the editor's settings from it, see EditorFolder (#582).
    bool PlaceApplicationFolder(FileSystem &file_system, DocumentFormat &format, LoggingContext &logging) override;

    /// The window is the editor's, not the game's: its size and its mode
    /// are those of the editor's own settings, as they were left, unless
    /// the command line names them, whatever the project's settings say,
    /// and it is named after the editor. The project's mode and size are
    /// what the game is played in. Without a window the same holds when
    /// the tools are drawn, `--tools` or `--editor-script`, since they
    /// stand for the window; otherwise the run is the game's, at its size.
    void AdjustSettings(
      SettingsConfig &settings,
      const Project &project,
      const CommandLineContext &command_line,
      Logger &logger) override;

    Runtime &MakeRuntime(const RuntimeSystems &systems) override;

    /// The pointer is the editor's, and the tool interface is drawn with
    /// Dear ImGui into the window through the backends of SDL2 and Vulkan.
    /// Without a window the tools are drawn only when `--tools` or a script
    /// of the editor asks for them, and the editor otherwise runs as the
    /// runtime does, for the checks of the scene. With the tools, the
    /// viewport looks through the editor's camera, which is handed to the
    /// pipeline after the scene's, the toolchain files of this machine are
    /// read, so that File > Open in VS Code knows where VS Code is, and the
    /// script `--editor-script` names is read; one that cannot be read
    /// stops the start-up.
    bool AddSystems(const RuntimeSystems &systems) override;
  };
} // neon

#endif //EDITOR_HOOKS_HPP
