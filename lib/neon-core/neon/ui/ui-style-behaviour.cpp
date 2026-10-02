#include "ui-style.hpp"

#include <algorithm>
#include <cctype>
#include <format>

#include "css-values.hpp"

// The properties of behaviour: what is scrolled, animated, and pointed at.
// They follow https://www.w3.org/TR/css-overflow-3/,
// https://www.w3.org/TR/css-scrollbars-1/,
// https://www.w3.org/TR/css-transitions-1/,
// https://www.w3.org/TR/css-animations-1/, and
// https://www.w3.org/TR/css-ui-4/.

namespace neon
{
  // Helpers of ui-style-behaviour.cpp, for this file alone.
  namespace
  {
    const std::vector<std::string> visibilities = {"visible", "hidden"};
    const std::vector<std::string> overflows = {"visible", "hidden", "scroll", "auto"};
    const std::vector<std::string> scrollbar_widths = {"auto", "thin", "none"};
    const std::vector<std::string> scroll_behaviors = {"auto", "smooth"};
    const std::vector<std::string> scroll_drags = {"none", "inertia"};
    const std::vector<std::string> directions = {"normal", "reverse", "alternate", "alternate-reverse"};
    const std::vector<std::string> fill_modes = {"none", "forwards", "backwards", "both"};
    const std::vector<std::string> play_states = {"running", "paused"};

    const std::vector<std::string> cursors = {
      "auto", "default", "pointer", "text", "wait", "progress", "crosshair", "move", "not-allowed",
      "ew-resize", "ns-resize", "nesw-resize", "nwse-resize", "grab", "grabbing", "none"
    };

    const std::string time_expected = "a time such as 0.2s or 150ms";
    const std::string timing_expected =
      "linear, ease, ease-in, ease-out, ease-in-out, step-start, step-end, cubic-bezier(), or steps()";

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

    std::string Joined(const std::vector<std::string> &words)
    {
      std::string joined;
      for (const auto &word : words)
      {
        if (!joined.empty()) { joined += ", "; }
        joined += word;
      }
      return joined;
    }

    std::string Describe(const DataValue &value)
    {
      if (std::string text; value.GetText(text)) { return "'" + text + "'"; }
      return DataValue::Describe(value.GetKind());
    }

    /// What is set apart by commas that are outside of brackets.
    std::vector<std::string> SplitAtCommas(const std::string &text)
    {
      std::vector<std::string> parts;
      std::string part;
      int depth = 0;

      for (const char letter : text)
      {
        if (letter == '(') { depth++; }
        if (letter == ')') { depth = std::max(0, depth - 1); }

        if (letter == ',' && depth == 0)
        {
          parts.push_back(Trimmed(part));
          part.clear();
        } else
        {
          part += letter;
        }
      }

      parts.push_back(Trimmed(part));
      return parts;
    }

    /// The name of a property as CSS writes it, from either way of
    /// writing it.
    std::string AsCssName(std::string name)
    {
      name = Lowered(Trimmed(name));
      std::ranges::replace(name, '_', '-');
      return name;
    }

    bool IsName(const std::string &text)
    {
      if (text.empty() || std::isdigit(static_cast<unsigned char>(text[0])) != 0) { return false; }

      return std::ranges::all_of(text, [](const char letter)
      {
        const auto byte = static_cast<unsigned char>(letter);
        return std::isalnum(byte) != 0 || letter == '-' || letter == '_' || byte >= 0x80;
      });
    }

    class Behaviour
    {
      const DataReader &_reader;

      void Expected(const std::string &name, const DataValue &value, const std::string &expected) const
      {
        _reader.Report(value, std::format(
                         "'{}' of {} is {}, where {} was expected",
                         name, _reader.GetWhere(), Describe(value), expected));
      }

      /// The items of a list: a list, or text with commas in it, or one
      /// value alone. A number is written as text.
      static std::vector<std::string> Items(const DataValue &value, bool &is_read)
      {
        is_read = true;
        std::vector<std::string> items;

        const auto as_text = [](const DataValue &each, std::string &text)
        {
          if (each.GetText(text)) { return true; }

          if (float number = 0.0f; each.GetNumber(number))
          {
            text = std::format("{}", number);
            return true;
          }
          return false;
        };

        if (value.IsList())
        {
          for (const auto &item : value.GetItems())
          {
            std::string text;
            if (!as_text(item, text)) { is_read = false; }
            items.push_back(Trimmed(text));
          }
        } else if (std::string text; as_text(value, text))
        {
          items = SplitAtCommas(text);
        } else
        {
          is_read = false;
        }

        if (items.empty() || std::ranges::any_of(items, [](const std::string &item) { return item.empty(); }))
        {
          is_read = false;
        }

        return items;
      }

