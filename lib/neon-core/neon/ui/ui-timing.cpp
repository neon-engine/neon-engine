#include "ui-timing.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <vector>

#include "css-values.hpp"
#include "css/css-expression.hpp"

namespace neon
{
  // Helpers of UiTimingFunction, for this file alone.
  namespace
  {
    std::string Tidied(const std::string &text)
    {
      std::string tidy;
      for (const char letter : text)
      {
        if (std::isspace(static_cast<unsigned char>(letter)) == 0)
        {
          tidy += static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
        }
      }
      return tidy;
    }

    std::vector<std::string> Arguments(const std::string &inside)
    {
      std::vector<std::string> arguments;
      std::string argument;

      for (const char letter : inside)
      {
        if (letter == ',')
        {
          arguments.push_back(argument);
          argument.clear();
        } else
        {
          argument += letter;
        }
      }

      arguments.push_back(argument);
      return arguments;
    }

    /// One coordinate of the curve at a place along it.
    double Bezier(const double first, const double second, const double along)
    {
      const double rest = 1.0 - along;
      return 3.0 * rest * rest * along * first + 3.0 * rest * along * along * second + along * along * along;
    }

    double BezierSlope(const double first, const double second, const double along)
    {
      const double rest = 1.0 - along;
      return 3.0 * rest * rest * first + 6.0 * rest * along * (second - first) +
             3.0 * along * along * (1.0 - second);
    }
  }

  UiTimingFunction UiTimingFunction::Linear()
  {
    UiTimingFunction function;
    function.kind = Kind::Linear;
    function.x1 = 0.0f;
    function.y1 = 0.0f;
    function.x2 = 1.0f;
    function.y2 = 1.0f;
    return function;
  }

  UiTimingFunction UiTimingFunction::Ease()
  {
    return CubicBezier(0.25f, 0.1f, 0.25f, 1.0f);
  }

  UiTimingFunction UiTimingFunction::EaseIn()
  {
    return CubicBezier(0.42f, 0.0f, 1.0f, 1.0f);
  }

  UiTimingFunction UiTimingFunction::EaseOut()
  {
    return CubicBezier(0.0f, 0.0f, 0.58f, 1.0f);
  }

  UiTimingFunction UiTimingFunction::EaseInOut()
  {
    return CubicBezier(0.42f, 0.0f, 0.58f, 1.0f);
  }

  UiTimingFunction UiTimingFunction::CubicBezier(const float x1, const float y1, const float x2, const float y2)
  {
    UiTimingFunction function;
    function.kind = Kind::CubicBezier;
    function.x1 = x1;
    function.y1 = y1;
    function.x2 = x2;
    function.y2 = y2;
    return function;
  }

  UiTimingFunction UiTimingFunction::Steps(const int steps, const StepPosition position)
  {
    UiTimingFunction function;
    function.kind = Kind::Steps;
    function.steps = std::max(1, steps);
    function.step_position = position;
    return function;
  }

  float UiTimingFunction::At(const float time, const bool before) const
  {
    switch (kind)
    {
      case Kind::Linear:
        return time;

      case Kind::Steps:
      {
        // https://www.w3.org/TR/css-easing-1/#step-easing-algo
        auto current = static_cast<int>(std::floor(static_cast<double>(time) * steps));

        if (step_position == StepPosition::JumpStart || step_position == StepPosition::JumpBoth) { current++; }

        if (before && std::fmod(static_cast<double>(time) * steps, 1.0) == 0.0) { current--; }

        if (time >= 0.0f && current < 0) { current = 0; }

        int jumps = steps;
        if (step_position == StepPosition::JumpNone) { jumps = steps - 1; }
        if (step_position == StepPosition::JumpBoth) { jumps = steps + 1; }

        jumps = std::max(1, jumps);

        if (time <= 1.0f && current > jumps) { current = jumps; }

        return static_cast<float>(current) / static_cast<float>(jumps);
      }

      case Kind::CubicBezier:
        break;
    }

    // outside of the time the curve goes on straight, as the specification
    // says
    if (time <= 0.0f)
    {
      if (x1 > 0.0f) { return time * y1 / x1; }
      if (y1 == 0.0f && x2 > 0.0f) { return time * y2 / x2; }
      return 0.0f;
    }

    if (time >= 1.0f)
    {
      if (x2 < 1.0f) { return 1.0f + (time - 1.0f) * (y2 - 1.0f) / (x2 - 1.0f); }
      if (y2 == 1.0f && x1 < 1.0f) { return 1.0f + (time - 1.0f) * (y1 - 1.0f) / (x1 - 1.0f); }
      return 1.0f;
    }

    // The curve says where it is for a place along it, and is asked for a
    // time. The place is found by the method of Newton, and by halving
    // where that does not get there.
    const double wanted = time;
    double along = wanted;

    for (int i = 0; i < 8; i++)
    {
      const double off = Bezier(x1, x2, along) - wanted;
      if (std::abs(off) < 1e-7) { return static_cast<float>(Bezier(y1, y2, along)); }

      const double slope = BezierSlope(x1, x2, along);
      if (std::abs(slope) < 1e-7) { break; }

      along -= off / slope;
    }

    double low = 0.0;
    double high = 1.0;
    along = wanted;

    for (int i = 0; i < 64 && low < high; i++)
    {
      const double x = Bezier(x1, x2, along);
      if (std::abs(x - wanted) < 1e-7) { break; }

      if (wanted > x) { low = along; }
      else { high = along; }

      along = (low + high) / 2.0;
    }

    return static_cast<float>(Bezier(y1, y2, along));
  }

