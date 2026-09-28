#include "application.hpp"


namespace neon
{
  Application::Application(
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

  void Application::Initialize() const
  {
    // order here matters
    // the input system and rendering backend are usually dependent on
    // the OS windowing system. Initializing the window first will make it possible
    // to set up both the input systems and render systems

    _window_system->Initialize();
    _input_system->Initialize();
    _render_system->Initialize();
    _render_pipeline->Initialize();
    _world_system->Initialize();
  }

  void Application::CleanUp()
  {
    if (_destroyed) { return; }
    _destroyed = true;

    _world_system->CleanUp();
    _render_pipeline->CleanUp();
    _render_system->CleanUp();
    _input_system->CleanUp();
    _window_system->CleanUp();
  }

  Application::~Application()
  {
    CleanUp();
  }

  void Application::Run()
  {
    Initialize();

    std::size_t frames_rendered = 0;

    while (_window_system->IsRunning())
    {
      _input_system->ProcessInput();
      _render_system->PrepareFrame();
      _world_system->Update();
      _render_system->FinishFrame();
      frames_rendered++;

      if (_settings_config.max_frames > 0 && frames_rendered >= _settings_config.max_frames)
      {
        _logger->Info("Rendered {} frames, stopping", frames_rendered);

        if (!_settings_config.screenshot_path.empty())
        {
          _render_system->CaptureFrame(_settings_config.screenshot_path);
        }

        _window_system->SignalToClose();
      }

      _window_system->Update();
    }
  }
} // neon