      /// A time in seconds. A number alone counts as seconds, which is
      /// what a file of YAML writes without quotes.
      static bool AsTime(const std::string &text, float &seconds)
      {
        if (ParseCssTime(text, seconds)) { return true; }
        return ParseCssNumber(text, seconds);
      }

    public:
      explicit Behaviour(const DataReader &reader) : _reader(reader) {}

      template<typename T>
      bool Keyword(const std::string &name, const std::vector<std::string> &keywords, T &value) const
      {
        std::size_t index = 0;
        if (!_reader.ReadChoice(name, keywords, index)) { return false; }

        value = static_cast<T>(index);
        return true;
      }

      /// What `overflow` holds, without asking for it: the properties
      /// that were there before ask for it and report what is wrong.
      void OverflowOfBoth(const UiOverflow overflow, UiStyle &style) const
      {
        if (!_reader.Has("overflow")) { return; }

        style.overflow_x = overflow;
        style.overflow_y = overflow;
      }

      bool OptionalColour(const std::string &name, std::optional<Color> &color) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        std::string text;
        if (value->GetText(text))
        {
          const std::string lowered = Lowered(Trimmed(text));

          if (lowered == "auto" || lowered == "currentcolor")
          {
            color.reset();
            return true;
          }

          if (Color read; ParseCssColor(text, read))
          {
            color = read;
            return true;
          }
        }

        Expected(name, *value, "auto, or a colour such as \"#ff8000\" or rgb(255, 128, 0)");
        return false;
      }

      /// `scrollbar_color` of CSS: `auto`, or the colour of what is
      /// dragged and the colour of what it is dragged along.
      bool ScrollbarColor(UiStyle &style) const
      {
        const auto *value = _reader.ReadValue("scrollbar_color");
        if (value == nullptr) { return false; }

        const std::string expected =
          "auto, or two colours: that of what is dragged and that of what it is dragged along, such as "
          "\"#ffffff80 #00000040\"";

        std::vector<std::string> parts;

        if (std::string text; value->GetText(text))
        {
          parts = SplitCssValues(text);
        } else if (value->IsList())
        {
          for (const auto &item : value->GetItems())
          {
            std::string each;
            if (item.GetText(each)) { parts.push_back(each); }
          }
        }

        if (parts.size() == 1 && Lowered(parts[0]) == "auto")
        {
          style.scrollbar_thumb_color.reset();
          style.scrollbar_track_color.reset();
          return true;
        }

        Color thumb;
        Color track;

        if (parts.size() != 2 || !ParseCssColor(parts[0], thumb) || !ParseCssColor(parts[1], track))
        {
          Expected("scrollbar_color", *value, expected);
          return false;
        }

        style.scrollbar_thumb_color = thumb;
        style.scrollbar_track_color = track;
        return true;
      }

      bool Times(const std::string &name, std::vector<float> &times, const bool allows_negative) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        bool is_read = false;
        const auto items = Items(*value, is_read);
        std::vector<float> read;

        for (const auto &item : items)
        {
          float seconds = 0.0f;
          if (!AsTime(item, seconds) || (seconds < 0.0f && !allows_negative)) { is_read = false; }
          read.push_back(seconds);
        }

        if (!is_read)
        {
          Expected(name, *value, allows_negative
                     ? time_expected + ", or a list of them"
                     : time_expected + " that is not below 0, or a list of them");
          return false;
        }

        times = read;
        return true;
      }

      bool TimingFunctions(const std::string &name, std::vector<UiTimingFunction> &functions) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        bool is_read = false;
        const auto items = Items(*value, is_read);
        std::vector<UiTimingFunction> read;

        for (const auto &item : items)
        {
          UiTimingFunction function;
          if (!ParseCssTimingFunction(item, function)) { is_read = false; }
          read.push_back(function);
        }

        if (!is_read)
        {
          Expected(name, *value, timing_expected + ", or a list of them");
          return false;
        }

