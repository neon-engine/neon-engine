#include "field-documents.hpp"

#include <format>

#include "field-text.hpp"

namespace neon
{
  // Helpers of field-documents.cpp, for this file alone.
  namespace
  {
    DataValue ListOf(const std::vector<float> &numbers)
    {
      auto list = DataValue::List();
      for (const float number : numbers) { list.Add(DataValue::Number(number)); }
      return list;
    }

    DataValue ListOf(const glm::vec3 &vector)
    {
      return ListOf(std::vector{vector.x, vector.y, vector.z});
    }

    /// `a` or `an`, as the word that follows asks for.
    std::string WithArticle(const std::string &name)
    {
      const bool vowel = !name.empty() && std::string("aeiou").find(name.front()) != std::string::npos;
      return std::format("{} '{}'", vowel ? "an" : "a", name);
    }

    /// Reports what breaks a rule of the type.
    void CheckRules(const TypeInfo &type, const DataReader &reader, const void *object)
    {
      for (const auto &rule : type.rules)
      {
        const auto problem = rule.check(object, reader.GetWhere());
        if (problem.empty()) { continue; }

        // a field that does not belong is not asked for, which would make it
        // known
        const auto *field = rule.field.empty() ? nullptr : type.Find(rule.field);
        const auto *written = field != nullptr && type.Belongs(*field, object) ? reader.ReadValue(rule.field) : nullptr;

        reader.Report(written == nullptr ? DataValue{} : *written, problem);
      }
    }

