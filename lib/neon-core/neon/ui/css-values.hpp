#ifndef CSS_VALUES_HPP
#define CSS_VALUES_HPP

#include <string>
#include <vector>

#include <neon/common/color.hpp>
#include <neon/layout/layout-style.hpp>

// Values as CSS writes them, read from text.

namespace neon
{
  /// Reads a colour the way CSS writes it:
  ///
  ///     #rgb  #rgba  #rrggbb  #rrggbbaa
  ///     rgb(255, 128, 0)  rgb(255 128 0)  rgb(100%, 50%, 0%)
  ///     rgba(255, 128, 0, 0.5)  rgb(255 128 0 / 50%)
  ///     transparent  black  white
  ///
  /// Returns false and leaves `color` alone for anything else, which
  /// includes the other names CSS has for colours.
  [[nodiscard]] bool ParseCssColor(const std::string &text, Color &color);

  /// Reads a length: a number of pixels as `12` or `12px`, a percentage as
  /// `50%`, or `auto`. A number without a unit counts as pixels, which CSS
  /// allows for 0 alone. The other units and `calc()` are worked out before
  /// a length gets here, by ResolveCssValue().
  [[nodiscard]] bool ParseCssLength(const std::string &text, LayoutLength &length);

  /// Reads a number, with nothing behind it.
  [[nodiscard]] bool ParseCssNumber(const std::string &text, float &number);

  /// The values of a property that takes several, such as `8px 16px`. They
  /// are set apart by spaces. What is between brackets stays together.
  [[nodiscard]] std::vector<std::string> SplitCssValues(const std::string &text);
} // neon

#endif //CSS_VALUES_HPP