        functions = read;
        return true;
      }

      bool Names(const std::string &name, std::vector<std::string> &names, const bool are_properties) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        bool is_read = false;
        const auto items = Items(*value, is_read);
        std::vector<std::string> read;

        for (const auto &item : items)
        {
          if (!IsName(item)) { is_read = false; }
          read.push_back(are_properties ? AsCssName(item) : item);
        }

        if (!is_read)
        {
          Expected(name, *value, are_properties
                     ? "all, none, or the names of properties such as opacity, background-color"
                     : "none, or the names of animations");
          return false;
        }

        names = read;
        return true;
      }

      template<typename T>
      bool Keywords(const std::string &name, const std::vector<std::string> &keywords, std::vector<T> &values) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        bool is_read = false;
        const auto items = Items(*value, is_read);
        std::vector<T> read;

        for (const auto &item : items)
        {
          const auto found = std::ranges::find(keywords, Lowered(item));
          if (found == keywords.end())
          {
            is_read = false;
            continue;
          }
          read.push_back(static_cast<T>(found - keywords.begin()));
        }

        if (!is_read)
        {
          Expected(name, *value, "one of these, or a list of them: " + Joined(keywords));
          return false;
        }

        values = read;
        return true;
      }

      bool IterationCounts(std::vector<float> &counts) const
      {
        const std::string name = "animation_iteration_count";

        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        bool is_read = false;
        const auto items = Items(*value, is_read);
        std::vector<float> read;

        for (const auto &item : items)
        {
          float count = 0.0f;

          if (Lowered(item) == "infinite")
          {
            read.push_back(-1.0f);
          } else if (ParseCssNumber(item, count) && count >= 0.0f)
          {
            read.push_back(count);
          } else
          {
            is_read = false;
          }
        }

        if (!is_read)
        {
          Expected(name, *value, "infinite, or a number that is not below 0, or a list of them");
          return false;
        }

        counts = read;
        return true;
      }

      /// `transition` of CSS: for each transition a property, a duration,
      /// a timing function, and a delay, in any order and each of them
      /// optional. The first time is the duration.
      bool Transition(UiTransitions &transitions) const
      {
        const auto *value = _reader.ReadValue("transition");
        if (value == nullptr) { return false; }

        const std::string expected =
          "none, or for each transition a property, a duration, a timing function, and a delay, such as "
          "\"opacity 0.2s ease-in, color 1s\"";

        bool is_read = false;
        const auto items = Items(*value, is_read);

        if (!is_read)
        {
          Expected("transition", *value, expected);
          return false;
        }

        UiTransitions read;
        read.properties.clear();
        read.durations.clear();
        read.delays.clear();
        read.timing_functions.clear();

        for (const auto &item : items)
        {
          std::string property = "all";
          float duration = 0.0f;
          float delay = 0.0f;
          UiTimingFunction function = UiTimingFunction::Ease();

          int times = 0;
          bool has_property = false;
          bool has_function = false;

          for (const auto &part : SplitCssValues(item))
          {
            float seconds = 0.0f;
            UiTimingFunction parsed;

            if (ParseCssTime(part, seconds))
            {
              if (times == 0) { duration = seconds; }
              else if (times == 1) { delay = seconds; }
              else { is_read = false; }
              times++;
            } else if (!has_function && ParseCssTimingFunction(part, parsed))
            {
              function = parsed;
              has_function = true;
            } else if (!has_property && IsName(part))
            {
              property = AsCssName(part);
              has_property = true;
            } else
            {
              is_read = false;
            }
          }

          if (duration < 0.0f) { is_read = false; }

          // `none` stands alone
          if (property == "none" && items.size() > 1) { is_read = false; }

          read.properties.push_back(property);
          read.durations.push_back(duration);
          read.delays.push_back(delay);
          read.timing_functions.push_back(function);
        }

        if (!is_read)
        {
          Expected("transition", *value, expected);
          return false;
        }

        transitions = read;
        return true;
      }

      /// `animation` of CSS: for each animation its name, a duration, a
      /// timing function, a delay, how often it runs, its direction, what
      /// holds in front of it and behind it, and whether it runs, in any
      /// order and each of them optional.
      bool Animation(UiAnimations &animations) const
      {
        const auto *value = _reader.ReadValue("animation");
        if (value == nullptr) { return false; }

        const std::string expected =
          "none, or for each animation its name, a duration, a timing function, a delay, how often it runs, "
          "its direction, its fill mode, and whether it runs, such as \"fade-in 0.3s ease-out both\"";

        bool is_read = false;
        const auto items = Items(*value, is_read);

        if (!is_read)
        {
          Expected("animation", *value, expected);
          return false;
        }

        UiAnimations read;
        read.names.clear();
        read.durations.clear();
        read.delays.clear();
        read.iteration_counts.clear();
        read.directions.clear();
        read.fill_modes.clear();
        read.play_states.clear();
        read.timing_functions.clear();

        for (const auto &item : items)
        {
          std::string name = "none";
          float duration = 0.0f;
          float delay = 0.0f;
          float count = 1.0f;
          auto direction = UiAnimationDirection::Normal;
          auto fill_mode = UiAnimationFillMode::None;
          auto play_state = UiAnimationPlayState::Running;
          UiTimingFunction function = UiTimingFunction::Ease();

          int times = 0;
          bool has_name = false;
          bool has_function = false;
          bool has_count = false;
          bool has_direction = false;
          bool has_fill_mode = false;
          bool has_play_state = false;

          const auto find = [](const std::vector<std::string> &keywords, const std::string &word, int &index)
          {
            const auto found = std::ranges::find(keywords, Lowered(word));
            if (found == keywords.end()) { return false; }

            index = static_cast<int>(found - keywords.begin());
            return true;
          };

          for (const auto &part : SplitCssValues(item))
          {
            float number = 0.0f;
            int index = 0;
            UiTimingFunction parsed;

            // As CSS says, a word is taken for what it can be other than
            // a name first, so an animation that is called `reverse` is
            // named by its longhand.
            if (ParseCssTime(part, number))
            {
              if (times == 0) { duration = number; }
              else if (times == 1) { delay = number; }
              else { is_read = false; }
              times++;
            } else if (!has_function && ParseCssTimingFunction(part, parsed))
            {
              function = parsed;
              has_function = true;
            } else if (!has_count && Lowered(part) == "infinite")
            {
              count = -1.0f;
              has_count = true;
            } else if (!has_count && ParseCssNumber(part, number) && number >= 0.0f)
            {
              count = number;
              has_count = true;
            } else if (!has_direction && find(directions, part, index))
            {
              direction = static_cast<UiAnimationDirection>(index);
              has_direction = true;
            } else if (!has_fill_mode && Lowered(part) != "none" && find(fill_modes, part, index))
            {
              fill_mode = static_cast<UiAnimationFillMode>(index);
              has_fill_mode = true;
            } else if (!has_play_state && find(play_states, part, index))
            {
              play_state = static_cast<UiAnimationPlayState>(index);
              has_play_state = true;
            } else if (!has_name && IsName(part))
            {
              name = part;
              has_name = true;
            } else
            {
              is_read = false;
            }
          }

          if (duration < 0.0f) { is_read = false; }

          read.names.push_back(name);
          read.durations.push_back(duration);
          read.delays.push_back(delay);
          read.iteration_counts.push_back(count);
          read.directions.push_back(direction);
          read.fill_modes.push_back(fill_mode);
          read.play_states.push_back(play_state);
          read.timing_functions.push_back(function);
        }

        if (!is_read)
        {
          Expected("animation", *value, expected);
          return false;
        }

        animations = read;
        return true;
      }
    };
  }

  void ReadUiBehaviourStyle(const DataReader &reader, UiStyle &style)
  {
    const Behaviour behaviour(reader);

    behaviour.Keyword("visibility", visibilities, style.visibility);
    behaviour.Keyword("cursor", cursors, style.cursor);

    // the shorthand in front of what it stands for
    behaviour.OverflowOfBoth(style.overflow, style);
    behaviour.Keyword("overflow_x", overflows, style.overflow_x);
    behaviour.Keyword("overflow_y", overflows, style.overflow_y);

    behaviour.Keyword("scrollbar_width", scrollbar_widths, style.scrollbar_width);
    behaviour.ScrollbarColor(style);
    behaviour.Keyword("scroll_behavior", scroll_behaviors, style.scroll_behavior);
    behaviour.Keyword("scroll_drag", scroll_drags, style.scroll_drag);

    behaviour.OptionalColour("caret_color", style.caret_color);

    behaviour.Transition(style.transitions);
    behaviour.Names("transition_property", style.transitions.properties, true);
    behaviour.Times("transition_duration", style.transitions.durations, false);
    behaviour.Times("transition_delay", style.transitions.delays, true);
    behaviour.TimingFunctions("transition_timing_function", style.transitions.timing_functions);

    behaviour.Animation(style.animations);
    behaviour.Names("animation_name", style.animations.names, false);
    behaviour.Times("animation_duration", style.animations.durations, false);
    behaviour.Times("animation_delay", style.animations.delays, true);
    behaviour.TimingFunctions("animation_timing_function", style.animations.timing_functions);
    behaviour.IterationCounts(style.animations.iteration_counts);
    behaviour.Keywords("animation_direction", directions, style.animations.directions);
    behaviour.Keywords("animation_fill_mode", fill_modes, style.animations.fill_modes);
    behaviour.Keywords("animation_play_state", play_states, style.animations.play_states);
  }
} // neon
