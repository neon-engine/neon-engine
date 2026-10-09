#include "css-expression.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <vector>

#include <neon/ui/css-values.hpp>

namespace neon
{
  // Helpers of CssValueContext, for this file alone.
  namespace
  {
    constexpr int max_depth = 32;

    bool IsSpace(const char letter)
    {
      return letter == ' ' || letter == '\t' || letter == '\n' || letter == '\r' || letter == '\f';
    }

    bool IsDigit(const char letter)
    {
      return letter >= '0' && letter <= '9';
    }

    bool IsNamePart(const char letter)
    {
      const auto byte = static_cast<unsigned char>(letter);
      return std::isalnum(byte) != 0 || letter == '-' || letter == '_' || byte >= 0x80;
    }

    std::string Trimmed(const std::string &text)
    {
      std::size_t first = 0;
      std::size_t last = text.size();

      while (first < last && IsSpace(text[first])) { first++; }
      while (last > first && IsSpace(text[last - 1])) { last--; }

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

    /// The place behind the bracket that closes the one at `open`, or
    /// npos.
    std::size_t FindClose(const std::string &text, const std::size_t open)
    {
      int depth = 0;

      for (std::size_t i = open; i < text.size(); i++)
      {
        const char letter = text[i];

        if (letter == '"' || letter == '\'')
        {
          i++;
          while (i < text.size() && text[i] != letter)
          {
            if (text[i] == '\\') { i++; }
            i++;
          }
          continue;
        }

        if (letter == '(') { depth++; }
        if (letter == ')')
        {
          depth--;
          if (depth == 0) { return i + 1; }
        }
      }

      return std::string::npos;
    }

    /// Whether `name(` starts at the place, as a word of its own.
    bool StartsFunction(const std::string &text, const std::size_t at, const std::string &name)
    {
      if (at + name.size() + 1 > text.size()) { return false; }
      if (at > 0 && IsNamePart(text[at - 1])) { return false; }

      for (std::size_t i = 0; i < name.size(); i++)
      {
        if (std::tolower(static_cast<unsigned char>(text[at + i])) != name[i]) { return false; }
      }

      return text[at + name.size()] == '(';
    }

    /// Puts what the custom properties hold in the place of every var().
    bool Substitute(
      const std::string &value,
      const CssValueContext &context,
      std::string &result,
      std::string &error,
      CssValueUses *uses,
      const int depth)
    {
      if (depth > max_depth)
      {
        error = "custom properties refer to each other in a circle";
        return false;
      }

      result.clear();

      for (std::size_t i = 0; i < value.size();)
      {
        const char letter = value[i];

        if (letter == '"' || letter == '\'')
        {
          const std::size_t start = i;
          i++;
          while (i < value.size() && value[i] != letter)
          {
            if (value[i] == '\\') { i++; }
            i++;
          }
          i = std::min(i + 1, value.size());
          result += value.substr(start, i - start);
          continue;
        }

        if (!StartsFunction(value, i, "var"))
        {
          result += letter;
          i++;
          continue;
        }

        const std::size_t close = FindClose(value, i + 3);
        if (close == std::string::npos)
        {
          error = "var( is not closed";
          return false;
        }

        if (uses != nullptr) { uses->variables = true; }

        const std::string inside = value.substr(i + 4, close - i - 5);
        i = close;

        // the name, and behind the first comma what stands in when the
        // property is not set
        std::size_t comma = std::string::npos;
        int brackets = 0;
        for (std::size_t k = 0; k < inside.size() && comma == std::string::npos; k++)
        {
          if (inside[k] == '(') { brackets++; }
          if (inside[k] == ')') { brackets--; }
          if (inside[k] == ',' && brackets == 0) { comma = k; }
        }

        const std::string name = Trimmed(inside.substr(0, comma));
        if (!name.starts_with("--") || name.size() < 3)
        {
          error = std::format(
            "var() holds '{}', where the name of a custom property was expected, such as --accent", name);
          return false;
        }

        std::string held;
        if (context.variable && context.variable(name, held))
        {
          std::string inner;
          if (!Substitute(held, context, inner, error, uses, depth + 1)) { return false; }

          result += Trimmed(inner);
          continue;
        }

        if (comma == std::string::npos)
        {
          error = std::format("the custom property '{}' is not set, and var() names nothing in its place", name);
          return false;
        }

        std::string fallback;
        if (!Substitute(inside.substr(comma + 1), context, fallback, error, uses, depth + 1)) { return false; }

        result += Trimmed(fallback);
      }

      return true;
    }

    /// Pixels for one of a unit, or not a number for a unit that is not
    /// one of a length.
    float PixelsOf(const std::string &unit, const CssValueContext &context, CssValueUses *uses)
    {
      if (unit == "px") { return 1.0f; }

      if (unit == "em")
      {
        if (uses != nullptr) { uses->font_size = true; }
        return context.font_size;
      }

      if (unit == "rem")
      {
        if (uses != nullptr) { uses->root_font_size = true; }
        return context.root_font_size;
      }

      if (unit == "vw" || unit == "vh" || unit == "vmin" || unit == "vmax")
      {
        if (uses != nullptr) { uses->viewport = true; }

        if (unit == "vw") { return context.viewport_width / 100.0f; }
        if (unit == "vh") { return context.viewport_height / 100.0f; }
        if (unit == "vmin") { return std::min(context.viewport_width, context.viewport_height) / 100.0f; }
        return std::max(context.viewport_width, context.viewport_height) / 100.0f;
      }

      // the lengths of paper, which CSS ties to the pixel
      if (unit == "in") { return 96.0f; }
      if (unit == "pt") { return 96.0f / 72.0f; }
      if (unit == "pc") { return 16.0f; }
      if (unit == "cm") { return 96.0f / 2.54f; }
      if (unit == "mm") { return 96.0f / 25.4f; }
      if (unit == "q") { return 96.0f / 101.6f; }

      return std::numeric_limits<float>::quiet_NaN();
    }

    /// A number of a unit in pixels. A hundredth of the viewport is
    /// divided by last, so that half of 1920 is 960 and nothing next to
    /// it.
    float InPixels(const float number, const std::string &unit, const CssValueContext &context, const float factor)
    {
      if (unit == "vw") { return number * context.viewport_width / 100.0f; }
      if (unit == "vh") { return number * context.viewport_height / 100.0f; }
      if (unit == "vmin") { return number * std::min(context.viewport_width, context.viewport_height) / 100.0f; }
      if (unit == "vmax") { return number * std::max(context.viewport_width, context.viewport_height) / 100.0f; }

      return number * factor;
    }

    /// A number with its unit, read from the place. Returns false when
    /// there is none.
    bool ReadDimension(const std::string &text, std::size_t &at, float &number, std::string &unit)
    {
      std::size_t position = at;

      if (position < text.size() && (text[position] == '+' || text[position] == '-')) { position++; }

      const std::size_t digits_start = position;
      while (position < text.size() && IsDigit(text[position])) { position++; }

      if (position < text.size() && text[position] == '.' && position + 1 < text.size() &&
          IsDigit(text[position + 1]))
      {
        position++;
        while (position < text.size() && IsDigit(text[position])) { position++; }
      }

      if (position == digits_start) { return false; }

      if (!ParseCssNumber(text.substr(at, position - at), number)) { return false; }

      std::size_t unit_end = position;
      if (unit_end < text.size() && text[unit_end] == '%')
      {
        unit_end++;
      } else
      {
        while (unit_end < text.size() && std::isalpha(static_cast<unsigned char>(text[unit_end])) != 0)
        {
          unit_end++;
        }
      }

      unit = Lowered(text.substr(position, unit_end - position));
      at = unit_end;
      return true;
    }

    /// What a part of calc() comes to: a number, or a length that is
    /// pixels and a percentage added up.
    struct Amount
    {
      bool is_length = false;
      float number = 0.0f;
      float pixels = 0.0f;
      float percent = 0.0f;
    };

    class Calculator
    {
      const std::string &_text;
      const CssValueContext &_context;
      CssValueUses *_uses;
      std::size_t _position = 0;
      std::string _error;

      bool Fail(const std::string &error)
      {
        if (_error.empty()) { _error = error; }
        return false;
      }

      bool SkipSpaces()
      {
        const std::size_t before = _position;
        while (_position < _text.size() && IsSpace(_text[_position])) { _position++; }
        return _position > before;
      }

      bool ReadUnit(Amount &amount)
      {
        SkipSpaces();

        if (_position >= _text.size()) { return Fail("calc() ends where a value was expected"); }

        if (_text[_position] == '(')
        {
          _position++;
          if (!ReadSum(amount)) { return false; }

          SkipSpaces();
          if (_position >= _text.size() || _text[_position] != ')') { return Fail("a bracket of calc() is not closed"); }

          _position++;
          return true;
        }

        if (StartsFunction(_text, _position, "calc"))
        {
          _position += 4;
          return ReadUnit(amount);
        }

        float number = 0.0f;
        std::string unit;
        const std::size_t start = _position;

        if (!ReadDimension(_text, _position, number, unit))
        {
          std::size_t end = start;
          while (end < _text.size() && !IsSpace(_text[end]) && _text[end] != ')') { end++; }

          return Fail(std::format(
            "calc() holds '{}', where a number, a length, or a percentage was expected",
            _text.substr(start, std::max<std::size_t>(end - start, 1))));
        }

        if (unit.empty())
        {
          amount = {false, number, 0.0f, 0.0f};
          return true;
        }

        if (unit == "%")
        {
          amount = {true, 0.0f, 0.0f, number};
          return true;
        }

        const float pixels = PixelsOf(unit, _context, _uses);
        if (std::isnan(pixels))
        {
          return Fail(std::format(
            "calc() holds the unit '{}', where px, %, em, rem, vw, vh, vmin, or vmax was expected", unit));
        }

        amount = {true, 0.0f, InPixels(number, unit, _context, pixels), 0.0f};
        return true;
      }

      bool ReadProduct(Amount &amount)
      {
        if (!ReadUnit(amount)) { return false; }

        while (true)
        {
          SkipSpaces();
          if (_position >= _text.size()) { return true; }

          const char sign = _text[_position];
          if (sign != '*' && sign != '/') { return true; }

          _position++;

          Amount other;
          if (!ReadUnit(other)) { return false; }

          if (sign == '*')
          {
            if (amount.is_length && other.is_length)
            {
              return Fail("calc() multiplies two lengths, where one of the two has to be a number");
            }

            if (other.is_length) { std::swap(amount, other); }

            // `other` is a number by now
            if (amount.is_length)
            {
              amount.pixels *= other.number;
              amount.percent *= other.number;
            } else
            {
              amount.number *= other.number;
            }
          } else
          {
            if (other.is_length) { return Fail("calc() divides by a length, where a number was expected"); }
            if (other.number == 0.0f) { return Fail("calc() divides by 0"); }

            if (amount.is_length)
            {
              amount.pixels /= other.number;
              amount.percent /= other.number;
            } else
            {
              amount.number /= other.number;
            }
          }
        }
      }

    public:
      Calculator(const std::string &text, const CssValueContext &context, CssValueUses *uses)
        : _text(text), _context(context)
      {
        _uses = uses;
      }

      [[nodiscard]] const std::string &GetError() const
      {
        return _error;
      }

      bool ReadSum(Amount &amount)
      {
        if (!ReadProduct(amount)) { return false; }

        while (true)
        {
          SkipSpaces();
          if (_position >= _text.size()) { return true; }

          const char sign = _text[_position];
          if (sign != '+' && sign != '-') { return true; }

          const bool space_before = _position > 0 && IsSpace(_text[_position - 1]);

          const bool space_after = _position + 1 < _text.size() && IsSpace(_text[_position + 1]);
          if (!space_before || !space_after)
          {
            return Fail(std::format(
              "calc() has '{}' without a space on both sides, which CSS asks for", sign));
          }

          _position++;

          Amount other;
          if (!ReadProduct(other)) { return false; }

          if (amount.is_length != other.is_length)
          {
            return Fail(std::format(
              "calc() {} a number and a length, where two of a kind were expected",
              sign == '+' ? "adds" : "subtracts"));
          }

          const float factor = sign == '+' ? 1.0f : -1.0f;
          amount.number += factor * other.number;
          amount.pixels += factor * other.pixels;
          amount.percent += factor * other.percent;
        }
      }

      bool ReadAll(Amount &amount)
      {
        if (Trimmed(_text).empty()) { return Fail("calc() holds nothing"); }
        if (!ReadSum(amount)) { return false; }

        SkipSpaces();
        if (_position < _text.size())
        {
          return Fail(std::format(
            "calc() holds '{}', where +, -, *, / or its end was expected", _text.substr(_position)));
        }
        return true;
      }
    };

    std::string Written(const Amount &amount)
    {
      if (!amount.is_length) { return FormatCssNumber(amount.number); }

      if (amount.percent == 0.0f) { return FormatCssNumber(amount.pixels) + "px"; }
      if (amount.pixels == 0.0f) { return FormatCssNumber(amount.percent) + "%"; }

      return "calc(" + FormatCssNumber(amount.percent) + "% + " + FormatCssNumber(amount.pixels) + "px)";
    }

    /// Works out the units and calc() of a value that holds no var() any
    /// more.
    bool ResolveUnits(
      const std::string &value,
      const CssValueContext &context,
      std::string &result,
      std::string &error,
      CssValueUses *uses)
    {
      result.clear();

      for (std::size_t i = 0; i < value.size();)
      {
        const char letter = value[i];

        if (letter == '"' || letter == '\'')
        {
          const std::size_t start = i;
          i++;
          while (i < value.size() && value[i] != letter)
          {
            if (value[i] == '\\') { i++; }
            i++;
          }
          i = std::min(i + 1, value.size());
          result += value.substr(start, i - start);
          continue;
        }

        if (StartsFunction(value, i, "calc"))
        {
          const std::size_t close = FindClose(value, i + 4);
          if (close == std::string::npos)
          {
            error = "calc( is not closed";
            return false;
          }

          const std::string inside = value.substr(i + 5, close - i - 6);

          Calculator calculator(inside, context, uses);
          Amount amount;
          if (!calculator.ReadAll(amount))
          {
            error = calculator.GetError();
            return false;
          }

          result += Written(amount);
          i = close;
          continue;
        }

        if (StartsFunction(value, i, "url"))
        {
          // a path is left as it is, whatever it looks like
          const std::size_t close = FindClose(value, i + 3);
          const std::size_t end = close == std::string::npos ? value.size() : close;
          result += value.substr(i, end - i);
          i = end;
          continue;
        }

        // a color with a hash, and a word, are left as they are
        if (letter == '#' || std::isalpha(static_cast<unsigned char>(letter)) != 0 || letter == '_')
        {
          while (i < value.size() && (IsNamePart(value[i]) || value[i] == '#'))
          {
            result += value[i];
            i++;
          }
          continue;
        }

        const bool starts_number =
          IsDigit(letter) ||
          ((letter == '.' || letter == '-' || letter == '+') && i + 1 < value.size() &&
           (IsDigit(value[i + 1]) || (value[i + 1] == '.' && i + 2 < value.size() && IsDigit(value[i + 2]))));

        // a hyphen in front of a word belongs to the word
        if (!starts_number)
        {
          if (letter == '-' && i + 1 < value.size() && IsNamePart(value[i + 1]))
          {
            while (i < value.size() && IsNamePart(value[i]))
            {
              result += value[i];
              i++;
            }
            continue;
          }

          result += letter;
          i++;
          continue;
        }

        float number = 0.0f;
        std::string unit;
        const std::size_t start = i;

        if (!ReadDimension(value, i, number, unit))
        {
          result += letter;
          i = start + 1;
          continue;
        }

        const float pixels = unit.empty() || unit == "%" || unit == "px"
          ? std::numeric_limits<float>::quiet_NaN()
          : PixelsOf(unit, context, uses);

        if (std::isnan(pixels))
        {
          // pixels, percentages, numbers, and the units of other things
          // than lengths, such as seconds
          result += value.substr(start, i - start);
        } else
        {
          result += FormatCssNumber(InPixels(number, unit, context, pixels)) + "px";
        }
      }

      return true;
    }

    struct NamedColor
    {
      const char *name;
      unsigned int value;
    };

    // https://www.w3.org/TR/css-color-4/#named-colors
    constexpr NamedColor named_colors[] = {
      {"aliceblue", 0xf0f8ff}, {"antiquewhite", 0xfaebd7}, {"aqua", 0x00ffff}, {"aquamarine", 0x7fffd4},
      {"azure", 0xf0ffff}, {"beige", 0xf5f5dc}, {"bisque", 0xffe4c4}, {"black", 0x000000},
      {"blanchedalmond", 0xffebcd}, {"blue", 0x0000ff}, {"blueviolet", 0x8a2be2}, {"brown", 0xa52a2a},
      {"burlywood", 0xdeb887}, {"cadetblue", 0x5f9ea0}, {"chartreuse", 0x7fff00}, {"chocolate", 0xd2691e},
      {"coral", 0xff7f50}, {"cornflowerblue", 0x6495ed}, {"cornsilk", 0xfff8dc}, {"crimson", 0xdc143c},
      {"cyan", 0x00ffff}, {"darkblue", 0x00008b}, {"darkcyan", 0x008b8b}, {"darkgoldenrod", 0xb8860b},
      {"darkgray", 0xa9a9a9}, {"darkgreen", 0x006400}, {"darkgrey", 0xa9a9a9}, {"darkkhaki", 0xbdb76b},
      {"darkmagenta", 0x8b008b}, {"darkolivegreen", 0x556b2f}, {"darkorange", 0xff8c00},
      {"darkorchid", 0x9932cc}, {"darkred", 0x8b0000}, {"darksalmon", 0xe9967a}, {"darkseagreen", 0x8fbc8f},
      {"darkslateblue", 0x483d8b}, {"darkslategray", 0x2f4f4f}, {"darkslategrey", 0x2f4f4f},
      {"darkturquoise", 0x00ced1}, {"darkviolet", 0x9400d3}, {"deeppink", 0xff1493},
      {"deepskyblue", 0x00bfff}, {"dimgray", 0x696969}, {"dimgrey", 0x696969}, {"dodgerblue", 0x1e90ff},
      {"firebrick", 0xb22222}, {"floralwhite", 0xfffaf0}, {"forestgreen", 0x228b22}, {"fuchsia", 0xff00ff},
      {"gainsboro", 0xdcdcdc}, {"ghostwhite", 0xf8f8ff}, {"gold", 0xffd700}, {"goldenrod", 0xdaa520},
      {"gray", 0x808080}, {"green", 0x008000}, {"greenyellow", 0xadff2f}, {"grey", 0x808080},
      {"honeydew", 0xf0fff0}, {"hotpink", 0xff69b4}, {"indianred", 0xcd5c5c}, {"indigo", 0x4b0082},
      {"ivory", 0xfffff0}, {"khaki", 0xf0e68c}, {"lavender", 0xe6e6fa}, {"lavenderblush", 0xfff0f5},
      {"lawngreen", 0x7cfc00}, {"lemonchiffon", 0xfffacd}, {"lightblue", 0xadd8e6}, {"lightcoral", 0xf08080},
      {"lightcyan", 0xe0ffff}, {"lightgoldenrodyellow", 0xfafad2}, {"lightgray", 0xd3d3d3},
      {"lightgreen", 0x90ee90}, {"lightgrey", 0xd3d3d3}, {"lightpink", 0xffb6c1}, {"lightsalmon", 0xffa07a},
      {"lightseagreen", 0x20b2aa}, {"lightskyblue", 0x87cefa}, {"lightslategray", 0x778899},
      {"lightslategrey", 0x778899}, {"lightsteelblue", 0xb0c4de}, {"lightyellow", 0xffffe0},
      {"lime", 0x00ff00}, {"limegreen", 0x32cd32}, {"linen", 0xfaf0e6}, {"magenta", 0xff00ff},
      {"maroon", 0x800000}, {"mediumaquamarine", 0x66cdaa}, {"mediumblue", 0x0000cd},
      {"mediumorchid", 0xba55d3}, {"mediumpurple", 0x9370db}, {"mediumseagreen", 0x3cb371},
      {"mediumslateblue", 0x7b68ee}, {"mediumspringgreen", 0x00fa9a}, {"mediumturquoise", 0x48d1cc},
      {"mediumvioletred", 0xc71585}, {"midnightblue", 0x191970}, {"mintcream", 0xf5fffa},
      {"mistyrose", 0xffe4e1}, {"moccasin", 0xffe4b5}, {"navajowhite", 0xffdead}, {"navy", 0x000080},
      {"oldlace", 0xfdf5e6}, {"olive", 0x808000}, {"olivedrab", 0x6b8e23}, {"orange", 0xffa500},
      {"orangered", 0xff4500}, {"orchid", 0xda70d6}, {"palegoldenrod", 0xeee8aa}, {"palegreen", 0x98fb98},
      {"paleturquoise", 0xafeeee}, {"palevioletred", 0xdb7093}, {"papayawhip", 0xffefd5},
      {"peachpuff", 0xffdab9}, {"peru", 0xcd853f}, {"pink", 0xffc0cb}, {"plum", 0xdda0dd},
      {"powderblue", 0xb0e0e6}, {"purple", 0x800080}, {"rebeccapurple", 0x663399}, {"red", 0xff0000},
      {"rosybrown", 0xbc8f8f}, {"royalblue", 0x4169e1}, {"saddlebrown", 0x8b4513}, {"salmon", 0xfa8072},
      {"sandybrown", 0xf4a460}, {"seagreen", 0x2e8b57}, {"seashell", 0xfff5ee}, {"sienna", 0xa0522d},
      {"silver", 0xc0c0c0}, {"skyblue", 0x87ceeb}, {"slateblue", 0x6a5acd}, {"slategray", 0x708090},
      {"slategrey", 0x708090}, {"snow", 0xfffafa}, {"springgreen", 0x00ff7f}, {"steelblue", 0x4682b4},
      {"tan", 0xd2b48c}, {"teal", 0x008080}, {"thistle", 0xd8bfd8}, {"tomato", 0xff6347},
      {"turquoise", 0x40e0d0}, {"violet", 0xee82ee}, {"wheat", 0xf5deb3}, {"white", 0xffffff},
      {"whitesmoke", 0xf5f5f5}, {"yellow", 0xffff00}, {"yellowgreen", 0x9acd32}
    };

    std::string WrittenColor(const float red, const float green, const float blue, const float alpha)
    {
      const auto byte = [](const float part)
      {
        return static_cast<int>(std::lround(std::clamp(part, 0.0f, 1.0f) * 255.0f));
      };

      return std::format(
        "rgba({}, {}, {}, {})",
        byte(red), byte(green), byte(blue), FormatCssNumber(std::clamp(alpha, 0.0f, 1.0f)));
    }

    /// `hsl()` as https://www.w3.org/TR/css-color-4/#hsl-to-rgb works it
    /// out.
    bool ResolveHsl(const std::string &inside, std::string &result)
    {
      std::string spaced = inside;
      std::ranges::replace(spaced, ',', ' ');
      std::ranges::replace(spaced, '/', ' ');

      const auto parts = SplitCssValues(spaced);
      if (parts.size() != 3 && parts.size() != 4) { return false; }

      std::string hue_text = Lowered(parts[0]);
      float turn = 360.0f;

      if (hue_text.ends_with("deg")) { hue_text = hue_text.substr(0, hue_text.size() - 3); }
      else if (hue_text.ends_with("grad"))
      {
        hue_text = hue_text.substr(0, hue_text.size() - 4);
        turn = 400.0f;
      } else if (hue_text.ends_with("turn"))
      {
        hue_text = hue_text.substr(0, hue_text.size() - 4);
        turn = 1.0f;
      } else if (hue_text.ends_with("rad"))
      {
        hue_text = hue_text.substr(0, hue_text.size() - 3);
        turn = 6.283185307f;
      }

      float hue = 0.0f;
      float saturation = 0.0f;
      float lightness = 0.0f;
      float alpha = 1.0f;

      const auto percent = [](std::string text, float &value)
      {
        if (!text.empty() && text.back() == '%') { text.pop_back(); }
        if (!ParseCssNumber(text, value)) { return false; }

        value = std::clamp(value / 100.0f, 0.0f, 1.0f);
        return true;
      };

      if (!ParseCssNumber(hue_text, hue) || !percent(parts[1], saturation) || !percent(parts[2], lightness))
      {
        return false;
      }

      if (parts.size() == 4)
      {
        if (parts[3].back() == '%')
        {
          if (!percent(parts[3], alpha)) { return false; }
        } else if (!ParseCssNumber(parts[3], alpha))
        {
          return false;
        }
      }

      hue = std::fmod(hue / turn * 360.0f, 360.0f);
      if (hue < 0.0f) { hue += 360.0f; }

      const auto channel = [&](const float n)
      {
        const float k = std::fmod(n + hue / 30.0f, 12.0f);
        const float a = saturation * std::min(lightness, 1.0f - lightness);
        return lightness - a * std::max(-1.0f, std::min({k - 3.0f, 9.0f - k, 1.0f}));
      };

      result = WrittenColor(channel(0.0f), channel(8.0f), channel(4.0f), alpha);
      return true;
    }
  }

