#ifndef UI_CASCADE_HPP
#define UI_CASCADE_HPP

#include <string>
#include <vector>

#include "ui-declarations.hpp"
#include "ui-element.hpp"
#include "ui-style-sheets.hpp"

namespace neon
{
  /// What the cascade works with next to the element.
  struct UiCascadeSettings
  {
    /// The style sheets of the file the element is from. nullptr for none.
    const UiStyleSheets *sheets = nullptr;

    /// The size of what is shown in units of the file, for `vw` and `vh`.
    float viewport_width = 1920.0f;
    float viewport_height = 1080.0f;

    /// Takes what cannot be read while a style is worked out, such as a
    /// custom property that is not set. nullptr leaves it unsaid.
    std::vector<std::string> *problems = nullptr;
  };

  /// Works out the style of an element by the rules of
  /// https://www.w3.org/TR/css-cascade-4/.
  ///
  /// What is written for a property in several places is decided in this
  /// order, the later winning over the earlier:
  ///
  ///   1. what the engine gives an element of its kind, and its states
  ///   2. the rules of the style sheets, by how much their selector counts
  ///      and then by the order they are written in
  ///   3. what the file of the element writes for it, and for its states
  ///   4. what the game set while it runs
  ///   5. what a style sheet marks as `!important`
  ///
  /// A property that nothing writes takes the value of the element above
  /// when it is one that is inherited, and its initial value otherwise.
  class UiCascade
  {
  public:
    /// Works out the style of the element and hands it to the element.
    static void Compute(UiElement &element, const UiCascadeSettings &settings);

    /// Works out the style of a part of an element, which a style sheet
    /// names as `::part`. `style` holds what the part starts with.
    static void ComputePart(
      const UiElement &element,
      const std::string &part,
      const UiCascadeSettings &settings,
      UiStyle &style);

    /// What the values of an element are worked out against, once it has
    /// a style: for what is read into its style behind the cascade, as the
    /// keyframes of an animation are.
    [[nodiscard]] static UiDeclarationContext ContextOf(const UiElement &element, const UiCascadeSettings &settings);

    /// The states an element is drawn in: those it is in, or `disabled`
    /// alone for one that cannot be used.
    [[nodiscard]] static UiStates DrawnStates(const UiStates &states);
  };
} // neon

#endif //UI_CASCADE_HPP
