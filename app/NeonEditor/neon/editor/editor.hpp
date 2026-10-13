#ifndef EDITOR_HPP
#define EDITOR_HPP

#include <memory>
#include <optional>

#include <neon/runtime/runtime.hpp>
#include <neon/tool-ui/tool-ui-system.hpp>

#include "camera/editor-camera.hpp"
#include "settings/editor-folder.hpp"
#include "scripts/editor-script.hpp"
#include "scripts/editor-script-runner.hpp"
#include "ui/editor-log.hpp"
#include "ui/editor-viewport.hpp"
#include "ui/editor-windows.hpp"

namespace neon
{
  /// What NeonEditor runs: the runtime with the tool interface on top of
  /// it. The scene is drawn into the viewport, a window of the editor that
  /// is always shown and fills the window at the start; a menu bar runs
  /// along the top, and the tool windows, the log among them, dock into the
  /// space under it once they are shown from the View menu, see
  /// EditorWindows (#590). Without a tool interface, as without a window,
  /// it runs as the runtime does, which is how the editor renders without
  /// a window for the checks (#310).
  ///
  /// Scene editing, play mode, and the other views are issues of their
  /// own under #82.
  class Editor final : public Runtime
  {
    ToolUiSystem *_tool_ui = nullptr;
    EditorFolder *_folder = nullptr;

    // the camera of the viewport, which the world is told to draw with
    // through an EditorCameraSubmission the application adds
    EditorCamera _camera;

    // made when the run starts, since the systems are up by then
    std::optional<EditorViewport> _viewport;
    std::optional<EditorLog> _log;

    // the tool windows, shown and hidden from the View menu
    EditorWindows _windows;
    bool _show_about = false;

    // the script that drives the tools, when there is one, and what runs
    // it once the run starts
    std::optional<EditorScript> _script;
    std::optional<EditorScriptRunner> _script_runner;

    bool _cleaned_up = false;

    /// Registers the editor's own tool windows, once the systems are up.
    void RegisterWindows();

    /// Brings back what the run before left in the editor's folder: the
    /// layout of the windows, and which tool windows were shown.
    void RestoreLayout();

    /// Keeps how the run ends for the next one: the window's size and
    /// mode, the tool windows that are shown, and the layout.
    void KeepLayout();
    /// Runs the script's part of the frame after it was drawn: a failed
    /// step fails the run, a finished script ends it unless --frames says
    /// more.
    void AfterScriptedFrame();

    /// Declares the menu bar: File, View, and Help.
    void DrawMenuBar();

    /// Declares the windows of this frame: the viewport, the tool windows
    /// that are shown, and the window about the editor.
    void DrawWindows();

  public:
    Editor(
      const SettingsConfig &settings_config,
      WindowSystem *window_system,
      InputSystem *input_system,
      RenderSystem *render_system,
      RenderPipeline *render_pipeline,
      LoggingSystem *logging_system,
      WorldSystem *world_system,
      const std::shared_ptr<Logger> &logger);

    ~Editor() override;

    /// Gives the editor the tool interface it draws its windows with.
    /// Call it before Run(). Without one the editor runs as the runtime.
    void SetToolUi(ToolUiSystem *tool_ui);

    /// Gives the editor its own folder, where the layout of its windows
    /// and its settings are kept between runs (#582). Call it before
    /// Run(). Without one nothing is kept.
    void SetEditorFolder(EditorFolder *folder);
    /// Gives the editor a script that drives its tools by their labels,
    /// see docs/editor.md#scripted-runs. Call it before Run(), with a tool
    /// interface set.
    void SetScript(const EditorScript &script);

    /// The camera of the viewport, for the EditorCameraSubmission the
    /// application adds to the world.
    [[nodiscard]] EditorCamera &GetCamera() { return _camera; }

    /// The tool windows, for whoever registers one next to the editor's
    /// own, see EditorWindows.
    [[nodiscard]] EditorWindows &GetWindows() { return _windows; }

    void Run() override;

    void CleanUp() override;
  };
} // neon

#endif //EDITOR_HPP