  std::string FormatCssNumber(const float number)
  {
    // what is off by the last digits of a float is a whole number
    const float rounded = std::round(number * 10000.0f) / 10000.0f;

    if (rounded == 0.0f) { return "0"; }

    return std::format("{}", rounded);
  }

  bool HasCssVariables(const std::string &value)
  {
    for (std::size_t i = 0; i + 4 <= value.size(); i++)
    {
      if (StartsFunction(value, i, "var")) { return true; }
    }
    return false;
  }

  bool SubstituteCssVariables(
    const std::string &value,
    const CssValueContext &context,
    std::string &resolved,
    std::string &error,
    CssValueUses *uses)
  {
    std::string substituted;
    if (!Substitute(value, context, substituted, error, uses, 0)) { return false; }

    resolved = Trimmed(substituted);
    return true;
  }

  bool ResolveCssValue(
    const std::string &value,
    const CssValueContext &context,
    std::string &resolved,
    std::string &error,
    CssValueUses *uses)
  {
    std::string substituted;
    if (!Substitute(value, context, substituted, error, uses, 0)) { return false; }

    std::string result;
    if (!ResolveUnits(substituted, context, result, error, uses)) { return false; }

    resolved = Trimmed(result);
    return true;
  }

  std::string ResolveCssColors(const std::string &value)
  {
    std::string result;

    for (std::size_t i = 0; i < value.size();)
    {
      const char letter = value[i];

      if (letter == '"' || letter == '\'')
      {
        const std::size_t start = i;
        i++;
        while (i < value.size() && value[i] != letter)
        {
          if (value[i] == '\\') { i++; }
          i++;
        }
        i = std::min(i + 1, value.size());
        result += value.substr(start, i - start);
        continue;
      }

      if (StartsFunction(value, i, "hsl") || StartsFunction(value, i, "hsla"))
      {
        const std::size_t open = value.find('(', i);
        const std::size_t close = FindClose(value, open);

        if (std::string written; close != std::string::npos &&
                                 ResolveHsl(value.substr(open + 1, close - open - 2), written))
        {
          result += written;
          i = close;
          continue;
        }
      }

      if (letter == '#')
      {
        // the digits of a color may spell a word
        result += letter;
        i++;
        while (i < value.size() && IsNamePart(value[i]))
        {
          result += value[i];
          i++;
        }
        continue;
      }

      if (std::isalpha(static_cast<unsigned char>(letter)) != 0 && (i == 0 || !IsNamePart(value[i - 1])))
      {
        std::size_t end = i;
        while (end < value.size() && IsNamePart(value[end])) { end++; }

        const std::string word = value.substr(i, end - i);
        const bool is_function = end < value.size() && value[end] == '(';

        if (is_function)
        {
          // what a function holds is its own business
          const std::size_t close = FindClose(value, end);
          const std::size_t stop = close == std::string::npos ? value.size() : close;
          result += value.substr(i, stop - i);
          i = stop;
          continue;
        }

        const std::string lowered = Lowered(word);
        bool is_color = false;

        for (const auto &[name, rgb] : named_colors)
        {
          if (lowered != name) { continue; }

          result += WrittenColor(
            static_cast<float>((rgb >> 16) & 0xff) / 255.0f,
            static_cast<float>((rgb >> 8) & 0xff) / 255.0f,
            static_cast<float>(rgb & 0xff) / 255.0f,
            1.0f);
          is_color = true;
          break;
        }

        if (!is_color) { result += word; }
        i = end;
        continue;
      }

      result += letter;
      i++;
    }

    return result;
  }
} // neon
