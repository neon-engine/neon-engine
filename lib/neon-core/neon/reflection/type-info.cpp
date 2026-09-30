#include "type-info.hpp"

#include <format>

namespace neon
{
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

    if (kind == FieldKind::Text && required && std::get<std::string>(value).empty())
    {
      return std::format("{} must not be empty", what);
    }

    // the numbers that the limits are about
    std::vector<float> numbers;
    if (kind == FieldKind::Number) { numbers.push_back(std::get<float>(value)); }
    if (kind == FieldKind::Whole) { numbers.push_back(static_cast<float>(std::get<int>(value))); }

    if (kind == FieldKind::NumberList)
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
      if (above.has_value() && !(number > *above))
      {
        return std::format("{} has to be above {}", what, *above);
      }
      if (at_least.has_value() && number < *at_least)
      {
        return std::format("{} has to be at least {}", what, *at_least);
      }
      if (at_most.has_value() && number > *at_most)
      {
        return std::format("{} has to be at most {}", what, *at_most);
      }
    }

    return {};
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
