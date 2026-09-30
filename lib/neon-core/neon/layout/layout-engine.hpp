#ifndef LAYOUT_ENGINE_HPP
#define LAYOUT_ENGINE_HPP

#include <functional>
#include <vector>

#include "layout-style.hpp"

namespace neon
{
  /// Names a box of a layout engine.
  using LayoutNode = int;

  constexpr LayoutNode No_Layout_Node = -1;

  struct LayoutSize
  {
    float width = 0.0f;
    float height = 0.0f;
  };

  /// Says how large the content of a box is, such as a text. Each of the two
  /// sizes it is given is the room there is for the content, or not a number
  /// when there is no limit. A text breaks its lines at the width.
  using LayoutMeasure = std::function<LayoutSize(float available_width, float available_height)>;

  /// Where a box ended up. `left` and `top` are those of its border box,
  /// counted from the corner of the border box of its parent.
  struct LayoutBox
  {
    float left = 0.0f;
    float top = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    LayoutEdges<float> padding;
    LayoutEdges<float> border;
  };

  /// Places boxes by the rules of CSS: the box model, `position`, and CSS
  /// Flexible Box Layout.
  ///
  /// It is an interface so that the rules can come from a library that is
  /// tested against the specification, such as Yoga, without anything that
  /// uses layout knowing of it.
  class LayoutEngine
  {
  protected:
    ~LayoutEngine() = default;

  public:
    virtual LayoutNode CreateNode() = 0;

    /// Removes a box, from its parent as well. Its children stay and have
    /// no parent afterwards.
    virtual void DestroyNode(LayoutNode node) = 0;

    virtual void SetStyle(LayoutNode node, const LayoutStyle &style) = 0;

    /// The boxes inside a box, in the order of the document.
    virtual void SetChildren(LayoutNode node, const std::vector<LayoutNode> &children) = 0;

    /// For a box without children whose content has a size of its own.
    virtual void SetMeasure(LayoutNode node, const LayoutMeasure &measure) = 0;

    /// Places the box and everything inside it, in a room of the given size.
    virtual void Calculate(LayoutNode root, float width, float height) = 0;

    /// What the last Calculate() came to.
    [[nodiscard]] virtual LayoutBox GetBox(LayoutNode node) const = 0;
  };
} // neon

#endif //LAYOUT_ENGINE_HPP
