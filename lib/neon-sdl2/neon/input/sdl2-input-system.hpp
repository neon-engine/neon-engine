#ifndef SDL_2_INPUT_SYSTEM_HPP
#define SDL_2_INPUT_SYSTEM_HPP

#include <array>

#include <neon/input/input-state.hpp>
#include <neon/input/input-system.hpp>
#include <neon/window/window-context.hpp>

// an event of SDL, which only the backend knows
union SDL_Event;

namespace neon
{
  /// The input of SDL as the engine sees it.
  ///
  /// Everything is taken from the events of SDL and nothing is asked of
  /// SDL's own state, with one exception: a controller. What a key, a
  /// button, and the pointer are is therefore decided by HandleEvent()
  /// alone, which a test hands events it made. That needs no display.
  // ReSharper disable once CppInconsistentNaming
  class SDL2_InputSystem final : public InputSystem {
    // as many as SDL has scancodes
    static constexpr std::size_t key_count = 512;

    WindowContext* _context;
    InputState _input_state;
    bool _window_focus = false;

    // While the cursor is hidden, the mouse turns the view and points at
    // nothing.
    bool _cursor_hidden = false;

    // the controller that was plugged in first, as SDL_GameController
    void *_controller = nullptr;

    // the keys that are held down, by their scancode
    std::array<bool, key_count> _keys{};

    // where the pointer is in points of the window, and whether it is over
    // the window
    bool _pointer_inside = false;
    double _pointer_x = 0.0;
    double _pointer_y = 0.0;
    bool _primary_down = false;

    TextComposition _composition;
    bool _text_input_active = false;

    void ReadKeyboard();

    void ReadPointer();

    void ReadController();

    void ReleaseKeys();

  public:
    explicit SDL2_InputSystem(
      const SettingsConfig &settings_config,
      WindowContext* context,
      const std::shared_ptr<Logger> &logger)
      : InputSystem(settings_config, logger), _input_state(logger)
    {
      _context = context;
    }

    void Initialize() override;

    void ProcessInput() override;

    void CleanUp() override;

    const InputState & GetInputState() override;

    void CenterAndHideCursor() override;

    void ShowCursor() override;

    /// Takes one event of SDL into what is held and into the state of the
    /// frame. ProcessInput() hands over every event there is.
    void HandleEvent(const SDL_Event &event);

    /// Shows the keyboard of the screen where the platform has one, and
    /// puts the candidates of an input method next to the caret.
    void StartTextInput(const TextInputArea &caret) override;

    void StopTextInput() override;
  };
} // neon

#endif //SDL_2_INPUT_SYSTEM_HPP
