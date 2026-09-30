#include "flex-layout-engine.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

// The numbers in the comments, such as 9.7, are the sections of
// https://www.w3.org/TR/css-flexbox-1/ that the code next to them follows.

namespace neon
{
  namespace
  {
    const float undefined = std::numeric_limits<float>::quiet_NaN();
    const float unlimited = std::numeric_limits<float>::infinity();

    // sizes are compared with this much room for rounding
    constexpr float epsilon = 0.001f;

    bool IsDefined(const float value)
    {
      return !std::isnan(value);
    }

    bool Same(const float a, const float b)
    {
      return (std::isnan(a) && std::isnan(b)) || a == b;
    }

    /// Not a number for `auto`, and for a percentage of what is not known.
    float Resolve(const LayoutLength &length, const float base)
    {
      switch (length.unit)
      {
        case LayoutLength::Unit::Pixels: return length.value;
        case LayoutLength::Unit::Percent: return IsDefined(base) ? base * length.value / 100.0f : undefined;
        case LayoutLength::Unit::Sum:
          return IsDefined(base) ? length.value + base * length.percent / 100.0f : undefined;
        default: return undefined;
      }
    }

    float ResolveOr(const LayoutLength &length, const float base, const float otherwise)
    {
      const float value = Resolve(length, base);
      return IsDefined(value) ? value : otherwise;
    }

    /// Percentages of padding refer to the width, for all four sides.
    LayoutEdges<float> ResolvePadding(const LayoutStyle &style, const float owner_width)
    {
      return {
        std::max(0.0f, ResolveOr(style.padding.top, owner_width, 0.0f)),
        std::max(0.0f, ResolveOr(style.padding.right, owner_width, 0.0f)),
        std::max(0.0f, ResolveOr(style.padding.bottom, owner_width, 0.0f)),
        std::max(0.0f, ResolveOr(style.padding.left, owner_width, 0.0f))
      };
    }

    LayoutEdges<float> ResolveBorder(const LayoutStyle &style)
    {
      return {
        std::max(0.0f, style.border.top),
        std::max(0.0f, style.border.right),
        std::max(0.0f, style.border.bottom),
        std::max(0.0f, style.border.left)
      };
    }

    /// An `auto` margin counts as 0 until what is left over is handed out.
    LayoutEdges<float> ResolveMargin(const LayoutStyle &style, const float owner_width)
    {
      return {
        ResolveOr(style.margin.top, owner_width, 0.0f),
        ResolveOr(style.margin.right, owner_width, 0.0f),
        ResolveOr(style.margin.bottom, owner_width, 0.0f),
        ResolveOr(style.margin.left, owner_width, 0.0f)
      };
    }

    bool IsRow(const FlexDirection direction)
    {
      return direction == FlexDirection::Row || direction == FlexDirection::RowReverse;
    }

    bool IsReverse(const FlexDirection direction)
    {
      return direction == FlexDirection::RowReverse || direction == FlexDirection::ColumnReverse;
    }

    /// A size as it is written, as the size of the border box.
    float ToBorderBox(const LayoutStyle &style, const float value, const float padding_and_border)
    {
      if (!IsDefined(value)) { return undefined; }

      return style.box_sizing == LayoutBoxSizing::ContentBox
        ? std::max(0.0f, value) + padding_and_border
        : std::max(value, padding_and_border);
    }

    /// The least wins over the most, as in CSS.
    float Clamp(const float value, const float least, const float most)
    {
      return std::max(least, std::min(value, most));
    }

    AlignItems AlignOf(const LayoutStyle &parent, const LayoutStyle &child)
    {
      switch (child.align_self)
      {
        case AlignSelf::Stretch: return AlignItems::Stretch;
        case AlignSelf::FlexStart: return AlignItems::FlexStart;
        case AlignSelf::FlexEnd: return AlignItems::FlexEnd;
        case AlignSelf::Center: return AlignItems::Center;
        default: return parent.align_items;
      }
    }

    /// Where the first of `count` things starts and how far they are apart,
    /// when `free` is left over. What CSS falls back to when nothing is left
    /// over, or less, is part of it.
    void Distribute(
      const JustifyContent justify,
      const float free,
      const std::size_t count,
      float &start,
      float &between)
    {
      start = 0.0f;
      between = 0.0f;

      switch (justify)
      {
        case JustifyContent::FlexEnd:
          start = free;
          break;
        case JustifyContent::Center:
          start = free / 2.0f;
          break;
        case JustifyContent::SpaceBetween:
          if (free > 0.0f && count > 1) { between = free / static_cast<float>(count - 1); }
          break;
        case JustifyContent::SpaceAround:
          if (free > 0.0f && count > 0)
          {
            between = free / static_cast<float>(count);
            start = between / 2.0f;
          } else
          {
            start = free / 2.0f;
          }
          break;
        case JustifyContent::SpaceEvenly:
          if (free > 0.0f)
          {
            between = free / static_cast<float>(count + 1);
            start = between;
          } else
          {
            start = free / 2.0f;
          }
          break;
        default:
          break;
      }
    }

