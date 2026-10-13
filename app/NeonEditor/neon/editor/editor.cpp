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

  void Editor::SetEditorFolder(EditorFolder *folder)
  {
    _folder = folder;
  }

  void Editor::RestoreLayout()
  {
    if (_folder == nullptr) { return; }

    if (std::string layout; _folder->ReadLayout(layout)) { _tool_ui->LoadLayout(layout); }

    for (const auto &title : _folder->GetSettings().open_windows)
    {
      if (!_windows.Show(title, true))
      {
        _logger->Info("The window '{}' of the editor's settings is not one of this editor's, and is not shown", title);
      }
    }
  }

  void Editor::KeepLayout()
  {
    if (_folder == nullptr) { return; }

    EditorSettings &own = _folder->GetSettings();
    own.window_mode = _window_system->GetWindowMode();
    // the size is the window's own in a window alone; a display is not
    if (own.window_mode == WindowMode::Windowed)
    {
      const WindowMetrics metrics = _window_system->GetMetrics();
      if (metrics.point_width > 0 && metrics.point_height > 0)
      {
        own.window_width = metrics.point_width;
        own.window_height = metrics.point_height;
      }
    }
    own.open_windows = _windows.GetShownTitles();

    _folder->WriteSettings();
    _folder->WriteLayout(_tool_ui->SaveLayout());
  }

  void Editor::RegisterWindows()
  {
    _windows.Add({
      .title = std::string(EditorLog::title),
      .shortcut = "Ctrl+L",
      .side = ToolUiDockSide::Bottom,
      .share = 0.25f,
      .draw = [this](ToolUiContext &ui) { _log->Draw(ui); }
    });
  }

  void Editor::SetScript(const EditorScript &script)
  {
    _script = script;
  }

  void Editor::AfterScriptedFrame()
  {
    _script_runner->AfterFrame(*_tool_ui);

    if (_script_runner->HasFailed())
    {
      _logger->Error("The script failed, so the run ends with exit code 1");
      _failed = true;
      _window_system->SignalToClose();
      return;
    }

    if (_script_runner->IsDone() && _settings_config.max_frames == 0)
    {
      _logger->Info("The script is done, so the run ends");
      _window_system->SignalToClose();
    }
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

    _windows.DrawViewMenu(*_tool_ui);

    if (_tool_ui->BeginMenu("Help"))
    {
      if (_tool_ui->MenuItem("About NeonEditor", "", _show_about)) { _show_about = !_show_about; }
      _tool_ui->EndMenu();
    }

    _tool_ui->EndMainMenuBar();
  }

  void Editor::DrawWindows()
  {
    // the viewport is always there; the tool windows while they are shown
    _viewport->Draw(*_tool_ui);
    _windows.DrawShown(*_tool_ui);

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
    RegisterWindows();
    RestoreLayout();

    if (_script.has_value())
    {
      _script_runner.emplace(&*_script, _render_system, _logging_system->CreateLogger("EditorScript"));
      const std::size_t count = _script->GetSteps().size();
      const std::string description = _script->GetDescription().empty() ? "" : ": " + _script->GetDescription();
      _logger->Info("A script of {} steps drives the tools{}", count, description);
    }

    const FrameCapture frame_capture(_settings_config, _render_system);
    std::size_t frames_rendered = 0;

    // a scene that cannot be read is said; the editor stays open all the
    // same, since it is where the scene would be corrected
    if (_world_system->HasNoScene()) { _logger->Error("There is no scene to show, see above"); }

    // the viewport fills the middle, and a tool window first shown takes a
    // side of it
    const ToolUiDockLayout layout = _windows.LayoutAround(std::string(EditorViewport::title));

    while (_window_system->IsRunning())
    {
      _input_system->ProcessInput();

      _render_system->PrepareFrame();
      if (_script_runner.has_value()) { _script_runner->BeforeFrame(*_tool_ui); }
      _tool_ui->BeginFrame();

      // the scene is drawn into the viewport, at the size it had in the
      // frame before
      _viewport->Prepare();
      _world_system->Update();

      // and the tools over the frame
      DrawMenuBar();
      _tool_ui->DockSpace(layout);
      _windows.HandleShortcuts(*_tool_ui);
      DrawWindows();
      _tool_ui->EndFrame();

      _render_system->FinishFrame();
      frames_rendered++;

      if (!frame_capture.AfterFrame(frames_rendered)) { _failed = true; }
      if (_script_runner.has_value()) { AfterScriptedFrame(); }

      if (_settings_config.max_frames > 0 && frames_rendered >= _settings_config.max_frames)
      {
        _logger->Info("Rendered {} frames, stopping", frames_rendered);
        _window_system->SignalToClose();
      }

      _window_system->Update();
    }

    // a script that was cut short by the end of the run, as by --frames,
    // has not proved what it was to prove
    if (_script_runner.has_value() && !_script_runner->IsDone() && !_script_runner->HasFailed())
    {
      const std::size_t done = _script_runner->GetStepsDone();
      const std::size_t count = _script_runner->GetStepCount();
      _logger->Error("The run ended with {} of the {} steps of the script done, so it ends with exit code 1", done, count);
      _failed = true;
    }
    KeepLayout();
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
