#ifndef HEADLESS_INPUT_SYSTEM_HPP
#define HEADLESS_INPUT_SYSTEM_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <neon/input/input-script.hpp>
#include <neon/input/input-state.hpp>
#include <neon/input/input-system.hpp>

namespace neon
{
  /// An input system without devices. Nothing is ever pressed or moved,
  /// unless a script says so: see InputScript. It is what --headless-renderer
  /// creates, so that a run without a window can still be played. A headless
  /// runtime, the dedicated server of #144, has no input system at all, so
  /// the name stays.
  // ReSharper disable once CppInconsistentNaming
  class Headless_InputSystem final : public InputSystem
  {
    InputState _input_state;
    InputScript _script;
    std::size_t _frame = 0;

    // where a text is typed, as the user interface said it
    bool _is_typing = false;
    TextInputArea _caret;

    // what the sensors read, as a test fed them, written into a frame while
    // they are on
    SensorState _gyro;
    SensorState _accelerometer;

    [[nodiscard]] std::string ActionNames() const;

    // the time a frame stands for unless the settings say, which is what the
    // headless window says as well
    static constexpr double frame_time = 1.0 / 60.0;

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
    /// ProcessInput() afterwards is frame 1. Returns false and keeps the
    /// script it had when a `hold` names an action the input map does not
    /// have, with a message in `errors` for each; the map is set first.
    bool SetScript(const InputScript &script, std::vector<std::string> &errors);

    void StartTextInput(const TextInputArea &caret) override;

    void StopTextInput() override;

    /// What a motion sensor reads from now on, as a controller would give
    /// it. It reaches the frame only while the sensor is on.
    void SetSensor(Sensor sensor, double x, double y, double z);

    /// Whether the user interface said that a text is typed, and where.
    /// There is no keyboard to show, so this is for tests and for tools.
    [[nodiscard]] bool IsTextInputActive() const;

    [[nodiscard]] const TextInputArea &GetTextInputArea() const;
  };
} // neon

#endif //HEADLESS_INPUT_SYSTEM_HPP
