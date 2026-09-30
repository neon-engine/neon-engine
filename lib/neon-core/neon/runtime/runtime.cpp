#include "runtime.hpp"

#include "frame-capture.hpp"


namespace neon
{
  Runtime::Runtime(
    const SettingsConfig &settings_config,
    WindowSystem *window_system,
    InputSystem *input_system,
    RenderSystem *render_system,
    RenderPipeline *render_pipeline,
    LoggingSystem *logging_system,
    WorldSystem *world_system,
    const std::shared_ptr<Logger> &logger)
  {
    _settings_config = settings_config;
    _window_system = window_system;
    _input_system = input_system;
    _render_system = render_system;
    _render_pipeline = render_pipeline;
    _logging_system = logging_system;
    _world_system = world_system,
    _logger = logger;
  }

  void Runtime::SetUiSystem(UiSystem *ui_system)
  {
    _ui_system = ui_system;
  }

  void Runtime::Initialize() const
  {
    // order here matters
    // the input system and rendering backend are usually dependent on
    // the OS windowing system. Initializing the window first will make it possible
    // to set up both the input systems and render systems

    _window_system->Initialize();
    _input_system->Initialize();
    _render_system->Initialize();
    _render_pipeline->Initialize();

    // in front of the world, whose scene may name a user interface to show
    if (_ui_system != nullptr) { _ui_system->Initialize(); }

    _world_system->Initialize();
  }

  void Runtime::CleanUp()
  {
    if (_destroyed) { return; }
    _destroyed = true;

    _world_system->CleanUp();
    if (_ui_system != nullptr) { _ui_system->CleanUp(); }
    _render_pipeline->CleanUp();
    _render_system->CleanUp();
    _input_system->CleanUp();
    _window_system->CleanUp();
  }

  bool Runtime::HasFailed() const
  {
    return _failed;
  }

  Runtime::~Runtime()
  {
    CleanUp();
  }

  void Runtime::Run()
  {
    Initialize();

    const FrameCapture frame_capture(_settings_config, _render_system);
    std::size_t frames_rendered = 0;

    while (_window_system->IsRunning())
    {
      _input_system->ProcessInput();

      // The user interface sees the input first, and takes what it uses
      // away from the world. It is drawn last, on top of the world.
      if (_ui_system != nullptr) { _ui_system->Update(); }

      _render_system->PrepareFrame();
      _world_system->Update();

      if (_ui_system != nullptr) { _ui_system->Draw(); }

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
} // neon