    JustifyContent AsJustify(const AlignContent align)
    {
      switch (align)
      {
        case AlignContent::FlexEnd: return JustifyContent::FlexEnd;
        case AlignContent::Center: return JustifyContent::Center;
        case AlignContent::SpaceBetween: return JustifyContent::SpaceBetween;
        case AlignContent::SpaceAround: return JustifyContent::SpaceAround;
        case AlignContent::SpaceEvenly: return JustifyContent::SpaceEvenly;
        default: return JustifyContent::FlexStart;
      }
    }
  }

  /// What a box that is taken out of the flow is placed against: the
  /// padding box of its parent, or the room for a box that has no parent.
  struct Flex_LayoutEngine::Block
  {
    /// The padding box, counted from the corner of the border box.
    float left = 0.0f;
    float top = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    /// What lies between the padding box and the content, where a box
    /// goes that names no side.
    LayoutEdges<float> padding;

    /// How the parent places its boxes, which a box that names no side
    /// follows.
    LayoutStyle style;
  };

  /// What a box is made of around its content, and the limits to its size.
  /// Every size is that of the border box.
  struct Flex_LayoutEngine::Frame
  {
    LayoutEdges<float> padding;
    LayoutEdges<float> border;
    float around_width = 0.0f;
    float around_height = 0.0f;
    float min_width = 0.0f;
    float max_width = unlimited;
    float min_height = 0.0f;
    float max_height = unlimited;

    [[nodiscard]] float ClampWidth(const float width) const
    {
      return Clamp(width, min_width, max_width);
    }

    [[nodiscard]] float ClampHeight(const float height) const
    {
      return Clamp(height, min_height, max_height);
    }
  };

  /// A box inside a flex container. `main` runs along the direction of the
  /// container and `cross` across it. Every size is that of the border box.
  struct Flex_LayoutEngine::Item
  {
    LayoutNode id = No_Layout_Node;

    float margin_main_start = 0.0f;
    float margin_main_end = 0.0f;
    float margin_cross_start = 0.0f;
    float margin_cross_end = 0.0f;
    bool auto_main_start = false;
    bool auto_main_end = false;
    bool auto_cross_start = false;
    bool auto_cross_end = false;

    float min_main = 0.0f;
    float max_main = unlimited;
    float min_cross = 0.0f;
    float max_cross = unlimited;

    /// The size across as it is written, or not a number.
    float written_cross = undefined;

    float base = 0.0f;
    float hypothetical = 0.0f;
    float target = 0.0f;
    bool frozen = false;

    float cross = 0.0f;
    bool stretches = false;

    /// Whether percentages inside the box can refer to its size along the
    /// direction, 9.8.
    bool main_is_definite = false;

    float main_position = 0.0f;
    float cross_position = 0.0f;

    [[nodiscard]] float MarginMain() const
    {
      return margin_main_start + margin_main_end;
    }

    [[nodiscard]] float MarginCross() const
    {
      return margin_cross_start + margin_cross_end;
    }
  };

  struct Flex_LayoutEngine::Line
  {
    std::size_t first = 0;
    std::size_t count = 0;
    float cross = 0.0f;
    float cross_position = 0.0f;
  };

  bool Flex_LayoutEngine::IsGiven(const Mode mode)
  {
    return mode == Mode::Exactly || mode == Mode::Fitted;
  }

  bool Flex_LayoutEngine::IsNode(const LayoutNode node) const
  {
    return node >= 0 && static_cast<std::size_t>(node) < _nodes.size() && _nodes[node].used;
  }

  LayoutNode Flex_LayoutEngine::CreateNode()
  {
    if (!_free.empty())
    {
      const LayoutNode node = _free.back();
      _free.pop_back();
      _nodes[node] = Node{};
      _nodes[node].used = true;
      return node;
    }

    _nodes.emplace_back();
    _nodes.back().used = true;
    return static_cast<LayoutNode>(_nodes.size()) - 1;
  }

  void Flex_LayoutEngine::DestroyNode(const LayoutNode node)
  {
    if (!IsNode(node)) { return; }

    if (const LayoutNode parent = _nodes[node].parent; IsNode(parent))
    {
      std::erase(_nodes[parent].children, node);
    }

    for (const LayoutNode child : _nodes[node].children)
    {
      if (IsNode(child)) { _nodes[child].parent = No_Layout_Node; }
    }

    _nodes[node] = Node{};
    _free.push_back(node);
  }

  void Flex_LayoutEngine::SetStyle(const LayoutNode node, const LayoutStyle &style)
  {
    if (IsNode(node)) { _nodes[node].style = style; }
  }

  void Flex_LayoutEngine::SetChildren(const LayoutNode node, const std::vector<LayoutNode> &children)
  {
    if (!IsNode(node)) { return; }

    for (const LayoutNode child : _nodes[node].children)
    {
      if (IsNode(child)) { _nodes[child].parent = No_Layout_Node; }
    }

    _nodes[node].children.clear();

    for (const LayoutNode child : children)
    {
      if (!IsNode(child) || child == node) { continue; }

      // a box is inside one box only
      if (const LayoutNode before = _nodes[child].parent; IsNode(before))
      {
        std::erase(_nodes[before].children, child);
      }

      _nodes[child].parent = node;
      _nodes[node].children.push_back(child);
    }
  }

