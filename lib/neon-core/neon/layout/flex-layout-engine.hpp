#ifndef FLEX_LAYOUT_ENGINE_HPP
#define FLEX_LAYOUT_ENGINE_HPP

#include <vector>

#include "layout-engine.hpp"

namespace neon
{
  /// The layout engine of the engine itself. It follows the algorithm of
  /// https://www.w3.org/TR/css-flexbox-1/#layout-algorithm and uses no
  /// library.
  ///
  /// What it leaves out of the specification:
  ///   - `align-items: baseline`, and `order`
  ///   - the least size a box gets from its content, `min-width: auto`.
  ///     A box shrinks below its content unless `min_width` says otherwise
  ///   - `visibility: collapse`, and writing modes. Rows run from left to
  ///     right and columns from top to bottom
  ///   - gaps in percent
  // ReSharper disable once CppInconsistentNaming
  class Flex_LayoutEngine final : public LayoutEngine
  {
    enum class Mode
    {
      /// As large as the content asks for.
      Undefined = 0,
      /// The size of the border box is given, and percentages inside the
      /// box refer to it.
      Exactly,
      /// The size of the border box is given, and followed from content.
      /// Percentages inside the box have nothing to refer to, as in CSS.
      Fitted,
      /// As large as the content asks for, and no larger than given.
      AtMost
    };

    struct Measured
    {
      float width = 0.0f;
      Mode width_mode = Mode::Undefined;
      float height = 0.0f;
      Mode height_mode = Mode::Undefined;
      float owner_width = 0.0f;
      float owner_height = 0.0f;
      LayoutSize size;
    };

    struct Node
    {
      bool used = false;
      LayoutStyle style;
      LayoutNode parent = No_Layout_Node;
      std::vector<LayoutNode> children;
      LayoutMeasure measure;
      LayoutBox box;

      // what was worked out during the Calculate() that is running, so that
      // a box is not measured again under the same conditions
      std::vector<Measured> measured;
    };

    struct Block;
    struct Frame;
    struct Item;
    struct Line;

    std::vector<Node> _nodes;
    std::vector<LayoutNode> _free;

    [[nodiscard]] static bool IsGiven(Mode mode);

    [[nodiscard]] bool IsNode(LayoutNode node) const;

    LayoutSize Layout(
      LayoutNode id,
      float width,
      Mode width_mode,
      float height,
      Mode height_mode,
      float owner_width,
      float owner_height,
      bool place);

    LayoutSize LayoutContainer(
      LayoutNode id,
      const Frame &frame,
      float width,
      Mode width_mode,
      float height,
      Mode height_mode,
      bool place);

    void PlaceAbsolute(const Block &block, LayoutNode id);

    void Hide(LayoutNode id);

  public:
    LayoutNode CreateNode() override;

    void DestroyNode(LayoutNode node) override;

    void SetStyle(LayoutNode node, const LayoutStyle &style) override;

    void SetChildren(LayoutNode node, const std::vector<LayoutNode> &children) override;

    void SetMeasure(LayoutNode node, const LayoutMeasure &measure) override;

    void Calculate(LayoutNode root, float width, float height) override;

    [[nodiscard]] LayoutBox GetBox(LayoutNode node) const override;
  };
} // neon

#endif //FLEX_LAYOUT_ENGINE_HPP
