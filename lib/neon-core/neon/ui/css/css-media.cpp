#include "css-media.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>

#include <neon/ui/css-values.hpp>

namespace neon
{
  // Helpers of CssEnvironment, for this file alone.
  namespace
  {
    constexpr float epsilon = 0.0001f;

    bool IsSpace(const char letter)
    {
      return letter == ' ' || letter == '\t' || letter == '\n' || letter == '\r' || letter == '\f';
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

    /// A number with a unit behind it, such as `12.5px`.
    bool SplitUnit(const std::string &text, float &number, std::string &unit)
    {
      std::size_t end = text.size();
      while (end > 0 && (std::isalpha(static_cast<unsigned char>(text[end - 1])) != 0 || text[end - 1] == '%'))
      {
        end--;
      }

      unit = text.substr(end);
      return end > 0 && ParseCssNumber(text.substr(0, end), number);
    }

    bool ReadLength(const std::string &text, float &points)
    {
      float number = 0.0f;
      std::string unit;
      if (!SplitUnit(text, number, unit)) { return false; }

      if (unit == "px" || (unit.empty() && number == 0.0f))
      {
        points = number;
        return true;
      }

      // of the font a browser starts with, since a query knows no element
      if (unit == "em" || unit == "rem")
      {
        points = number * 16.0f;
        return true;
      }

      return false;
    }

    bool ReadResolution(const std::string &text, float &density)
    {
      float number = 0.0f;
      std::string unit;
      if (!SplitUnit(text, number, unit) || number < 0.0f) { return false; }

      if (unit == "dppx" || unit == "x")
      {
        density = number;
        return true;
      }

      // 96 to the inch is a density of 1
      if (unit == "dpi")
      {
        density = number / 96.0f;
        return true;
      }

      if (unit == "dpcm")
      {
        density = number * 2.54f / 96.0f;
        return true;
      }

      return false;
    }

    bool ReadRatio(const std::string &text, float &ratio)
    {
      const std::size_t slash = text.find('/');

      float width = 0.0f;
      float height = 1.0f;

      if (slash == std::string::npos)
      {
        if (!ParseCssNumber(text, width)) { return false; }
      } else if (!ParseCssNumber(text.substr(0, slash), width) || !ParseCssNumber(text.substr(slash + 1), height))
      {
        return false;
      }

      if (width < 0.0f || height <= 0.0f) { return false; }

      ratio = width / height;
      return true;
    }

    bool IsKnown(const std::string &name)
    {
      return name == "width" || name == "height" || name == "aspect-ratio" || name == "resolution" ||
             name == "orientation" || name == "prefers-reduced-motion";
    }

    bool IsRange(const std::string &name)
    {
      return name == "width" || name == "height" || name == "aspect-ratio" || name == "resolution";
    }

    bool ReadValue(const std::string &name, const std::string &written, CssMediaFeature &feature, std::string &error)
    {
      const std::string value = Lowered(Trimmed(written));

      if (name == "width" || name == "height")
      {
        if (!ReadLength(value, feature.value) || feature.value < 0.0f)
        {
          error = std::format("'{}' is '{}', where a length such as 800px was expected", name, value);
          return false;
        }
        return true;
      }

      if (name == "resolution")
      {
        if (!ReadResolution(value, feature.value))
        {
          error = std::format(
            "'resolution' is '{}', where a density such as 2dppx, 2x, or 192dpi was expected", value);
          return false;
        }
        return true;
      }

      if (name == "aspect-ratio")
      {
        if (!ReadRatio(value, feature.value))
        {
          error = std::format("'aspect-ratio' is '{}', where a ratio such as 16/9 was expected", value);
          return false;
        }
        return true;
      }

      if (name == "orientation")
      {
        if (value != "portrait" && value != "landscape")
        {
          error = std::format("'orientation' is '{}', where portrait or landscape was expected", value);
          return false;
        }
        feature.keyword = value;
        return true;
      }

      if (value != "reduce" && value != "no-preference")
      {
        error = std::format(
          "'prefers-reduced-motion' is '{}', where reduce or no-preference was expected", value);
        return false;
      }

      feature.keyword = value;
      return true;
    }

    const std::string known_features =
      "width, height, aspect-ratio, orientation, resolution, prefers-reduced-motion";

    /// The opposite way around, for a value that is written in front:
    /// `600px <= width` is `width >= 600px`.
    CssMediaFeature::Compare Turned(const CssMediaFeature::Compare compare)
    {
      switch (compare)
      {
        case CssMediaFeature::Compare::AtLeast: return CssMediaFeature::Compare::AtMost;
        case CssMediaFeature::Compare::AtMost: return CssMediaFeature::Compare::AtLeast;
        case CssMediaFeature::Compare::Above: return CssMediaFeature::Compare::Below;
        case CssMediaFeature::Compare::Below: return CssMediaFeature::Compare::Above;
        default: return compare;
      }
    }

    /// What is between brackets, as one feature or as the two a range
    /// with two sides stands for.
    bool ParseFeature(const std::string &inside, std::vector<CssMediaFeature> &features, std::string &error)
    {
      const std::string text = Trimmed(inside);
      if (text.empty())
      {
        error = "the brackets hold nothing, where a feature such as min-width: 800px was expected";
        return false;
      }

      // `name: value`
      if (const std::size_t colon = text.find(':'); colon != std::string::npos)
      {
        std::string name = Lowered(Trimmed(text.substr(0, colon)));
        CssMediaFeature feature;
        feature.compare = CssMediaFeature::Compare::Equal;

        if (name.starts_with("min-"))
        {
          feature.compare = CssMediaFeature::Compare::AtLeast;
          name = name.substr(4);
        } else if (name.starts_with("max-"))
        {
          feature.compare = CssMediaFeature::Compare::AtMost;
          name = name.substr(4);
        }

        if (!IsKnown(name))
        {
          error = std::format("'{}' is not a feature that is known. Known are: {}", name, known_features);
          return false;
        }

        if (feature.compare != CssMediaFeature::Compare::Equal && !IsRange(name))
        {
          error = std::format("'{}' has no least and no most, and is written without min- and max-", name);
          return false;
        }

        feature.name = name;
        if (!ReadValue(name, text.substr(colon + 1), feature, error)) { return false; }

        features.push_back(feature);
        return true;
      }

      // `name`, `name >= value`, `value <= name`, and `value <= name <= value`
      std::vector<std::string> parts;
      std::vector<CssMediaFeature::Compare> compares;
      std::string part;

      for (std::size_t i = 0; i < text.size(); i++)
      {
        const char letter = text[i];

        if (letter == '<' || letter == '>' || letter == '=')
        {
          const bool with_equal = letter != '=' && i + 1 < text.size() && text[i + 1] == '=';

          if (letter == '=') { compares.push_back(CssMediaFeature::Compare::Equal); }
          else if (letter == '<')
          {
            compares.push_back(with_equal ? CssMediaFeature::Compare::AtMost : CssMediaFeature::Compare::Below);
          } else
          {
            compares.push_back(with_equal ? CssMediaFeature::Compare::AtLeast : CssMediaFeature::Compare::Above);
          }

          if (with_equal) { i++; }

          parts.push_back(Trimmed(part));
          part.clear();
        } else
        {
          part += letter;
        }
      }
      parts.push_back(Trimmed(part));

      if (compares.empty())
      {
        const std::string name = Lowered(parts[0]);
        if (!IsKnown(name))
        {
          error = std::format("'{}' is not a feature that is known. Known are: {}", name, known_features);
          return false;
        }

        CssMediaFeature feature;
        feature.name = name;
        feature.compare = CssMediaFeature::Compare::Has;
        features.push_back(feature);
        return true;
      }

      if (compares.size() > 2 || std::ranges::any_of(parts, [](const std::string &each) { return each.empty(); }))
      {
        error = std::format("'{}' cannot be read, where a range such as width >= 800px was expected", text);
        return false;
      }

      // the name is the part that is known as a feature
      std::size_t name_at = parts.size();
      for (std::size_t i = 0; i < parts.size(); i++)
      {
        if (IsKnown(Lowered(parts[i]))) { name_at = i; }
      }

      if (name_at == parts.size() || (compares.size() == 2 && name_at != 1))
      {
        error = std::format(
          "'{}' names no feature that is known. Known are: {}", text, known_features);
        return false;
      }

      const std::string name = Lowered(parts[name_at]);
      if (!IsRange(name))
      {
        error = std::format("'{}' has no least and no most, and is compared with a colon", name);
        return false;
      }

      for (std::size_t i = 0; i < compares.size(); i++)
      {
        // the comparison stands between part i and part i + 1
        const bool name_is_left = name_at == i;
        const std::string &value = name_is_left ? parts[i + 1] : parts[i];

        if (!name_is_left && name_at != i + 1)
        {
          error = std::format("'{}' cannot be read, where a range such as width >= 800px was expected", text);
          return false;
        }

        CssMediaFeature feature;
        feature.name = name;
        feature.compare = name_is_left ? compares[i] : Turned(compares[i]);

        if (!ReadValue(name, value, feature, error)) { return false; }
        features.push_back(feature);
      }

      return true;
    }

    bool Compare(const float actual, const CssMediaFeature::Compare compare, const float wanted)
    {
      switch (compare)
      {
        case CssMediaFeature::Compare::Has: return actual != 0.0f;
        case CssMediaFeature::Compare::Equal: return std::abs(actual - wanted) <= epsilon;
        case CssMediaFeature::Compare::AtLeast: return actual >= wanted - epsilon;
        case CssMediaFeature::Compare::AtMost: return actual <= wanted + epsilon;
        case CssMediaFeature::Compare::Above: return actual > wanted + epsilon;
        case CssMediaFeature::Compare::Below: return actual < wanted - epsilon;
      }
      return false;
    }

    bool Holds(const CssMediaFeature &feature, const CssEnvironment &environment)
    {
      if (feature.name == "width") { return Compare(environment.width, feature.compare, feature.value); }
      if (feature.name == "height") { return Compare(environment.height, feature.compare, feature.value); }
      if (feature.name == "resolution") { return Compare(environment.resolution, feature.compare, feature.value); }

      if (feature.name == "aspect-ratio")
      {
        const float ratio = environment.height > 0.0f ? environment.width / environment.height : 0.0f;
        return Compare(ratio, feature.compare, feature.value);
      }

      if (feature.name == "orientation")
      {
        // as the specification says, a square is a portrait
        const std::string actual = environment.width > environment.height ? "landscape" : "portrait";
        return feature.compare == CssMediaFeature::Compare::Has || actual == feature.keyword;
      }

      if (feature.name == "prefers-reduced-motion")
      {
        if (feature.compare == CssMediaFeature::Compare::Has) { return environment.reduced_motion; }
        return environment.reduced_motion == (feature.keyword == "reduce");
      }

      return false;
    }

    bool ParseQuery(const std::string &written, CssMediaQuery &query, std::string &error)
    {
      const std::string text = Trimmed(written);
      if (text.empty())
      {
        error = "a query is empty";
        return false;
      }

      std::size_t position = 0;
      bool expects_and = false;
      bool has_anything = false;

      while (position < text.size())
      {
        while (position < text.size() && IsSpace(text[position])) { position++; }
        if (position >= text.size()) { break; }

        if (text[position] == '(')
        {
          if (expects_and)
          {
            error = "two parts of a query follow each other, where 'and' was expected between them";
            return false;
          }

          int depth = 0;
          const std::size_t start = position + 1;

          while (position < text.size())
          {
            if (text[position] == '(') { depth++; }
            if (text[position] == ')')
            {
              depth--;
              if (depth == 0) { break; }
            }
            position++;
          }

          if (position >= text.size())
          {
            error = "a bracket is not closed";
            return false;
          }

          if (!ParseFeature(text.substr(start, position - start), query.features, error)) { return false; }

          position++;
          expects_and = true;
          has_anything = true;
          continue;
        }

        std::size_t end = position;
        while (end < text.size() && !IsSpace(text[end]) && text[end] != '(') { end++; }

        const std::string word = Lowered(text.substr(position, end - position));
        position = end;

        if (word == "and")
        {
          if (!expects_and)
          {
            error = "'and' has nothing in front of it";
            return false;
          }
          expects_and = false;
        } else if (expects_and)
        {
          error = std::format("'{}' follows a part of a query, where 'and' was expected", word);
          return false;
        } else if (word == "not" && !has_anything && !query.is_negated)
        {
          query.is_negated = true;
        } else if (word == "only" && !has_anything)
        {
          // for browsers that are older than media queries
        } else if ((word == "all" || word == "screen" || word == "print") && !has_anything)
        {
          query.type = word;
          expects_and = true;
          has_anything = true;
        } else
        {
          error = std::format(
            "'{}' cannot be read, where all, screen, print, or a feature in brackets was expected", word);
          return false;
        }
      }

      if (!has_anything)
      {
        error = "a query names nothing, where all, screen, print, or a feature in brackets was expected";
        return false;
      }

      if (!expects_and)
      {
        error = "a query ends with 'and', where a feature in brackets was expected behind it";
        return false;
      }

      return true;
    }
  }

