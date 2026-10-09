#ifndef COLOR_SPACE_HPP
#define COLOR_SPACE_HPP

#include "color.hpp"

namespace neon
{
  // Colors are written the way a screen shows them, in sRGB, as image files
  // and the style sheets of CSS have them. A step in such a number is a step
  // the eye sees as even, which is not the same as an even step in light:
  // 0.5 is about a fifth of the light of 1. Light adds up and is blended in
  // linear terms, so a renderer that lights or mixes colors turns them into
  // linear light first and back into sRGB at the end.

  /// The light an sRGB value from 0 to 1 stands for, from 0 to 1.
  [[nodiscard]] float SrgbToLinear(float value);

  /// The sRGB value that shows a linear amount of light, from 0 to 1.
  [[nodiscard]] float LinearToSrgb(float value);

  /// A color in linear light. Alpha is how much the color covers, and not
  /// a light, and stays as it is.
  [[nodiscard]] Color SrgbToLinear(const Color &color);

  /// The light an sRGB byte stands for, from 0 to 1. Read from a table.
  [[nodiscard]] float SrgbByteToLinear(unsigned char value);

  /// The sRGB byte that shows a linear amount of light, rounded to the
  /// nearest.
  [[nodiscard]] unsigned char LinearToSrgbByte(float value);
} // neon

#endif //COLOR_SPACE_HPP
