#include "ui-call-field.hpp"

#include <format>

#include "ui-fields.hpp"

namespace neon
{
  void UiCallField::Read(const DataReader &reader, const std::string &name, UiCall &call)
  {
    const DataValue *value = reader.ReadValue(name);
    if (value == nullptr) { return; }

    std::string text;
    if (!value->GetText(text))
    {
      reader.Report(*value, std::format(
                      "'{}' of {} is {}, where the call of a function was expected, such as unlock or open('safe')",
                      name,
                      reader.GetWhere(),
                      DataValue::Describe(value->GetKind())));
      return;
    }

    UiCall read;
    if (std::string problem; !UiCall::Parse(text, read, problem))
    {
      reader.Report(*value, std::format(
                      "'{}' of {} is '{}', which is no call of a function: {}",
                      name,
                      reader.GetWhere(),
                      text,
                      problem));
      return;
    }

    call = read;
  }

  bool UiCallField::Set(const FieldValue &value, const std::string &what, UiCall &call, std::string &error)
  {
    std::string text;
    if (!TakeText(value, what, text, error)) { return false; }

    // an empty text takes the call away
    if (text.empty())
    {
      call = {};
      return true;
    }

    UiCall parsed;
    if (std::string problem; !UiCall::Parse(text, parsed, problem))
    {
      error = std::format("{} is '{}', which is no call of a function: {}", what, text, problem);
      return false;
    }

    call = parsed;
    return true;
  }
} // neon