  bool CssMediaQueryList::Matches(const CssEnvironment &environment) const
  {
    if (queries.empty()) { return true; }

    return std::ranges::any_of(queries, [&environment](const CssMediaQuery &query)
    {
      // a game is shown on a screen
      const bool holds = query.type != "print" &&
                         std::ranges::all_of(query.features, [&environment](const CssMediaFeature &feature)
                         {
                           return Holds(feature, environment);
                         });

      return query.is_negated ? !holds : holds;
    });
  }

  bool ParseCssMedia(const std::string &text, CssMediaQueryList &list, std::string &error)
  {
    list = CssMediaQueryList{};
    list.text = Trimmed(text);

    if (list.text.empty()) { return true; }

    std::size_t start = 0;
    int depth = 0;

    for (std::size_t i = 0; i <= text.size(); i++)
    {
      if (i < text.size() && text[i] == '(') { depth++; }
      if (i < text.size() && text[i] == ')') { depth--; }

      if (i == text.size() || (text[i] == ',' && depth == 0))
      {
        CssMediaQuery query;
        if (!ParseQuery(text.substr(start, i - start), query, error))
        {
          list.queries.clear();
          return false;
        }

        list.queries.push_back(query);
        start = i + 1;
      }
    }

    return true;
  }
} // neon
