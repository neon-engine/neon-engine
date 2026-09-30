#include "ui-properties.hpp"

#include <algorithm>
#include <cmath>
#include <format>

#include <neon/data/data-reader.hpp>

#include "css/css-expression.hpp"

namespace neon
{
  namespace
  {
    const std::vector<std::string> displays = {"flex", "none"};
    const std::vector<std::string> positions = {"relative", "absolute"};
    const std::vector<std::string> box_sizings = {"content-box", "border-box"};
    const std::vector<std::string> flex_directions = {"row", "row-reverse", "column", "column-reverse"};
    const std::vector<std::string> flex_wraps = {"nowrap", "wrap", "wrap-reverse"};
    const std::vector<std::string> justify_contents = {
      "flex-start", "flex-end", "center", "space-between", "space-around", "space-evenly"
    };
    const std::vector<std::string> align_items = {"stretch", "flex-start", "flex-end", "center"};
    const std::vector<std::string> align_selfs = {"auto", "stretch", "flex-start", "flex-end", "center"};
    const std::vector<std::string> align_contents = {
      "stretch", "flex-start", "flex-end", "center", "space-between", "space-around", "space-evenly"
    };
    const std::vector<std::string> overflows = {"visible", "hidden", "scroll", "auto"};
    const std::vector<std::string> pointer_events = {"auto", "none"};
    const std::vector<std::string> text_aligns = {"left", "center", "right"};
    const std::vector<std::string> object_fits = {"fill", "contain", "cover"};
    const std::vector<std::string> visibilities = {"visible", "hidden"};
    const std::vector<std::string> scrollbar_widths = {"auto", "thin", "none"};
    const std::vector<std::string> scroll_behaviors = {"auto", "smooth"};
    const std::vector<std::string> scroll_drags = {"none", "inertia"};
    const std::vector<std::string> directions = {"normal", "reverse", "alternate", "alternate-reverse"};
    const std::vector<std::string> fill_modes = {"none", "forwards", "backwards", "both"};
    const std::vector<std::string> play_states = {"running", "paused"};
    const std::vector<std::string> text_transforms = {"none", "uppercase", "lowercase", "capitalize"};
    const std::vector<std::string> white_spaces = {"normal", "nowrap", "pre", "pre-wrap", "pre-line"};
    const std::vector<std::string> text_overflows = {"clip", "ellipsis"};
    const std::vector<std::string> text_directions = {"ltr", "rtl"};
    const std::vector<std::string> image_renderings = {"auto", "pixelated"};
    const std::vector<std::string> background_repeats = {"repeat", "no-repeat", "repeat-x", "repeat-y"};
    const std::vector<std::string> border_image_repeats = {"stretch", "repeat", "round"};
    const std::vector<std::string> cursors = {
      "auto", "default", "pointer", "text", "wait", "progress", "crosshair", "move", "not-allowed",
      "ew-resize", "ns-resize", "nesw-resize", "nwse-resize", "grab", "grabbing", "none"
    };

    constexpr bool inherited = true;
    constexpr bool not_inherited = false;
    constexpr bool layout = true;
    constexpr bool no_layout = false;

    /// Writes the table, a property at a time.
    class Table
    {
      std::vector<UiProperty> &_properties;

      UiProperty &Add(
        const std::string &name,
        const UiValueKind kind,
        const bool is_inherited,
        const bool affects_layout,
        const std::string &description)
      {
        auto &property = _properties.emplace_back();
        property.name = name;
        property.yaml_name = UiProperties::ToYamlName(name);
        property.kind = kind;
        property.is_inherited = is_inherited;
        property.affects_layout = affects_layout;
        property.description = description;
        return property;
      }

    public:
      explicit Table(std::vector<UiProperty> &properties) : _properties(properties) {}

      template<typename Reach>
      void Number(
        const std::string &name,
        Reach reach,
        const std::string &unit,
        const bool is_inherited,
        const bool affects_layout,
        const std::string &description)
      {
        auto &property = Add(name, UiValueKind::Number, is_inherited, affects_layout, description);
        property.unit = unit;

        property.get = [reach](const UiStyle &style)
        {
          UiPropertyValue value;
          value.kind = UiValueKind::Number;
          value.number = reach(const_cast<UiStyle &>(style));
          return value;
        };

        property.set = [reach](UiStyle &style, const UiPropertyValue &value) { reach(style) = value.number; };
      }

      template<typename Reach>
      void Whole(
        const std::string &name,
        Reach reach,
        const bool is_inherited,
        const bool affects_layout,
        const std::string &description)
      {
        auto &property = Add(name, UiValueKind::Whole, is_inherited, affects_layout, description);

        property.get = [reach](const UiStyle &style)
        {
          UiPropertyValue value;
          value.kind = UiValueKind::Whole;
          value.number = static_cast<float>(reach(const_cast<UiStyle &>(style)));
          return value;
        };

        property.set = [reach](UiStyle &style, const UiPropertyValue &value)
        {
          reach(style) = static_cast<int>(std::lround(value.number));
        };
      }

      template<typename Reach>
      void Length(const std::string &name, Reach reach, const std::string &description)
      {
        auto &property = Add(name, UiValueKind::Length, not_inherited, layout, description);
        property.unit = "px";

        property.get = [reach](const UiStyle &style)
        {
          UiPropertyValue value;
          value.kind = UiValueKind::Length;
          value.length = reach(const_cast<UiStyle &>(style));
          return value;
        };

        property.set = [reach](UiStyle &style, const UiPropertyValue &value) { reach(style) = value.length; };
      }

      template<typename Reach>
      void Colour(
        const std::string &name,
        Reach reach,
        const bool is_inherited,
        const std::string &description)
      {
        auto &property = Add(name, UiValueKind::Color, is_inherited, no_layout, description);

        property.get = [reach](const UiStyle &style)
        {
          UiPropertyValue value;
          value.kind = UiValueKind::Color;
          value.color = reach(const_cast<UiStyle &>(style));
          value.flag = true;
          return value;
        };

        property.set = [reach](UiStyle &style, const UiPropertyValue &value) { reach(style) = value.color; };
      }

