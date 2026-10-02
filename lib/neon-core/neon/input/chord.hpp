#ifndef CHORD_HPP
#define CHORD_HPP

#include <initializer_list>
#include <vector>

namespace neon
{
  /// Keys or buttons held together as one binding of an input map: `w`
  /// alone, or `left-shift` with `w`. A chord of one is written as the key
  /// itself, in the file and in code, so that every binding is a chord and
  /// `keys: [space]` reads as it always did.
  template<typename Device>
  struct Chord
  {
    /// All of them have to be down for the chord to be.
    std::vector<Device> parts;

    Chord() = default;

    /// A chord of one, so that `.keys = {Key::W}` is a key.
    // ReSharper disable once CppNonExplicitConvertingConstructor
    Chord(const Device one) : parts{one} {} // NOLINT(google-explicit-constructor)

    Chord(std::initializer_list<Device> together) : parts(together) {}

    /// Whether every part is down, asked of each with `is_down`. An empty
    /// chord never is.
    template<typename Ask>
    [[nodiscard]] bool IsDown(const Ask &is_down) const
    {
      if (parts.empty()) { return false; }
      for (const Device part : parts)
      {
        if (!is_down(part)) { return false; }
      }
      return true;
    }

    bool operator==(const Chord &other) const = default;
  };
} // neon

#endif //CHORD_HPP
