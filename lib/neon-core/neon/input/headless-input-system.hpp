#ifndef HEADLESS_INPUT_SYSTEM_HPP
#define HEADLESS_INPUT_SYSTEM_HPP

#include <cstddef>
#include <memory>
#include <neon/input/input-script.hpp>
#include <neon/input/input-state.hpp>
#include <neon/input/input-system.hpp>

namespace neon
{
  /// An input system without devices. Nothing is ever pressed or moved,
  /// unless a script says so: see InputScript.
  // ReSharper disable once CppInconsistentNaming
  class Headless_InputSystem final : public InputSystem
  {
    InputState _input_state;
    InputScript _script;
    std::size_t _frame = 0;

    // where a text is typed, as the user interface said it
    bool _is_typing = false;
    TextInputArea _caret;

  public:
    explicit Headless_InputSystem(const SettingsConfig &settings_config, const std::shared_ptr<Logger> &logger)
      : InputSystem(settings_config, logger), _input_state(logger) {}

    void Initialize() override;

    void ProcessInput() override;

    void CleanUp() override;

    const InputState &GetInputState() override;

    void CenterAndHideCursor() override;

    void ShowCursor() override;

    /// The input of the frames to come, in place of none. The first call of
    /// ProcessInput() afterwards is frame 1.
    void SetScript(const InputScript &script);

    void StartTextInput(const TextInputArea &caret) override;

    void StopTextInput() override;

    /// Whether the user interface said that a text is typed, and where.
    /// There is no keyboard to show, so this is for tests and for tools.
    [[nodiscard]] bool IsTextInputActive() const;

    [[nodiscard]] const TextInputArea &GetTextInputArea() const;
  };
} // neon

#endif //HEADLESS_INPUT_SYSTEM_HPP
