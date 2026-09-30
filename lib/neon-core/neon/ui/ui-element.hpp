#ifndef UI_ELEMENT_HPP
#define UI_ELEMENT_HPP

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <neon/data/data-reader.hpp>
#include <neon/layout/layout-engine.hpp>

#include "ui-painter.hpp"
#include "ui-resources.hpp"
#include "ui-style.hpp"
#include "ui-values.hpp"

namespace neon
{
  /// What an element measures and draws itself with.
  struct UiFrame
  {
    UiResources *resources = nullptr;
    const UiValues *values = nullptr;

    /// Pixels of the frame for each unit of the file.
    float scale = 1.0f;
  };

  /// The states of an element, named as the pseudo-classes of CSS.
  struct UiStates
  {
    bool focus = false;
    bool hover = false;
    bool active = false;
    bool disabled = false;

    bool operator==(const UiStates &other) const = default;
  };

  /// A part of a user interface: a panel, a label, a button, and so on.
  /// Elements form a tree.
  ///
  /// A kind of element is a class that derives from this one and is added
  /// to the UiElementTypes under the name files call it by. It says what is
  /// written next to the properties every element has, how large its
  /// content is, and how that is drawn. Where it goes, its background, its
  /// border, and its states are the same for every kind and are done here.
  class UiElement
  {
  public:
    /// One style for each combination of `focus`, `hover`, and `active`,
    /// and one for `disabled`.
    static constexpr std::size_t kStyle_Count = 9;

    [[nodiscard]] static std::size_t StyleIndex(const UiStates &states);

  private:
    std::string _type;
    std::string _name;
    std::size_t _line = 0;

    std::array<UiStyle, kStyle_Count> _styles;
    UiFlag _hidden{false};

    UiElement *_parent = nullptr;
    std::vector<std::unique_ptr<UiElement>> _children;

    LayoutNode _node = No_Layout_Node;
    UiStates _states;
    bool _is_hidden = false;

    // the border box in units of the file, from the corner of the document
    UiRectangle _box;
    LayoutBox _layout_box;

  public:
    UiElement() = default;

    virtual ~UiElement() = default;

    UiElement(const UiElement &) = delete;

    UiElement &operator=(const UiElement &) = delete;

    // What a kind of element is. Each of these can be left as it is.

    /// The style an element of this kind has when a file says nothing, as
    /// the style sheet of a browser has one for every element of HTML.
    virtual void ApplyDefaults(UiStyle &style) const;

    /// What a state changes about that style. What a file writes for the
    /// element itself wins over it, as the style sheet of an author wins
    /// over that of a browser.
    virtual void ApplyStateDefaults(UiStyle &style, const UiStates &states) const;

    /// Reads what is written next to the properties every element has,
    /// such as the text of a label.
    virtual void ReadAttributes(const DataReader &reader);

    /// Whether a file can write `children` for it.
    [[nodiscard]] virtual bool TakesChildren() const;

    /// Whether the focus can be moved to it with keys and a controller.
    [[nodiscard]] virtual bool IsFocusable() const;

    /// Whether it reports a click.
    [[nodiscard]] virtual bool IsClickable() const;

    /// Whether it has the focus when its file is loaded.
    [[nodiscard]] virtual bool WantsFocus() const;

    /// Whether it can be used. One that cannot is drawn in its state
    /// `disabled`, and takes neither focus nor clicks.
    [[nodiscard]] virtual bool IsEnabled() const;

    /// Whether it has content with a size of its own, which Measure() is
    /// then asked for.
    [[nodiscard]] virtual bool HasContent() const;

    /// The size of the content in units of the file. Each of the sizes it
    /// is given is the room there is, or not a number for no limit.
    [[nodiscard]] virtual LayoutSize Measure(
      const UiFrame &frame,
      float available_width,
      float available_height);

    /// Called once per frame before anything is measured, to follow the
    /// values of the game.
    virtual void Update(const UiFrame &frame);

    /// Draws the content into its box, which is in pixels of the frame.
    /// Background, border, and children are drawn by what calls this.
    virtual void PaintContent(
      UiPainter &painter,
      const UiFrame &frame,
      const UiRectangle &content_box,
      float opacity);

    // What is the same for every kind.

    [[nodiscard]] const std::string &GetType() const;

    [[nodiscard]] const std::string &GetName() const;

    /// The line of the file the element starts at.
    [[nodiscard]] std::size_t GetLine() const;

    void SetIdentity(const std::string &type, const std::string &name, std::size_t line);

    /// What the element is called in a message: `button 'start'`.
    [[nodiscard]] std::string Describe() const;

    void SetStyle(std::size_t index, const UiStyle &style);

    /// The style of the state the element is in.
    [[nodiscard]] const UiStyle &GetStyle() const;

    void SetHidden(const UiFlag &hidden);

    UiElement &AddChild(std::unique_ptr<UiElement> child);

    [[nodiscard]] UiElement *GetParent() const;

    [[nodiscard]] const std::vector<std::unique_ptr<UiElement>> &GetChildren() const;

    /// The children in the order they are drawn in: by `z_index`, and by
    /// the order of the file where that is the same.
    [[nodiscard]] std::vector<UiElement *> GetChildrenInPaintOrder() const;

    [[nodiscard]] const UiStates &GetStates() const;

    void SetStates(const UiStates &states);

    /// Whether the element is hidden, by itself or by one above it.
    [[nodiscard]] bool IsHidden() const;

    /// Creates the boxes of the element and of everything below it.
    void CreateLayout(LayoutEngine &engine, const UiFrame *frame);

    void DestroyLayout(LayoutEngine &engine);

    [[nodiscard]] LayoutNode GetLayoutNode() const;

    /// Follows the values, and hands the styles to the layout engine.
    void Prepare(LayoutEngine &engine, const UiFrame &frame, bool parent_is_hidden);

    /// Takes over where the layout engine put everything.
    void Arrange(const LayoutEngine &engine, float parent_left, float parent_top);

    /// The border box, in units of the file.
    [[nodiscard]] const UiRectangle &GetBox() const;

    /// The box inside the border, and the one inside the padding as well.
    [[nodiscard]] UiRectangle GetPaddingBox() const;

    [[nodiscard]] UiRectangle GetContentBox() const;

    /// Draws the element and everything below it.
    void Paint(UiPainter &painter, const UiFrame &frame, float opacity);

    /// A point in pixels of what the element is drawn to as the point of
    /// the element it lies on, which undoes the `transform` of the
    /// element. What is inside the element is asked with the point that
    /// comes out. Returns false for an element that is flattened to a
    /// line, which no point lies on.
    [[nodiscard]] bool ToLocal(float scale, float &x, float &y) const;

    /// Whether a point is on the element, its round corners taken into
    /// account. The point is one ToLocal() has handed out.
    [[nodiscard]] bool Contains(float scale, float x, float y) const;

    /// Whether a point is on what is inside the border of the element,
    /// which is what is left of its children when it cuts them off.
    [[nodiscard]] bool ContainsInPadding(float scale, float x, float y) const;
  };

  /// A rectangle in units of the file as one in pixels of the frame. Every
  /// side lands on a whole pixel, so that two boxes that touch share an
  /// edge and leave no gap.
  [[nodiscard]] UiRectangle ToPixels(const UiRectangle &rectangle, float scale);
} // neon

#endif //UI_ELEMENT_HPP
