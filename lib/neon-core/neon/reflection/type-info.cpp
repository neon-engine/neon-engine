#include "type-info.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace neon
{
  // Helpers of TypeBuilder, for this file alone.
  namespace
  {
    std::string Join(const std::vector<std::string> &words)
    {
      std::string joined;
      for (const auto &word : words)
      {
        if (!joined.empty()) { joined += ", "; }
        joined += word;
      }
      return joined;
    }

    /// A value as a message shows it: `0.5`, and `[1, 0, 1]` for more than
    /// one number.
    std::string Written(const FieldKind kind, const std::vector<float> &numbers)
    {
      if (kind == FieldKind::Float || IsWholeKind(kind)) { return std::format("{}", numbers.front()); }

      std::string written;
      for (const float number : numbers)
      {
        if (!written.empty()) { written += ", "; }
        written += std::format("{}", number);
      }
      return "[" + written + "]";
    }

    /// What a field may hold, as a message says it: `a number above 0 was
    /// expected`, `numbers from 0 to 1 were expected`, or with a unit,
    /// `degrees from 0 to 90 were expected`. It is how the engine says it
    /// wherever it reads a file.
    std::string Expected(const FieldInfo &field)
    {
      std::vector<std::string> limits;
      if (field.above.has_value()) { limits.push_back(std::format("above {}", *field.above)); }

      if (field.at_least.has_value() && field.at_most.has_value())
      {
        limits.push_back(std::format("from {} to {}", *field.at_least, *field.at_most));
      } else if (field.at_least.has_value())
      {
        limits.push_back(std::format("of {} or above", *field.at_least));
      } else if (field.at_most.has_value())
      {
        limits.push_back(std::format("of {} or below", *field.at_most));
      }

      std::string range;
      for (const auto &limit : limits)
      {
        if (!range.empty()) { range += " and "; }
        range += limit;
      }

      const bool one = field.kind == FieldKind::Float || field.kind == FieldKind::Integer
                       || field.kind == FieldKind::Double;

      if (!field.unit.empty()) { return std::format("{} {} were expected", field.unit, range); }
      if (one) { return std::format("a number {} was expected", range); }
      return std::format("numbers {} were expected", range);
    }

    const FieldInfo *FindIn(const std::vector<FieldInfo> &fields, const std::string &path)
    {
      const auto dot = path.find('.');
      const auto name = path.substr(0, dot);

      for (const auto &field : fields)
      {
        if (field.name != name) { continue; }

        if (dot == std::string::npos) { return &field; }
        return field.kind == FieldKind::Group ? FindIn(field.fields, path.substr(dot + 1)) : nullptr;
      }

      return nullptr;
    }

    void CollectPaths(
      const std::vector<FieldInfo> &fields,
      const std::string &prefix,
      std::vector<std::string> &paths)
    {
      for (const auto &field : fields)
      {
        if (field.kind == FieldKind::Group)
        {
          CollectPaths(field.fields, prefix + field.name + ".", paths);
        } else
        {
          paths.push_back(prefix + field.name);
        }
      }
    }
  }

  bool IsLayer(const float number)
  {
    return number >= 1.0f && number <= 32.0f && number == std::floor(number);
  }

  std::string FieldInfo::Check(const FieldValue &value, const std::string &what) const
  {
    if (kind == FieldKind::Group)
    {
      return std::format("{} is a group and holds no value of its own", what);
    }

    if (!Holds(value, kind))
    {
      return std::format("{} takes {}", what, Describe(kind));
    }

    if (kind == FieldKind::Choice)
    {
      const auto &word = std::get<std::string>(value);
      for (const auto &choice : choices)
      {
        if (choice == word) { return {}; }
      }
      return std::format("{} is '{}', where one of these was expected: {}", what, word, Join(choices));
    }

    if (kind == FieldKind::String && required && std::get<std::string>(value).empty())
    {
      return std::format("{} must not be empty", what);
    }

    if (kind == FieldKind::Layers)
    {
      for (const float number : std::get<std::vector<float>>(value))
      {
        if (!IsLayer(number))
        {
          return std::format("{} holds {}, where a layer from 1 to 32 was expected", what, number);
        }
      }
      return {};
    }

    if (kind == FieldKind::Double)
    {
      // compared as a double, so that a tiny number above 0 is not rounded
      // down onto the limit
      const double number = std::get<double>(value);
      const bool fits = (!above.has_value() || number > *above)
                        && (!at_least.has_value() || number >= *at_least)
                        && (!at_most.has_value() || number <= *at_most);

      if (!fits) { return std::format("{} is {}, where {}", what, number, Expected(*this)); }
      return {};
    }

    // the numbers that the limits are about
    std::vector<float> numbers;
    if (kind == FieldKind::Float) { numbers.push_back(std::get<float>(value)); }
    if (IsWholeKind(kind)) { numbers.push_back(static_cast<float>(WholeNumber(value))); }

    if (kind == FieldKind::Vector3)
    {
      const auto &vector = std::get<glm::vec3>(value);
      numbers = {vector.x, vector.y, vector.z};
    }
    if (kind == FieldKind::Vector2)
    {
      const auto &vector = std::get<glm::vec2>(value);
      numbers = {vector.x, vector.y};
    }
    if (kind == FieldKind::Vector4)
    {
      const auto &vector = std::get<glm::vec4>(value);
      numbers = {vector.x, vector.y, vector.z, vector.w};
    }
    if (kind == FieldKind::IntegerVector2)
    {
      const auto &vector = std::get<glm::ivec2>(value);
      numbers = {static_cast<float>(vector.x), static_cast<float>(vector.y)};
    }
    if (kind == FieldKind::IntegerVector3)
    {
      const auto &vector = std::get<glm::ivec3>(value);
      numbers = {static_cast<float>(vector.x), static_cast<float>(vector.y), static_cast<float>(vector.z)};
    }
    if (kind == FieldKind::IntegerList)
    {
      for (const int number : std::get<std::vector<int>>(value)) { numbers.push_back(static_cast<float>(number)); }
    }
    if (kind == FieldKind::Vector3List)
    {
      for (const auto &vector : std::get<std::vector<glm::vec3>>(value))
      {
        numbers.insert(numbers.end(), {vector.x, vector.y, vector.z});
      }
    }

    if (kind == FieldKind::FloatList)
    {
      numbers = std::get<std::vector<float>>(value);

      if (count.has_value() && numbers.size() != *count)
      {
        return std::format(
          "{} holds {} number{}, where {} were expected",
          what, numbers.size(), numbers.size() == 1 ? "" : "s", *count);
      }
    }

    for (const float number : numbers)
    {
      // written so that what is not a number is refused as well
      const bool fits = (!above.has_value() || number > *above)
                        && (!at_least.has_value() || number >= *at_least)
                        && (!at_most.has_value() || number <= *at_most);

      if (!fits) { return std::format("{} is {}, where {}", what, Written(kind, numbers), Expected(*this)); }
    }

    return {};
  }

  bool TypeInfo::Belongs(const FieldInfo &field, const void *object) const
  {
    if (!field.only_when.has_value()) { return true; }

    const auto *choice = Find(field.only_when->choice);
    if (choice == nullptr) { return false; }

    const auto word = std::get<std::string>(choice->get(object));
    return std::ranges::find(field.only_when->words, word) != field.only_when->words.end();
  }

  const FieldInfo *TypeInfo::Find(const std::string &path) const
  {
    return FindIn(fields, path);
  }

  std::vector<std::string> TypeInfo::GetPaths() const
  {
    std::vector<std::string> paths;
    CollectPaths(fields, "", paths);
    return paths;
  }
} // neon