  void Flex_LayoutEngine::SetMeasure(const LayoutNode node, const LayoutMeasure &measure)
  {
    if (IsNode(node)) { _nodes[node].measure = measure; }
  }

  LayoutBox Flex_LayoutEngine::GetBox(const LayoutNode node) const
  {
    return IsNode(node) ? _nodes[node].box : LayoutBox{};
  }

  void Flex_LayoutEngine::Calculate(const LayoutNode root, const float width, const float height)
  {
    if (!IsNode(root)) { return; }

    for (auto &node : _nodes) { node.measured.clear(); }

    const LayoutStyle &style = _nodes[root].style;
    if (style.display == LayoutDisplay::None)
    {
      Hide(root);
      return;
    }

    if (style.position == LayoutPosition::Absolute)
    {
      // placed against the room, as a box without a parent is in CSS
      Block room;
      room.width = width;
      room.height = height;

      PlaceAbsolute(room, root);
      return;
    }

    const auto padding = ResolvePadding(style, width);
    const auto border = ResolveBorder(style);
    const auto margin = ResolveMargin(style, width);

    // a root that says nothing about its size fills the room it is given
    float root_width = ToBorderBox(
      style, Resolve(style.width, width), padding.left + padding.right + border.left + border.right);
    float root_height = ToBorderBox(
      style, Resolve(style.height, height), padding.top + padding.bottom + border.top + border.bottom);

    if (!IsDefined(root_width)) { root_width = std::max(0.0f, width - margin.left - margin.right); }
    if (!IsDefined(root_height)) { root_height = std::max(0.0f, height - margin.top - margin.bottom); }

    Layout(root, root_width, Mode::Exactly, root_height, Mode::Exactly, width, height, true);

    _nodes[root].box.left = margin.left;
    _nodes[root].box.top = margin.top;
  }

  void Flex_LayoutEngine::Hide(const LayoutNode id)
  {
    _nodes[id].box = LayoutBox{};

    for (const LayoutNode child : _nodes[id].children) { Hide(child); }
  }

  LayoutSize Flex_LayoutEngine::Layout(
    const LayoutNode id,
    float width,
    const Mode width_mode,
    float height,
    Mode height_mode,
    const float owner_width,
    const float owner_height,
    const bool place)
  {
    // A height is as large as its content asks for. Only a width is held to
    // the room there is, which is what breaks the lines of a text.
    if (height_mode == Mode::AtMost) { height_mode = Mode::Undefined; }

    if (width_mode == Mode::Undefined) { width = undefined; }
    if (height_mode == Mode::Undefined) { height = undefined; }

    if (!place)
    {
      for (const auto &measured : _nodes[id].measured)
      {
        if (measured.width_mode == width_mode && measured.height_mode == height_mode &&
            Same(measured.width, width) && Same(measured.height, height) &&
            Same(measured.owner_width, owner_width) && Same(measured.owner_height, owner_height))
        {
          return measured.size;
        }
      }
    }

    const LayoutStyle &style = _nodes[id].style;

    Frame frame;
    frame.padding = ResolvePadding(style, owner_width);
    frame.border = ResolveBorder(style);
    frame.around_width = frame.padding.left + frame.padding.right + frame.border.left + frame.border.right;
    frame.around_height = frame.padding.top + frame.padding.bottom + frame.border.top + frame.border.bottom;

    const float min_width = ToBorderBox(style, Resolve(style.min_width, owner_width), frame.around_width);
    const float max_width = ToBorderBox(style, Resolve(style.max_width, owner_width), frame.around_width);
    const float min_height = ToBorderBox(style, Resolve(style.min_height, owner_height), frame.around_height);
    const float max_height = ToBorderBox(style, Resolve(style.max_height, owner_height), frame.around_height);

    frame.min_width = std::max(frame.around_width, IsDefined(min_width) ? min_width : 0.0f);
    frame.max_width = IsDefined(max_width) ? max_width : unlimited;
    frame.min_height = std::max(frame.around_height, IsDefined(min_height) ? min_height : 0.0f);
    frame.max_height = IsDefined(max_height) ? max_height : unlimited;

    if (IsGiven(width_mode)) { width = std::max(width, frame.around_width); }
    if (IsGiven(height_mode)) { height = std::max(height, frame.around_height); }

    LayoutSize size;

    if (_nodes[id].children.empty())
    {
      LayoutSize content;

      if (_nodes[id].measure && (!IsGiven(width_mode) || !IsGiven(height_mode)))
      {
        content = _nodes[id].measure(
          width_mode == Mode::Undefined ? undefined : std::max(0.0f, width - frame.around_width),
          height_mode == Mode::Undefined ? undefined : std::max(0.0f, height - frame.around_height));
      }

      if (IsGiven(width_mode))
      {
        size.width = width;
      } else
      {
        size.width = content.width + frame.around_width;
        if (width_mode == Mode::AtMost) { size.width = std::min(size.width, width); }
        size.width = frame.ClampWidth(size.width);
      }

      size.height = IsGiven(height_mode)
        ? height
        : frame.ClampHeight(content.height + frame.around_height);
    } else
    {
      size = LayoutContainer(id, frame, width, width_mode, height, height_mode, place);
    }

    if (place)
    {
      Node &node = _nodes[id];
      node.box.width = size.width;
      node.box.height = size.height;
      node.box.padding = frame.padding;
      node.box.border = frame.border;

      // 4.1: a box that is taken out of the flow is placed against the
      // padding box of its parent, whose size is known by now
      Block block;
      block.left = frame.border.left;
      block.top = frame.border.top;
      block.width = std::max(0.0f, size.width - frame.border.left - frame.border.right);
      block.height = std::max(0.0f, size.height - frame.border.top - frame.border.bottom);
      block.padding = frame.padding;
      block.style = node.style;

      const std::vector<LayoutNode> children = node.children;
      for (const LayoutNode child : children)
      {
        const LayoutStyle &child_style = _nodes[child].style;
        if (child_style.display != LayoutDisplay::None && child_style.position == LayoutPosition::Absolute)
        {
          PlaceAbsolute(block, child);
        }
      }
    } else
    {
      _nodes[id].measured.push_back({width, width_mode, height, height_mode, owner_width, owner_height, size});
    }

    return size;
  }

