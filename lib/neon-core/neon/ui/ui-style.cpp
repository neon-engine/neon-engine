#include "ui-style.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <vector>

#include "css-functions.hpp"
#include "css-values.hpp"

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

    /// Reads the properties of behaviour where the function it is created
    /// in ends. They are then asked for behind the properties that were
    /// there before them, and a message that lists what is known lists them
    /// last. `overflow` is also read in front of `overflow_x` and
    /// `overflow_y` that way, as a shorthand is.
    class BehaviourAtTheEnd
    {
      const DataReader &_reader;
      UiStyle &_style;

    public:
      BehaviourAtTheEnd(const DataReader &reader, UiStyle &style) : _reader(reader), _style(style) {}

      BehaviourAtTheEnd(const BehaviourAtTheEnd &) = delete;

      BehaviourAtTheEnd &operator=(const BehaviourAtTheEnd &) = delete;

      ~BehaviourAtTheEnd()
      {
        ReadUiBehaviourStyle(_reader, _style);
      }
    };
    const std::vector<std::string> pointer_events = {"auto", "none"};
    const std::vector<std::string> text_aligns = {"left", "center", "right", "start", "end"};
    const std::vector<std::string> object_fits = {"fill", "contain", "cover", "none", "scale-down"};
    const std::vector<std::string> text_transforms = {"none", "uppercase", "lowercase", "capitalize"};
    const std::vector<std::string> white_spaces = {"normal", "nowrap", "pre", "pre-wrap", "pre-line"};
    const std::vector<std::string> text_overflows = {"clip", "ellipsis"};
    const std::vector<std::string> font_styles = {"normal", "italic"};
    const std::vector<std::string> directions = {"ltr", "rtl"};
    const std::vector<std::string> image_renderings = {"auto", "pixelated"};
    const std::vector<std::string> background_repeats = {"repeat", "no-repeat", "repeat-x", "repeat-y"};
    const std::vector<std::string> border_image_repeats = {"stretch", "repeat", "round"};

    /// What a value is called in a message: the text itself, or its kind.
    std::string Describe(const DataValue &value)
    {
      if (std::string text; value.GetText(text)) { return "'" + text + "'"; }
      return DataValue::Describe(value.GetKind());
    }

    std::string LengthExpected(const bool allows_percent, const bool allows_auto)
    {
      std::string expected = "a number of pixels";
      if (allows_percent) { expected += allows_auto ? ", a percentage such as 50%," : " or a percentage such as 50%"; }
      if (allows_auto) { expected += " or auto"; }
      return expected;
    }

    /// Reads the properties of one element. Every function returns whether
    /// the name was written and what it holds was read.
    class Properties
    {
      const DataReader &_reader;

      void Expected(const std::string &name, const DataValue &value, const std::string &expected) const
      {
        _reader.Report(value, std::format(
                         "'{}' of {} is {}, where {} was expected",
                         name, _reader.GetWhere(), Describe(value), expected));
      }

      /// One value of a length, which is a number or text.
      static bool AsLength(
        const DataValue &value,
        const bool allows_percent,
        const bool allows_auto,
        LayoutLength &length)
      {
        LayoutLength read;

        if (float number = 0.0f; value.GetNumber(number))
        {
          read = LayoutLength::Pixels(number);
        } else if (std::string text; !value.GetText(text) || !ParseCssLength(text, read))
        {
          return false;
        }

        if (read.HasPercent() && !allows_percent) { return false; }
        if (read.IsAuto() && !allows_auto) { return false; }

        length = read;
        return true;
      }

      /// The values of a property that takes several: a list, or text with
      /// spaces between them, or one value alone.
      static std::vector<DataValue> Several(const DataValue &value)
      {
        if (value.IsList()) { return value.GetItems(); }

        if (std::string text; value.GetText(text))
        {
          std::vector<DataValue> values;
          for (const auto &part : SplitCssValues(text))
          {
            auto each = DataValue::Text(part);
            each.SetLine(value.GetLine());
            values.push_back(each);
          }
          return values;
        }

        return {value};
      }

    public:
      explicit Properties(const DataReader &reader) : _reader(reader) {}

      bool Length(
        const std::string &name,
        LayoutLength &length,
        const bool allows_percent,
        const bool allows_auto) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        if (!AsLength(*value, allows_percent, allows_auto, length))
        {
          Expected(name, *value, LengthExpected(allows_percent, allows_auto));
          return false;
        }
        return true;
      }

      bool Pixels(const std::string &name, float &pixels, const bool allows_negative = false) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        LayoutLength length;
        if (!AsLength(*value, false, false, length) || (length.value < 0.0f && !allows_negative))
        {
          Expected(name, *value, allows_negative ? "a number of pixels" : "a number of pixels that is not below 0");
          return false;
        }

        pixels = length.value;
        return true;
      }

      /// One to four values, in the order of the shorthands of CSS: all
      /// four sides, or top and bottom then left and right, or top then
      /// left and right then bottom, or top, right, bottom, left.
      bool Edges(
        const std::string &name,
        LayoutEdges<LayoutLength> &edges,
        const bool allows_percent,
        const bool allows_auto,
        const bool allows_negative) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        const std::string expected = "one to four values, each " + LengthExpected(allows_percent, allows_auto);

        const auto values = Several(*value);
        if (values.empty() || values.size() > 4)
        {
          Expected(name, *value, expected);
          return false;
        }

        std::vector<LayoutLength> lengths(values.size());
        for (std::size_t i = 0; i < values.size(); i++)
        {
          if (!AsLength(values[i], allows_percent, allows_auto, lengths[i]) ||
              (lengths[i].value < 0.0f && !allows_negative))
          {
            Expected(name, *value, allows_negative ? expected : expected + ", and none below 0");
            return false;
          }
        }

        edges.top = lengths[0];
        edges.right = lengths.size() > 1 ? lengths[1] : lengths[0];
        edges.bottom = lengths.size() > 2 ? lengths[2] : lengths[0];
        edges.left = lengths.size() > 3 ? lengths[3] : edges.right;
        return true;
      }

      bool PixelEdges(const std::string &name, LayoutEdges<float> &edges) const
      {
        LayoutEdges<LayoutLength> lengths;
        if (!Edges(name, lengths, false, false, false)) { return false; }

        edges = {lengths.top.value, lengths.right.value, lengths.bottom.value, lengths.left.value};
        return true;
      }

      bool Colour(const std::string &name, Color &color) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        if (std::string text; value->GetText(text))
        {
          if (ParseCssColor(text, color)) { return true; }
        } else if (value->IsList())
        {
          // as scene files write a colour
          const auto &items = value->GetItems();
          float numbers[4] = {0.0f, 0.0f, 0.0f, 1.0f};
          bool read = items.size() == 3 || items.size() == 4;

          for (std::size_t i = 0; read && i < items.size(); i++) { read = items[i].GetNumber(numbers[i]); }

          if (read)
          {
            color = {numbers[0], numbers[1], numbers[2], numbers[3]};
            return true;
          }
        }

        Expected(
          name, *value,
          "a colour such as \"#ff8000\", \"#ff800080\", or rgb(255, 128, 0), or a list of 3 to 4 numbers");
        return false;
      }

      bool Number(const std::string &name, float &number, const float least, const float most) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        float read = 0.0f;
        if (!value->GetNumber(read) || read < least || read > most)
        {
          Expected(name, *value, most < 1e9f
                     ? std::format("a number from {} to {}", least, most)
                     : std::format("a number that is not below {}", least));
          return false;
        }

        number = read;
        return true;
      }

      template<typename T>
      bool Keyword(const std::string &name, const std::vector<std::string> &keywords, T &value) const
      {
        std::size_t index = 0;
        if (!_reader.ReadChoice(name, keywords, index)) { return false; }

        value = static_cast<T>(index);
        return true;
      }

      bool Path(const std::string &name, std::string &path) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        std::string text;
        if (!value->GetText(text))
        {
          Expected(name, *value, "a virtual path such as assets://ui/panel.png, or none");
          return false;
        }

        path = text == "none" ? "" : text;
        return true;
      }

      /// `flex` of CSS: `none`, `auto`, a grow factor alone, or a grow
      /// factor, a shrink factor, and a basis.
      bool Flex(LayoutStyle &layout) const
      {
        const auto *value = _reader.ReadValue("flex");
        if (value == nullptr) { return false; }

        const auto values = Several(*value);
        const std::string expected =
          "none, auto, or a number to grow by, which a number to shrink by and a basis may follow";

        std::string keyword;
        if (values.size() == 1 && values[0].GetText(keyword) && (keyword == "none" || keyword == "auto"))
        {
          layout.flex_grow = keyword == "auto" ? 1.0f : 0.0f;
          layout.flex_shrink = keyword == "auto" ? 1.0f : 0.0f;
          layout.flex_basis = LayoutLength::Auto();
          return true;
        }

        const auto as_number = [](const DataValue &each, float &number)
        {
          if (each.GetNumber(number)) { return number >= 0.0f; }

          std::string text;
          return each.GetText(text) && ParseCssNumber(text, number) && number >= 0.0f;
        };

        float grow = 0.0f;
        float shrink = 1.0f;
        // a basis of 0 is what makes `flex: 1` share a row in equal parts
        LayoutLength basis = LayoutLength::Pixels(0.0f);

        bool read = !values.empty() && values.size() <= 3 && as_number(values[0], grow);

        if (read && values.size() == 2)
        {
          // a number is the shrink factor, anything else the basis
          if (!as_number(values[1], shrink))
          {
            shrink = 1.0f;
            read = AsLength(values[1], true, true, basis);
          }
        }

        if (read && values.size() == 3)
        {
          read = as_number(values[1], shrink) && AsLength(values[2], true, true, basis);
        }

        if (!read)
        {
          Expected("flex", *value, expected);
          return false;
        }

        layout.flex_grow = grow;
        layout.flex_shrink = shrink;
        layout.flex_basis = basis;
        return true;
      }

      /// `border` of CSS: a width, `solid` or `none`, and a colour, in any
      /// order and each of them optional.
      bool Border(UiStyle &style) const
      {
        const auto *value = _reader.ReadValue("border");
        if (value == nullptr) { return false; }

        const std::string expected = "a width, solid or none, and a colour, such as \"2px solid #ffffff\"";

        std::string text;
        if (float number = 0.0f; value->GetNumber(number)) { text = std::format("{}", number); }
        else if (!value->GetText(text))
        {
          Expected("border", *value, expected);
          return false;
        }

        float width = 3.0f; // `medium`, which is what CSS starts with
        bool is_drawn = true;
        std::optional<Color> color;

        const auto parts = SplitCssValues(text);
        if (parts.empty())
        {
          Expected("border", *value, expected);
          return false;
        }

        for (const auto &part : parts)
        {
          LayoutLength length;
          Color read;

          if (part == "solid")
          {
            is_drawn = true;
          } else if (part == "none")
          {
            is_drawn = false;
          } else if (ParseCssLength(part, length) && length.unit == LayoutLength::Unit::Pixels && length.value >= 0.0f)
          {
            width = length.value;
          } else if (ParseCssColor(part, read))
          {
            color = read;
          } else
          {
            Expected("border", *value, expected);
            return false;
          }
        }

        if (!is_drawn) { width = 0.0f; }

        style.layout.border = {width, width, width, width};
        style.border_color = color;
        return true;
      }

      /// `gap` of CSS: between rows, then between columns. One value
      /// stands for both.
      bool Gap(LayoutStyle &layout) const
      {
        const auto *value = _reader.ReadValue("gap");
        if (value == nullptr) { return false; }

        const auto values = Several(*value);
        LayoutLength row;
        LayoutLength column;

        if (values.empty() || values.size() > 2 ||
            !AsLength(values[0], false, false, row) || row.value < 0.0f ||
            !AsLength(values.back(), false, false, column) || column.value < 0.0f)
        {
          Expected("gap", *value, "one or two numbers of pixels that are not below 0");
          return false;
        }

        layout.row_gap = row.value;
        layout.column_gap = column.value;
        return true;
      }

      bool FontWeight(int &weight) const
      {
        const auto *value = _reader.ReadValue("font_weight");
        if (value == nullptr) { return false; }

        float number = 0.0f;
        std::string text;

        if (value->GetNumber(number) && number >= 1.0f && number <= 1000.0f)
        {
          weight = static_cast<int>(std::round(number));
          return true;
        }

        if (value->GetText(text) && (text == "normal" || text == "bold"))
        {
          weight = text == "bold" ? 700 : 400;
          return true;
        }

        Expected("font_weight", *value, "a number from 1 to 1000, normal, or bold");
        return false;
      }

      /// The text of a value, where a number counts as text as well.
      static bool AsText(const DataValue &value, std::string &text)
      {
        if (float number = 0.0f; value.GetNumber(number))
        {
          text = std::format("{}", number);
          return true;
        }

        return value.GetText(text);
      }

      /// A colour, or a gradient in its place.
      bool ColourOrGradient(const std::string &name, Color &color, std::optional<UiGradient> &gradient) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        if (std::string text; value->GetText(text))
        {
          if (UiGradient read; ParseCssGradient(text, read))
          {
            gradient = read;

            // what stands in for the gradient where one colour is asked
            // for, such as the line under a text
            color = read.stops.front().color;
            return true;
          }

          if (text.find("-gradient(") != std::string::npos)
          {
            Expected(
              name, *value,
              "a gradient such as linear-gradient(90deg, #f00, #00f) with 2 to 8 colours");
            return false;
          }
        }

        if (!Colour(name, color)) { return false; }

        gradient.reset();
        return true;
      }

      bool Spacing(const std::string &name, float &spacing) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        if (std::string text; value->GetText(text) && text == "normal")
        {
          spacing = 0.0f;
          return true;
        }

        return Pixels(name, spacing, true);
      }

      /// `text-decoration` of CSS: which lines, their colour, and how
      /// thick they are, in any order.
      bool TextDecoration(UiStyle &style) const
      {
        const auto *value = _reader.ReadValue("text_decoration");
        if (value == nullptr) { return false; }

        const std::string expected =
          "none, underline, line-through, a colour, and a thickness, such as \"underline #ff8000 2px\"";

        std::string text;
        if (!value->GetText(text))
        {
          Expected("text_decoration", *value, expected);
          return false;
        }

        bool underline = false;
        bool line_through = false;
        std::optional<Color> color;
        float thickness = 0.0f;

        const auto parts = SplitCssValues(text);
        if (parts.empty())
        {
          Expected("text_decoration", *value, expected);
          return false;
        }

        for (const auto &part : parts)
        {
          LayoutLength length;
          Color read;

          if (part == "none")
          {
            underline = false;
            line_through = false;
          } else if (part == "underline")
          {
            underline = true;
          } else if (part == "line-through")
          {
            line_through = true;
          } else if (part == "solid" || part == "auto")
          {
            // the one style of line there is
          } else if (ParseCssLength(part, length) && length.unit == LayoutLength::Unit::Pixels && length.value >= 0.0f)
          {
            thickness = length.value;
          } else if (ParseCssColor(part, read))
          {
            color = read;
          } else
          {
            Expected("text_decoration", *value, expected);
            return false;
          }
        }

        style.text_underline = underline;
        style.text_line_through = line_through;
        style.text_decoration_color = color;
        style.text_decoration_thickness = thickness;
        return true;
      }

      bool TextDecorationLine(UiStyle &style) const
      {
        const auto *value = _reader.ReadValue("text_decoration_line");
        if (value == nullptr) { return false; }

        std::string text;
        bool underline = false;
        bool line_through = false;
        bool read = value->GetText(text) && !text.empty();

        for (const auto &part : SplitCssValues(text))
        {
          if (part == "underline") { underline = true; }
          else if (part == "line-through") { line_through = true; }
          else if (part != "none") { read = false; }
        }

        if (!read)
        {
          Expected("text_decoration_line", *value, "none, underline, line-through, or both of them");
          return false;
        }

        style.text_underline = underline;
        style.text_line_through = line_through;
        return true;
      }

      bool Shadows(const std::string &name, const bool of_text, std::vector<UiShadow> &shadows) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        // a list holds one shadow in each of its items
        std::string text;
        if (value->IsList())
        {
          for (const auto &item : value->GetItems())
          {
            std::string each;
            if (!item.GetText(each))
            {
              text.clear();
              break;
            }
            text += (text.empty() ? "" : ", ") + each;
          }
        } else
        {
          (void) value->GetText(text);
        }

        if (text.empty() || !ParseCssShadows(text, of_text, shadows))
        {
          Expected(
            name, *value,
            of_text
              ? "none, or shadows such as \"0 2px 4px #000000\": to the right, down, a blur that is not "
                "below 0, and a colour"
              : "none, or shadows such as \"0 4px 12px 0 rgba(0, 0, 0, 0.5)\": inset or not, to the right, "
                "down, a blur that is not below 0, how much larger, and a colour");
          return false;
        }

        return true;
      }

      bool Gradient(const std::string &name, std::optional<UiGradient> &gradient) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        std::string text;
        UiGradient read;
        if (!value->GetText(text) || !ParseCssGradient(text, read))
        {
          Expected(name, *value, "a gradient such as linear-gradient(90deg, #f00, #00f) with 2 to 8 colours");
          return false;
        }

        gradient = read;
        return true;
      }

      bool Place(const std::string &name, UiPlace &place) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        std::string text;
        if (value->IsList())
        {
          for (const auto &item : value->GetItems())
          {
            std::string each;
            if (AsText(item, each)) { text += (text.empty() ? "" : " ") + each; }
          }
        } else
        {
          (void) AsText(*value, text);
        }

        if (!ParseCssPlace(text, place))
        {
          Expected(
            name, *value,
            "one or two of left, center, right, top, bottom, a number of pixels, or a percentage");
          return false;
        }

        return true;
      }

      bool BackgroundSize(UiBackgroundSize &size) const
      {
        const auto *value = _reader.ReadValue("background_size");
        if (value == nullptr) { return false; }

        const auto values = Several(*value);
        std::string keyword;

        if (values.size() == 1 && values[0].GetText(keyword) && (keyword == "cover" || keyword == "contain"))
        {
          size.kind = keyword == "cover" ? UiBackgroundSize::Kind::Cover : UiBackgroundSize::Kind::Contain;
          return true;
        }

        LayoutLength width;
        LayoutLength height;

        if (values.empty() || values.size() > 2 || !AsLength(values[0], true, true, width) || width.value < 0.0f ||
            (values.size() == 2 && (!AsLength(values[1], true, true, height) || height.value < 0.0f)))
        {
          Expected(
            "background_size", *value,
            "auto, cover, contain, or one to two values, each a number of pixels, a percentage, or auto");
          return false;
        }

        if (width.IsAuto() && height.IsAuto())
        {
          size.kind = UiBackgroundSize::Kind::Auto;
          return true;
        }

        size.kind = UiBackgroundSize::Kind::Lengths;
        size.width = width;
        size.height = height;
        return true;
      }

      /// One to four values, from the left top corner around to the left
      /// bottom one, as the shorthand of CSS has them.
      bool Radii(UiCornerRadii &radii) const
      {
        const auto *value = _reader.ReadValue("border_radius");
        if (value == nullptr) { return false; }

        const auto values = Several(*value);
        std::vector<LayoutLength> lengths(values.size());
        bool read = !values.empty() && values.size() <= 4;

        for (std::size_t i = 0; read && i < values.size(); i++)
        {
          read = AsLength(values[i], true, false, lengths[i]) && lengths[i].value >= 0.0f;
        }

        if (!read)
        {
          Expected(
            "border_radius", *value,
            "one to four values, each a number of pixels or a percentage such as 50%, and none below 0");
          return false;
        }

        radii.top_left = lengths[0];
        radii.top_right = lengths.size() > 1 ? lengths[1] : lengths[0];
        radii.bottom_right = lengths.size() > 2 ? lengths[2] : lengths[0];
        radii.bottom_left = lengths.size() > 3 ? lengths[3] : radii.top_right;
        return true;
      }

      bool Radius(const std::string &name, LayoutLength &radius) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        if (!Length(name, radius, true, false)) { return false; }

        if (radius.value < 0.0f)
        {
          Expected(name, *value, "a number of pixels or a percentage that is not below 0");
          radius = LayoutLength::Pixels(0.0f);
          return false;
        }
        return true;
      }

      /// `border_top` and the like: a width, `solid` or `none`, and a
      /// colour, for one side.
      bool BorderSide(const std::string &name, float &width, std::optional<Color> &color) const
      {
        const auto *value = _reader.ReadValue(name);
        if (value == nullptr) { return false; }

        const std::string expected = "a width, solid or none, and a colour, such as \"2px solid #ffffff\"";

        std::string text;
        if (!AsText(*value, text))
        {
          Expected(name, *value, expected);
          return false;
        }

        float read_width = 3.0f;
        bool is_drawn = true;
        std::optional<Color> read_color;

        const auto parts = SplitCssValues(text);
        bool read = !parts.empty();

        for (const auto &part : parts)
        {
          LayoutLength length;
          Color each;

          if (part == "solid") { is_drawn = true; }
          else if (part == "none") { is_drawn = false; }
          else if (ParseCssLength(part, length) && length.unit == LayoutLength::Unit::Pixels && length.value >= 0.0f)
          {
            read_width = length.value;
          } else if (ParseCssColor(part, each))
          {
            read_color = each;
          } else
          {
            read = false;
          }
        }

        if (!read)
        {
          Expected(name, *value, expected);
          return false;
        }

        width = is_drawn ? read_width : 0.0f;
        if (read_color.has_value()) { color = read_color; }
        return true;
      }

      bool Transform(std::vector<UiTransformStep> &steps) const
      {
        const auto *value = _reader.ReadValue("transform");
        if (value == nullptr) { return false; }

        std::string text;
        if (!value->GetText(text) || !ParseCssTransform(text, steps))
        {
          Expected(
            "transform", *value,
            "none, or steps such as \"translate(10px, 50%) rotate(45deg) scale(1.5)\"");
          return false;
        }
        return true;
      }

      /// `background` of CSS, of which a colour, a gradient, or an image
      /// is read.
      bool Background(UiStyle &style) const
      {
        const auto *value = _reader.ReadValue("background");
        if (value == nullptr) { return false; }

        std::string text;
        Color color;
        UiGradient gradient;

        if (value->GetText(text))
        {
          if (text == "none")
          {
            style.background_color = {0.0f, 0.0f, 0.0f, 0.0f};
            style.background_image.clear();
            style.background_gradient.reset();
            return true;
          }

          if (ParseCssGradient(text, gradient))
          {
            style.background_gradient = gradient;
            return true;
          }

          if (ParseCssColor(text, color))
          {
            style.background_color = color;
            return true;
          }

          if (text.find("://") != std::string::npos && text.find("-gradient(") == std::string::npos)
          {
            style.background_image = text;
            return true;
          }
        }

        Expected(
          "background", *value,
          "none, a colour, a gradient such as linear-gradient(90deg, #f00, #00f), or the virtual path of an "
          "image");
        return false;
      }

      /// The values a shader is given: a number, a colour, a list of up to
      /// four numbers, or the name of a value of the game in brackets.
      bool ShaderValues(std::vector<UiShaderValue> &values) const
      {
        const auto *value = _reader.ReadValue("shader_values");
        if (value == nullptr) { return false; }

        if (!value->IsMap())
        {
          Expected("shader_values", *value, "a map of names and values, such as { intensity: 0.5 }");
          return false;
        }

        std::vector<UiShaderValue> read;
        bool is_read = true;

        for (const auto &[name, written] : value->GetEntries())
        {
          UiShaderValue each;
          each.name = name;

          std::string text;
          Color color;
          float number = 0.0f;

          if (written.GetNumber(number))
          {
            each.numbers[0] = number;
          } else if (bool flag = false; written.GetBool(flag))
          {
            each.numbers[0] = flag ? 1.0f : 0.0f;
          } else if (written.IsList() && !written.GetItems().empty() && written.GetItems().size() <= 4)
          {
            each.count = written.GetItems().size();
            for (std::size_t i = 0; i < each.count; i++)
            {
              if (!written.GetItems()[i].GetNumber(each.numbers[i])) { is_read = false; }
            }
          } else if (written.GetText(text) && text.size() > 2 && text.front() == '{' && text.back() == '}' &&
                     text.find(' ') == std::string::npos)
          {
            each.bound_to = text.substr(1, text.size() - 2);
          } else if (written.GetText(text) && ParseCssColor(text, color))
          {
            each.count = 4;
            each.numbers[0] = color.r;
            each.numbers[1] = color.g;
            each.numbers[2] = color.b;
            each.numbers[3] = color.a;
          } else
          {
            is_read = false;
          }

          if (!is_read)
          {
            _reader.Report(written, std::format(
                             "'{}' of 'shader_values' of {} is {}, where a number, a colour, a list of 1 to 4 "
                             "numbers, or a value such as \"{{charge}}\" was expected",
                             name, _reader.GetWhere(), Describe(written)));
            return false;
          }

          read.push_back(each);
        }

        values = read;
        return true;
      }

      bool LineHeight(UiStyle &style) const
      {
        const auto *value = _reader.ReadValue("line_height");
        if (value == nullptr) { return false; }

        float number = 0.0f;
        std::string text;

        // a number alone is a multiple of the size of the font, as in CSS
        if (value->GetNumber(number) && number >= 0.0f)
        {
          style.line_height = number;
          style.line_height_is_multiple = true;
          return true;
        }

        if (value->GetText(text))
        {
          if (text == "normal")
          {
            style.line_height = 0.0f;
            style.line_height_is_multiple = false;
            return true;
          }

          if (LayoutLength length; ParseCssLength(text, length) && !length.IsAuto() && length.value >= 0.0f)
          {
            const bool is_percent = length.unit == LayoutLength::Unit::Percent;
            style.line_height = is_percent ? length.value / 100.0f : length.value;
            style.line_height_is_multiple = is_percent;
            return true;
          }
        }

        Expected(
          "line_height", *value,
          "normal, a multiple of the size of the font such as 1.5, or a number of pixels such as \"24px\"");
        return false;
      }
    };
  }

  void ReadUiStyle(const DataReader &reader, UiStyle &style)
  {
    const BehaviourAtTheEnd behaviour(reader, style);

    const Properties properties(reader);
    LayoutStyle &layout = style.layout;

    // A shorthand is read before the properties it stands for, so that
    // `margin` and `margin_top` next to each other give what CSS gives
    // when the second is written below the first.

    properties.Keyword("display", displays, layout.display);
    properties.Keyword("position", positions, layout.position);
    properties.Keyword("box_sizing", box_sizings, layout.box_sizing);

    properties.Length("width", layout.width, true, true);
    properties.Length("height", layout.height, true, true);
    properties.Length("min_width", layout.min_width, true, true);
    properties.Length("min_height", layout.min_height, true, true);

    // `none` is what CSS calls no limit
    for (const auto &[name, length] : {
           std::pair<std::string, LayoutLength *>{"max_width", &layout.max_width},
           std::pair<std::string, LayoutLength *>{"max_height", &layout.max_height}
         })
    {
      const auto *value = reader.ReadValue(name);
      if (std::string text; value != nullptr && value->GetText(text) && text == "none")
      {
        *length = LayoutLength::Auto();
      } else
      {
        properties.Length(name, *length, true, false);
      }
    }

    properties.Edges("margin", layout.margin, true, true, true);
    properties.Length("margin_top", layout.margin.top, true, true);
    properties.Length("margin_right", layout.margin.right, true, true);
    properties.Length("margin_bottom", layout.margin.bottom, true, true);
    properties.Length("margin_left", layout.margin.left, true, true);

    properties.Edges("padding", layout.padding, true, false, false);
    properties.Length("padding_top", layout.padding.top, true, false);
    properties.Length("padding_right", layout.padding.right, true, false);
    properties.Length("padding_bottom", layout.padding.bottom, true, false);
    properties.Length("padding_left", layout.padding.left, true, false);

    properties.Border(style);
    properties.PixelEdges("border_width", layout.border);
    if (Color color; properties.Colour("border_color", color)) { style.border_color = color; }

    properties.Length("top", layout.inset.top, true, true);
    properties.Length("right", layout.inset.right, true, true);
    properties.Length("bottom", layout.inset.bottom, true, true);
    properties.Length("left", layout.inset.left, true, true);

    properties.Keyword("flex_direction", flex_directions, layout.flex_direction);
    properties.Keyword("flex_wrap", flex_wraps, layout.flex_wrap);
    properties.Keyword("justify_content", justify_contents, layout.justify_content);
    properties.Keyword("align_items", align_items, layout.align_items);
    properties.Keyword("align_self", align_selfs, layout.align_self);
    properties.Keyword("align_content", align_contents, layout.align_content);

    properties.Flex(layout);
    properties.Number("flex_grow", layout.flex_grow, 0.0f, 1e9f);
    properties.Number("flex_shrink", layout.flex_shrink, 0.0f, 1e9f);
    properties.Length("flex_basis", layout.flex_basis, true, true);

    properties.Gap(layout);
    properties.Pixels("row_gap", layout.row_gap);
    properties.Pixels("column_gap", layout.column_gap);

    properties.Colour("background_color", style.background_color);
    properties.Path("background_image", style.background_image);

    properties.Path("border_image_source", style.border_image_source);
    properties.PixelEdges("border_image_slice", style.border_image_slice);
    properties.PixelEdges("border_image_width", style.border_image_width);

    properties.Pixels("outline_width", style.outline_width);
    properties.Pixels("outline_offset", style.outline_offset, true);
    if (Color color; properties.Colour("outline_color", color)) { style.outline_color = color; }

    properties.Number("opacity", style.opacity, 0.0f, 1.0f);
    properties.Keyword("overflow", overflows, style.overflow);
    properties.Keyword("pointer_events", pointer_events, style.pointer_events);

    if (const auto *value = reader.ReadValue("z_index"); value != nullptr)
    {
      if (float z_index = 0.0f; value->GetNumber(z_index) && z_index == std::round(z_index) &&
                                std::abs(z_index) <= 1e6f)
      {
        style.z_index = static_cast<int>(z_index);
      } else
      {
        reader.Report(*value, std::format(
                        "'z_index' of {} is {}, where a whole number was expected",
                        reader.GetWhere(), Describe(*value)));
      }
    }

    properties.ColourOrGradient("color", style.color, style.color_gradient);
    reader.Read("font_family", style.font_family);

    if (float size = 0.0f; properties.Pixels("font_size", size))
    {
      if (size > 0.0f)
      {
        style.font_size = size;
      } else if (const auto *value = reader.ReadValue("font_size"); value != nullptr)
      {
        reader.Report(*value, std::format(
                        "'font_size' of {} is 0, where a number of pixels above 0 was expected",
                        reader.GetWhere()));
      }
    }

    properties.FontWeight(style.font_weight);
    properties.Keyword("text_align", text_aligns, style.text_align);
    properties.LineHeight(style);

    properties.Colour("accent_color", style.accent_color);
    properties.Keyword("object_fit", object_fits, style.object_fit);

    // Text

    properties.Spacing("letter_spacing", style.letter_spacing);
    properties.Spacing("word_spacing", style.word_spacing);
    properties.Keyword("text_transform", text_transforms, style.text_transform);

    properties.TextDecoration(style);
    properties.TextDecorationLine(style);
    if (Color color; properties.Colour("text_decoration_color", color)) { style.text_decoration_color = color; }
    properties.Pixels("text_decoration_thickness", style.text_decoration_thickness);

    properties.Shadows("text_shadow", true, style.text_shadow);
    properties.Keyword("white_space", white_spaces, style.white_space);
    properties.Keyword("text_overflow", text_overflows, style.text_overflow);

    if (std::size_t index = 0; reader.ReadChoice("font_style", font_styles, index)) { style.font_italic = index == 1; }

    properties.Pixels("text_stroke_width", style.text_stroke_width);
    if (Color color; properties.Colour("text_stroke_color", color)) { style.text_stroke_color = color; }

    properties.Keyword("direction", directions, style.direction);

    // Images

    properties.Keyword("image_rendering", image_renderings, style.image_rendering);
    properties.Place("object_position", style.object_position);

    properties.Background(style);
    properties.BackgroundSize(style.background_size);
    properties.Place("background_position", style.background_position);
    properties.Keyword("background_repeat", background_repeats, style.background_repeat);
    properties.Keyword("border_image_repeat", border_image_repeats, style.border_image_repeat);

    // `background_image` holds a gradient as well as an image, as in CSS
    if (UiGradient gradient; ParseCssGradient(style.background_image, gradient))
    {
      style.background_gradient = gradient;
      style.background_image.clear();
    } else if (style.background_image.find("-gradient(") != std::string::npos)
    {
      if (const auto *value = reader.ReadValue("background_image"); value != nullptr)
      {
        reader.Report(*value, std::format(
                        "'background_image' of {} is {}, where the virtual path of an image, none, or a "
                        "gradient such as linear-gradient(90deg, #f00, #00f) with 2 to 8 colours was expected",
                        reader.GetWhere(), Describe(*value)));
      }
      style.background_image.clear();
    }

    // The box

    properties.Radii(style.border_radius);
    properties.Radius("border_top_left_radius", style.border_radius.top_left);
    properties.Radius("border_top_right_radius", style.border_radius.top_right);
    properties.Radius("border_bottom_right_radius", style.border_radius.bottom_right);
    properties.Radius("border_bottom_left_radius", style.border_radius.bottom_left);

    properties.BorderSide("border_top", layout.border.top, style.border_top_color);
    properties.BorderSide("border_right", layout.border.right, style.border_right_color);
    properties.BorderSide("border_bottom", layout.border.bottom, style.border_bottom_color);
    properties.BorderSide("border_left", layout.border.left, style.border_left_color);

    properties.Pixels("border_top_width", layout.border.top);
    properties.Pixels("border_right_width", layout.border.right);
    properties.Pixels("border_bottom_width", layout.border.bottom);
    properties.Pixels("border_left_width", layout.border.left);

    if (Color color; properties.Colour("border_top_color", color)) { style.border_top_color = color; }
    if (Color color; properties.Colour("border_right_color", color)) { style.border_right_color = color; }
    if (Color color; properties.Colour("border_bottom_color", color)) { style.border_bottom_color = color; }
    if (Color color; properties.Colour("border_left_color", color)) { style.border_left_color = color; }

    properties.Shadows("box_shadow", false, style.box_shadow);

    properties.Transform(style.transform);
    properties.Place("transform_origin", style.transform_origin);

    if (properties.Path("shader", style.shader))
    {
      if (const auto *value = reader.ReadValue("shader"); !style.shader.empty() &&
                                                         style.shader.find("://") == std::string::npos)
      {
        reader.Report(*value, std::format(
                        "'shader' of {} is '{}', where a virtual path without an extension such as "
                        "assets://shaders/ui/shine, or none, was expected",
                        reader.GetWhere(), style.shader));
        style.shader.clear();
      }
    }
    properties.ShaderValues(style.shader_values);
  }
} // neon
