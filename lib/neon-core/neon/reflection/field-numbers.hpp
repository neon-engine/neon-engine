#ifndef FIELD_NUMBERS_HPP
#define FIELD_NUMBERS_HPP

#include <cstddef>

#include "field-value.hpp"

namespace neon
{
  /// The most numbers a value is written as: a vector of four, a color, a
  /// quaternion.
  // ReSharper disable once CppInconsistentNaming
  inline constexpr std::size_t Max_Field_Numbers = 4;

  /// Writes a value as plain numbers, in the order of its members: one for
  /// a number or a boolean (0 or 1), two to four for a vector, r, g, b, a
  /// for a color, x, y, z, w for a quaternion. It is how a value crosses to
  /// code that knows none of the engine's types. Returns how many were
  /// written, or 0 for a value that is not made of up to four numbers.
  [[nodiscard]] std::size_t ToNumbers(const FieldValue &value, double *numbers);

  /// The value of a kind from plain numbers, as ToNumbers writes them.
  /// Returns false for a kind that is not made of up to four numbers.
  [[nodiscard]] bool FromNumbers(FieldKind kind, const double *numbers, FieldValue &value);
} // neon

#endif //FIELD_NUMBERS_HPP