  LayoutSize Flex_LayoutEngine::LayoutContainer(
    const LayoutNode id,
    const Frame &frame,
    const float width,
    const Mode width_mode,
    const float height,
    const Mode height_mode,
    const bool place)
  {
    // a copy, since a box that is measured is handed to Layout() in turn
    const LayoutStyle style = _nodes[id].style;
    const std::vector<LayoutNode> children = _nodes[id].children;

    const bool row = IsRow(style.flex_direction);
    const bool wraps = style.flex_wrap != FlexWrap::NoWrap;

    // what percentages of the boxes inside refer to
    const float inner_width = width_mode == Mode::Exactly ? width - frame.around_width : undefined;
    const float inner_height = height_mode == Mode::Exactly ? height - frame.around_height : undefined;

    // the room there is, which is the size or a limit to it
    const float room_width = width_mode == Mode::Undefined ? undefined : std::max(0.0f, width - frame.around_width);
    const float room_height = IsGiven(height_mode) ? height - frame.around_height : undefined;

    const Mode main_mode = row ? width_mode : height_mode;
    const Mode cross_mode = row ? height_mode : width_mode;
    const float room_main = row ? room_width : room_height;
    const float room_cross = row ? room_height : room_width;
    const float main_gap = std::max(0.0f, row ? style.column_gap : style.row_gap);
    const float cross_gap = std::max(0.0f, row ? style.row_gap : style.column_gap);

    // calls Layout() with sizes along and across the direction
    const auto layout = [&](
      const LayoutNode child,
      const float main,
      const Mode child_main_mode,
      const float cross,
      const Mode child_cross_mode,
      const bool place_child)
    {
      const LayoutSize size = row
        ? Layout(child, main, child_main_mode, cross, child_cross_mode, inner_width, inner_height, place_child)
        : Layout(child, cross, child_cross_mode, main, child_main_mode, inner_width, inner_height, place_child);

      return row ? size : LayoutSize{size.height, size.width};
    };

    // 9.2: the boxes of the flow, and the size each would like to have
    std::vector<Item> items;
    items.reserve(children.size());

    for (const LayoutNode child : children)
    {
      const LayoutStyle &child_style = _nodes[child].style;

      if (child_style.display == LayoutDisplay::None)
      {
        if (place) { Hide(child); }
        continue;
      }

      if (child_style.position == LayoutPosition::Absolute) { continue; }

      const auto padding = ResolvePadding(child_style, inner_width);
      const auto border = ResolveBorder(child_style);
      const auto margin = ResolveMargin(child_style, inner_width);
      const float around_width = padding.left + padding.right + border.left + border.right;
      const float around_height = padding.top + padding.bottom + border.top + border.bottom;
      const float around_main = row ? around_width : around_height;

      const float written_width = ToBorderBox(child_style, Resolve(child_style.width, inner_width), around_width);
      const float written_height = ToBorderBox(
        child_style, Resolve(child_style.height, inner_height), around_height);

      const float min_width = ToBorderBox(child_style, Resolve(child_style.min_width, inner_width), around_width);
      const float max_width = ToBorderBox(child_style, Resolve(child_style.max_width, inner_width), around_width);
      const float min_height = ToBorderBox(
        child_style, Resolve(child_style.min_height, inner_height), around_height);
      const float max_height = ToBorderBox(
        child_style, Resolve(child_style.max_height, inner_height), around_height);

      const float least_width = std::max(around_width, IsDefined(min_width) ? min_width : 0.0f);
      const float most_width = IsDefined(max_width) ? max_width : unlimited;
      const float least_height = std::max(around_height, IsDefined(min_height) ? min_height : 0.0f);
      const float most_height = IsDefined(max_height) ? max_height : unlimited;

      Item item;
      item.id = child;

      item.margin_main_start = row ? margin.left : margin.top;
      item.margin_main_end = row ? margin.right : margin.bottom;
      item.margin_cross_start = row ? margin.top : margin.left;
      item.margin_cross_end = row ? margin.bottom : margin.right;
      item.auto_main_start = (row ? child_style.margin.left : child_style.margin.top).IsAuto();
      item.auto_main_end = (row ? child_style.margin.right : child_style.margin.bottom).IsAuto();
      item.auto_cross_start = (row ? child_style.margin.top : child_style.margin.left).IsAuto();
      item.auto_cross_end = (row ? child_style.margin.bottom : child_style.margin.right).IsAuto();

      item.min_main = row ? least_width : least_height;
      item.max_main = row ? most_width : most_height;
      item.min_cross = row ? least_height : least_width;
      item.max_cross = row ? most_height : most_width;
      item.written_cross = row ? written_height : written_width;

      item.stretches = AlignOf(style, child_style) == AlignItems::Stretch &&
                       !IsDefined(item.written_cross) &&
                       !item.auto_cross_start && !item.auto_cross_end;

      const float written_main = row ? written_width : written_height;
      const float basis = ToBorderBox(
        child_style, Resolve(child_style.flex_basis, row ? inner_width : inner_height), around_main);

      item.main_is_definite = IsDefined(basis) || IsDefined(written_main) || main_mode == Mode::Exactly;

      if (IsDefined(basis))
      {
        item.base = basis;
      } else if (IsDefined(written_main))
      {
        item.base = written_main;
      } else
      {
        // the size of the content
        float cross = undefined;
        Mode child_cross_mode = Mode::Undefined;

        if (IsDefined(item.written_cross))
        {
          cross = Clamp(item.written_cross, item.min_cross, item.max_cross);
          child_cross_mode = Mode::Exactly;
        } else if (item.stretches && IsGiven(cross_mode) && !wraps)
        {
          cross = Clamp(room_cross - item.MarginCross(), item.min_cross, item.max_cross);
          child_cross_mode = Mode::Exactly;
        } else if (IsDefined(room_cross))
        {
          cross = std::max(0.0f, room_cross - item.MarginCross());
          child_cross_mode = Mode::AtMost;
        }

        item.base = layout(child, undefined, Mode::Undefined, cross, child_cross_mode, false).width;
      }

      item.hypothetical = Clamp(item.base, item.min_main, item.max_main);
      item.target = item.hypothetical;
      items.push_back(item);
    }

    // 9.3: the lines
    std::vector<Line> lines;

    if (!wraps || !IsDefined(room_main))
    {
      lines.push_back({0, items.size(), 0.0f, 0.0f});
    } else
    {
      Line line;
      float used = 0.0f;

      for (std::size_t i = 0; i < items.size(); i++)
      {
        const float outer = items[i].hypothetical + items[i].MarginMain();

        if (line.count > 0 && used + main_gap + outer > room_main + epsilon)
        {
          lines.push_back(line);
          line = {i, 0, 0.0f, 0.0f};
          used = 0.0f;
        }

        used += (line.count > 0 ? main_gap : 0.0f) + outer;
        line.count++;
      }

      lines.push_back(line);
    }

    // 9.2, 4: the size along the direction
    float inner_main;

    if (IsGiven(main_mode))
    {
      inner_main = room_main;
    } else
    {
      float content = 0.0f;
      for (const auto &line : lines)
      {
        float used = 0.0f;
        for (std::size_t i = line.first; i < line.first + line.count; i++)
        {
          used += items[i].hypothetical + items[i].MarginMain();
        }
        if (line.count > 1) { used += main_gap * static_cast<float>(line.count - 1); }
        content = std::max(content, used);
      }

      inner_main = content;
      if (main_mode == Mode::AtMost) { inner_main = std::min(inner_main, room_main); }

      inner_main = row
        ? frame.ClampWidth(inner_main + frame.around_width) - frame.around_width
        : frame.ClampHeight(inner_main + frame.around_height) - frame.around_height;
    }

    // 9.7: what is left over is handed to the boxes that grow, and what is
    // missing is taken from those that shrink
    for (const auto &line : lines)
    {
      const std::size_t end = line.first + line.count;
      const float gaps = line.count > 1 ? main_gap * static_cast<float>(line.count - 1) : 0.0f;

      float hypothetical = gaps;
      for (std::size_t i = line.first; i < end; i++)
      {
        hypothetical += items[i].hypothetical + items[i].MarginMain();
      }

      const bool grows = hypothetical < inner_main;

      const auto factor_of = [&](const Item &item)
      {
        const LayoutStyle &child_style = _nodes[item.id].style;
        return std::max(0.0f, grows ? child_style.flex_grow : child_style.flex_shrink);
      };

      for (std::size_t i = line.first; i < end; i++)
      {
        Item &item = items[i];
        const float factor = factor_of(item);

        item.frozen = factor == 0.0f ||
                      (grows && item.base > item.hypothetical) ||
                      (!grows && item.base < item.hypothetical);
        item.target = item.frozen ? item.hypothetical : item.base;
      }

      const auto free_space = [&]
      {
        float used = gaps;
        for (std::size_t i = line.first; i < end; i++)
        {
          used += (items[i].frozen ? items[i].target : items[i].base) + items[i].MarginMain();
        }
        return inner_main - used;
      };

      const float initial_free = free_space();

      // every round freezes a box, so there are no more rounds than boxes
      for (std::size_t round = 0; round <= line.count; round++)
      {
        float factors = 0.0f;
        float scaled_factors = 0.0f;
        bool any = false;

        for (std::size_t i = line.first; i < end; i++)
        {
          if (items[i].frozen) { continue; }

          any = true;
          factors += factor_of(items[i]);
          scaled_factors += factor_of(items[i]) * items[i].base;
        }

        if (!any) { break; }

        float free = free_space();

        // factors that add up to less than 1 hand out that part only
        if (factors < 1.0f && std::abs(initial_free * factors) < std::abs(free))
        {
          free = initial_free * factors;
        }

        for (std::size_t i = line.first; i < end; i++)
        {
          Item &item = items[i];
          if (item.frozen) { continue; }

          if (grows)
          {
            item.target = item.base + (factors > 0.0f ? free * factor_of(item) / factors : 0.0f);
          } else
          {
            // a box shrinks by its factor times its size, so that a large
            // box gives up more than a small one
            item.target = item.base + (scaled_factors > 0.0f
              ? free * factor_of(item) * item.base / scaled_factors
              : 0.0f);
          }
        }

        float violation = 0.0f;
        for (std::size_t i = line.first; i < end; i++)
        {
          if (items[i].frozen) { continue; }
          violation += Clamp(items[i].target, items[i].min_main, items[i].max_main) - items[i].target;
        }

        for (std::size_t i = line.first; i < end; i++)
        {
          Item &item = items[i];
          if (item.frozen) { continue; }

          const float clamped = Clamp(item.target, item.min_main, item.max_main);
          const bool was_too_small = clamped > item.target;
          const bool was_too_large = clamped < item.target;
          item.target = clamped;

          if (std::abs(violation) <= epsilon ||
              (violation > 0.0f && was_too_small) ||
              (violation < 0.0f && was_too_large))
          {
            item.frozen = true;
          }
        }
      }
    }

    // 9.4: the size across the direction
    for (auto &item : items)
    {
      if (IsDefined(item.written_cross))
      {
        item.cross = Clamp(item.written_cross, item.min_cross, item.max_cross);
        continue;
      }

      float cross = undefined;
      Mode child_cross_mode = Mode::Undefined;

      if (IsDefined(room_cross))
      {
        cross = std::max(0.0f, room_cross - item.MarginCross());
        child_cross_mode = Mode::AtMost;
      }

      item.cross = Clamp(
        layout(
          item.id,
          item.target,
          item.main_is_definite ? Mode::Exactly : Mode::Fitted,
          cross,
          child_cross_mode,
          false).height,
        item.min_cross,
        item.max_cross);
    }

    float content_cross = lines.size() > 1 ? cross_gap * static_cast<float>(lines.size() - 1) : 0.0f;

    for (auto &line : lines)
    {
      for (std::size_t i = line.first; i < line.first + line.count; i++)
      {
        line.cross = std::max(line.cross, items[i].cross + items[i].MarginCross());
      }
      content_cross += line.cross;
    }

    float inner_cross;

    if (IsGiven(cross_mode))
    {
      inner_cross = room_cross;
    } else
    {
      inner_cross = content_cross;
      if (cross_mode == Mode::AtMost) { inner_cross = std::min(inner_cross, room_cross); }

      inner_cross = row
        ? frame.ClampHeight(inner_cross + frame.around_height) - frame.around_height
        : frame.ClampWidth(inner_cross + frame.around_width) - frame.around_width;
    }

    // the one line of a container that does not wrap is as large as the
    // container
    if (!wraps) { lines.front().cross = inner_cross; }

    const LayoutSize size = row
      ? LayoutSize{inner_main + frame.around_width, inner_cross + frame.around_height}
      : LayoutSize{inner_cross + frame.around_width, inner_main + frame.around_height};

    if (!place) { return size; }

    // 9.4, 9.6: the lines across the direction
    if (wraps)
    {
      const float free = inner_cross - content_cross;

      if (style.align_content == AlignContent::Stretch && free > 0.0f)
      {
        for (auto &line : lines) { line.cross += free / static_cast<float>(lines.size()); }
      }

      float start = 0.0f;
      float between = 0.0f;
      if (style.align_content != AlignContent::Stretch)
      {
        Distribute(AsJustify(style.align_content), free, lines.size(), start, between);
      }

      float position = start;
      for (auto &line : lines)
      {
        line.cross_position = position;
        position += line.cross + cross_gap + between;
      }
    }

    for (const auto &line : lines)
    {
      const std::size_t end = line.first + line.count;

      // 9.4, 11: a box that stretches is as large as its line
      for (std::size_t i = line.first; i < end; i++)
      {
        Item &item = items[i];
        if (item.stretches)
        {
          item.cross = Clamp(line.cross - item.MarginCross(), item.min_cross, item.max_cross);
        }
      }

      // 9.5: along the direction. Margins that are `auto` take what is left
      // over before anything else gets it
      float used = line.count > 1 ? main_gap * static_cast<float>(line.count - 1) : 0.0f;
      std::size_t auto_margins = 0;

      for (std::size_t i = line.first; i < end; i++)
      {
        used += items[i].target + items[i].MarginMain();
        auto_margins += (items[i].auto_main_start ? 1 : 0) + (items[i].auto_main_end ? 1 : 0);
      }

      const float free = inner_main - used;
      float start = 0.0f;
      float between = 0.0f;
      float auto_margin = 0.0f;

      if (auto_margins > 0)
      {
        auto_margin = std::max(0.0f, free) / static_cast<float>(auto_margins);
      }

      if (auto_margins == 0 || free < 0.0f)
      {
        Distribute(style.justify_content, free, line.count, start, between);
      }

      float position = start;
      for (std::size_t i = line.first; i < end; i++)
      {
        Item &item = items[i];

        position += item.margin_main_start + (item.auto_main_start ? auto_margin : 0.0f);
        item.main_position = position;
        position += item.target + item.margin_main_end + (item.auto_main_end ? auto_margin : 0.0f);
        position += main_gap + between;
      }

      // 9.6: across the direction
      for (std::size_t i = line.first; i < end; i++)
      {
        Item &item = items[i];
        const float left_over = line.cross - item.cross - item.MarginCross();
        float offset = 0.0f;

        if (item.auto_cross_start || item.auto_cross_end)
        {
          if (left_over > 0.0f)
          {
            if (item.auto_cross_start && item.auto_cross_end)
            {
              offset = left_over / 2.0f;
            } else if (item.auto_cross_start)
            {
              offset = left_over;
            }
          }
        } else
        {
          switch (AlignOf(style, _nodes[item.id].style))
          {
            case AlignItems::FlexEnd:
              offset = left_over;
              break;
            case AlignItems::Center:
              offset = left_over / 2.0f;
              break;
            default:
              break;
          }
        }

        item.cross_position = line.cross_position + item.margin_cross_start + offset;
      }
    }

    // a direction that is reversed is the same picture in a mirror
    const bool mirror_main = IsReverse(style.flex_direction);
    const bool mirror_cross = style.flex_wrap == FlexWrap::WrapReverse;

    for (auto &item : items)
    {
      if (mirror_main) { item.main_position = inner_main - item.main_position - item.target; }
      if (mirror_cross) { item.cross_position = inner_cross - item.cross_position - item.cross; }

      // 9.8: a size that was written or stretched is one that percentages
      // inside the box refer to. One that followed from content is not
      layout(
        item.id,
        item.target,
        item.main_is_definite ? Mode::Exactly : Mode::Fitted,
        item.cross,
        IsDefined(item.written_cross) || item.stretches ? Mode::Exactly : Mode::Fitted,
        true);

      Node &child = _nodes[item.id];
      child.box.left = frame.border.left + frame.padding.left + (row ? item.main_position : item.cross_position);
      child.box.top = frame.border.top + frame.padding.top + (row ? item.cross_position : item.main_position);

      // `position: relative` moves the box from where it was placed, and
      // leaves the boxes around it where they are
      const LayoutStyle &child_style = child.style;
      const float content_width = row ? inner_main : inner_cross;
      const float content_height = row ? inner_cross : inner_main;
      const float left = Resolve(child_style.inset.left, content_width);
      const float right = Resolve(child_style.inset.right, content_width);
      const float top = Resolve(child_style.inset.top, content_height);
      const float bottom = Resolve(child_style.inset.bottom, content_height);

      if (IsDefined(left))
      {
        child.box.left += left;
      } else if (IsDefined(right))
      {
        child.box.left -= right;
      }

      if (IsDefined(top))
      {
        child.box.top += top;
      } else if (IsDefined(bottom))
      {
        child.box.top -= bottom;
      }
    }

    return size;
  }