      /// A colour that follows that of the text unless it is set, which
      /// CSS calls `currentcolor`.
      template<typename Reach>
      void ColourOfTheText(
        const std::string &name,
        Reach reach,
        const bool is_inherited,
        const std::string &description)
      {
        auto &property = Add(name, UiValueKind::Color, is_inherited, no_layout, description);

        property.get = [reach](const UiStyle &style)
        {
          const std::optional<Color> &color = reach(const_cast<UiStyle &>(style));

          UiPropertyValue value;
          value.kind = UiValueKind::Color;
          value.color = color.value_or(style.color);
          value.flag = color.has_value();
          return value;
        };

        property.set = [reach](UiStyle &style, const UiPropertyValue &value)
        {
          if (value.flag)
          {
            reach(style) = value.color;
          } else
          {
            reach(style).reset();
          }
        };
      }

      template<typename Reach>
      void Keyword(
        const std::string &name,
        Reach reach,
        const std::vector<std::string> &keywords,
        const bool is_inherited,
        const bool affects_layout,
        const std::string &description)
      {
        auto &property = Add(name, UiValueKind::Keyword, is_inherited, affects_layout, description);
        property.keywords = keywords;

        property.get = [reach](const UiStyle &style)
        {
          UiPropertyValue value;
          value.kind = UiValueKind::Keyword;
          value.keyword = static_cast<int>(reach(const_cast<UiStyle &>(style)));
          return value;
        };

        property.set = [reach](UiStyle &style, const UiPropertyValue &value)
        {
          using Enum = std::remove_reference_t<decltype(reach(style))>;
          reach(style) = static_cast<Enum>(value.keyword);
        };
      }

      template<typename Reach>
      void Text(
        const std::string &name,
        Reach reach,
        const bool is_inherited,
        const bool affects_layout,
        const std::string &description)
      {
        auto &property = Add(name, UiValueKind::Text, is_inherited, affects_layout, description);

        property.get = [reach](const UiStyle &style)
        {
          UiPropertyValue value;
          value.kind = UiValueKind::Text;
          value.text = reach(const_cast<UiStyle &>(style));
          return value;
        };

        property.set = [reach](UiStyle &style, const UiPropertyValue &value) { reach(style) = value.text; };
      }

      template<typename Reach>
      void Edges(
        const std::string &name,
        Reach reach,
        const bool affects_layout,
        const std::string &description)
      {
        auto &property = Add(name, UiValueKind::Edges, not_inherited, affects_layout, description);
        property.unit = "px";

        property.get = [reach](const UiStyle &style)
        {
          const LayoutEdges<float> &edges = reach(const_cast<UiStyle &>(style));

          UiPropertyValue value;
          value.kind = UiValueKind::Edges;
          value.edges = {edges.top, edges.right, edges.bottom, edges.left};
          return value;
        };

        property.set = [reach](UiStyle &style, const UiPropertyValue &value)
        {
          reach(style) = {value.edges[0], value.edges[1], value.edges[2], value.edges[3]};
        };
      }

      /// Something of its own, such as a list. A value holds it as text
      /// alone, so it is taken from one style to another by `copy`.
      void Other(
        const std::string &name,
        const std::function<void(const UiStyle &from, UiStyle &to)> &copy,
        const std::function<std::string(const UiStyle &)> &format,
        const std::string &description)
      {
        auto &property = Add(name, UiValueKind::Other, not_inherited, no_layout, description);
        property.copy = copy;

        property.get = [format](const UiStyle &style)
        {
          UiPropertyValue value;
          value.kind = UiValueKind::Other;
          value.text = format(style);
          return value;
        };

        property.set = [](UiStyle &, const UiPropertyValue &) {};
      }

      void Shorthand(
        const std::string &name,
        const std::vector<std::string> &longhands,
        const std::string &description)
      {
        auto &property = Add(name, UiValueKind::Other, not_inherited, no_layout, description);
        property.longhands = longhands;

        property.get = [](const UiStyle &)
        {
          UiPropertyValue value;
          value.kind = UiValueKind::Other;
          return value;
        };

        property.set = [](UiStyle &, const UiPropertyValue &) {};
      }

      UiProperty &Last()
      {
        return _properties.back();
      }
    };

    template<typename T, typename Format>
    std::string Listed(const std::vector<T> &items, Format format)
    {
      std::string listed;
      for (const auto &item : items)
      {
        if (!listed.empty()) { listed += ", "; }
        listed += format(item);
      }
      return listed;
    }

    std::string Seconds(const float seconds)
    {
      return FormatCssNumber(seconds) + "s";
    }

    std::string Pixels(const float pixels)
    {
      return FormatCssNumber(pixels) + "px";
    }

    std::string FormatPlace(const UiPlace &place)
    {
      return FormatCssLength(place.x) + " " + FormatCssLength(place.y);
    }

    /// Shadows as CSS writes them: to the right, down, the blur, the
    /// spread of a box, and the colour when one was given.
    std::string FormatShadows(const std::vector<UiShadow> &shadows, const bool with_spread)
    {
      if (shadows.empty()) { return "none"; }

      return Listed(shadows, [with_spread](const UiShadow &shadow)
      {
        std::string text = Pixels(shadow.offset_x) + " " + Pixels(shadow.offset_y) + " " + Pixels(shadow.blur);
        if (with_spread) { text += " " + Pixels(shadow.spread); }
        if (shadow.has_color) { text += " " + FormatCssColor(shadow.color); }
        return text;
      });
    }

    std::string FormatBackgroundSize(const UiBackgroundSize &size)
    {
      switch (size.kind)
      {
        case UiBackgroundSize::Kind::Cover: return "cover";
        case UiBackgroundSize::Kind::Contain: return "contain";
        case UiBackgroundSize::Kind::Lengths: return FormatCssLength(size.width) + " " + FormatCssLength(size.height);
        default: return "auto";
      }
    }

    std::string FormatTransform(const std::vector<UiTransformStep> &steps)
    {
      if (steps.empty()) { return "none"; }

      std::string text;
      for (const auto &step : steps)
      {
        if (!text.empty()) { text += ' '; }

        switch (step.kind)
        {
          case UiTransformStep::Kind::Translate:
            text += "translate(" + FormatCssLength(step.x) + ", " + FormatCssLength(step.y) + ")";
            break;
          case UiTransformStep::Kind::Rotate:
            text += "rotate(" + FormatCssNumber(step.angle) + "deg)";
            break;
          case UiTransformStep::Kind::Scale:
            text += "scale(" + FormatCssNumber(step.x.value) + ", " + FormatCssNumber(step.y.value) + ")";
            break;
        }
      }
      return text;
    }

