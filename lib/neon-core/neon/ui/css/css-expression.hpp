#ifndef CSS_EXPRESSION_HPP
#define CSS_EXPRESSION_HPP

#include <functional>
#include <string>

// What a value of CSS is worked out from before it is read as a length, a
// color, or a word: custom properties, the units that refer to something,
// and calc(). See https://www.w3.org/TR/css-variables-1/ and
// https://www.w3.org/TR/css-values-4/.

namespace neon
{
  /// What the units and the custom properties of a value refer to.
  struct CssValueContext
  {
    /// The size of the font of the element, which an `em` is. For
    /// `font-size` itself it is that of the element above.
    float font_size = 16.0f;

    /// The size of the font of the element at the top, which a `rem` is.
    float root_font_size = 16.0f;

    /// The size of what is shown, which `vw`, `vh`, `vmin`, and `vmax` are
    /// a hundredth of.
    float viewport_width = 1920.0f;
    float viewport_height = 1080.0f;

    /// Looks up a custom property. Returns false for one that is not set.
    /// Without it, none is.
    std::function<bool(const std::string &name, std::string &value)> variable;
  };

  /// What a value was found to refer to, so that it is worked out again
  /// when that changes.
  struct CssValueUses
  {
    bool font_size = false;
    bool root_font_size = false;
    bool viewport = false;
    bool variables = false;
  };

  /// Works out a value as far as it can be without knowing the property:
  ///
  ///     var(--gap, 8px)           what the custom property holds
  ///     2em  1.5rem  10vw  5vh    pixels
  ///     1in  12pt  1cm            pixels
  ///     calc(100% - 2 * 16px)     one length
  ///
  /// What comes out is written with `px` and `%` alone, and with a sum of
  /// the two as `calc(100% + -32px)`. Text in quotes and in `url()` is left
  /// as it is. Returns false and says why when the value cannot be worked
  /// out, such as for a custom property that is not set.
  [[nodiscard]] bool ResolveCssValue(
    const std::string &value,
    const CssValueContext &context,
    std::string &resolved,
    std::string &error,
    CssValueUses *uses = nullptr);

  /// Puts what the custom properties hold in the place of every `var()`,
  /// and leaves everything else as it is. For a value that is text, such as
  /// a path, in which a number with a unit is no length.
  [[nodiscard]] bool SubstituteCssVariables(
    const std::string &value,
    const CssValueContext &context,
    std::string &resolved,
    std::string &error,
    CssValueUses *uses = nullptr);

  /// Whether a value holds `var()`, and can therefore not be checked before
  /// the element it is for is known.
  [[nodiscard]] bool HasCssVariables(const std::string &value);

  /// Writes the colors a value names as `rgba()`: the names of CSS such as
  /// `rebeccapurple`, and `hsl()`. Everything else is left as it is.
  [[nodiscard]] std::string ResolveCssColors(const std::string &value);

  /// A number as CSS writes it, without digits that say nothing.
  [[nodiscard]] std::string FormatCssNumber(float number);
} // neon

#endif //CSS_EXPRESSION_HPP
