#include "field-documents.hpp"

#include <format>

namespace neon
{
  namespace
  {
    DataValue ListOf(const glm::vec3 &vector)
    {
      auto list = DataValue::List();
      list.Add(DataValue::Number(vector.x));
      list.Add(DataValue::Number(vector.y));
      list.Add(DataValue::Number(vector.z));
      return list;
    }

    /// `a` or `an`, as the word that follows asks for.
    std::string WithArticle(const std::string &name)
    {
      const bool vowel = !name.empty() && std::string("aeiou").find(name.front()) != std::string::npos;
      return std::format("{} '{}'", vowel ? "an" : "a", name);
    }

    /// Reads one field. Returns whether a value was written for it.
    bool ReadField(const FieldInfo &field, const DataReader &reader, void *object)
    {
      auto value = FieldValue{};

      switch (field.kind)
      {
        case FieldKind::Bool:
        {
          bool read = false;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Whole:
        {
          int read = 0;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Number:
        {
          float read = 0.0f;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Text:
        {
          std::string read;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Vector:
        {
          glm::vec3 read{0.0f};
          const bool was_read = field.one_number_for_all
            ? reader.ReadScale(field.name, read)
            : reader.Read(field.name, read);
          if (!was_read) { return false; }
          value = read;
          break;
        }
        case FieldKind::Color:
        {
          Color read;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::TextList:
        {
          std::vector<std::string> read;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Choice:
        {
          std::size_t index = 0;
          if (!reader.ReadChoice(field.name, field.choices, index)) { return false; }
          value = field.choices[index];
          break;
        }
        case FieldKind::Group:
        {
          bool found = false;
          const auto group = reader.ReadMap(field.name, found);

          TypeInfo fields;
          fields.fields = field.fields;
          ReadFields(fields, group, object);
          group.Finish();
          return found;
        }
      }

      const auto what = std::format("'{}' of {}", field.name, reader.GetWhere());
      if (const auto problem = field.Check(value, what); !problem.empty())
      {
        const auto *written = reader.ReadValue(field.name);
        reader.Report(written == nullptr ? DataValue{} : *written, problem);
        return false;
      }

      field.set(object, value);
      return true;
    }
  }

  void ReadFields(const TypeInfo &type, const DataReader &reader, void *object)
  {
    for (const auto &field : type.fields)
    {
      ReadField(field, reader, object);

      if (!field.required || field.kind != FieldKind::Text) { continue; }

      // the value may come from the file or from the defaults of the type
      if (std::get<std::string>(field.get(object)).empty())
      {
        reader.Report({}, std::format("{} needs {}", reader.GetWhere(), WithArticle(field.name)));
      }
    }
  }

  DataValue ToDataValue(const FieldValue &value)
  {
    if (const auto *flag = std::get_if<bool>(&value)) { return DataValue::Bool(*flag); }
    if (const auto *whole = std::get_if<int>(&value)) { return DataValue::Number(*whole); }
    if (const auto *number = std::get_if<float>(&value)) { return DataValue::Number(*number); }
    if (const auto *text = std::get_if<std::string>(&value)) { return DataValue::Text(*text); }
    if (const auto *vector = std::get_if<glm::vec3>(&value)) { return ListOf(*vector); }

    if (const auto *color = std::get_if<Color>(&value))
    {
      auto list = ListOf({color->r, color->g, color->b});

      // an alpha of 1 is what a color has when none is written
      if (color->a != 1.0f) { list.Add(DataValue::Number(color->a)); }
      return list;
    }

    if (const auto *texts = std::get_if<std::vector<std::string>>(&value))
    {
      auto list = DataValue::List();
      for (const auto &text : *texts) { list.Add(DataValue::Text(text)); }
      return list;
    }

    return {};
  }

  void WriteFields(const TypeInfo &type, const void *object, const void *standard, DataValue &map)
  {
    for (const auto &field : type.fields)
    {
      if (field.kind == FieldKind::Group)
      {
        TypeInfo fields;
        fields.fields = field.fields;

        auto group = DataValue::Map();
        WriteFields(fields, object, standard, group);

        if (!group.GetEntries().empty() || field.always_written) { map.Set(field.name, group); }
        continue;
      }

      const auto value = field.get(object);
      if (!field.always_written && Same(value, field.get(standard))) { continue; }

      map.Set(field.name, ToDataValue(value));
    }
  }
} // neon
