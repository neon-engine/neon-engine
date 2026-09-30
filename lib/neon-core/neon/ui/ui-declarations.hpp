#ifndef UI_DECLARATIONS_HPP
#define UI_DECLARATIONS_HPP

#include <cstddef>
#include <string>
#include <vector>

#include <neon/data/data-value.hpp>

#include "css/css-expression.hpp"
#include "ui-style.hpp"

// How a declaration, `name: value`, gets into a style. It is the one way
// for every place a property is written in: a style sheet, a file of YAML,
// and what a game sets while it runs.

namespace neon
{
  /// What the values of an element are worked out against.
  struct UiDeclarationContext
  {
    /// Units and custom properties. `font_size` is that of the element.
    CssValueContext values;

    /// The size of the font of the element above, which `em` and `%` of
    /// `font_size` itself refer to.
    float parent_font_size = 16.0f;

    /// The style of the element above, which `inherit` takes from.
    /// nullptr for the element at the top, which takes the initial value.
    const UiStyle *parent = nullptr;

    /// The style the engine gives the element, which `revert` goes back
    /// to. nullptr goes back to the initial value.
    const UiStyle *defaults = nullptr;

    /// Whether the names of colours and `hsl()` are read. They are in
    /// style sheets. A file of YAML reads the notations it read before.
    bool reads_color_names = true;
  };

  /// Puts one declaration into a style. The name is written either way,
  /// with hyphens or with underscores. Returns false, leaves the style as
  /// it was, and says why in `problems` when the name is not known or the
  /// value cannot be read. `where` is what the declaration belongs to in a
  /// message, such as `the rule 'button:hover'`.
  bool ApplyUiDeclaration(
    const std::string &name,
    const std::string &value,
    const UiDeclarationContext &context,
    const std::string &where,
    UiStyle &style,
    std::vector<std::string> &problems,
    CssValueUses *uses = nullptr);

  /// Whether a declaration can be read, against values that stand in for
  /// those of an element. A value with `var()` in it counts as readable
  /// when its name is known, since what it holds is not known yet.
  [[nodiscard]] bool CheckUiDeclaration(
    const std::string &name,
    const std::string &value,
    const std::string &where,
    std::vector<std::string> &problems);

  /// What a file of YAML writes for an element, made ready to be read as a
  /// style.
  struct UiPreparedProperties
  {
    /// The map with the values of its properties worked out: units,
    /// `calc()`, and custom properties. What is no property is as it was.
    /// `children` is left out.
    DataValue map = DataValue::Map();

    /// The properties that hold `inherit`, `initial`, `unset`, or `revert`,
    /// which are left out of the map: the name as the file writes it, and
    /// the word.
    std::vector<std::pair<std::string, std::string>> keywords;

    /// The custom properties the map sets, which are left out of it.
    std::vector<std::pair<std::string, std::string>> variables;

    CssValueUses uses;
  };

  /// Works out the values of the properties of a map. With `checks_only`,
  /// a property with `var()` in it is left out, since it cannot be read
  /// before the element is known. A value that cannot be worked out is
  /// reported, with the document and the line.
  [[nodiscard]] UiPreparedProperties PrepareUiProperties(
    const DataValue &map,
    const UiDeclarationContext &context,
    bool checks_only,
    const std::string &document,
    const std::string &where,
    std::vector<std::string> &problems);

  /// Puts the properties of a prepared map into a style, the shorthands in
  /// front of what they stand for. `problems` takes what cannot be read,
  /// with the document and the line.
  void ApplyUiProperties(
    const UiPreparedProperties &prepared,
    const UiDeclarationContext &context,
    const std::string &document,
    const std::string &where,
    UiStyle &style,
    std::vector<std::string> &problems);

  /// Sets a property to `inherit`, `initial`, `unset`, or `revert`. Returns
  /// false for a property that is not in the table of properties.
  bool ApplyUiKeyword(
    const std::string &name,
    const std::string &keyword,
    const UiDeclarationContext &context,
    UiStyle &style);

  /// Whether a value is one of `inherit`, `initial`, `unset`, and `revert`.
  [[nodiscard]] bool IsCssWideKeyword(const std::string &value);
} // neon

#endif //UI_DECLARATIONS_HPP
