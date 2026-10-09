#ifndef PRINT_CHORD_HPP
#define PRINT_CHORD_HPP

#include <cstddef>
#include <ostream>

#include <neon/input/chord.hpp>
#include <neon/input/controller-button.hpp>
#include <neon/input/key.hpp>

namespace neon
{
  /// How GoogleTest prints a chord when an expectation on it fails: by the
  /// names an input map file gives its keys and buttons, a chord of one as
  /// the name itself, `w`, and several held together as a list,
  /// `[left-shift, w]`. An empty chord is `[]`. It is in the namespace of
  /// Chord, and not in `neon::testing`, so that GoogleTest finds it.
  template<typename Device>
  void PrintTo(const Chord<Device> &chord, std::ostream *out)
  {
    if (chord.parts.size() == 1)
    {
      *out << NameOf(chord.parts.front());
      return;
    }

    *out << '[';
    for (std::size_t i = 0; i < chord.parts.size(); i++)
    {
      if (i > 0) { *out << ", "; }
      *out << NameOf(chord.parts[i]);
    }
    *out << ']';
  }
} // neon

#endif //PRINT_CHORD_HPP
