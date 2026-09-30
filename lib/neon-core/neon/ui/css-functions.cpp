#include "css-functions.hpp"

#include <cctype>
#include <cstddef>

#include "css-values.hpp"

namespace neon
{
  namespace
  {
    std::string Trimmed(const std::string &text)
    {
      std::size_t first = 0;
      std::size_t last = text.size();

      while (first < last && std::isspace(static_cast<unsigned char>(text[first]))) { first++; }
      while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1]))) { last--; }

      return text.substr(first, last - first);
    }

    bool EndsWith(const std::string &text, const std::string &end)
    {
      return text.size() >= end.size() && text.compare(text.size() - end.size(), end.size(), end) == 0;
    }

    bool ParsePixels(const std::string &text, float &pixels)
    {
      LayoutLength length;
      if (!ParseCssLength(text, length) || length.unit != LayoutLength::Unit::Pixels) { return false; }

      pixels = length.value;
      return true;
    }

    bool ParseLengthOrPercent(const std::string &text, LayoutLength &length)
    {
      LayoutLength read;
      if (!ParseCssLength(text, read) || read.IsAuto()) { return false; }

      length = read;
      return true;
    }

    /// `to right`, `to left top`, and so on, as an angle.
    bool ParseSide(const std::vector<std::string> &words, float &degrees)
    {
      if (words.size() < 2 || words.size() > 3 || words[0] != "to") { return false; }

      int right = 0;
      int down = 0;

      for (std::size_t i = 1; i < words.size(); i++)
      {
        if (words[i] == "right" && right == 0) { right = 1; }
        else if (words[i] == "left" && right == 0) { right = -1; }
        else if (words[i] == "bottom" && down == 0) { down = 1; }
        else if (words[i] == "top" && down == 0) { down = -1; }
        else { return false; }
      }

      // To a corner is taken as half way between the two sides. CSS turns
      // it so that the corners of the box have the colours of the ends,
      // which is the same for a square.
      if (right == 0) { degrees = down > 0 ? 180.0f : 0.0f; }
      else if (down == 0) { degrees = right > 0 ? 90.0f : 270.0f; }
      else if (right > 0) { degrees = down > 0 ? 135.0f : 45.0f; }
      else { degrees = down > 0 ? 225.0f : 315.0f; }

      return true;
    }

    bool ParseStop(const std::string &text, UiGradientStop &stop)
    {
      const auto parts = SplitCssValues(text);
      if (parts.empty() || parts.size() > 2) { return false; }
      if (!ParseCssColor(parts[0], stop.color)) { return false; }

      if (parts.size() == 2)
      {
        if (!EndsWith(parts[1], "%")) { return false; }

        float percent = 0.0f;
        if (!ParseCssNumber(parts[1].substr(0, parts[1].size() - 1), percent)) { return false; }

        stop.position = percent / 100.0f;
        stop.has_position = true;
      }

      return true;
    }
  }

  std::vector<std::string> SplitCssList(const std::string &text, const char separator)
  {
    std::vector<std::string> parts;
    std::string part;
    int depth = 0;

    for (const char letter : text)
    {
      if (letter == '(') { depth++; }
      if (letter == ')' && depth > 0) { depth--; }

      if (letter == separator && depth == 0)
      {
        parts.push_back(Trimmed(part));
        part.clear();
        continue;
      }

      part += letter;
    }

    if (const std::string last = Trimmed(part); !last.empty() || !parts.empty()) { parts.push_back(last); }
    return parts;
  }

  bool ParseCssFunction(const std::string &text, const std::string &name, std::string &inside)
  {
    const std::string trimmed = Trimmed(text);

    if (trimmed.size() < name.size() + 2 || trimmed.compare(0, name.size(), name) != 0 ||
        trimmed[name.size()] != '(' || trimmed.back() != ')')
    {
      return false;
    }

    // the bracket at the end is the one that closes the first
    int depth = 0;
    for (std::size_t i = name.size(); i < trimmed.size(); i++)
    {
      if (trimmed[i] == '(') { depth++; }
      if (trimmed[i] == ')') { depth--; }
      if (depth == 0 && i + 1 < trimmed.size()) { return false; }
    }
    if (depth != 0) { return false; }

    inside = Trimmed(trimmed.substr(name.size() + 1, trimmed.size() - name.size() - 2));
    return true;
  }

  bool ParseCssAngle(const std::string &text, float &degrees)
  {
    const std::string trimmed = Trimmed(text);
    float number = 0.0f;

    const auto with_unit = [&](const std::string &unit, const float factor)
    {
      if (!EndsWith(trimmed, unit)) { return false; }
      if (!ParseCssNumber(trimmed.substr(0, trimmed.size() - unit.size()), number)) { return false; }

      degrees = number * factor;
      return true;
    };

    // `grad` ends with `rad`, so it is asked for first
    if (with_unit("grad", 0.9f) || with_unit("deg", 1.0f) || with_unit("turn", 360.0f) ||
        with_unit("rad", 180.0f / 3.14159265358979f))
    {
      return true;
    }

    if (ParseCssNumber(trimmed, number) && number == 0.0f)
    {
      degrees = 0.0f;
      return true;
    }

    return false;
  }

  bool ParseCssGradient(const std::string &text, UiGradient &gradient)
  {
    UiGradient read;
    std::string inside;

    if (ParseCssFunction(text, "linear-gradient", inside))
    {
      read.kind = UiGradient::Kind::Linear;
    } else if (ParseCssFunction(text, "radial-gradient", inside))
    {
      read.kind = UiGradient::Kind::Radial;
    } else
    {
      return false;
    }

    auto parts = SplitCssList(inside, ',');
    if (parts.empty()) { return false; }

    // what comes first may say which way the gradient runs
    if (read.kind == UiGradient::Kind::Linear)
    {
      if (float degrees = 0.0f; ParseCssAngle(parts[0], degrees) || ParseSide(SplitCssValues(parts[0]), degrees))
      {
        read.angle = degrees;
        parts.erase(parts.begin());
      }
    } else if (parts[0] == "ellipse" || parts[0] == "ellipse at center" || parts[0] == "farthest-corner")
    {
      parts.erase(parts.begin());
    }

    if (parts.size() < 2 || parts.size() > UiGradient::kMax_Stops) { return false; }

    for (const auto &part : parts)
    {
      UiGradientStop stop;
      if (!ParseStop(part, stop)) { return false; }
      read.stops.push_back(stop);
    }

    read.Settle();
    gradient = read;
    return true;
  }

  bool ParseCssShadows(const std::string &text, const bool of_text, std::vector<UiShadow> &shadows)
  {
    std::vector<UiShadow> read;

    if (Trimmed(text) == "none")
    {
      shadows.clear();
      return true;
    }

    const auto parts = SplitCssList(text, ',');
    if (parts.empty()) { return false; }

    for (const auto &part : parts)
    {
      UiShadow shadow;
      std::vector<float> lengths;

      for (const auto &word : SplitCssValues(part))
      {
        float pixels = 0.0f;
        Color color;

        if (word == "inset" && !of_text && !shadow.is_inset)
        {
          shadow.is_inset = true;
        } else if (ParsePixels(word, pixels))
        {
          lengths.push_back(pixels);
        } else if (!shadow.has_color && ParseCssColor(word, color))
        {
          shadow.color = color;
          shadow.has_color = true;
        } else
        {
          return false;
        }
      }

      if (lengths.size() < 2 || lengths.size() > (of_text ? 3u : 4u)) { return false; }

      shadow.offset_x = lengths[0];
      shadow.offset_y = lengths[1];
      if (lengths.size() > 2) { shadow.blur = lengths[2]; }
      if (lengths.size() > 3) { shadow.spread = lengths[3]; }

      if (shadow.blur < 0.0f) { return false; }

      read.push_back(shadow);
    }

    shadows = read;
    return true;
  }

  bool ParseCssTransform(const std::string &text, std::vector<UiTransformStep> &steps)
  {
    std::vector<UiTransformStep> read;

    if (Trimmed(text) == "none")
    {
      steps.clear();
      return true;
    }

    const auto parts = SplitCssValues(text);
    if (parts.empty()) { return false; }

    for (const auto &part : parts)
    {
      UiTransformStep step;
      std::string inside;

      const auto arguments = [&inside]
      {
        return SplitCssList(inside, ',');
      };

      if (ParseCssFunction(part, "translate", inside))
      {
        const auto values = arguments();
        if (values.empty() || values.size() > 2 || !ParseLengthOrPercent(values[0], step.x)) { return false; }
        if (values.size() == 2 && !ParseLengthOrPercent(values[1], step.y)) { return false; }
      } else if (ParseCssFunction(part, "translateX", inside))
      {
        if (!ParseLengthOrPercent(inside, step.x)) { return false; }
      } else if (ParseCssFunction(part, "translateY", inside))
      {
        if (!ParseLengthOrPercent(inside, step.y)) { return false; }
      } else if (ParseCssFunction(part, "rotate", inside))
      {
        step.kind = UiTransformStep::Kind::Rotate;
        if (!ParseCssAngle(inside, step.angle)) { return false; }
      } else if (ParseCssFunction(part, "scale", inside))
      {
        step.kind = UiTransformStep::Kind::Scale;

        const auto values = arguments();
        if (values.empty() || values.size() > 2 || !ParseCssNumber(values[0], step.scale_x)) { return false; }

        step.scale_y = step.scale_x;
        if (values.size() == 2 && !ParseCssNumber(values[1], step.scale_y)) { return false; }
      } else if (ParseCssFunction(part, "scaleX", inside))
      {
        step.kind = UiTransformStep::Kind::Scale;
        if (!ParseCssNumber(inside, step.scale_x)) { return false; }
      } else if (ParseCssFunction(part, "scaleY", inside))
      {
        step.kind = UiTransformStep::Kind::Scale;
        if (!ParseCssNumber(inside, step.scale_y)) { return false; }
      } else
      {
        return false;
      }

      read.push_back(step);
    }

    steps = read;
    return true;
  }

  bool ParseCssPlace(const std::string &text, UiPlace &place)
  {
    const auto words = SplitCssValues(text);
    if (words.empty() || words.size() > 2) { return false; }

    UiPlace read;
    bool has_x = false;
    bool has_y = false;

    // a word that names a side says which way it counts. Anything else
    // counts from side to side first
    std::vector<std::string> others;

    for (const auto &word : words)
    {
      if (word == "left" || word == "right")
      {
        if (has_x) { return false; }
        read.x = LayoutLength::Percent(word == "left" ? 0.0f : 100.0f);
        has_x = true;
      } else if (word == "top" || word == "bottom")
      {
        if (has_y) { return false; }
        read.y = LayoutLength::Percent(word == "top" ? 0.0f : 100.0f);
        has_y = true;
      } else
      {
        others.push_back(word);
      }
    }

    for (const auto &word : others)
    {
      LayoutLength length = LayoutLength::Percent(50.0f);
      if (word != "center" && !ParseLengthOrPercent(word, length)) { return false; }

      if (!has_x)
      {
        read.x = length;
        has_x = true;
      } else if (!has_y)
      {
        read.y = length;
        has_y = true;
      } else
      {
        return false;
      }
    }

    place = read;
    return true;
  }
} // neon