    std::string FormatShaderValues(const std::vector<UiShaderValue> &values)
    {
      if (values.empty()) { return "none"; }

      return Listed(values, [](const UiShaderValue &value)
      {
        if (!value.bound_to.empty()) { return value.name + ": ${" + value.bound_to + "}"; }

        std::string numbers;
        for (std::size_t i = 0; i < value.count && i < 4; i++)
        {
          if (!numbers.empty()) { numbers += ' '; }
          numbers += FormatCssNumber(value.numbers[i]);
        }
        return value.name + ": " + numbers;
      });
    }

    /// The colour of one side of the border, which is that of the border
    /// unless the side has one of its own.
    template<typename Reach>
    std::string FormatSideColor(const UiStyle &style, Reach reach)
    {
      const std::optional<Color> &color = reach(const_cast<UiStyle &>(style));
      return FormatCssColor(color.value_or(style.BorderColor()));
    }

    /// How far two names are apart: the letters that have to be changed,
    /// added, or left out to get from one to the other.
    std::size_t Distance(const std::string &a, const std::string &b)
    {
      std::vector<std::size_t> row(b.size() + 1);
      for (std::size_t i = 0; i <= b.size(); i++) { row[i] = i; }

      for (std::size_t i = 1; i <= a.size(); i++)
      {
        std::size_t corner = row[0];
        row[0] = i;

        for (std::size_t k = 1; k <= b.size(); k++)
        {
          const std::size_t above = row[k];
          row[k] = std::min({row[k] + 1, row[k - 1] + 1, corner + (a[i - 1] == b[k - 1] ? 0 : 1)});
          corner = above;
        }
      }

      return row[b.size()];
    }
  }

  bool UiPropertyValue::operator==(const UiPropertyValue &other) const
  {
    if (kind != other.kind) { return false; }

    switch (kind)
    {
      case UiValueKind::Number:
      case UiValueKind::Whole:
        return number == other.number && flag == other.flag;
      case UiValueKind::Length:
        return length == other.length;
      case UiValueKind::Color:
        return color.r == other.color.r && color.g == other.color.g && color.b == other.color.b &&
               color.a == other.color.a && flag == other.flag;
      case UiValueKind::Keyword:
        return keyword == other.keyword;
      case UiValueKind::Text:
      case UiValueKind::Other:
        return text == other.text;
      case UiValueKind::Edges:
        return edges == other.edges;
    }

    return false;
  }

  void UiProperty::Copy(const UiStyle &from, UiStyle &to) const
  {
    if (copy)
    {
      copy(from, to);
    } else if (get && set)
    {
      set(to, get(from));
    }
  }

  std::string FormatCssColor(const Color &color)
  {
    const auto byte = [](const float part)
    {
      return static_cast<int>(std::lround(std::clamp(part, 0.0f, 1.0f) * 255.0f));
    };

    if (color.a >= 1.0f) { return std::format("rgb({}, {}, {})", byte(color.r), byte(color.g), byte(color.b)); }

    return std::format(
      "rgba({}, {}, {}, {})",
      byte(color.r), byte(color.g), byte(color.b), FormatCssNumber(std::clamp(color.a, 0.0f, 1.0f)));
  }

  std::string FormatCssLength(const LayoutLength &length)
  {
    switch (length.unit)
    {
      case LayoutLength::Unit::Pixels:
        return FormatCssNumber(length.value) + "px";
      case LayoutLength::Unit::Percent:
        return FormatCssNumber(length.value) + "%";
      case LayoutLength::Unit::Sum:
        return "calc(" + FormatCssNumber(length.percent) + "% + " + FormatCssNumber(length.value) + "px)";
      default:
        return "auto";
    }
  }

  std::string UiProperty::Format(const UiPropertyValue &value) const
  {
    switch (kind)
    {
      case UiValueKind::Number:
        if (name == "line-height")
        {
          if (value.number <= 0.0f) { return "normal"; }
          return value.flag ? FormatCssNumber(value.number) : FormatCssNumber(value.number) + "px";
        }
        return FormatCssNumber(value.number) + unit;

      case UiValueKind::Whole:
        return std::format("{}", static_cast<int>(std::lround(value.number)));

      case UiValueKind::Length:
        // no limit is written as `none`
        if (value.length.IsAuto() && (name == "max-width" || name == "max-height")) { return "none"; }
        return FormatCssLength(value.length);

      case UiValueKind::Color:
        return FormatCssColor(value.color);

      case UiValueKind::Keyword:
        return value.keyword >= 0 && static_cast<std::size_t>(value.keyword) < keywords.size()
          ? keywords[static_cast<std::size_t>(value.keyword)]
          : "";

      case UiValueKind::Text:
        if (value.text.empty() && name != "font-family") { return "none"; }
        return value.text;

      case UiValueKind::Edges:
      {
        std::string written;
        for (const float edge : value.edges)
        {
          if (!written.empty()) { written += ' '; }
          written += FormatCssNumber(edge) + unit;
        }
        return written;
      }

      case UiValueKind::Other:
        return value.text;
    }

    return "";
  }

