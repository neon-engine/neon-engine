#include "editor.hpp"

#include <neon/runtime/frame-capture.hpp>

namespace neon
{
  Editor::Editor(
    const SettingsConfig &settings_config,
    WindowSystem *window_system,
    InputSystem *input_system,
    RenderSystem *render_system,
    RenderPipeline *render_pipeline,
    LoggingSystem *logging_system,
    WorldSystem *world_system,
    const std::shared_ptr<Logger> &logger)
    : Runtime(
      settings_config,
      window_system,
      input_system,
      render_system,
      render_pipeline,
      logging_system,
      world_system,
      logger) {}

  Editor::~Editor()
  {
    Editor::CleanUp();
  }

  void Editor::SetToolUi(ToolUiSystem *tool_ui)
  {
    _tool_ui = tool_ui;
  }

  void Editor::DrawMenuBar()
  {
    if (!_tool_ui->BeginMainMenuBar()) { return; }

    if (_tool_ui->BeginMenu("File"))
    {
      if (_tool_ui->MenuItem("Quit"))
      {
        _logger->Info("Quit was chosen in the File menu");
        _window_system->SignalToClose();
      }
      _tool_ui->EndMenu();
    }

    if (_tool_ui->BeginMenu("View"))
    {
      if (_tool_ui->MenuItem(std::string(EditorViewport::title), "", _show_viewport)) { _show_viewport = !_show_viewport; }
      if (_tool_ui->MenuItem(std::string(EditorLog::title), "", _show_log)) { _show_log = !_show_log; }
      _tool_ui->EndMenu();
    }

    if (_tool_ui->BeginMenu("Help"))
    {
      if (_tool_ui->MenuItem("About NeonEditor", "", _show_about)) { _show_about = !_show_about; }
      _tool_ui->EndMenu();
    }

    _tool_ui->EndMainMenuBar();
  }

  void Editor::DrawWindows()
  {
    if (_show_viewport) { _viewport->Draw(*_tool_ui); }
    if (_show_log) { _log->Draw(*_tool_ui); }

    if (_show_about)
    {
      if (_tool_ui->BeginWindow("About NeonEditor"))
      {
        _tool_ui->Text("NeonEditor edits and runs a Neon Engine project.");
        _tool_ui->Text("The project is " + _settings_config.application + " by " + _settings_config.organization + ".");
        _tool_ui->Separator();
        if (_tool_ui->Button("Close")) { _show_about = false; }
      }
      _tool_ui->EndWindow();
    }
  }

  void Editor::Run()
  {
    // without the tools, as without a window, the editor is the runtime
    if (_tool_ui == nullptr)
    {
      Runtime::Run();
      return;
    }

    Initialize();

    if (!_tool_ui->Initialize())
    {
      _logger->Critical("The tool interface cannot be drawn, so the editor stops");
      _failed = true;
      return;
    }

    _viewport.emplace(_render_system, _render_pipeline, _window_system, _input_system, &_camera, _logger);
    _log.emplace(_logging_system);

    const FrameCapture frame_capture(_settings_config, _render_system);
    std::size_t frames_rendered = 0;

    // a scene that cannot be read is said; the editor stays open all the
    // same, since it is where the scene would be corrected
    if (_world_system->HasNoScene()) { _logger->Error("There is no scene to show, see above"); }

    const ToolUiDockLayout layout{
      .center = std::string(EditorViewport::title),
      .bottom = std::string(EditorLog::title),
      .bottom_share = 0.25f
    };

    while (_window_system->IsRunning())
    {
      _input_system->ProcessInput();

      _render_system->PrepareFrame();
      _tool_ui->BeginFrame();

      // the scene is drawn into the viewport, at the size it had in the
      // frame before
      _viewport->Prepare();
      _world_system->Update();

      // and the tools over the frame
      DrawMenuBar();
      _tool_ui->DockSpace(layout);
      DrawWindows();
      _tool_ui->EndFrame();

      _render_system->FinishFrame();
      frames_rendered++;

      if (!frame_capture.AfterFrame(frames_rendered)) { _failed = true; }

      if (_settings_config.max_frames > 0 && frames_rendered >= _settings_config.max_frames)
      {
        _logger->Info("Rendered {} frames, stopping", frames_rendered);
        _window_system->SignalToClose();
      }

      _window_system->Update();
    }
  }

  void Editor::CleanUp()
  {
    if (_cleaned_up) { return; }
    _cleaned_up = true;

    // the tools draw with the renderer, so they go before it does
    if (_viewport.has_value()) { _viewport->CleanUp(); }
    if (_tool_ui != nullptr) { _tool_ui->CleanUp(); }

    Runtime::CleanUp();
  }
} // neon
