#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include "settings-config.hpp"
#include "neon/input/input-system.hpp"
#include "neon/logging/logging-system.hpp"
#include "neon/random/entropy-context.hpp"
#include "neon/world-system/world-system.hpp"
#include "neon/render/render-pipeline.hpp"
#include "neon/render/render-system.hpp"
#include "neon/ui/ui-system.hpp"
#include "neon/window/window-system.hpp"

namespace neon
{
  class Runtime
  {
    bool _destroyed = false;
    bool _failed = false;

  protected:
    SettingsConfig _settings_config;
    WindowSystem *_window_system;
    InputSystem *_input_system;
    RenderSystem *_render_system;
    RenderPipeline *_render_pipeline;
    LoggingSystem *_logging_system;
    WorldSystem* _world_system;
    UiSystem *_ui_system = nullptr;
    EntropyContext *_entropy = nullptr;
    std::shared_ptr<Logger> _logger;

    // the pause menu while it is shown, and whether pause was held in the
    // frame before, to tell when it is pressed
    int _pause_document = -1;
    bool _pause_was_down = false;

    // the settings menu while the pause menu has it shown in its place
    int _settings_document = -1;

    /// Shows the pause menu when pause is pressed, takes it away when
    /// resume is chosen, closes the window on quit, opens the settings menu
    /// in its place when settings is chosen and shows it again when that is
    /// closed, and holds the world still while either is shown.
    void UpdatePauseMenu();

    /// Loads the pause menu, and says when it cannot be shown.
    void ShowPauseMenu();

    Runtime(
      const SettingsConfig &settings_config,
      WindowSystem *window_system,
      InputSystem *input_system,
      RenderSystem *render_system,
      RenderPipeline *render_pipeline,
      LoggingSystem *logging_system,
      WorldSystem *world_system,
      const std::shared_ptr<Logger> &logger);

  public:
    virtual ~Runtime();

    /// Gives the runtime a user interface, which is then updated before
    /// the world and drawn on top of it. Call it before Initialize(). A
    /// runtime without one draws the world alone.
    void SetUiSystem(UiSystem *ui_system);

    /// Gives the runtime the random numbers of the operating system, for the
    /// games it runs. The runtime draws none itself. A game reaches them
    /// through GetEntropy(), and through Lua once there is one (#57).
    void SetEntropy(EntropyContext *entropy);

    /// The random numbers of the operating system, or null without any.
    [[nodiscard]] EntropyContext *GetEntropy() const;

    virtual void Run();

    void Initialize() const;

    void CleanUp();

    /// Whether something the run was asked to do did not happen: a
    /// screenshot that could not be written, a scene or a user interface
    /// with a problem, anything that was logged as an error. The run goes on
    /// all the same, as a game does; an application turns this into its exit
    /// code, so that a script learns of it.
    [[nodiscard]] bool HasFailed() const;
  };
} // neon

#endif //RUNTIME_HPP
