#ifndef CSS_FUNCTIONS_HPP
#define CSS_FUNCTIONS_HPP

#include <string>
#include <vector>

#include "ui-paint.hpp"

// Values of CSS that are written as functions or as lists, read from text.

namespace neon
{
  /// The parts of a list, which are set apart by `separator`. What is
  /// between brackets stays together. Spaces around a part are left out.
  [[nodiscard]] std::vector<std::string> SplitCssList(const std::string &text, char separator);

  /// When `text` is `name(...)`, what is between the brackets.
  [[nodiscard]] bool ParseCssFunction(const std::string &text, const std::string &name, std::string &inside);

  /// An angle in degrees: `45deg`, `0.5turn`, `1.57rad`, or `0`.
  [[nodiscard]] bool ParseCssAngle(const std::string &text, float &degrees);

  /// Reads a gradient:
  ///
  ///     linear-gradient(#f00, #00f)
  ///     linear-gradient(90deg, #f00 0%, #0f0 50%, #00f 100%)
  ///     linear-gradient(to right, rgba(0, 0, 0, 0.5), transparent)
  ///     radial-gradient(#fff, #000)
  ///
  /// A radial gradient is an ellipse around the middle of the box that
  /// reaches its corners. Up to 8 colours are read.
  [[nodiscard]] bool ParseCssGradient(const std::string &text, UiGradient &gradient);

  /// Reads shadows, with commas between them:
  ///
  ///     0 4px 12px rgba(0, 0, 0, 0.5)
  ///     inset 0 0 8px 2px #000, 0 2px 4px #0008
  ///
  /// Two to four lengths: to the right, down, the blur, and how much
  /// larger the shadow is. The shadow of a text has no `inset` and is no
  /// larger than the text.
  [[nodiscard]] bool ParseCssShadows(const std::string &text, bool of_text, std::vector<UiShadow> &shadows);

  /// Reads the steps of a `transform`:
  ///
  ///     translate(10px, 50%) rotate(45deg) scale(1.5)
  ///
  /// Known are translate, translateX, translateY, rotate, scale, scaleX,
  /// and scaleY. `none` has no steps.
  [[nodiscard]] bool ParseCssTransform(const std::string &text, std::vector<UiTransformStep> &steps);

  /// Reads a place in a box: one or two of `left`, `center`, `right`,
  /// `top`, `bottom`, a length, or a percentage.
  [[nodiscard]] bool ParseCssPlace(const std::string &text, UiPlace &place);
} // neon

#endif //CSS_FUNCTIONS_HPP
