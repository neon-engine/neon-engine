#ifndef UI_CALL_FIELD_HPP
#define UI_CALL_FIELD_HPP

#include <string>

#include <neon/data/data-reader.hpp>
#include <neon/reflection/field-value.hpp>

#include "ui-call.hpp"

namespace neon
{
  /// A field of an element that names a function of the game, such as
  /// `on_click` of a button and `on_change` of a slider: how a file writes
  /// it, and how a script sets it.
  namespace UiCallField
  {
    /// Reads the call a file writes under `name`, when it writes one. What
    /// is no call of a function is reported, and leaves `call` as it was.
    void Read(const DataReader &reader, const std::string &name, UiCall &call);

    /// Sets the call from text, as a script sets a field. An empty text
    /// takes the call away. Returns false and says why in `error` for what
    /// is no call of a function. `what` is what the field is called in the
    /// message, such as `'on_click' of button 'start'`.
    [[nodiscard]] bool Set(const FieldValue &value, const std::string &what, UiCall &call, std::string &error);
  }
} // neon

#endif //UI_CALL_FIELD_HPP
