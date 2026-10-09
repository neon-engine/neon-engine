#include "field-text.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>

namespace neon
{
  // Helpers of field-text.cpp, for this file alone.
  namespace
  {
    std::string Trimmed(const std::string &text)
    {
      std::size_t first = 0;
      std::size_t last = text.size();

      while (first < last && std::isspace(static_cast<unsigned char>(text[first])) != 0) { first++; }
      while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1])) != 0) { last--; }

      return text.substr(first, last - first);
    }

    std::string Lowered(std::string text)
    {
      std::ranges::transform(text, text.begin(), [](const unsigned char letter)
      {
        return static_cast<char>(std::tolower(letter));
      });
      return text;
    }

    // Read by hand. The functions of the standard library follow the
    // language of the machine, where the dot may be a comma.
    bool ParseNumber(const std::string &written, double &number)
    {
      const std::string text = Trimmed(written);
      std::size_t position = 0;
      bool negative = false;

      if (position < text.size() && (text[position] == '-' || text[position] == '+'))
      {
        negative = text[position] == '-';
        position++;
      }

      // all the digits as one whole number, divided once at the end, so
      // that the number is exact as far as the 15 digits a double keeps
      double value = 0.0;
      int fraction_digits = 0;
      std::size_t digits = 0;
      bool after_point = false;

      for (; position < text.size(); position++)
      {
        const char letter = text[position];

        if (letter == '.' && !after_point)
        {
          after_point = true;
        } else if (letter >= '0' && letter <= '9')
        {
          digits++;
          value = value * 10.0 + (letter - '0');
          if (after_point) { fraction_digits++; }
        } else
        {
          return false;
        }
      }

      if (digits == 0) { return false; }

      value /= std::pow(10.0, fraction_digits);
      number = negative ? -value : value;
      return true;
    }

    bool ParseNumber(const std::string &written, float &number)
    {
      double precise = 0.0;
      if (!ParseNumber(written, precise)) { return false; }

      number = static_cast<float>(precise);
      return true;
    }

    std::string FormatNumber(const float number)
    {
      const float rounded = std::round(number * 10000.0f) / 10000.0f;
      if (rounded == 0.0f) { return "0"; }

      return std::format("{}", rounded);
    }

    /// A precise number keeps every digit, so that it reads back as the same.
    std::string FormatNumber(const double number)
    {
      return std::format("{}", number);
    }

    /// What is set apart by spaces and commas.
    std::vector<std::string> Words(const std::string &text)
    {
      std::vector<std::string> words;
      std::string word;

      for (const char letter : text + " ")
      {
        if (std::isspace(static_cast<unsigned char>(letter)) != 0 || letter == ',')
        {
          if (!word.empty()) { words.push_back(word); }
          word.clear();
        } else
        {
          word += letter;
        }
      }

      return words;
    }

    int HexDigit(const char digit)
    {
      if (digit >= '0' && digit <= '9') { return digit - '0'; }
      if (digit >= 'a' && digit <= 'f') { return digit - 'a' + 10; }
      if (digit >= 'A' && digit <= 'F') { return digit - 'A' + 10; }
      return -1;
    }
  }

  std::string FormatFieldLength(const FieldLength &length)
  {
    if (length.is_auto) { return "auto"; }

    if (length.percent == 0.0f) { return FormatNumber(length.pixels) + "px"; }
    if (length.pixels == 0.0f) { return FormatNumber(length.percent) + "%"; }

    return "calc(" + FormatNumber(length.percent) + "% + " + FormatNumber(length.pixels) + "px)";
  }

  bool ParseFieldLength(const std::string &written, FieldLength &length)
  {
    const std::string text = Lowered(Trimmed(written));
    if (text.empty()) { return false; }

    if (text == "auto")
    {
      length = FieldLength::Auto();
      return true;
    }

    float number = 0.0f;

    if (text.starts_with("calc(") && text.back() == ')')
    {
      const std::string inside = text.substr(5, text.size() - 6);
      const std::size_t plus = inside.find(" + ");
      if (plus == std::string::npos) { return false; }

      const std::string percent = Trimmed(inside.substr(0, plus));
      const std::string pixels = Trimmed(inside.substr(plus + 3));

      float percent_number = 0.0f;
      float pixel_number = 0.0f;

      if (percent.empty() || percent.back() != '%' || !pixels.ends_with("px") ||
          !ParseNumber(percent.substr(0, percent.size() - 1), percent_number) ||
          !ParseNumber(pixels.substr(0, pixels.size() - 2), pixel_number))
      {
        return false;
      }

      length = FieldLength::Sum(pixel_number, percent_number);
      return true;
    }

    if (text.back() == '%')
    {
      if (!ParseNumber(text.substr(0, text.size() - 1), number)) { return false; }

      length = FieldLength::Percent(number);
      return true;
    }

    const std::string digits = text.ends_with("px") ? text.substr(0, text.size() - 2) : text;
    if (digits.empty() || std::isspace(static_cast<unsigned char>(digits.back())) != 0) { return false; }
    if (!ParseNumber(digits, number)) { return false; }

    length = FieldLength::Pixels(number);
    return true;
  }

  std::string FormatFieldColor(const Color &color)
  {
    const auto byte = [](const float part)
    {
      return static_cast<int>(std::lround(std::clamp(part, 0.0f, 1.0f) * 255.0f));
    };

    if (byte(color.a) == 255) { return std::format("#{:02x}{:02x}{:02x}", byte(color.r), byte(color.g), byte(color.b)); }

    return std::format("#{:02x}{:02x}{:02x}{:02x}", byte(color.r), byte(color.g), byte(color.b), byte(color.a));
  }

  bool ParseFieldColor(const std::string &written, Color &color)
  {
    const std::string text = Lowered(Trimmed(written));
    if (text.empty()) { return false; }

    if (text.front() == '#')
    {
      const std::string digits = text.substr(1);
      if (digits.size() != 3 && digits.size() != 4 && digits.size() != 6 && digits.size() != 8) { return false; }

      for (const char digit : digits)
      {
        if (HexDigit(digit) < 0) { return false; }
      }

      const bool is_short = digits.size() <= 4;
      const std::size_t channels = is_short ? digits.size() : digits.size() / 2;
      float values[4] = {0.0f, 0.0f, 0.0f, 1.0f};

      for (std::size_t i = 0; i < channels; i++)
      {
        const int value = is_short
          ? HexDigit(digits[i]) * 17
          : HexDigit(digits[i * 2]) * 16 + HexDigit(digits[i * 2 + 1]);

        values[i] = static_cast<float>(value) / 255.0f;
      }

      color = {values[0], values[1], values[2], values[3]};
      return true;
    }

    for (const std::string name : {"rgba(", "rgb("})
    {
      if (!text.starts_with(name) || text.back() != ')') { continue; }

      std::string inside = text.substr(name.size(), text.size() - name.size() - 1);
      std::ranges::replace(inside, '/', ' ');

      const auto parts = Words(inside);
      if (parts.size() != 3 && parts.size() != 4) { return false; }

      float values[4] = {0.0f, 0.0f, 0.0f, 1.0f};

      for (std::size_t i = 0; i < parts.size(); i++)
      {
        std::string part = parts[i];
        const bool is_percent = part.back() == '%';
        if (is_percent) { part.pop_back(); }

        float number = 0.0f;
        if (!ParseNumber(part, number)) { return false; }

        // the colors count to 255, and alpha to 1
        const float whole = is_percent ? 100.0f : (i < 3 ? 255.0f : 1.0f);
        values[i] = std::clamp(number / whole, 0.0f, 1.0f);
      }

      color = {values[0], values[1], values[2], values[3]};
      return true;
    }

    return false;
  }

  std::string FormatField(const FieldValue &value)
  {
    if (const auto *flag = std::get_if<bool>(&value)) { return *flag ? "true" : "false"; }
    if (const auto *whole = std::get_if<int>(&value)) { return std::format("{}", *whole); }
    if (const auto *number = std::get_if<float>(&value)) { return FormatNumber(*number); }
    if (const auto *precise = std::get_if<double>(&value)) { return FormatNumber(*precise); }
    if (const auto *text = std::get_if<std::string>(&value)) { return *text; }

    if (const auto *vector = std::get_if<glm::vec3>(&value))
    {
      return FormatNumber(vector->x) + " " + FormatNumber(vector->y) + " " + FormatNumber(vector->z);
    }

    if (const auto *color = std::get_if<Color>(&value)) { return FormatFieldColor(*color); }

    if (const auto *texts = std::get_if<std::vector<std::string>>(&value))
    {
      std::string joined;
      for (const auto &text : *texts)
      {
        if (!joined.empty()) { joined += ", "; }
        joined += text;
      }
      return joined;
    }

    if (const auto *length = std::get_if<FieldLength>(&value)) { return FormatFieldLength(*length); }

    if (const auto *numbers = std::get_if<std::vector<float>>(&value))
    {
      std::string joined;
      for (const float number : *numbers)
      {
        if (!joined.empty()) { joined += ' '; }
        joined += FormatNumber(number);
      }
      return joined;
    }

    if (const auto *character = std::get_if<char>(&value)) { return std::string(1, *character); }
    if (std::holds_alternative<std::uint8_t>(value) || std::holds_alternative<std::int16_t>(value)
        || std::holds_alternative<std::uint16_t>(value) || std::holds_alternative<std::uint32_t>(value)
        || std::holds_alternative<std::int64_t>(value) || std::holds_alternative<std::uint64_t>(value))
    {
      return std::format("{:.0f}", WholeNumber(value));
    }

    if (const auto *vector = std::get_if<glm::vec2>(&value))
    {
      return FormatNumber(vector->x) + " " + FormatNumber(vector->y);
    }
    if (const auto *vector = std::get_if<glm::vec4>(&value))
    {
      return FormatNumber(vector->x) + " " + FormatNumber(vector->y) + " " + FormatNumber(vector->z) + " "
             + FormatNumber(vector->w);
    }
    if (const auto *vector = std::get_if<glm::ivec2>(&value)) { return std::format("{} {}", vector->x, vector->y); }
    if (const auto *vector = std::get_if<glm::ivec3>(&value))
    {
      return std::format("{} {} {}", vector->x, vector->y, vector->z);
    }
    if (const auto *wholes = std::get_if<std::vector<int>>(&value))
    {
      std::string joined;
      for (const int number : *wholes)
      {
        if (!joined.empty()) { joined += ' '; }
        joined += std::format("{}", number);
      }
      return joined;
    }
    if (const auto *quaternion = std::get_if<glm::quat>(&value))
    {
      return FormatNumber(quaternion->x) + " " + FormatNumber(quaternion->y) + " " + FormatNumber(quaternion->z) + " "
             + FormatNumber(quaternion->w);
    }
    if (const auto *matrix = std::get_if<glm::mat3>(&value))
    {
      // rows apart by commas; glm keeps columns, so a row is read across
      std::string joined;
      for (int row = 0; row < 3; row++)
      {
        if (!joined.empty()) { joined += ", "; }
        for (int column = 0; column < 3; column++)
        {
          if (column > 0) { joined += ' '; }
          joined += FormatNumber((*matrix)[column][row]);
        }
      }
      return joined;
    }
    if (const auto *matrix = std::get_if<glm::mat4>(&value))
    {
      std::string joined;
      for (int row = 0; row < 4; row++)
      {
        if (!joined.empty()) { joined += ", "; }
        for (int column = 0; column < 4; column++)
        {
          if (column > 0) { joined += ' '; }
          joined += FormatNumber((*matrix)[column][row]);
        }
      }
      return joined;
    }
    if (const auto *vectors = std::get_if<std::vector<glm::vec3>>(&value))
    {
      // one vector a line, as a list of lists has no other flat form
      std::string joined;
      for (const auto &vector : *vectors)
      {
        if (!joined.empty()) { joined += ", "; }
        joined += FormatNumber(vector.x) + " " + FormatNumber(vector.y) + " " + FormatNumber(vector.z);
      }
      return joined;
    }

    return "";
  }

  bool ParseField(
    const std::string &text,
    const FieldInfo &field,
    const std::string &what,
    FieldValue &value,
    std::string &error)
  {
    const auto refuse = [&](const std::string &expected)
    {
      error = std::format("{} is '{}', where {} was expected", what, text, expected);
      return false;
    };

    switch (field.kind)
    {
      case FieldKind::Boolean:
      {
        const std::string word = Lowered(Trimmed(text));
        if (word != "true" && word != "false") { return refuse("true or false"); }

        value = word == "true";
        return true;
      }

      case FieldKind::Integer:
      {
        float number = 0.0f;
        if (!ParseNumber(text, number) || number != std::round(number) || std::abs(number) > 2.0e9f)
        {
          return refuse("a whole number");
        }

        value = static_cast<int>(number);
        return true;
      }

      case FieldKind::Float:
      {
        float number = 0.0f;
        if (!ParseNumber(text, number)) { return refuse("a number"); }

        value = number;
        return true;
      }

      case FieldKind::Double:
      {
        double number = 0.0;
        if (!ParseNumber(text, number)) { return refuse("a number"); }

        value = number;
        return true;
      }

      case FieldKind::String:
      case FieldKind::Choice:
        // which words a choice has is looked at by Check()
        value = field.kind == FieldKind::Choice ? Trimmed(text) : text;
        return true;

      case FieldKind::Vector3:
      {
        const auto words = Words(text);
        glm::vec3 vector{0.0f};

        if (words.size() == 1 && field.one_number_for_all && ParseNumber(words[0], vector.x))
        {
          value = glm::vec3{vector.x};
          return true;
        }

        if (words.size() != 3 || !ParseNumber(words[0], vector.x) || !ParseNumber(words[1], vector.y) ||
            !ParseNumber(words[2], vector.z))
        {
          return refuse("three numbers, such as 1 2 3");
        }

        value = vector;
        return true;
      }

      case FieldKind::Color:
      {
        Color color;
        if (!ParseFieldColor(text, color)) { return refuse("a color such as #ff8000 or rgb(255, 128, 0)"); }

        value = color;
        return true;
      }

      case FieldKind::StringList:
      {
        std::vector<std::string> texts;
        std::string each;

        for (const char letter : text)
        {
          if (letter == ',')
          {
            texts.push_back(Trimmed(each));
            each.clear();
          } else
          {
            each += letter;
          }
        }

        if (!Trimmed(each).empty() || !texts.empty()) { texts.push_back(Trimmed(each)); }

        value = texts;
        return true;
      }

      case FieldKind::Length:
      {
        FieldLength length;
        if (!ParseFieldLength(text, length)) { return refuse("a length such as 12px, 50%, or auto"); }

        value = length;
        return true;
      }

      case FieldKind::FloatList:
      {
        std::vector<float> numbers;

        for (const auto &word : Words(text))
        {
          float number = 0.0f;
          if (!ParseNumber(word, number)) { return refuse("numbers, such as 1 2 3 4"); }

          numbers.push_back(number);
        }

        value = numbers;
        return true;
      }

      case FieldKind::Layers:
      {
        std::vector<float> layers;

        for (const auto &word : Words(text))
        {
          float layer = 0.0f;
          if (!ParseNumber(word, layer)) { return refuse("the numbers of layers, such as 1 3"); }

          layers.push_back(layer);
        }

        // which numbers are layers is looked at by Check()
        value = layers;
        return true;
      }

      case FieldKind::Group:
        error = std::format("{} is a group and holds no value of its own", what);
        return false;

      case FieldKind::Byte:
      case FieldKind::Short:
      case FieldKind::UnsignedShort:
      case FieldKind::UnsignedInteger:
      case FieldKind::Long:
      case FieldKind::UnsignedLong:
      {
        double number = 0.0;
        double least = 0.0;
        double most = 0.0;
        RangeOfWholeKind(field.kind, least, most);
        if (!ParseNumber(text, number) || number != std::round(number) || number < least || number > most)
        {
          return refuse(Describe(field.kind));
        }
        value = WholeValue(field.kind, number);
        return true;
      }

      case FieldKind::Char:
      {
        const std::string trimmed = Trimmed(text);
        if (trimmed.size() != 1) { return refuse("one character"); }
        value = trimmed.front();
        return true;
      }

      case FieldKind::Vector2:
      {
        const auto words = Words(text);
        glm::vec2 vector{0.0f};
        if (words.size() != 2 || !ParseNumber(words[0], vector.x) || !ParseNumber(words[1], vector.y))
        {
          return refuse("two numbers, such as 1 2");
        }
        value = vector;
        return true;
      }

      case FieldKind::Vector4:
      {
        const auto words = Words(text);
        glm::vec4 vector{0.0f};
        if (words.size() != 4 || !ParseNumber(words[0], vector.x) || !ParseNumber(words[1], vector.y)
            || !ParseNumber(words[2], vector.z) || !ParseNumber(words[3], vector.w))
        {
          return refuse("four numbers, such as 1 2 3 4");
        }
        value = vector;
        return true;
      }

      case FieldKind::IntegerVector2:
      case FieldKind::IntegerVector3:
      case FieldKind::IntegerList:
      {
        const auto words = Words(text);
        const std::size_t wanted = field.kind == FieldKind::IntegerVector2 ? 2 : field.kind == FieldKind::IntegerVector3 ? 3 : 0;
        if (wanted != 0 && words.size() != wanted)
        {
          return refuse(std::format("{} whole numbers", wanted));
        }

        std::vector<int> wholes;
        for (const auto &word : words)
        {
          float number = 0.0f;
          if (!ParseNumber(word, number) || number != std::round(number) || std::abs(number) > 2.0e9f)
          {
            return refuse("whole numbers, such as 1 2 3");
          }
          wholes.push_back(static_cast<int>(number));
        }

        if (field.kind == FieldKind::IntegerVector2) { value = glm::ivec2{wholes[0], wholes[1]}; }
        else if (field.kind == FieldKind::IntegerVector3) { value = glm::ivec3{wholes[0], wholes[1], wholes[2]}; }
        else { value = wholes; }
        return true;
      }

      case FieldKind::Quaternion:
      {
        const auto words = Words(text);
        glm::quat quaternion{1.0f, 0.0f, 0.0f, 0.0f};
        if (words.size() != 4 || !ParseNumber(words[0], quaternion.x) || !ParseNumber(words[1], quaternion.y)
            || !ParseNumber(words[2], quaternion.z) || !ParseNumber(words[3], quaternion.w))
        {
          return refuse("a quaternion of four numbers x y z w, such as 0 0 0 1");
        }
        value = quaternion;
        return true;
      }

      case FieldKind::Matrix3:
      case FieldKind::Matrix4:
      {
        // rows apart by commas, their numbers by spaces
        const int size = field.kind == FieldKind::Matrix3 ? 3 : 4;
        glm::mat4 matrix{1.0f};
        int row = 0;
        std::string each;
        for (const char letter : text + ",")
        {
          if (letter != ',')
          {
            each += letter;
            continue;
          }
          const auto words = Words(each);
          each.clear();
          if (words.empty()) { continue; }
          if (row >= size || static_cast<int>(words.size()) != size)
          {
            return refuse(std::format("{} rows of {} numbers apart by commas", size, size));
          }
          for (int column = 0; column < size; column++)
          {
            if (!ParseNumber(words[static_cast<std::size_t>(column)], matrix[column][row]))
            {
              return refuse(std::format("{} rows of {} numbers apart by commas", size, size));
            }
          }
          row++;
        }
        if (row != size) { return refuse(std::format("{} rows of {} numbers apart by commas", size, size)); }

        if (size == 3) { value = glm::mat3(matrix); }
        else { value = matrix; }
        return true;
      }

      case FieldKind::Vector3List:
      {
        // vectors apart by commas, their numbers by spaces
        std::vector<glm::vec3> vectors;
        std::string each;
        for (const char letter : text + ",")
        {
          if (letter != ',')
          {
            each += letter;
            continue;
          }
          const auto words = Words(each);
          each.clear();
          if (words.empty()) { continue; }

          glm::vec3 vector{0.0f};
          if (words.size() != 3 || !ParseNumber(words[0], vector.x) || !ParseNumber(words[1], vector.y)
              || !ParseNumber(words[2], vector.z))
          {
            return refuse("vectors of three numbers apart by commas, such as 1 2 3, 4 5 6");
          }
          vectors.push_back(vector);
        }
        value = vectors;
        return true;
      }
    }

    return false;
  }
} // neon
