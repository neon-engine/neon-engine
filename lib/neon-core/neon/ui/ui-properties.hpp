#ifndef UI_PROPERTIES_HPP
#define UI_PROPERTIES_HPP

#include <array>
#include <functional>
#include <string>
#include <vector>

#include "ui-style.hpp"

namespace neon
{
  /// What a property holds, which decides how it goes from one value to
  /// another over time.
  enum class UiValueKind
  {
    /// A number, with or without a unit.
    Number = 0,

    /// A number without a fraction.
    Whole,

    /// A length of CSS: pixels, a percentage, both added up, or `auto`.
    Length,

    Color,

    /// One of a few words.
    Keyword,

    Text,

    /// A number for each side: top, right, bottom, left.
    Edges,

    /// Something of its own, such as a list of timing functions. It is
    /// handed on as text.
    Other
  };

  /// The value of a property, whatever the property is. It is what lets
  /// code that knows nothing of a property read it, change it, hand it to
  /// what is inside an element, and move it over time.
  struct UiPropertyValue
  {
    UiValueKind kind = UiValueKind::Number;

    float number = 0.0f;
    LayoutLength length;
    Color color;
    int keyword = 0;
    std::string text;
    std::array<float, 4> edges{0.0f, 0.0f, 0.0f, 0.0f};

    /// What a number and a colour cannot say by themselves. For
    /// `line_height`, whether the number is a multiple of the size of the
    /// font. For a colour that follows that of the text unless it is set,
    /// whether it is set.
    bool flag = false;

    bool operator==(const UiPropertyValue &other) const;
  };

  /// A property of CSS as the engine knows it: what it is called, what it
  /// holds, and how it is reached in a style.
  ///
  /// This is the one table of properties. Whether a property is handed to
  /// what is inside an element is one word in it, `inherited`. A property
  /// that ReadUiStyle() reads and that is missing here is still written in
  /// files and in style sheets. It is not inherited, not moved over time,
  /// and has no value a script can read, until it is added here.
  struct UiProperty
  {
    /// As CSS writes it, with hyphens: `background-color`.
    std::string name;

    /// As a file of YAML writes it, with underscores: `background_color`.
    std::string yaml_name;

    UiValueKind kind = UiValueKind::Number;

    /// Whether an element that says nothing takes the value of the
    /// element it is inside of, as https://www.w3.org/TR/css-cascade-4/
    /// says for the property.
    bool is_inherited = false;

    /// Whether a change moves or resizes something.
    bool affects_layout = false;

    /// The unit a number is written with, such as `px`. Empty for none.
    std::string unit;

    /// The words of a keyword, in the order of its enum.
    std::vector<std::string> keywords;

    /// What the property is for, in a sentence. For the editor.
    std::string description;

    /// For a shorthand, the names of what it stands for. A shorthand holds
    /// no value itself.
    std::vector<std::string> longhands;

    std::function<UiPropertyValue(const UiStyle &style)> get;
    std::function<void(UiStyle &style, const UiPropertyValue &value)> set;

    /// For what a value cannot hold, such as a list: takes it from one
    /// style to another. Without it, get and set do.
    std::function<void(const UiStyle &from, UiStyle &to)> copy;

    [[nodiscard]] bool IsShorthand() const
    {
      return !longhands.empty();
    }

    /// Takes the value of one style over into another.
    void Copy(const UiStyle &from, UiStyle &to) const;

    /// The value as CSS writes it, such as `12px` and `rgb(255, 128, 0)`.
    [[nodiscard]] std::string Format(const UiPropertyValue &value) const;
  };

  /// The properties of CSS that the engine knows, by name.
  class UiProperties
  {
    std::vector<UiProperty> _properties;

    UiProperties();

  public:
    /// The table. It is built once.
    [[nodiscard]] static const UiProperties &Get();

    /// The property of a name, written either way, or nullptr.
    [[nodiscard]] const UiProperty *Find(const std::string &name) const;

    [[nodiscard]] const std::vector<UiProperty> &GetAll() const;

    /// The property itself, or for a shorthand what it stands for.
    [[nodiscard]] std::vector<const UiProperty *> GetLonghands(const UiProperty &property) const;

    /// A name as CSS writes it, and as a file of YAML writes it.
    [[nodiscard]] static std::string ToCssName(const std::string &name);

    [[nodiscard]] static std::string ToYamlName(const std::string &name);

    /// Every name ReadUiStyle() reads, as a file of YAML writes it. It is
    /// asked of ReadUiStyle() itself, so that a property that was added
    /// there is known here without a word.
    [[nodiscard]] static const std::vector<std::string> &GetStyleNames();

    [[nodiscard]] static bool IsStyleName(const std::string &yaml_name);

    /// The names that are nearest to one that is not known, for a message
    /// about a name that was misspelled. As CSS writes them.
    [[nodiscard]] static std::vector<std::string> GetSimilarNames(const std::string &name);
  };

  /// The value that is part of the way from one value to another, by the
  /// rules of https://www.w3.org/TR/css-values-4/#combining-values for what
  /// the values hold: numbers and lengths go straight, colours with their
  /// alpha multiplied in, and what cannot be moved switches halfway.
  [[nodiscard]] UiPropertyValue InterpolateUiValue(
    const UiPropertyValue &from,
    const UiPropertyValue &to,
    float progress);

  /// Whether a value can be moved to another, and does not switch halfway.
  [[nodiscard]] bool CanInterpolateUiValue(const UiPropertyValue &from, const UiPropertyValue &to);

  /// A colour as CSS writes a value it worked out: `rgb(255, 128, 0)`, and
  /// `rgba(255, 128, 0, 0.5)` for one that shows through.
  [[nodiscard]] std::string FormatCssColor(const Color &color);

  /// A length as CSS writes it.
  [[nodiscard]] std::string FormatCssLength(const LayoutLength &length);
} // neon

#endif //UI_PROPERTIES_HPP
