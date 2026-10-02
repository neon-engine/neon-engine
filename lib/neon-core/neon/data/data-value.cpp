#include "data-value.hpp"

namespace neon
{
  DataValue DataValue::Bool(const bool value)
  {
    DataValue result;
    result._kind = Kind::Bool;
    result._bool = value;
    return result;
  }

  DataValue DataValue::Number(const double value)
  {
    DataValue result;
    result._kind = Kind::Number;
    result._number = value;
    return result;
  }

  DataValue DataValue::Number(const float value)
  {
    DataValue result;
    result._kind = Kind::Number;
    result._number = static_cast<double>(value);
    result._single_precision = true;
    return result;
  }

  DataValue DataValue::Number(const int value)
  {
    return Number(static_cast<double>(value));
  }

  DataValue DataValue::PreciseNumber(const double value)
  {
    DataValue result = Number(value);
    result._precise = true;
    return result;
  }

  DataValue DataValue::Text(const std::string &value)
  {
    DataValue result;
    result._kind = Kind::Text;
    result._text = value;
    return result;
  }

  DataValue DataValue::List()
  {
    DataValue result;
    result._kind = Kind::List;
    return result;
  }

  DataValue DataValue::Map()
  {
    DataValue result;
    result._kind = Kind::Map;
    return result;
  }

  DataValue::Kind DataValue::GetKind() const
  {
    return _kind;
  }

  bool DataValue::IsEmpty() const
  {
    return _kind == Kind::Empty;
  }

  bool DataValue::IsList() const
  {
    return _kind == Kind::List;
  }

  bool DataValue::IsMap() const
  {
    return _kind == Kind::Map;
  }

  std::size_t DataValue::GetLine() const
  {
    return _line;
  }

  void DataValue::SetLine(const std::size_t line)
  {
    _line = line;
  }

  bool DataValue::GetBool(bool &value) const
  {
    if (_kind != Kind::Bool) { return false; }
    value = _bool;
    return true;
  }

  bool DataValue::GetNumber(double &value) const
  {
    if (_kind != Kind::Number) { return false; }
    value = _number;
    return true;
  }

  bool DataValue::GetNumber(float &value) const
  {
    if (_kind != Kind::Number) { return false; }
    value = static_cast<float>(_number);
    return true;
  }

  bool DataValue::GetText(std::string &value) const
  {
    if (_kind != Kind::Text) { return false; }
    value = _text;
    return true;
  }

  bool DataValue::IsSinglePrecision() const
  {
    return _single_precision;
  }

  bool DataValue::IsPrecise() const
  {
    return _precise;
  }

  const std::vector<DataValue> &DataValue::GetItems() const
  {
    return _items;
  }

  const std::vector<DataValue::Entry> &DataValue::GetEntries() const
  {
    return _entries;
  }

  const DataValue *DataValue::Find(const std::string &name) const
  {
    for (const auto &[entry_name, value] : _entries)
    {
      if (entry_name == name) { return &value; }
    }
    return nullptr;
  }

  DataValue &DataValue::Add(const DataValue &value)
  {
    _kind = Kind::List;
    _items.push_back(value);
    return _items.back();
  }

  DataValue &DataValue::Set(const std::string &name, const DataValue &value)
  {
    _kind = Kind::Map;

    for (auto &[entry_name, entry_value] : _entries)
    {
      if (entry_name == name)
      {
        entry_value = value;
        return entry_value;
      }
    }

    _entries.emplace_back(name, value);
    return _entries.back().second;
  }

  std::string DataValue::Describe(const Kind kind)
  {
    switch (kind)
    {
      case Kind::Empty: return "nothing";
      case Kind::Bool: return "true or false";
      case Kind::Number: return "a number";
      case Kind::Text: return "text";
      case Kind::List: return "a list";
      case Kind::Map: return "a map";
    }
    return "unknown";
  }
} // neon