    /// Reads one field. Returns whether a value was written for it.
    bool ReadField(const FieldInfo &field, const DataReader &reader, void *object)
    {
      auto value = FieldValue{};

      switch (field.kind)
      {
        case FieldKind::Boolean:
        {
          bool read = false;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Integer:
        {
          int read = 0;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Float:
        {
          float read = 0.0f;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Double:
        {
          double read = 0.0;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::String:
        {
          std::string read;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Vector3:
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
        case FieldKind::StringList:
        {
          std::vector<std::string> read;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::FloatList:
        {
          std::vector<float> read;
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
        case FieldKind::Byte:
        case FieldKind::Short:
        case FieldKind::UnsignedShort:
        case FieldKind::UnsignedInteger:
        case FieldKind::Long:
        case FieldKind::UnsignedLong:
        {
          double least = 0.0;
          double most = 0.0;
          RangeOfWholeKind(field.kind, least, most);
          double read = 0.0;
          if (!reader.ReadWhole(field.name, least, most, Describe(field.kind), read)) { return false; }
          value = WholeValue(field.kind, read);
          break;
        }
        case FieldKind::Char:
        {
          std::string read;
          if (!reader.Read(field.name, read)) { return false; }
          if (read.size() != 1)
          {
            reader.Report(*reader.ReadValue(field.name), std::format("'{}' of {} is '{}', where one character was expected", field.name, reader.GetWhere(), read));
            return false;
          }
          value = read.front();
          break;
        }
        case FieldKind::Vector2:
        {
          glm::vec2 read{0.0f};
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Vector4:
        {
          glm::vec4 read{0.0f};
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::IntegerVector2:
        {
          glm::ivec2 read{0};
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::IntegerVector3:
        {
          glm::ivec3 read{0};
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::IntegerList:
        {
          std::vector<int> read;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Vector3List:
        {
          std::vector<glm::vec3> read;
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Quaternion:
        {
          glm::quat read{1.0f, 0.0f, 0.0f, 0.0f};
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Matrix3:
        {
          glm::mat3 read{1.0f};
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Matrix4:
        {
          glm::mat4 read{1.0f};
          if (!reader.Read(field.name, read)) { return false; }
          value = read;
          break;
        }
        case FieldKind::Length:
        {
          const auto *written = reader.ReadValue(field.name);
          if (written == nullptr) { return false; }

          // pixels as a number, and everything else as text
          FieldLength read;
          std::string text;
          std::string error;

          if (float pixels = 0.0f; written->GetNumber(pixels))
          {
            read = FieldLength::Pixels(pixels);
          } else if (!written->GetText(text) || !ParseFieldLength(text, read))
          {
            reader.Report(*written, std::format(
                            "'{}' of {} is {}, where a length such as 12, \"12px\", \"50%\", or auto was expected",
                            field.name, reader.GetWhere(),
                            written->GetText(text) ? "'" + text + "'" : DataValue::Describe(written->GetKind())));
            return false;
          }

          value = read;
          break;
        }
        case FieldKind::Layers:
        {
          const auto *written = reader.ReadValue(field.name);
          if (written == nullptr) { return false; }

          // one layer, or a list of them, where an empty list is none
          std::vector<DataValue> items;
          if (written->IsList())
          {
            items = written->GetItems();
          } else if (float number = 0.0f; written->GetNumber(number))
          {
            items.push_back(*written);
          } else
          {
            reader.Report(*written, std::format(
                            "'{}' of {} is {}, where a layer from 1 to 32 or a list of layers was expected",
                            field.name, reader.GetWhere(), DataValue::Describe(written->GetKind())));
            return false;
          }

          // each is reported at its own line
          std::vector<float> layers;
          for (const auto &item : items)
          {
            float number = 0.0f;
            if (!item.GetNumber(number) || !IsLayer(number))
            {
              const auto held = item.GetNumber(number)
                ? std::format("{}", number)
                : DataValue::Describe(item.GetKind());

              reader.Report(item, std::format(
                              "'{}' of {} holds {}, where a layer from 1 to 32 was expected",
                              field.name, reader.GetWhere(), held));
              return false;
            }
            layers.push_back(number);
          }

          value = layers;
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
      // what does not belong is not asked for, so it is reported as not
      // known when it is written
      if (!type.Belongs(field, object)) { continue; }

      ReadField(field, reader, object);

      if (!field.required || field.kind != FieldKind::String) { continue; }

      // the value may come from the file or from the defaults of the type
      if (!std::get<std::string>(field.get(object)).empty()) { continue; }

      if (field.only_when.has_value())
      {
        // it is needed because of what the choice holds, which is said
        const auto word = std::get<std::string>(type.Find(field.only_when->choice)->get(object));
        reader.Report({}, std::format(
                        "{} needs {} for the {} {}",
                        reader.GetWhere(), WithArticle(field.name), field.only_when->choice, word));
      } else
      {
        reader.Report({}, std::format("{} needs {}", reader.GetWhere(), WithArticle(field.name)));
      }
    }

    CheckRules(type, reader, object);

    // what keeps a copy of the object is told that it may have changed
    if (type.written) { type.written(object); }
  }

  DataValue ToDataValue(const FieldValue &value)
  {
    if (const auto *flag = std::get_if<bool>(&value)) { return DataValue::Bool(*flag); }
    if (const auto *whole = std::get_if<int>(&value)) { return DataValue::Number(*whole); }
    if (const auto *number = std::get_if<float>(&value)) { return DataValue::Number(*number); }
    if (const auto *precise = std::get_if<double>(&value)) { return DataValue::Number(*precise); }
    if (const auto *text = std::get_if<std::string>(&value)) { return DataValue::Text(*text); }
    if (const auto *vector = std::get_if<glm::vec3>(&value)) { return ListOf(*vector); }

    if (const auto *color = std::get_if<Color>(&value))
    {
      auto list = ListOf(std::vector{color->r, color->g, color->b});

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

    if (const auto *length = std::get_if<FieldLength>(&value))
    {
      // pixels are a number, as they are written without a unit
      if (!length->is_auto && length->percent == 0.0f) { return DataValue::Number(length->pixels); }
      return DataValue::Text(FormatFieldLength(*length));
    }

    if (const auto *numbers = std::get_if<std::vector<float>>(&value)) { return ListOf(*numbers); }

    if (const auto *character = std::get_if<char>(&value)) { return DataValue::Text(std::string(1, *character)); }
    if (std::holds_alternative<std::uint8_t>(value) || std::holds_alternative<std::int16_t>(value)
        || std::holds_alternative<std::uint16_t>(value) || std::holds_alternative<std::uint32_t>(value)
        || std::holds_alternative<std::int64_t>(value) || std::holds_alternative<std::uint64_t>(value))
    {
      return DataValue::Number(WholeNumber(value));
    }
    if (const auto *vector = std::get_if<glm::vec2>(&value)) { return ListOf(std::vector{vector->x, vector->y}); }
    if (const auto *vector = std::get_if<glm::vec4>(&value))
    {
      return ListOf(std::vector{vector->x, vector->y, vector->z, vector->w});
    }
    if (const auto *vector = std::get_if<glm::ivec2>(&value))
    {
      auto list = DataValue::List();
      list.Add(DataValue::Number(vector->x));
      list.Add(DataValue::Number(vector->y));
      return list;
    }
    if (const auto *vector = std::get_if<glm::ivec3>(&value))
    {
      auto list = DataValue::List();
      list.Add(DataValue::Number(vector->x));
      list.Add(DataValue::Number(vector->y));
      list.Add(DataValue::Number(vector->z));
      return list;
    }
    if (const auto *wholes = std::get_if<std::vector<int>>(&value))
    {
      auto list = DataValue::List();
      for (const int number : *wholes) { list.Add(DataValue::Number(number)); }
      return list;
    }
    if (const auto *vectors = std::get_if<std::vector<glm::vec3>>(&value))
    {
      auto list = DataValue::List();
      for (const auto &vector : *vectors) { list.Add(ListOf(vector)); }
      return list;
    }
    if (const auto *quaternion = std::get_if<glm::quat>(&value))
    {
      return ListOf(std::vector{quaternion->x, quaternion->y, quaternion->z, quaternion->w});
    }
    if (const auto *matrix = std::get_if<glm::mat3>(&value))
    {
      auto rows = DataValue::List();
      for (int row = 0; row < 3; row++)
      {
        rows.Add(ListOf(std::vector{(*matrix)[0][row], (*matrix)[1][row], (*matrix)[2][row]}));
      }
      return rows;
    }
    if (const auto *matrix = std::get_if<glm::mat4>(&value))
    {
      auto rows = DataValue::List();
      for (int row = 0; row < 4; row++)
      {
        rows.Add(ListOf(std::vector{(*matrix)[0][row], (*matrix)[1][row], (*matrix)[2][row], (*matrix)[3][row]}));
      }
      return rows;
    }

    return {};
  }

  void WriteFields(const TypeInfo &type, const void *object, const void *standard, DataValue &map)
  {
    for (const auto &field : type.fields)
    {
      // what does not belong would be refused when the map is read
      if (!type.Belongs(field, object)) { continue; }

      if (field.kind == FieldKind::Group)
      {
        TypeInfo fields;
        fields.fields = field.fields;

        auto group = DataValue::Map();
        WriteFields(fields, object, standard, group);

        if (!group.GetEntries().empty() || field.always_written) { map.Set(field.name, group); }
        continue;
      }

      // A field that may be left out is written when it is given, unless
      // the standard gives the same, as the prefab of an entity does
      const auto value = field.get(object);
      if (field.given)
      {
        if (!field.given(object)) { continue; }
        if (field.given(standard) && Same(value, field.get(standard))) { continue; }
      } else if (!field.always_written && Same(value, field.get(standard))) { continue; }

      map.Set(field.name, ToDataValue(value));
    }
  }
} // neon
