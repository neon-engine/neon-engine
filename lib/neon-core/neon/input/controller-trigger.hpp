#ifndef CONTROLLER_TRIGGER_HPP
#define CONTROLLER_TRIGGER_HPP

#include <string>

namespace neon
{
  /// An analog trigger of a controller, as an input map names it. It gives
  /// how far it is pulled, from 0 to 1; pulled past the half it is also the
  /// button `left-trigger` or `right-trigger`.
  enum class ControllerTrigger
  {
    Left = 0,
    Right
  };

  /// What a trigger is called in an input map: `left` or `right`.
  [[nodiscard]] inline std::string NameOf(const ControllerTrigger trigger)
  {
    return trigger == ControllerTrigger::Left ? "left" : "right";
  }

  /// The trigger of a name. Returns false when there is none.
  [[nodiscard]] inline bool TriggerOf(const std::string &name, ControllerTrigger &trigger)
  {
    if (name == "left") { trigger = ControllerTrigger::Left; return true; }
    if (name == "right") { trigger = ControllerTrigger::Right; return true; }
    return false;
  }
} // neon

#endif //CONTROLLER_TRIGGER_HPP