  UiProperties::UiProperties()
  {
    Table table(_properties);

    // The properties of behaviour.

    table.Keyword(
      "visibility", [](UiStyle &style) -> UiVisibility & { return style.visibility; },
      visibilities, inherited, no_layout,
      "Whether the element is drawn. One that is hidden keeps its room");

    table.Keyword(
      "cursor", [](UiStyle &style) -> UiCursor & { return style.cursor; },
      cursors, inherited, no_layout,
      "The shape of the cursor over the element");

    table.Shorthand("overflow", {"overflow-x", "overflow-y"}, "What happens to what does not fit");

    table.Keyword(
      "overflow-x", [](UiStyle &style) -> UiOverflow & { return style.overflow_x; },
      overflows, not_inherited, layout,
      "What happens to what does not fit along the width");

    table.Keyword(
      "overflow-y", [](UiStyle &style) -> UiOverflow & { return style.overflow_y; },
      overflows, not_inherited, layout,
      "What happens to what does not fit along the height");

    table.Keyword(
      "scrollbar-width", [](UiStyle &style) -> UiScrollbarWidth & { return style.scrollbar_width; },
      scrollbar_widths, not_inherited, layout,
      "How wide the scrollbars are, or none for no scrollbars");

    table.Other(
      "scrollbar-color",
      [](const UiStyle &from, UiStyle &to)
      {
        to.scrollbar_thumb_color = from.scrollbar_thumb_color;
        to.scrollbar_track_color = from.scrollbar_track_color;
      },
      [](const UiStyle &style)
      {
        if (!style.scrollbar_thumb_color.has_value() || !style.scrollbar_track_color.has_value())
        {
          return std::string("auto");
        }

        return FormatCssColor(*style.scrollbar_thumb_color) + " " + FormatCssColor(*style.scrollbar_track_color);
      },
      "The colour of what is dragged of a scrollbar, and of what it is dragged along");
    table.Last().is_inherited = true;

    table.Keyword(
      "scroll-behavior", [](UiStyle &style) -> UiScrollBehavior & { return style.scroll_behavior; },
      scroll_behaviors, not_inherited, no_layout,
      "Whether scrolling to a place takes a short time");

    table.Keyword(
      "scroll-drag", [](UiStyle &style) -> UiScrollDrag & { return style.scroll_drag; },
      scroll_drags, not_inherited, no_layout,
      "Whether what is inside is scrolled by dragging it. Not part of CSS");

    table.ColourOfTheText(
      "caret-color", [](UiStyle &style) -> std::optional<Color> & { return style.caret_color; },
      inherited,
      "The colour of the caret of a text that is typed");

    table.Shorthand(
      "transition",
      {"transition-property", "transition-duration", "transition-timing-function", "transition-delay"},
      "How a change takes its time");

    table.Other(
      "transition-property",
      [](const UiStyle &from, UiStyle &to) { to.transitions.properties = from.transitions.properties; },
      [](const UiStyle &style)
      {
        return Listed(style.transitions.properties, [](const std::string &name) { return name; });
      },
      "The properties whose changes take time");
    table.Other(
      "transition-duration",
      [](const UiStyle &from, UiStyle &to) { to.transitions.durations = from.transitions.durations; },
      [](const UiStyle &style) { return Listed(style.transitions.durations, Seconds); },
      "How long a change takes");

    table.Other(
      "transition-timing-function",
      [](const UiStyle &from, UiStyle &to) { to.transitions.timing_functions = from.transitions.timing_functions; },
      [](const UiStyle &style) { return Listed(style.transitions.timing_functions, FormatCssTimingFunction); },
      "How a change speeds up and slows down");

    table.Other(
      "transition-delay",
      [](const UiStyle &from, UiStyle &to) { to.transitions.delays = from.transitions.delays; },
      [](const UiStyle &style) { return Listed(style.transitions.delays, Seconds); },
      "How long a change waits before it starts");

    table.Shorthand(
      "animation",
      {
        "animation-name", "animation-duration", "animation-timing-function", "animation-delay",
        "animation-iteration-count", "animation-direction", "animation-fill-mode", "animation-play-state"
      },
      "The animations that run on the element");

    table.Other(
      "animation-name",
      [](const UiStyle &from, UiStyle &to) { to.animations.names = from.animations.names; },
      [](const UiStyle &style)
      {
        if (style.animations.names.empty()) { return std::string("none"); }
        return Listed(style.animations.names, [](const std::string &name) { return name; });
      },
      "The names of the keyframes that run");

    table.Other(
      "animation-duration",
      [](const UiStyle &from, UiStyle &to) { to.animations.durations = from.animations.durations; },
      [](const UiStyle &style) { return Listed(style.animations.durations, Seconds); },
      "How long one run takes");

    table.Other(
      "animation-timing-function",
      [](const UiStyle &from, UiStyle &to) { to.animations.timing_functions = from.animations.timing_functions; },
      [](const UiStyle &style) { return Listed(style.animations.timing_functions, FormatCssTimingFunction); },
      "How an animation speeds up and slows down between two keyframes");

    table.Other(
      "animation-delay",
      [](const UiStyle &from, UiStyle &to) { to.animations.delays = from.animations.delays; },
      [](const UiStyle &style) { return Listed(style.animations.delays, Seconds); },
      "How long an animation waits before it starts");

    table.Other(
      "animation-iteration-count",
      [](const UiStyle &from, UiStyle &to) { to.animations.iteration_counts = from.animations.iteration_counts; },
      [](const UiStyle &style)
      {
        return Listed(style.animations.iteration_counts, [](const float count)
        {
          return count < 0.0f ? std::string("infinite") : FormatCssNumber(count);
        });
      },
      "How often an animation runs");

    table.Other(
      "animation-direction",
      [](const UiStyle &from, UiStyle &to) { to.animations.directions = from.animations.directions; },
      [](const UiStyle &style)
      {
        return Listed(style.animations.directions, [](const UiAnimationDirection direction)
        {
          return directions[static_cast<std::size_t>(direction)];
        });
      },
      "Whether an animation runs forwards, backwards, or there and back");

    table.Other(
      "animation-fill-mode",
      [](const UiStyle &from, UiStyle &to) { to.animations.fill_modes = from.animations.fill_modes; },
      [](const UiStyle &style)
      {
        return Listed(style.animations.fill_modes, [](const UiAnimationFillMode mode)
        {
          return fill_modes[static_cast<std::size_t>(mode)];
        });
      },
      "What holds before an animation starts and after it ends");

    table.Other(
      "animation-play-state",
      [](const UiStyle &from, UiStyle &to) { to.animations.play_states = from.animations.play_states; },
      [](const UiStyle &style)
      {
        return Listed(style.animations.play_states, [](const UiAnimationPlayState state)
        {
          return play_states[static_cast<std::size_t>(state)];
        });
      },
      "Whether an animation runs or stands still");

    // The box model.

    table.Keyword(
      "display", [](UiStyle &style) -> LayoutDisplay & { return style.layout.display; },
      displays, not_inherited, layout, "Whether the element is there at all");

    table.Keyword(
      "position", [](UiStyle &style) -> LayoutPosition & { return style.layout.position; },
      positions, not_inherited, layout, "Whether the element is placed by its sides");

    table.Keyword(
      "box-sizing", [](UiStyle &style) -> LayoutBoxSizing & { return style.layout.box_sizing; },
      box_sizings, not_inherited, layout, "What width and height measure");

    table.Length("width", [](UiStyle &style) -> LayoutLength & { return style.layout.width; }, "The width");
    table.Length("height", [](UiStyle &style) -> LayoutLength & { return style.layout.height; }, "The height");

    table.Length(
      "min-width", [](UiStyle &style) -> LayoutLength & { return style.layout.min_width; }, "The least width");
    table.Length(
      "min-height", [](UiStyle &style) -> LayoutLength & { return style.layout.min_height; },
      "The least height");
    table.Length(
      "max-width", [](UiStyle &style) -> LayoutLength & { return style.layout.max_width; }, "The most width");
    table.Length(
      "max-height", [](UiStyle &style) -> LayoutLength & { return style.layout.max_height; },
      "The most height");

    table.Shorthand(
      "margin", {"margin-top", "margin-right", "margin-bottom", "margin-left"},
      "The room around the border");

    table.Length(
      "margin-top", [](UiStyle &style) -> LayoutLength & { return style.layout.margin.top; },
      "The room above the border");
    table.Length(
      "margin-right", [](UiStyle &style) -> LayoutLength & { return style.layout.margin.right; },
      "The room right of the border");
    table.Length(
      "margin-bottom", [](UiStyle &style) -> LayoutLength & { return style.layout.margin.bottom; },
      "The room below the border");
    table.Length(
      "margin-left", [](UiStyle &style) -> LayoutLength & { return style.layout.margin.left; },
      "The room left of the border");

    table.Shorthand(
      "padding", {"padding-top", "padding-right", "padding-bottom", "padding-left"},
      "The room inside the border");

    table.Length(
      "padding-top", [](UiStyle &style) -> LayoutLength & { return style.layout.padding.top; },
      "The room inside the border at the top");
    table.Length(
      "padding-right", [](UiStyle &style) -> LayoutLength & { return style.layout.padding.right; },
      "The room inside the border at the right");
    table.Length(
      "padding-bottom", [](UiStyle &style) -> LayoutLength & { return style.layout.padding.bottom; },
      "The room inside the border at the bottom");
    table.Length(
      "padding-left", [](UiStyle &style) -> LayoutLength & { return style.layout.padding.left; },
      "The room inside the border at the left");

    table.Shorthand("border", {"border-width", "border-color"}, "The width and the colour of the border");

    table.Edges(
      "border-width", [](UiStyle &style) -> LayoutEdges<float> & { return style.layout.border; },
      layout, "The widths of the border");

    table.ColourOfTheText(
      "border-color", [](UiStyle &style) -> std::optional<Color> & { return style.border_color; },
      not_inherited, "The colour of the border");

    // Position.

    table.Length("top", [](UiStyle &style) -> LayoutLength & { return style.layout.inset.top; }, "From the top");
    table.Length(
      "right", [](UiStyle &style) -> LayoutLength & { return style.layout.inset.right; }, "From the right");
    table.Length(
      "bottom", [](UiStyle &style) -> LayoutLength & { return style.layout.inset.bottom; }, "From the bottom");
    table.Length(
      "left", [](UiStyle &style) -> LayoutLength & { return style.layout.inset.left; }, "From the left");

    table.Whole(
      "z-index", [](UiStyle &style) -> int & { return style.z_index; },
      not_inherited, no_layout, "Higher is drawn later, among the elements next to it");

    // Flexbox.

    table.Keyword(
      "flex-direction", [](UiStyle &style) -> FlexDirection & { return style.layout.flex_direction; },
      flex_directions, not_inherited, layout, "The way what is inside is lined up");

    table.Keyword(
      "flex-wrap", [](UiStyle &style) -> FlexWrap & { return style.layout.flex_wrap; },
      flex_wraps, not_inherited, layout, "Whether what is inside goes on in a new line");

    table.Keyword(
      "justify-content", [](UiStyle &style) -> JustifyContent & { return style.layout.justify_content; },
      justify_contents, not_inherited, layout, "Where what is inside goes along the line");

    table.Keyword(
      "align-items", [](UiStyle &style) -> AlignItems & { return style.layout.align_items; },
      align_items, not_inherited, layout, "Where what is inside goes across the line");

    table.Keyword(
      "align-self", [](UiStyle &style) -> AlignSelf & { return style.layout.align_self; },
      align_selfs, not_inherited, layout, "Where the element itself goes across the line");

    table.Keyword(
      "align-content", [](UiStyle &style) -> AlignContent & { return style.layout.align_content; },
      align_contents, not_inherited, layout, "Where the lines go");

    table.Shorthand("flex", {"flex-grow", "flex-shrink", "flex-basis"}, "How the element grows and shrinks");

    table.Number(
      "flex-grow", [](UiStyle &style) -> float & { return style.layout.flex_grow; },
      "", not_inherited, layout, "How much of the room that is left the element takes");

    table.Number(
      "flex-shrink", [](UiStyle &style) -> float & { return style.layout.flex_shrink; },
      "", not_inherited, layout, "How much of the room that is missing the element gives");

    table.Length(
      "flex-basis", [](UiStyle &style) -> LayoutLength & { return style.layout.flex_basis; },
      "The size the element grows and shrinks from");

    table.Shorthand("gap", {"row-gap", "column-gap"}, "The room between what is inside");

    table.Number(
      "row-gap", [](UiStyle &style) -> float & { return style.layout.row_gap; },
      "px", not_inherited, layout, "The room between rows");

    table.Number(
      "column-gap", [](UiStyle &style) -> float & { return style.layout.column_gap; },
      "px", not_inherited, layout, "The room between columns");

    // Colours and images.

    table.Colour(
      "background-color", [](UiStyle &style) -> Color & { return style.background_color; },
      not_inherited, "The colour behind the element");

    table.Text(
      "background-image", [](UiStyle &style) -> std::string & { return style.background_image; },
      not_inherited, no_layout, "Virtual path of an image behind the element");

    table.Text(
      "border-image-source", [](UiStyle &style) -> std::string & { return style.border_image_source; },
      not_inherited, no_layout, "Virtual path of an image that is drawn in nine parts");

    table.Edges(
      "border-image-slice", [](UiStyle &style) -> LayoutEdges<float> & { return style.border_image_slice; },
      no_layout, "How far the corners reach into the image");
    table.Last().unit = "";

    table.Edges(
      "border-image-width", [](UiStyle &style) -> LayoutEdges<float> & { return style.border_image_width; },
      no_layout, "How wide the parts at the sides are drawn");

    table.Number(
      "outline-width", [](UiStyle &style) -> float & { return style.outline_width; },
      "px", not_inherited, no_layout, "The width of the line around the border");

    table.Number(
      "outline-offset", [](UiStyle &style) -> float & { return style.outline_offset; },
      "px", not_inherited, no_layout, "How far the line is from the border");

    table.ColourOfTheText(
      "outline-color", [](UiStyle &style) -> std::optional<Color> & { return style.outline_color; },
      not_inherited, "The colour of the line around the border");

    table.Number(
      "opacity", [](UiStyle &style) -> float & { return style.opacity; },
      "", not_inherited, no_layout, "How much of the element is seen, from 0 to 1");

    table.Colour(
      "accent-color", [](UiStyle &style) -> Color & { return style.accent_color; },
      not_inherited, "What a bar, a slider, and a checkbox are filled with");

    table.Keyword(
      "object-fit", [](UiStyle &style) -> UiObjectFit & { return style.object_fit; },
      object_fits, not_inherited, no_layout, "How an image fills its box");

    table.Keyword(
      "pointer-events", [](UiStyle &style) -> UiPointerEvents & { return style.pointer_events; },
      pointer_events, not_inherited, no_layout, "Whether the element takes the pointer");

    // Text. These are inherited, as in CSS.

    table.Colour(
      "color", [](UiStyle &style) -> Color & { return style.color; },
      inherited, "The colour of text");

    table.Text(
      "font-family", [](UiStyle &style) -> std::string & { return style.font_family; },
      inherited, layout, "The family of the font");

    table.Number(
      "font-size", [](UiStyle &style) -> float & { return style.font_size; },
      "px", inherited, layout, "The size of the font");

    table.Whole(
      "font-weight", [](UiStyle &style) -> int & { return style.font_weight; },
      inherited, layout, "How bold the font is, from 1 to 1000");

    table.Keyword(
      "text-align", [](UiStyle &style) -> TextAlign & { return style.text_align; },
      text_aligns, inherited, no_layout, "Where the lines of a text go in their box");

    table.Number(
      "line-height", [](UiStyle &style) -> float & { return style.line_height; },
      "", inherited, layout, "The height of a line");

    table.Last().get = [](const UiStyle &style)
    {
      UiPropertyValue value;
      value.kind = UiValueKind::Number;
      value.number = style.line_height;
      value.flag = style.line_height_is_multiple;
      return value;
    };

    table.Last().set = [](UiStyle &style, const UiPropertyValue &value)
    {
      style.line_height = value.number;
      style.line_height_is_multiple = value.flag;
    };

    // How text is drawn. Inherited as in CSS, but for the decoration and
    // the overflow.

    table.Number(
      "letter-spacing", [](UiStyle &style) -> float & { return style.letter_spacing; },
      "px", inherited, layout, "What is added behind every character");

    table.Number(
      "word-spacing", [](UiStyle &style) -> float & { return style.word_spacing; },
      "px", inherited, layout, "What is added behind every space");

    table.Keyword(
      "text-transform", [](UiStyle &style) -> TextTransform & { return style.text_transform; },
      text_transforms, inherited, layout, "Whether the text is shown in capitals or in small letters");

    table.Shorthand(
      "text-decoration",
      {"text-decoration-line", "text-decoration-color", "text-decoration-thickness"},
      "Lines under and through the text");

    table.Other(
      "text-decoration-line",
      [](const UiStyle &from, UiStyle &to)
      {
        to.text_underline = from.text_underline;
        to.text_line_through = from.text_line_through;
      },
      [](const UiStyle &style)
      {
        std::string lines;
        if (style.text_underline) { lines = "underline"; }
        if (style.text_line_through) { lines += std::string(lines.empty() ? "" : " ") + "line-through"; }
        return lines.empty() ? std::string("none") : lines;
      },
      "A line under the text, and one through it");

    table.ColourOfTheText(
      "text-decoration-color",
      [](UiStyle &style) -> std::optional<Color> & { return style.text_decoration_color; },
      not_inherited, "The colour of the lines under and through the text");

    table.Number(
      "text-decoration-thickness",
      [](UiStyle &style) -> float & { return style.text_decoration_thickness; },
      "px", not_inherited, no_layout, "How thick the lines are, or 0 for what the font asks for");

    table.Other(
      "text-shadow",
      [](const UiStyle &from, UiStyle &to) { to.text_shadow = from.text_shadow; },
      [](const UiStyle &style) { return FormatShadows(style.text_shadow, false); },
      "Shadows behind the text, the first on top");
    table.Last().is_inherited = true;

    table.Keyword(
      "white-space", [](UiStyle &style) -> WhiteSpace & { return style.white_space; },
      white_spaces, inherited, layout, "How spaces and line feeds are kept, and where lines are broken");

    table.Keyword(
      "text-overflow", [](UiStyle &style) -> TextOverflow & { return style.text_overflow; },
      text_overflows, not_inherited, no_layout, "What is shown of a line that is not broken and does not fit");

    table.Other(
      "font-style",
      [](const UiStyle &from, UiStyle &to) { to.font_italic = from.font_italic; },
      [](const UiStyle &style) { return std::string(style.font_italic ? "italic" : "normal"); },
      "Whether the font is upright or italic");
    table.Last().is_inherited = true;
    table.Last().affects_layout = true;

    table.Number(
      "text-stroke-width", [](UiStyle &style) -> float & { return style.text_stroke_width; },
      "px", inherited, no_layout, "The width of the line around every glyph");

    table.ColourOfTheText(
      "text-stroke-color", [](UiStyle &style) -> std::optional<Color> & { return style.text_stroke_color; },
      inherited, "The colour of the line around every glyph");

    table.Keyword(
      "direction", [](UiStyle &style) -> TextDirection & { return style.direction; },
      text_directions, inherited, layout, "Whether the text runs from the left or from the right");

    // Images.

    table.Keyword(
      "image-rendering", [](UiStyle &style) -> UiImageRendering & { return style.image_rendering; },
      image_renderings, inherited, no_layout, "Whether an image is smooth or in blocks when it is enlarged");

    table.Other(
      "object-position",
      [](const UiStyle &from, UiStyle &to) { to.object_position = from.object_position; },
      [](const UiStyle &style) { return FormatPlace(style.object_position); },
      "Where an image goes in its box when it does not fill it");

    table.Shorthand(
      "background",
      {"background-color", "background-image", "background-size", "background-position", "background-repeat"},
      "What is behind the element: a colour, and an image or a gradient");

    table.Other(
      "background-size",
      [](const UiStyle &from, UiStyle &to) { to.background_size = from.background_size; },
      [](const UiStyle &style) { return FormatBackgroundSize(style.background_size); },
      "How large the image behind the element is drawn");

    table.Other(
      "background-position",
      [](const UiStyle &from, UiStyle &to) { to.background_position = from.background_position; },
      [](const UiStyle &style) { return FormatPlace(style.background_position); },
      "Where the image behind the element goes");

    table.Keyword(
      "background-repeat", [](UiStyle &style) -> UiBackgroundRepeat & { return style.background_repeat; },
      background_repeats, not_inherited, no_layout, "Whether the image behind the element is repeated");

    table.Keyword(
      "border-image-repeat", [](UiStyle &style) -> UiBorderImageRepeat & { return style.border_image_repeat; },
      border_image_repeats, not_inherited, no_layout, "How the sides of an image in nine parts fill their room");

    // The box.

    table.Shorthand(
      "border-radius",
      {"border-top-left-radius", "border-top-right-radius", "border-bottom-right-radius", "border-bottom-left-radius"},
      "How round the corners are");

    table.Other(
      "border-top-left-radius",
      [](const UiStyle &from, UiStyle &to) { to.border_radius.top_left = from.border_radius.top_left; },
      [](const UiStyle &style) { return FormatCssLength(style.border_radius.top_left); },
      "How round the corner at the left top is");

    table.Other(
      "border-top-right-radius",
      [](const UiStyle &from, UiStyle &to) { to.border_radius.top_right = from.border_radius.top_right; },
      [](const UiStyle &style) { return FormatCssLength(style.border_radius.top_right); },
      "How round the corner at the right top is");

    table.Other(
      "border-bottom-right-radius",
      [](const UiStyle &from, UiStyle &to) { to.border_radius.bottom_right = from.border_radius.bottom_right; },
      [](const UiStyle &style) { return FormatCssLength(style.border_radius.bottom_right); },
      "How round the corner at the right bottom is");

    table.Other(
      "border-bottom-left-radius",
      [](const UiStyle &from, UiStyle &to) { to.border_radius.bottom_left = from.border_radius.bottom_left; },
      [](const UiStyle &style) { return FormatCssLength(style.border_radius.bottom_left); },
      "How round the corner at the left bottom is");

    table.Shorthand("border-top", {"border-top-width", "border-top-color"}, "The line at the top");
    table.Shorthand("border-right", {"border-right-width", "border-right-color"}, "The line at the right");
    table.Shorthand("border-bottom", {"border-bottom-width", "border-bottom-color"}, "The line at the bottom");
    table.Shorthand("border-left", {"border-left-width", "border-left-color"}, "The line at the left");

    table.Number(
      "border-top-width", [](UiStyle &style) -> float & { return style.layout.border.top; },
      "px", not_inherited, layout, "The width of the line at the top");

    table.Number(
      "border-right-width", [](UiStyle &style) -> float & { return style.layout.border.right; },
      "px", not_inherited, layout, "The width of the line at the right");

    table.Number(
      "border-bottom-width", [](UiStyle &style) -> float & { return style.layout.border.bottom; },
      "px", not_inherited, layout, "The width of the line at the bottom");

    table.Number(
      "border-left-width", [](UiStyle &style) -> float & { return style.layout.border.left; },
      "px", not_inherited, layout, "The width of the line at the left");

    table.Other(
      "border-top-color",
      [](const UiStyle &from, UiStyle &to) { to.border_top_color = from.border_top_color; },
      [](const UiStyle &style)
      {
        return FormatSideColor(style, [](UiStyle &each) -> std::optional<Color> & { return each.border_top_color; });
      },
      "The colour of the line at the top, where it differs from that of the border");

    table.Other(
      "border-right-color",
      [](const UiStyle &from, UiStyle &to) { to.border_right_color = from.border_right_color; },
      [](const UiStyle &style)
      {
        return FormatSideColor(style, [](UiStyle &each) -> std::optional<Color> & { return each.border_right_color; });
      },
      "The colour of the line at the right, where it differs from that of the border");

    table.Other(
      "border-bottom-color",
      [](const UiStyle &from, UiStyle &to) { to.border_bottom_color = from.border_bottom_color; },
      [](const UiStyle &style)
      {
        return FormatSideColor(
          style, [](UiStyle &each) -> std::optional<Color> & { return each.border_bottom_color; });
      },
      "The colour of the line at the bottom, where it differs from that of the border");

    table.Other(
      "border-left-color",
      [](const UiStyle &from, UiStyle &to) { to.border_left_color = from.border_left_color; },
      [](const UiStyle &style)
      {
        return FormatSideColor(style, [](UiStyle &each) -> std::optional<Color> & { return each.border_left_color; });
      },
      "The colour of the line at the left, where it differs from that of the border");

    table.Other(
      "box-shadow",
      [](const UiStyle &from, UiStyle &to) { to.box_shadow = from.box_shadow; },
      [](const UiStyle &style) { return FormatShadows(style.box_shadow, true); },
      "Shadows around the box, the first on top");

    table.Other(
      "transform",
      [](const UiStyle &from, UiStyle &to) { to.transform = from.transform; },
      [](const UiStyle &style) { return FormatTransform(style.transform); },
      "How the element is moved, turned, and scaled where it is drawn");

    table.Other(
      "transform-origin",
      [](const UiStyle &from, UiStyle &to) { to.transform_origin = from.transform_origin; },
      [](const UiStyle &style) { return FormatPlace(style.transform_origin); },
      "The point the element is turned and scaled around");

    table.Text(
      "shader", [](UiStyle &style) -> std::string & { return style.shader; },
      not_inherited, no_layout, "Virtual path of the shader the element is drawn with, without an extension");

    table.Other(
      "shader-values",
      [](const UiStyle &from, UiStyle &to) { to.shader_values = from.shader_values; },
      [](const UiStyle &style) { return FormatShaderValues(style.shader_values); },
      "The values the shader is given, each by its name");

    // A colour of the text and an image behind the element may be a
    // gradient, which goes with them.
    for (auto &property : _properties)
    {
      if (property.name == "color")
      {
        property.copy = [](const UiStyle &from, UiStyle &to)
        {
          to.color = from.color;
          to.color_gradient = from.color_gradient;
        };
      }

      if (property.name == "background-image")
      {
        property.copy = [](const UiStyle &from, UiStyle &to)
        {
          to.background_image = from.background_image;
          to.background_gradient = from.background_gradient;
        };
      }
    }
  }

