#ifndef LAYOUT_STYLE_HPP
#define LAYOUT_STYLE_HPP

// The names and the meanings are those of CSS: the box model, positioned
// layout, and CSS Flexible Box Layout. What a value does is said by
// https://www.w3.org/TR/css-flexbox-1/ and not by this file.

namespace neon
{
  /// A length of CSS: a number of pixels, a percentage, or `auto`.
  struct LayoutLength
  {
    enum class Unit
    {
      Auto = 0,
      Pixels,
      Percent,

      /// Pixels and a percentage added up, which is what `calc()` of CSS
      /// comes to when it holds both: `calc(100% - 20px)`.
      Sum
    };

    Unit unit = Unit::Auto;

    /// The pixels, or the percentage. For a sum, the pixels.
    float value = 0.0f;

    /// The percentage of a sum.
    float percent = 0.0f;

    static LayoutLength Sum(const float pixels, const float percent)
    {
      return {Unit::Sum, pixels, percent};
    }

    /// Whether the length holds a percentage, which needs something to be
    /// a percentage of.
    [[nodiscard]] bool HasPercent() const
    {
      return unit == Unit::Percent || unit == Unit::Sum;
    }

    static LayoutLength Auto()
    {
      return {};
    }

    static LayoutLength Pixels(const float pixels)
    {
      return {Unit::Pixels, pixels};
    }

    static LayoutLength Percent(const float percent)
    {
      return {Unit::Percent, percent};
    }

    [[nodiscard]] bool IsAuto() const
    {
      return unit == Unit::Auto;
    }

    bool operator==(const LayoutLength &other) const = default;
  };

  /// A value for each side of a box, in the order CSS names them.
  template<typename T>
  struct LayoutEdges
  {
    T top{};
    T right{};
    T bottom{};
    T left{};

    bool operator==(const LayoutEdges &other) const = default;
  };

  enum class LayoutDisplay
  {
    Flex = 0,
    None
  };

  enum class LayoutPosition
  {
    Relative = 0,
    Absolute
  };

  enum class LayoutBoxSizing
  {
    ContentBox = 0,
    BorderBox
  };

  enum class FlexDirection
  {
    Row = 0,
    RowReverse,
    Column,
    ColumnReverse
  };

  enum class FlexWrap
  {
    NoWrap = 0,
    Wrap,
    WrapReverse
  };

  enum class JustifyContent
  {
    FlexStart = 0,
    FlexEnd,
    Center,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
  };

  enum class AlignItems
  {
    Stretch = 0,
    FlexStart,
    FlexEnd,
    Center
  };

  enum class AlignSelf
  {
    Auto = 0,
    Stretch,
    FlexStart,
    FlexEnd,
    Center
  };

  enum class AlignContent
  {
    Stretch = 0,
    FlexStart,
    FlexEnd,
    Center,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
  };

  /// What decides where a box goes and how large it is. Every value starts
  /// as the initial value CSS gives it, with one exception: `position`
  /// starts as `relative`, so that a box with `position: absolute` is always
  /// placed against its parent.
  struct LayoutStyle
  {
    LayoutDisplay display = LayoutDisplay::Flex;
    LayoutPosition position = LayoutPosition::Relative;
    LayoutBoxSizing box_sizing = LayoutBoxSizing::ContentBox;

    LayoutLength width;
    LayoutLength height;

    /// `auto` stands for 0 as the least, and for no limit as the most.
    LayoutLength min_width;
    LayoutLength min_height;
    LayoutLength max_width;
    LayoutLength max_height;

    LayoutEdges<LayoutLength> margin{
      LayoutLength::Pixels(0.0f),
      LayoutLength::Pixels(0.0f),
      LayoutLength::Pixels(0.0f),
      LayoutLength::Pixels(0.0f)
    };

    LayoutEdges<LayoutLength> padding{
      LayoutLength::Pixels(0.0f),
      LayoutLength::Pixels(0.0f),
      LayoutLength::Pixels(0.0f),
      LayoutLength::Pixels(0.0f)
    };

    /// The widths of the border, in pixels.
    LayoutEdges<float> border;

    /// `top`, `right`, `bottom`, and `left` of CSS.
    LayoutEdges<LayoutLength> inset;

    FlexDirection flex_direction = FlexDirection::Row;
    FlexWrap flex_wrap = FlexWrap::NoWrap;
    JustifyContent justify_content = JustifyContent::FlexStart;
    AlignItems align_items = AlignItems::Stretch;
    AlignSelf align_self = AlignSelf::Auto;
    AlignContent align_content = AlignContent::Stretch;

    float flex_grow = 0.0f;
    float flex_shrink = 1.0f;
    LayoutLength flex_basis;

    /// Between rows, and between columns, in pixels.
    float row_gap = 0.0f;
    float column_gap = 0.0f;

    bool operator==(const LayoutStyle &other) const = default;
  };
} // neon

#endif //LAYOUT_STYLE_HPP
