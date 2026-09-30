#ifndef UI_FIELDS_HPP
#define UI_FIELDS_HPP

#include <string>

#include <neon/reflection/field-value.hpp>

#include "ui-values.hpp"

// What the kinds of elements share in how their fields are read and set by
// name.

namespace neon
{
  /// Takes the text out of a value. Returns false and says why when the
  /// value is no text. `what` is what the field is called in the message,
  /// such as `'text' of button 'start'`.
  [[nodiscard]] inline bool TakeText(
    const FieldValue &value,
    const std::string &what,
    std::string &text,
    std::string &error)
  {
    if (const auto *held = std::get_if<std::string>(&value))
    {
      text = *held;
      return true;
    }

    error = what + " takes text";
    return false;
  }

  [[nodiscard]] inline bool TakeFlag(const FieldValue &value, const std::string &what, bool &flag, std::string &error)
  {
    if (const auto *held = std::get_if<bool>(&value))
    {
      flag = *held;
      return true;
    }

    error = what + " takes true or false";
    return false;
  }

  /// A number, which a whole number is as well.
  [[nodiscard]] inline bool TakeNumber(
    const FieldValue &value,
    const std::string &what,
    float &number,
    std::string &error)
  {
    if (const auto *held = std::get_if<float>(&value))
    {
      number = *held;
      return true;
    }

    if (const auto *whole = std::get_if<int>(&value))
    {
      number = static_cast<float>(*whole);
      return true;
    }

    error = what + " takes a number";
    return false;
  }

  [[nodiscard]] inline bool TakeWhole(const FieldValue &value, const std::string &what, int &whole, std::string &error)
  {
    if (const auto *held = std::get_if<int>(&value))
    {
      whole = *held;
      return true;
    }

    error = what + " takes a number without a fraction";
    return false;
  }
} // neon

#endif //UI_FIELDS_HPP