  const UiProperties &UiProperties::Get()
  {
    static const UiProperties properties;
    return properties;
  }

  const UiProperty *UiProperties::Find(const std::string &name) const
  {
    const std::string css_name = ToCssName(name);

    for (const auto &property : _properties)
    {
      if (property.name == css_name) { return &property; }
    }
    return nullptr;
  }

  const std::vector<UiProperty> &UiProperties::GetAll() const
  {
    return _properties;
  }

  std::vector<const UiProperty *> UiProperties::GetLonghands(const UiProperty &property) const
  {
    if (!property.IsShorthand()) { return {&property}; }

    std::vector<const UiProperty *> longhands;
    for (const auto &name : property.longhands)
    {
      if (const UiProperty *found = Find(name); found != nullptr) { longhands.push_back(found); }
    }
    return longhands;
  }

  std::string UiProperties::ToCssName(const std::string &name)
  {
    // the name of a custom property is what it is
    if (name.starts_with("--")) { return name; }

    std::string css_name = name;
    std::ranges::replace(css_name, '_', '-');
    return css_name;
  }

  std::string UiProperties::ToYamlName(const std::string &name)
  {
    if (name.starts_with("--")) { return name; }

    std::string yaml_name = name;
    std::ranges::replace(yaml_name, '-', '_');
    return yaml_name;
  }

