#ifndef EDITOR_HOOKS_HPP
#define EDITOR_HOOKS_HPP

#include <optional>
#include <string>

#include <neon/command-line/editor-options.hpp>
#include <neon/editor/editor.hpp>
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

    // the editor before the tools, since the tools are taken down first
    std::optional<Editor> _editor;
    std::optional<ImGui_ToolUiSystem> _tool_ui;

  public:
    void RegisterOptions(CommandLine &command_line) override;

    bool ApplyOptions(const CommandLineContext &command_line, SettingsConfig &settings, std::string &error) override;

    /// The folder the command line names, which holds the project's assets
    /// and extensions. Without one the editor says so and stops.
    bool ChooseProjectFolder(const CommandLine &command_line, SettingsConfig &settings, std::string &error) override;

    [[nodiscard]] std::string AdviseWithoutAProject(const SettingsConfig &settings) const override;

    /// The window is the editor's, not the game's: always a window that can
    /// be moved and resized, whatever window.mode of the project's settings
    /// or the command line says, at a size of its own unless the command
    /// line names one, and named after the editor. The project's mode and
    /// size are what the game is played in. Where the editor keeps a size
    /// of its own is #582.
    void AdjustSettings(
      SettingsConfig &settings,
      const Project &project,
      const CommandLineContext &command_line,
      Logger &logger) override;

    Runtime &MakeRuntime(const RuntimeSystems &systems) override;

    /// The pointer is the editor's, and the tool interface is drawn with
    /// Dear ImGui into the window through the backends of SDL2 and Vulkan.
    /// Without a window there are no tools, and the editor runs as the
    /// runtime does, for the checks. With them, the viewport looks through
    /// the editor's camera, which is handed to the pipeline after the
    /// scene's.
    void AddSystems(const RuntimeSystems &systems) override;
  };
} // neon

#endif //EDITOR_HOOKS_HPP
