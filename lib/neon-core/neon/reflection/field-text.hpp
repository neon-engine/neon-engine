#ifndef FIELD_TEXT_HPP
#define FIELD_TEXT_HPP

#include <string>

#include "field-value.hpp"
#include "type-info.hpp"

// Values as text. It is what an inspector shows in a box that is typed
// into, what a console takes, and what a script hands over when it knows a
// field by its name alone. Lengths and colours are written as a style sheet
// writes them.

namespace neon
{
  /// A length as a style sheet writes it: `12px`, `50%`, `auto`, and
  /// `calc(50% + 12px)` for both added up.
  [[nodiscard]] std::string FormatFieldLength(const FieldLength &length);

  /// Reads `12`, `12px`, `50%`, `auto`, and `calc(50% + 12px)`. A number
  /// without a unit counts as pixels.
  [[nodiscard]] bool ParseFieldLength(const std::string &text, FieldLength &length);

  /// A colour in the notation of CSS: `#ff8000`, and `#ff800080` for one
  /// that shows through.
  [[nodiscard]] std::string FormatFieldColor(const Color &color);

  /// Reads `#rgb`, `#rgba`, `#rrggbb`, `#rrggbbaa`, `rgb(255, 128, 0)`, and
  /// `rgba(255, 128, 0, 0.5)`.
  [[nodiscard]] bool ParseFieldColor(const std::string &text, Color &color);

  /// The value of a field as text.
  ///
  /// | Kind | As text |
  /// |---|---|
  /// | Bool | `true`, `false` |
  /// | Whole, Number | `12`, `0.5` |
  /// | Text, Choice | The text itself |
  /// | Vector | `1 2 3` |
  /// | Color | `#ff8000`, `#ff800080` |
  /// | TextList | The texts with a comma and a space between them |
  /// | Length | `12px`, `50%`, `auto` |
  /// | NumberList | `1 2 3 4` |
  /// | Layers | `1 3` |
  [[nodiscard]] std::string FormatField(const FieldValue &value);

  /// Reads the value of a field from text, as the kind of the field asks
  /// for. Returns false and says why when the text is no such value. `what`
  /// is what the field is called in the message, such as `'fov' of Camera`.
  /// What the field may hold is looked at by FieldInfo::Check().
  [[nodiscard]] bool ParseField(
    const std::string &text,
    const FieldInfo &field,
    const std::string &what,
    FieldValue &value,
    std::string &error);
} // neon

#endif //FIELD_TEXT_HPP
