#ifndef STICK_HPP
#define STICK_HPP

#include <string>

namespace neon
{
  /// A stick of a controller, as an input map names it.
  enum class Stick
  {
    Left = 0,
    Right
  };

  /// What a stick is called in an input map: `left` or `right`.
  [[nodiscard]] inline std::string NameOf(const Stick stick)
  {
    return stick == Stick::Left ? "left" : "right";
  }

  /// The stick of a name. Returns false when there is none.
  [[nodiscard]] inline bool StickOf(const std::string &name, Stick &stick)
  {
    if (name == "left") { stick = Stick::Left; return true; }
    if (name == "right") { stick = Stick::Right; return true; }
    return false;
  }
} // neon

#endif //STICK_HPP