  bool ParseCssTimingFunction(const std::string &text, UiTimingFunction &function)
  {
    const std::string value = Tidied(text);

    if (value == "linear") { function = UiTimingFunction::Linear(); }
    else if (value == "ease") { function = UiTimingFunction::Ease(); }
    else if (value == "ease-in") { function = UiTimingFunction::EaseIn(); }
    else if (value == "ease-out") { function = UiTimingFunction::EaseOut(); }
    else if (value == "ease-in-out") { function = UiTimingFunction::EaseInOut(); }
    else if (value == "step-start")
    {
      function = UiTimingFunction::Steps(1, UiTimingFunction::StepPosition::JumpStart);
    } else if (value == "step-end")
    {
      function = UiTimingFunction::Steps(1, UiTimingFunction::StepPosition::JumpEnd);
    } else if (value.starts_with("cubic-bezier(") && value.back() == ')')
    {
      const auto arguments = Arguments(value.substr(13, value.size() - 14));
      float numbers[4] = {0.0f, 0.0f, 0.0f, 0.0f};

      if (arguments.size() != 4) { return false; }

      for (std::size_t i = 0; i < 4; i++)
      {
        if (!ParseCssNumber(arguments[i], numbers[i])) { return false; }
      }

      // time does not run backwards
      if (numbers[0] < 0.0f || numbers[0] > 1.0f || numbers[2] < 0.0f || numbers[2] > 1.0f) { return false; }

      function = UiTimingFunction::CubicBezier(numbers[0], numbers[1], numbers[2], numbers[3]);
    } else if (value.starts_with("steps(") && value.back() == ')')
    {
      const auto arguments = Arguments(value.substr(6, value.size() - 7));
      if (arguments.empty() || arguments.size() > 2) { return false; }

      float count = 0.0f;
      if (!ParseCssNumber(arguments[0], count) || count != std::round(count) || count < 1.0f) { return false; }

      auto position = UiTimingFunction::StepPosition::JumpEnd;

      if (arguments.size() == 2)
      {
        const std::string &word = arguments[1];

        if (word == "jump-start" || word == "start") { position = UiTimingFunction::StepPosition::JumpStart; }
        else if (word == "jump-end" || word == "end") { position = UiTimingFunction::StepPosition::JumpEnd; }
        else if (word == "jump-none") { position = UiTimingFunction::StepPosition::JumpNone; }
        else if (word == "jump-both") { position = UiTimingFunction::StepPosition::JumpBoth; }
        else { return false; }
      }

      // without a jump at either end, one step would never move
      if (position == UiTimingFunction::StepPosition::JumpNone && count < 2.0f) { return false; }

      function = UiTimingFunction::Steps(static_cast<int>(count), position);
    } else
    {
      return false;
    }

    return true;
  }

  bool ParseCssTime(const std::string &text, float &seconds)
  {
    const std::string value = Tidied(text);
    float number = 0.0f;

    if (value.ends_with("ms"))
    {
      if (!ParseCssNumber(value.substr(0, value.size() - 2), number)) { return false; }
      seconds = number / 1000.0f;
      return true;
    }

    if (value.ends_with("s"))
    {
      if (!ParseCssNumber(value.substr(0, value.size() - 1), number)) { return false; }
      seconds = number;
      return true;
    }

    // a time has a unit, as CSS says, even when it is 0
    return false;
  }

  std::string FormatCssTimingFunction(const UiTimingFunction &function)
  {
    switch (function.kind)
    {
      case UiTimingFunction::Kind::Linear:
        return "linear";

      case UiTimingFunction::Kind::Steps:
      {
        const char *positions[] = {"jump-start", "jump-end", "jump-none", "jump-both"};
        return std::format("steps({}, {})", function.steps, positions[static_cast<int>(function.step_position)]);
      }

      case UiTimingFunction::Kind::CubicBezier:
        break;
    }

    if (function == UiTimingFunction::Ease()) { return "ease"; }
    if (function == UiTimingFunction::EaseIn()) { return "ease-in"; }
    if (function == UiTimingFunction::EaseOut()) { return "ease-out"; }
    if (function == UiTimingFunction::EaseInOut()) { return "ease-in-out"; }

    return std::format(
      "cubic-bezier({}, {}, {}, {})",
      FormatCssNumber(function.x1), FormatCssNumber(function.y1),
      FormatCssNumber(function.x2), FormatCssNumber(function.y2));
  }
} // neon