  const std::vector<std::string> &UiProperties::GetStyleNames()
  {
    static const std::vector<std::string> names = []
    {
      // A name that is not known is answered with the names that are,
      // which are those ReadUiStyle() asked for.
      const std::string unknown = "not a property";

      DataValue map = DataValue::Map();
      map.Set(unknown, DataValue::Bool(true));

      std::vector<std::string> errors;
      const DataReader reader(map, "", "", errors);

      UiStyle style;
      ReadUiStyle(reader, style);
      reader.Finish();

      std::vector<std::string> read;
      if (errors.empty()) { return read; }

      const std::string marker = "Known are: ";
      const std::size_t start = errors.back().find(marker);
      if (start == std::string::npos) { return read; }

      std::string name;
      for (const char letter : errors.back().substr(start + marker.size()))
      {
        if (letter == ',' || letter == ' ')
        {
          if (!name.empty()) { read.push_back(name); }
          name.clear();
        } else
        {
          name += letter;
        }
      }

      if (!name.empty()) { read.push_back(name); }
      return read;
    }();

    return names;
  }

  bool UiProperties::IsStyleName(const std::string &yaml_name)
  {
    const auto &names = GetStyleNames();
    return std::ranges::find(names, yaml_name) != names.end();
  }

  std::vector<std::string> UiProperties::GetSimilarNames(const std::string &name)
  {
    const std::string wanted = ToYamlName(name);

    std::vector<std::pair<std::size_t, std::string>> near;
    for (const auto &known : GetStyleNames())
    {
      const std::size_t distance = Distance(wanted, known);

      // Two letters may be wrong, which is what two that changed places
      // count as, and three in a long name. In a short name, one.
      const std::size_t most = wanted.size() <= 3
        ? 1
        : std::min<std::size_t>(3, std::max<std::size_t>(2, wanted.size() / 3));

      if (distance <= most)
      {
        near.emplace_back(distance, ToCssName(known));
      }
    }

    std::ranges::stable_sort(near, [](const auto &a, const auto &b) { return a.first < b.first; });

    std::vector<std::string> names;
    for (std::size_t i = 0; i < near.size() && i < 3; i++) { names.push_back(near[i].second); }
    return names;
  }

