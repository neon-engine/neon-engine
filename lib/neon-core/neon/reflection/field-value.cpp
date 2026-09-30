#include "field-value.hpp"

namespace neon
{
  std::string Describe(const FieldKind kind)
  {
    switch (kind)
    {
      case FieldKind::Bool: return "true or false";
      case FieldKind::Whole: return "a whole number";
      case FieldKind::Number: return "a number";
      case FieldKind::Text: return "text";
      case FieldKind::Vector: return "three numbers";
      case FieldKind::Color: return "a color";
      case FieldKind::TextList: return "a list of texts";
      case FieldKind::Choice: return "one of a few words";
      case FieldKind::Group: return "a group";
      case FieldKind::Length: return "a length";
      case FieldKind::NumberList: return "a list of numbers";
    }
    return "unknown";
  }

  bool Holds(const FieldValue &value, const FieldKind kind)
  {
    switch (kind)
    {
      case FieldKind::Bool: return std::holds_alternative<bool>(value);
      case FieldKind::Whole: return std::holds_alternative<int>(value);
      case FieldKind::Number: return std::holds_alternative<float>(value);
      case FieldKind::Text: return std::holds_alternative<std::string>(value);
      case FieldKind::Vector: return std::holds_alternative<glm::vec3>(value);
      case FieldKind::Color: return std::holds_alternative<Color>(value);
      case FieldKind::TextList: return std::holds_alternative<std::vector<std::string>>(value);
      case FieldKind::Choice: return std::holds_alternative<std::string>(value);
      case FieldKind::Group: return false;
      case FieldKind::Length: return std::holds_alternative<FieldLength>(value);
      case FieldKind::NumberList: return std::holds_alternative<std::vector<float>>(value);
    }
    return false;
  }

  bool Same(const FieldValue &left, const FieldValue &right)
  {
    if (left.index() != right.index()) { return false; }

    if (const auto *color = std::get_if<Color>(&left))
    {
      const auto &other = std::get<Color>(right);
      return color->r == other.r && color->g == other.g && color->b == other.b && color->a == other.a;
    }

    if (const auto *vector = std::get_if<glm::vec3>(&left))
    {
      return *vector == std::get<glm::vec3>(right);
    }

    if (const auto *flag = std::get_if<bool>(&left)) { return *flag == std::get<bool>(right); }
    if (const auto *whole = std::get_if<int>(&left)) { return *whole == std::get<int>(right); }
    if (const auto *number = std::get_if<float>(&left)) { return *number == std::get<float>(right); }
    if (const auto *text = std::get_if<std::string>(&left)) { return *text == std::get<std::string>(right); }

    if (const auto *texts = std::get_if<std::vector<std::string>>(&left))
    {
      return *texts == std::get<std::vector<std::string>>(right);
    }

    if (const auto *length = std::get_if<FieldLength>(&left)) { return *length == std::get<FieldLength>(right); }

    if (const auto *numbers = std::get_if<std::vector<float>>(&left))
    {
      return *numbers == std::get<std::vector<float>>(right);
    }

    // both hold nothing
    return true;
  }
} // neon
