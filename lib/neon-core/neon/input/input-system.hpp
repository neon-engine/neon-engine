#ifndef INPUT_SYSTEM_HPP
#define INPUT_SYSTEM_HPP

#include <bitset>
#include <memory>
#include <string>

#include "input-actions.hpp"
#include "input-context.hpp"
#include "input-map.hpp"
#include "neon/runtime/settings-config.hpp"
#include "neon/logging/logger.hpp"

namespace neon
{
  /// An input system reads the devices into an InputState once a frame, and
  /// works the actions of the input map out from it. Which devices there are
  /// is up to the backend; the map and the actions are the same for all.
  class InputSystem : public InputContext
  {
  protected:
    SettingsConfig _settings_config;
    std::shared_ptr<Logger> _logger;

    // the engine's default until the project's map is set
    InputMap _map = InputMap::Default();
    std::string _state = _map.GetFirstState();
    InputActions _actions;

    // the motion sensors that are on, which is none until something says
    std::bitset<kSensor_Size> _sensors_enabled{};

    ~InputSystem() = default;

    /// Whether a sensor is on, for a backend that reads it.
    [[nodiscard]] bool IsSensorEnabled(Sensor sensor) const;

    /// Told when a sensor was turned on or off, for a backend that has to
    /// tell the controller. Nothing by default.
    virtual void OnSensorEnabled(Sensor sensor, bool enabled) {}

    /// Works the actions out from the state of the frame. A backend calls it
    /// at the end of ProcessInput(), with how much time the frame stands
    /// for, which is written into the state for what reads it after.
    void RefreshActions(InputState &input, double delta_time);

  public:
    explicit InputSystem(const SettingsConfig &settings_config, const std::shared_ptr<Logger> &logger)
    {
      _settings_config = settings_config;
      _logger = logger;
    }

    virtual void Initialize() = 0;

    virtual void ProcessInput() = 0;

    virtual void CleanUp() = 0;

    /// The map the project plays with, in place of the engine's default.
    /// The game is put into its first state, and a sensor an action of the
    /// map says is `enabled` is turned on.
    void SetInputMap(const InputMap &map);

    const InputMap &GetInputMap() override;

    bool IsActionDown(const std::string &name) override;

    bool WasActionPressed(const std::string &name) override;

    glm::vec2 ActionAxis(const std::string &name) override;

    float ActionAmount(const std::string &name) override;

    glm::vec3 ActionAxis3(const std::string &name) override;

    bool SetSensorEnabled(const std::string &sensor, bool enabled) override;

    bool IsSensorEnabled(const std::string &sensor) override;

    bool SetState(const std::string &name) override;

    const std::string &GetState() override;
  };
} // neon

#endif //INPUT_SYSTEM_HPP