  void Flex_LayoutEngine::PlaceAbsolute(const Block &block, const LayoutNode id)
  {
    const LayoutStyle &parent_style = block.style;
    const LayoutStyle style = _nodes[id].style;

    const float block_left = block.left;
    const float block_top = block.top;
    const float block_width = block.width;
    const float block_height = block.height;

    const auto padding = ResolvePadding(style, block_width);
    const auto border = ResolveBorder(style);
    const auto margin = ResolveMargin(style, block_width);
    const float around_width = padding.left + padding.right + border.left + border.right;
    const float around_height = padding.top + padding.bottom + border.top + border.bottom;

    const float left = Resolve(style.inset.left, block_width);
    const float right = Resolve(style.inset.right, block_width);
    const float top = Resolve(style.inset.top, block_height);
    const float bottom = Resolve(style.inset.bottom, block_height);

    const float min_width = ToBorderBox(style, Resolve(style.min_width, block_width), around_width);
    const float max_width = ToBorderBox(style, Resolve(style.max_width, block_width), around_width);
    const float min_height = ToBorderBox(style, Resolve(style.min_height, block_height), around_height);
    const float max_height = ToBorderBox(style, Resolve(style.max_height, block_height), around_height);

    const float least_width = std::max(around_width, IsDefined(min_width) ? min_width : 0.0f);
    const float most_width = IsDefined(max_width) ? max_width : unlimited;
    const float least_height = std::max(around_height, IsDefined(min_height) ? min_height : 0.0f);
    const float most_height = IsDefined(max_height) ? max_height : unlimited;

    float width = ToBorderBox(style, Resolve(style.width, block_width), around_width);
    Mode width_mode = Mode::Exactly;
    bool width_is_written = IsDefined(width);

    if (!IsDefined(width))
    {
      if (IsDefined(left) && IsDefined(right))
      {
        // both sides are held, so the box fills what is between them
        width = block_width - left - right - margin.left - margin.right;
      } else
      {
        width = block_width - margin.left - margin.right -
                (IsDefined(left) ? left : 0.0f) - (IsDefined(right) ? right : 0.0f);
        width_mode = Mode::AtMost;
      }
    }

    float height = ToBorderBox(style, Resolve(style.height, block_height), around_height);
    Mode height_mode = Mode::Exactly;
    bool height_is_written = IsDefined(height);

    if (!IsDefined(height))
    {
      if (IsDefined(top) && IsDefined(bottom))
      {
        height = block_height - top - bottom - margin.top - margin.bottom;
      } else
      {
        height = undefined;
        height_mode = Mode::Undefined;
      }
    }

    if (width_mode == Mode::Exactly) { width = Clamp(width, least_width, most_width); }
    if (width_mode == Mode::AtMost) { width = std::max(0.0f, width); }
    if (height_mode == Mode::Exactly) { height = Clamp(height, least_height, most_height); }

    LayoutSize size = Layout(id, width, width_mode, height, height_mode, block_width, block_height, false);

    // what followed from the content is held to the limits as well
    size.width = Clamp(size.width, least_width, most_width);
    size.height = Clamp(size.height, least_height, most_height);
    Layout(
      id,
      size.width,
      width_mode == Mode::Exactly ? Mode::Exactly : Mode::Fitted,
      size.height,
      height_mode == Mode::Exactly ? Mode::Exactly : Mode::Fitted,
      block_width,
      block_height,
      true);

    const bool row = IsRow(parent_style.flex_direction);
    const bool reverse = IsReverse(parent_style.flex_direction);
    const bool wrap_reverse = parent_style.flex_wrap == FlexWrap::WrapReverse;

    // 4.1: a box that names no side is placed as if it were the only box
    // of the flow
    const auto along = [&](const float free)
    {
      float start = 0.0f;
      float between = 0.0f;
      Distribute(parent_style.justify_content, free, 1, start, between);

      if (parent_style.justify_content == JustifyContent::SpaceBetween) { start = 0.0f; }
      if (parent_style.justify_content == JustifyContent::SpaceAround ||
          parent_style.justify_content == JustifyContent::SpaceEvenly)
      {
        start = free / 2.0f;
      }

      return reverse ? free - start : start;
    };

    const auto across = [&](const float free)
    {
      float start = 0.0f;
      switch (AlignOf(parent_style, style))
      {
        case AlignItems::FlexEnd:
          start = free;
          break;
        case AlignItems::Center:
          start = free / 2.0f;
          break;
        default:
          break;
      }

      return wrap_reverse ? free - start : start;
    };

    Node &node = _nodes[id];

    if (IsDefined(left))
    {
      float margin_left = margin.left;

      // margins that are `auto` share what is left over between two sides
      // that are held
      if (IsDefined(right) && width_is_written)
      {
        const float free = block_width - left - right - size.width - margin.left - margin.right;
        const bool auto_left = style.margin.left.IsAuto();
        const bool auto_right = style.margin.right.IsAuto();

        if (auto_left && auto_right)
        {
          margin_left = std::max(0.0f, free / 2.0f);
        } else if (auto_left)
        {
          margin_left = free;
        }
      }

      node.box.left = block_left + left + margin_left;
    } else if (IsDefined(right))
    {
      node.box.left = block_left + block_width - right - margin.right - size.width;
    } else
    {
      const float content_width = block_width - block.padding.left - block.padding.right;
      const float free = content_width - size.width - margin.left - margin.right;

      node.box.left = block_left + block.padding.left + margin.left + (row ? along(free) : across(free));
    }

    if (IsDefined(top))
    {
      float margin_top = margin.top;

      if (IsDefined(bottom) && height_is_written)
      {
        const float free = block_height - top - bottom - size.height - margin.top - margin.bottom;
        const bool auto_top = style.margin.top.IsAuto();
        const bool auto_bottom = style.margin.bottom.IsAuto();

        if (auto_top && auto_bottom)
        {
          margin_top = std::max(0.0f, free / 2.0f);
        } else if (auto_top)
        {
          margin_top = free;
        }
      }

      node.box.top = block_top + top + margin_top;
    } else if (IsDefined(bottom))
    {
      node.box.top = block_top + block_height - bottom - margin.bottom - size.height;
    } else
    {
      const float content_height = block_height - block.padding.top - block.padding.bottom;
      const float free = content_height - size.height - margin.top - margin.bottom;

      node.box.top = block_top + block.padding.top + margin.top + (row ? across(free) : along(free));
    }
  }
} // neon
