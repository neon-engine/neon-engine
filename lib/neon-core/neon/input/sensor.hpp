#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <cstddef>
#include <string>

namespace neon
{
  /// A motion sensor of a controller, as an input map names it. A sensor is
  /// something the player turns on and off, and it is off until something
  /// turns it on: the map, or the settings.
  enum class Sensor
  {
    /// How fast the controller turns, in radians a second about its x, y,
    /// and z: pitch, yaw, and roll.
    Gyro = 0,

    /// How the controller is accelerated, in meters a second squared along
    /// x, y, and z, gravity included.
    Accelerometer,

    // used only to keep track of the total count of sensors
    // ReSharper disable once CppInconsistentNaming
    COUNT
  };

  constexpr std::size_t kSensor_Size = static_cast<std::size_t>(Sensor::COUNT);

  /// What a sensor is called in an input map and in the settings: `gyro` or
  /// `accelerometer`.
  [[nodiscard]] inline std::string NameOf(const Sensor sensor)
  {
    return sensor == Sensor::Gyro ? "gyro" : "accelerometer";
  }

  /// The sensor of a name. Returns false when there is none.
  [[nodiscard]] inline bool SensorOf(const std::string &name, Sensor &sensor)
  {
    if (name == "gyro") { sensor = Sensor::Gyro; return true; }
    if (name == "accelerometer") { sensor = Sensor::Accelerometer; return true; }
    return false;
  }
} // neon

#endif //SENSOR_HPP