  bool CanInterpolateUiValue(const UiPropertyValue &from, const UiPropertyValue &to)
  {
    if (from.kind != to.kind) { return false; }

    switch (from.kind)
    {
      case UiValueKind::Number:
        return from.flag == to.flag;
      case UiValueKind::Whole:
      case UiValueKind::Color:
      case UiValueKind::Edges:
        return true;
      case UiValueKind::Length:
        return !from.length.IsAuto() && !to.length.IsAuto();
      default:
        return false;
    }
  }

  UiPropertyValue InterpolateUiValue(const UiPropertyValue &from, const UiPropertyValue &to, const float progress)
  {
    if (!CanInterpolateUiValue(from, to))
    {
      // as CSS says for what cannot be moved
      return progress < 0.5f ? from : to;
    }

    const auto mix = [progress](const float a, const float b) { return a + (b - a) * progress; };

    UiPropertyValue value = to;

    switch (from.kind)
    {
      case UiValueKind::Number:
        value.number = mix(from.number, to.number);
        break;

      case UiValueKind::Whole:
        value.number = std::round(mix(from.number, to.number));
        break;

      case UiValueKind::Length:
      {
        // each as pixels and a percentage added up
        const auto pixels = [](const LayoutLength &length)
        {
          return length.unit == LayoutLength::Unit::Percent ? 0.0f : length.value;
        };

        const auto percent = [](const LayoutLength &length)
        {
          if (length.unit == LayoutLength::Unit::Percent) { return length.value; }
          return length.unit == LayoutLength::Unit::Sum ? length.percent : 0.0f;
        };

        const float mixed_pixels = mix(pixels(from.length), pixels(to.length));
        const float mixed_percent = mix(percent(from.length), percent(to.length));

        const bool has_percent = from.length.HasPercent() || to.length.HasPercent();
        const bool has_pixels = from.length.unit != LayoutLength::Unit::Percent ||
                                to.length.unit != LayoutLength::Unit::Percent;

        if (!has_percent) { value.length = LayoutLength::Pixels(mixed_pixels); }
        else if (!has_pixels) { value.length = LayoutLength::Percent(mixed_percent); }
        else { value.length = LayoutLength::Sum(mixed_pixels, mixed_percent); }
        break;
      }

      case UiValueKind::Color:
      {
        // With the alpha multiplied in, so that a colour that cannot be
        // seen adds nothing of its own on the way.
        const float alpha = mix(from.color.a, to.color.a);

        const auto channel = [&](const float a, const float b)
        {
          if (alpha <= 0.0f) { return 0.0f; }
          return mix(a * from.color.a, b * to.color.a) / alpha;
        };

        value.color = {
          channel(from.color.r, to.color.r),
          channel(from.color.g, to.color.g),
          channel(from.color.b, to.color.b),
          alpha
        };
        value.flag = true;
        break;
      }

      case UiValueKind::Edges:
        for (std::size_t i = 0; i < 4; i++) { value.edges[i] = mix(from.edges[i], to.edges[i]); }
        break;

      default:
        break;
    }

    return value;
  }
} // neon
