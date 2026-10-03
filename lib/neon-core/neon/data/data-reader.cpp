#include "data-reader.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace neon
{
  // Helpers of DataReader, for this file alone.
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

    if (found->IsPrecise())
    {
      Report(*found, std::format(
               "'{}' of {} is written as a double, with d, where a float is read and would lose that "
               "precision. Write it without the suffix, or with f",
               name, _where));
      return false;
    }
    return true;
  }

  bool DataReader::Read(const std::string &name, double &value) const
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

  bool DataReader::Read(const std::string &name, std::vector<float> &value) const
  {
    const auto *found = Ask(name);
    if (found == nullptr) { return false; }

    if (!found->IsList())
    {
      Report(*found, std::format(
               "'{}' of {} is {}, where a list of numbers was expected",
               name, _where, DataValue::Describe(found->GetKind())));
      return false;
    }

    std::vector<float> numbers;
    for (const auto &item : found->GetItems())
    {
      float number = 0.0f;
      if (!item.GetNumber(number))
      {
        Report(item, std::format(
                 "'{}' of {} holds {}, where a number was expected",
                 name, _where, DataValue::Describe(item.GetKind())));
        return false;
      }

      if (item.IsPrecise())
      {
        Report(item, std::format(
                 "'{}' of {} holds a number written as a double, with d, where floats are read and would "
                 "lose that precision. Write it without the suffix, or with f",
                 name, _where));
        return false;
      }
      numbers.push_back(number);
    }

    value = numbers;
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

      if (items[i].IsPrecise())
      {
        Report(items[i], std::format(
                 "'{}' of {} holds a number written as a double, with d, where floats are read and would "
                 "lose that precision. Write it without the suffix, or with f",
                 name, _where));
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

  bool DataReader::Read(const std::string &name, glm::vec2 &value) const
  {
    float numbers[2] = {};
    std::size_t count = 0;
    if (!ReadNumbers(name, numbers, 2, 2, count)) { return false; }

    value = {numbers[0], numbers[1]};
    return true;
  }

  bool DataReader::Read(const std::string &name, glm::vec4 &value) const
  {
    float numbers[4] = {};
    std::size_t count = 0;
    if (!ReadNumbers(name, numbers, 4, 4, count)) { return false; }

    value = {numbers[0], numbers[1], numbers[2], numbers[3]};
    return true;
  }

  bool DataReader::Read(const std::string &name, glm::ivec2 &value) const
  {
    std::vector<int> wholes;
    if (!Read(name, wholes)) { return false; }
    if (wholes.size() != 2)
    {
      Report(*Ask(name), std::format("'{}' of {} holds {} values, where two whole numbers were expected", name, _where, wholes.size()));
      return false;
    }
    value = {wholes[0], wholes[1]};
    return true;
  }

  bool DataReader::Read(const std::string &name, glm::ivec3 &value) const
  {
    std::vector<int> wholes;
    if (!Read(name, wholes)) { return false; }
    if (wholes.size() != 3)
    {
      Report(*Ask(name), std::format("'{}' of {} holds {} values, where three whole numbers were expected", name, _where, wholes.size()));
      return false;
    }
    value = {wholes[0], wholes[1], wholes[2]};
    return true;
  }

  bool DataReader::Read(const std::string &name, std::vector<int> &value) const
  {
    const auto *found = Ask(name);
    if (found == nullptr) { return false; }

    if (!found->IsList())
    {
      Report(*found, std::format(
               "'{}' of {} is {}, where a list of whole numbers was expected",
               name, _where, DataValue::Describe(found->GetKind())));
      return false;
    }

    std::vector<int> wholes;
    for (const auto &item : found->GetItems())
    {
      double number = 0.0;
      if (!item.GetNumber(number) || number != std::floor(number) || std::fabs(number) > 2147483647.0)
      {
        Report(item, std::format("'{}' of {} holds {}, where a whole number was expected", name, _where, DataValue::Describe(item.GetKind())));
        return false;
      }
      wholes.push_back(static_cast<int>(number));
    }

    value = wholes;
    return true;
  }

  bool DataReader::Read(const std::string &name, std::vector<glm::vec3> &value) const
  {
    const auto *found = Ask(name);
    if (found == nullptr) { return false; }

    if (!found->IsList())
    {
      Report(*found, std::format(
               "'{}' of {} is {}, where a list of lists of three numbers was expected",
               name, _where, DataValue::Describe(found->GetKind())));
      return false;
    }

    std::vector<glm::vec3> vectors;
    for (const auto &item : found->GetItems())
    {
      if (!item.IsList() || item.GetItems().size() != 3)
      {
        Report(item, std::format("'{}' of {} holds {}, where a list of three numbers was expected", name, _where, DataValue::Describe(item.GetKind())));
        return false;
      }
      glm::vec3 vector{0.0f};
      float *parts[3] = {&vector.x, &vector.y, &vector.z};
      for (std::size_t i = 0; i < 3; i++)
      {
        if (!item.GetItems()[i].GetNumber(*parts[i]))
        {
          Report(item, std::format("'{}' of {} holds {}, where a number was expected", name, _where, DataValue::Describe(item.GetItems()[i].GetKind())));
          return false;
        }
      }
      vectors.push_back(vector);
    }

    value = vectors;
    return true;
  }

  bool DataReader::Read(const std::string &name, glm::quat &value) const
  {
    float numbers[4] = {};
    std::size_t count = 0;
    if (!ReadNumbers(name, numbers, 4, 4, count)) { return false; }

    value = glm::quat{numbers[3], numbers[0], numbers[1], numbers[2]};
    return true;
  }

  // Helpers of the matrices, for this file alone.
  namespace
  {
    /// Reads `size` rows of `size` numbers into the columns of a matrix,
    /// which is how glm keeps one.
    bool read_rows(const DataReader &reader, const DataValue *found, const std::string &name, const std::string &where, const int size, glm::mat4 &matrix)
    {
      const std::string expected = std::format("{} rows of {} numbers", size, size);
      if (!found->IsList() || static_cast<int>(found->GetItems().size()) != size)
      {
        reader.Report(*found, std::format("'{}' of {} is {}, where {} were expected", name, where, DataValue::Describe(found->GetKind()), expected));
        return false;
      }
      for (int row = 0; row < size; row++)
      {
        const DataValue &line = found->GetItems()[static_cast<std::size_t>(row)];
        if (!line.IsList() || static_cast<int>(line.GetItems().size()) != size)
        {
          reader.Report(line, std::format("'{}' of {} holds a row that is not {} numbers, where {} were expected", name, where, size, expected));
          return false;
        }
        for (int column = 0; column < size; column++)
        {
          if (!line.GetItems()[static_cast<std::size_t>(column)].GetNumber(matrix[column][row]))
          {
            reader.Report(line, std::format("'{}' of {} holds a row with something that is not a number, where {} were expected", name, where, expected));
            return false;
          }
        }
      }
      return true;
    }
  }

  bool DataReader::Read(const std::string &name, glm::mat3 &value) const
  {
    const auto *found = Ask(name);
    if (found == nullptr) { return false; }
    glm::mat4 matrix{1.0f};
    if (!read_rows(*this, found, name, _where, 3, matrix)) { return false; }
    value = glm::mat3(matrix);
    return true;
  }

  bool DataReader::Read(const std::string &name, glm::mat4 &value) const
  {
    const auto *found = Ask(name);
    if (found == nullptr) { return false; }
    glm::mat4 matrix{1.0f};
    if (!read_rows(*this, found, name, _where, 4, matrix)) { return false; }
    value = matrix;
    return true;
  }

  bool DataReader::ReadWhole(
    const std::string &name,
    const double least,
    const double most,
    const std::string &expected,
    double &value) const
  {
    const auto *found = Ask(name);
    if (found == nullptr) { return false; }

    double number = 0.0;
    if (!found->GetNumber(number))
    {
      Report(*found, std::format("'{}' of {} is {}, where {} was expected", name, _where, DataValue::Describe(found->GetKind()), expected));
      return false;
    }

    if (number != std::floor(number) || number < least || number > most)
    {
      Report(*found, std::format("'{}' of {} is {}, where {} was expected", name, _where, number, expected));
      return false;
    }

    value = number;
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
