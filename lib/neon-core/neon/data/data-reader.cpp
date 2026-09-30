#include "data-reader.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace neon
{
  namespace
  {
    const DataValue empty_map = DataValue::Map();

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
  }

  DataReader::DataReader(
    const DataValue &map,
    const std::string &document,
    const std::string &where,
    std::vector<std::string> &errors)
  {
    _map = &map;
    _document = document;
    _where = where;
    _errors = &errors;
    _line = map.GetLine();

    if (!map.IsMap() && !map.IsEmpty())
    {
      Report(map, std::format("{} is {}, where a map was expected", where, DataValue::Describe(map.GetKind())));
      _map = &empty_map;
    }
  }

  const std::string &DataReader::GetWhere() const
  {
    return _where;
  }

  const std::string &DataReader::GetDocument() const
  {
    return _document;
  }

  std::vector<std::string> &DataReader::GetErrors() const
  {
    return *_errors;
  }

  void DataReader::Report(const DataValue &value, const std::string &message) const
  {
    if (value.GetLine() > 0)
    {
      _errors->push_back(std::format("{}:{}: {}", _document, value.GetLine(), message));
    } else
    {
      _errors->push_back(std::format("{}: {}", _document, message));
    }
  }

  void DataReader::Report(const std::string &message) const
  {
    DataValue whole;
    whole.SetLine(_line);
    Report(whole, message);
  }

  const DataValue *DataReader::Ask(const std::string &name) const
  {
    if (std::ranges::find(_asked, name) == _asked.end()) { _asked.push_back(name); }

    const auto *value = _map->Find(name);

    // a name with nothing behind it counts as not written
    return value == nullptr || value->IsEmpty() ? nullptr : value;
  }

  bool DataReader::Has(const std::string &name) const
  {
    return Ask(name) != nullptr;
  }

  bool DataReader::Read(const std::string &name, bool &value) const
  {
    const auto *found = Ask(name);
    if (found == nullptr) { return false; }

    if (!found->GetBool(value))
    {
      Report(*found, std::format(
               "'{}' of {} is {}, where true or false was expected",
               name, _where, DataValue::Describe(found->GetKind())));
      return false;
    }
    return true;
  }

  bool DataReader::Read(const std::string &name, float &value) const
  {
    const auto *found = Ask(name);
    if (found == nullptr) { return false; }

    if (!found->GetNumber(value))
    {
      Report(*found, std::format(
               "'{}' of {} is {}, where a number was expected",
               name, _where, DataValue::Describe(found->GetKind())));
      return false;
    }
    return true;
  }

  bool DataReader::Read(const std::string &name, int &value) const
  {
    const auto *found = Ask(name);
    if (found == nullptr) { return false; }

    double number = 0.0;
    if (!found->GetNumber(number))
    {
      Report(*found, std::format(
               "'{}' of {} is {}, where a whole number was expected",
               name, _where, DataValue::Describe(found->GetKind())));
      return false;
    }

    if (number != std::floor(number) || std::fabs(number) > 2147483647.0)
    {
      Report(*found, std::format(
               "'{}' of {} is {}, where a whole number was expected",
               name, _where, number));
      return false;
    }

    value = static_cast<int>(number);
    return true;
  }

  bool DataReader::Read(const std::string &name, std::string &value) const
  {
    const auto *found = Ask(name);
    if (found == nullptr) { return false; }

    if (!found->GetText(value))
    {
      Report(*found, std::format(
               "'{}' of {} is {}, where text was expected",
               name, _where, DataValue::Describe(found->GetKind())));
      return false;
    }
    return true;
  }

  bool DataReader::Read(const std::string &name, std::vector<std::string> &value) const
  {
    const auto *found = Ask(name);
    if (found == nullptr) { return false; }

    if (!found->IsList())
    {
      Report(*found, std::format(
               "'{}' of {} is {}, where a list of texts was expected",
               name, _where, DataValue::Describe(found->GetKind())));
      return false;
    }

    std::vector<std::string> texts;
    for (const auto &item : found->GetItems())
    {
      std::string text;
      if (!item.GetText(text))
      {
        Report(item, std::format(
                 "'{}' of {} holds {}, where text was expected",
                 name, _where, DataValue::Describe(item.GetKind())));
        return false;
      }
      texts.push_back(text);
    }

    value = texts;
    return true;
  }

  bool DataReader::ReadNumbers(
    const std::string &name,
    float *numbers,
    const std::size_t least,
    const std::size_t most,
    std::size_t &count) const
  {
    const auto *found = Ask(name);
    if (found == nullptr) { return false; }

    const std::string expected = least == most
      ? std::format("a list of {} numbers", least)
      : std::format("a list of {} to {} numbers", least, most);

    if (!found->IsList())
    {
      Report(*found, std::format(
               "'{}' of {} is {}, where {} was expected",
               name, _where, DataValue::Describe(found->GetKind()), expected));
      return false;
    }

    const auto &items = found->GetItems();
    if (items.size() < least || items.size() > most)
    {
      Report(*found, std::format(
               "'{}' of {} holds {} values, where {} was expected",
               name, _where, items.size(), expected));
      return false;
    }

    for (std::size_t i = 0; i < items.size(); i++)
    {
      if (!items[i].GetNumber(numbers[i]))
      {
        Report(items[i], std::format(
                 "'{}' of {} holds {}, where a number was expected",
                 name, _where, DataValue::Describe(items[i].GetKind())));
        return false;
      }
    }

    count = items.size();
    return true;
  }

  bool DataReader::Read(const std::string &name, glm::vec3 &value) const
  {
    float numbers[3] = {};
    std::size_t count = 0;
    if (!ReadNumbers(name, numbers, 3, 3, count)) { return false; }

    value = {numbers[0], numbers[1], numbers[2]};
    return true;
  }

  bool DataReader::ReadScale(const std::string &name, glm::vec3 &value) const
  {
    if (const auto *found = Ask(name); found != nullptr)
    {
      if (float all = 0.0f; found->GetNumber(all))
      {
        value = glm::vec3{all};
        return true;
      }
    }

    return Read(name, value);
  }

  bool DataReader::Read(const std::string &name, Color &value) const
  {
    float numbers[4] = {};
    std::size_t count = 0;
    if (!ReadNumbers(name, numbers, 3, 4, count)) { return false; }

    value.r = numbers[0];
    value.g = numbers[1];
    value.b = numbers[2];
    value.a = count == 4 ? numbers[3] : 1.0f;
    return true;
  }

  bool DataReader::ReadChoice(
    const std::string &name,
    const std::vector<std::string> &choices,
    std::size_t &value) const
  {
    const auto *found = Ask(name);
    if (found == nullptr) { return false; }

    std::string text;
    const bool is_text = found->GetText(text);

    if (const auto it = std::ranges::find(choices, text); is_text && it != choices.end())
    {
      value = static_cast<std::size_t>(it - choices.begin());
      return true;
    }

    Report(*found, std::format(
             "'{}' of {} is {}, where one of these was expected: {}",
             name, _where, is_text ? "'" + text + "'" : DataValue::Describe(found->GetKind()), Join(choices)));
    return false;
  }

  DataReader DataReader::ReadMap(const std::string &name, bool &found) const
  {
    const auto *value = Ask(name);
    found = value != nullptr;

    return {
      found ? *value : empty_map,
      _document,
      std::format("'{}' of {}", name, _where),
      *_errors
    };
  }

  const DataValue *DataReader::ReadValue(const std::string &name) const
  {
    return Ask(name);
  }

  void DataReader::Finish() const
  {
    for (const auto &[name, value] : _map->GetEntries())
    {
      if (std::ranges::find(_asked, name) != _asked.end()) { continue; }

      Report(value, std::format(
               "'{}' is not known to {}. Known are: {}",
               name, _where, _asked.empty() ? "none" : Join(_asked)));
    }
  }
} // neon
