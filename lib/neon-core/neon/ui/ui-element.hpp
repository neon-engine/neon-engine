#ifndef UI_ELEMENT_HPP
#define UI_ELEMENT_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <neon/data/data-reader.hpp>
#include <neon/input/clipboard-context.hpp>
#include <neon/input/key.hpp>
#include <neon/layout/layout-engine.hpp>
#include <neon/reflection/field-value.hpp>

#include "css/css-selector.hpp"
#include "ui-element-host.hpp"
#include "ui-painter.hpp"
#include "ui-resources.hpp"
#include "ui-style.hpp"
#include "ui-values.hpp"

namespace neon
{
  class UiTextMeasure;
  class UiElement;

  /// What an element measures and draws itself with.
  struct UiFrame
  {
    UiResources *resources = nullptr;
    const UiValues *values = nullptr;

    /// Pixels of the frame for each unit of the file.
    float scale = 1.0f;

    /// The size of what is shown, in units of the file.
    float width = 1920.0f;
    float height = 1080.0f;

    /// Seconds the user interface has been running for, by its own time.
    /// It is what a caret blinks by.
    double time = 0.0;

    /// Says where the characters of a text are. nullptr uses what places
    /// the characters of a font one behind the other.
    const UiTextMeasure *text_measure = nullptr;
  };

  /// What the pointer, a key, or the wheel did to an element, which the
  /// element may react to. An element that used it says so, and it is then
  /// not handed to anything else.
  struct UiInteraction
  {
    enum class Kind
    {
      PointerDown = 0,
      PointerMove,
      PointerUp,
      KeyDown,
      Text,
      Composition,
      Wheel,

      /// Accept was pressed while the element has the focus.
      Accept,

      /// A direction was pressed while the element has the focus. `key`
      /// holds which.
      Direction,

      /// The focus came to the element, and left it.
      Focus,
      Blur,

      /// Cancel was pressed while the element has the focus. An element
      /// that has something open closes it, and says that it used it.
      Cancel
    };

    Kind kind = Kind::PointerDown;

    /// Where the pointer is, in units of the file.
    float x = 0.0f;
    float y = 0.0f;

    /// How often the button went down in one place without a pause: 2 for
    /// a double click.
    int clicks = 1;

    /// Whether Accept came from a click of the pointer, and not from a
    /// key or a controller. What the pointer chose was pressed already.
    bool by_pointer = false;

    Key key = Key::Unknown;
    KeyModifiers modifiers;

    /// The text that was typed, or that is being put together.
    std::string text;
    int composition_cursor = 0;

    /// How far the wheel was turned, in units of the file.
    float wheel_x = 0.0f;
    float wheel_y = 0.0f;

    /// Set by the element that used it.
    bool is_used = false;

    /// What is cut and copied goes to it, and what is pasted comes from
    /// it. nullptr for none.
    ClipboardContext *clipboard = nullptr;
  };

  /// Something an element wants the game to know of, such as a text that
  /// was changed. It is handed to the user interface, which reports it.
  struct UiNotice
  {
    /// What happened, named as its event: `changed`, `submitted`.
    std::string name;

    /// What the element holds now, as text.
    std::string value;

    /// The value of the game the element follows, which what it holds is
    /// written back to. Empty for none.
    std::string binding;
    UiValue bound_value;
  };

  /// The states of an element, named as the pseudo-classes of CSS.
  struct UiStates
  {
    bool focus = false;
    bool hover = false;
    bool active = false;
    bool disabled = false;

    /// Whether the element, or one inside it, has the focus.
    bool focus_within = false;

    /// Whether the element is chosen, as a checkbox that is ticked.
    bool checked = false;

    /// Whether what the element holds is not what it should be, as a text
    /// that does not fit its pattern.
    bool invalid = false;

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
  class UiElement : public CssElement
  {
  public:
    /// One style for each combination of `focus`, `hover`, and `active`,
    /// and one for `disabled`.
    static constexpr std::size_t kStyle_Count = 9;

    [[nodiscard]] static std::size_t StyleIndex(const UiStates &states);

    /// What a custom property holds, by its name.
    using Variables = std::map<std::string, std::string>;

  private:
    // What an element is to style sheets and to scripts, and what changed
    // about it.

    UiElementHost *_host = nullptr;

    // what a handle names the element by, given by the user interface
    std::uint64_t _id = 0;

    std::vector<std::string> _classes;
    std::string _title;
    int _tab_index = 0;

    // what the file writes for the element, without what is inside it
    DataValue _written = DataValue::Map();
    std::string _document;

    // what a game set while it runs, in the order it was set in
    std::vector<std::pair<std::string, std::string>> _set_properties;

    // the style the cascade came to, and the same with what is animated
    UiStyle _computed;
    UiStyle _used;
    bool _style_is_dirty = true;
    bool _is_computing = false;
    bool _has_computed_style = false;

    std::shared_ptr<const Variables> _variables;
    std::size_t _style_fingerprint = 0;

    // the styles of the parts of the element, each worked out when it is
    // first asked for
    mutable std::map<std::string, UiStyle> _part_styles;

    unsigned _dirty = UiDirty::Layout | UiDirty::Paint;

    // how far what is inside was scrolled, and how large it is
    float _scroll_x = 0.0f;
    float _scroll_y = 0.0f;
    float _content_width = 0.0f;
    float _content_height = 0.0f;

    // the part of the frame the element can be seen in, in units of the
    // file
    UiRectangle _visible_box;
    bool _is_clipped_away = false;

    std::vector<UiNotice> _notices;

    std::string _type;
    std::string _name;
    std::size_t _line = 0;

    // the styles that were set by hand, which count in place of what the
    // cascade comes to
    std::unique_ptr<std::array<UiStyle, kStyle_Count>> _styles;
    std::array<bool, kStyle_Count> _has_style{};

    UiFlag _hidden{false};

    UiElement *_parent = nullptr;
    std::vector<std::unique_ptr<UiElement>> _children;

    LayoutNode _node = No_Layout_Node;
    UiStates _states;
    bool _is_hidden = false;
    bool _is_hidden_by_itself = false;

    // how large the content was when it was last asked, for an element
    // that does not tell when it changed
    LayoutSize _measured;

    // what the layout engine last asked the content with, and what it was
    // told, so that a change that measures the same needs no layout
    bool _was_measured_by_layout = false;
    float _measured_available_width = 0.0f;
    float _measured_available_height = 0.0f;
    LayoutSize _measured_by_layout;

    // the style the layout engine was last given
    LayoutStyle _pushed_layout;
    bool _has_pushed_layout = false;

    // what is drawn of the element and of what is inside it, and what it
    // was cut off at
    UiRectangle _bounds;
    UiRectangle _clip;

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

    // What a kind of element is that reacts to more than a click. Each of
    // these can be left as it is as well.

    /// The style a part of an element of this kind has when a style sheet
    /// says nothing, such as the thumb of a slider. `style` holds what the
    /// part takes from the element.
    virtual void ApplyPartDefaults(const std::string &part, UiStyle &style) const;

    /// What the pointer, a key, or the wheel did while the element is what
    /// it was done to. An element that used it sets `is_used`.
    virtual void Interact(UiInteraction &interaction, const UiFrame &frame);

    /// Whether a text is typed into it while it has the focus. The keys
    /// that type are then kept from the game.
    [[nodiscard]] virtual bool TakesText() const;

    /// Whether it uses the keys that move the focus while it has the
    /// focus: a slider its left and right, a text its arrows.
    [[nodiscard]] virtual bool UsesDirection(Key direction) const;

    /// Whether it is chosen, as a checkbox that is ticked. It is what
    /// `:checked` of CSS asks for.
    [[nodiscard]] virtual bool IsChecked() const;

    /// Whether what it holds is not what it should be, as a text that
    /// does not fit its pattern. It is what `:invalid` of CSS asks for.
    [[nodiscard]] virtual bool IsInvalid() const;

    /// Where the caret is, in units of the file, for an element a text is
    /// typed into. An input method shows its candidates next to it.
    /// Returns false for an element without a caret.
    [[nodiscard]] virtual bool GetCaretBox(const UiFrame &frame, UiRectangle &box) const;

    /// How large what the element draws itself is, for an element that
    /// scrolls its own content, such as a text that is typed into. In
    /// units of the file, without the padding. Returns false for an
    /// element whose content does not scroll.
    [[nodiscard]] virtual bool GetContentSize(float &width, float &height) const;

    /// Whether it has something drawn on top of everything else while it
    /// has the focus, as the list of a select is. The pointer reaches it
    /// before anything under it.
    [[nodiscard]] virtual bool HasTopLayer() const;

    /// Whether a place, in pixels of the frame, is on what it draws on top
    /// of everything else.
    [[nodiscard]] virtual bool TopLayerContains(const UiFrame &frame, float x, float y) const;

    /// Draws what is on top of everything else. Called after every file
    /// was drawn.
    virtual void PaintTopLayer(UiPainter &painter, const UiFrame &frame);

    /// Whether what it draws changes without anything telling: it is then
    /// drawn again in every frame. An element that says Invalidate() when
    /// it changed leaves this as it is.
    [[nodiscard]] virtual bool PaintsEveryFrame() const;

    /// Whether it tells when what it shows changed, by Invalidate(). For an
    /// element that does not, its content is measured again whenever a
    /// value of the game changed.
    [[nodiscard]] virtual bool TellsWhenItChanged() const;

    /// The fields of the element next to its properties, such as the text
    /// of a label, by name. They are what a script reads and changes, and
    /// what `[name=value]` of a style sheet asks for. Returns false for a
    /// name the element does not have.
    [[nodiscard]] virtual bool GetField(const std::string &name, FieldValue &value) const;

    /// Returns false and says why when the element has no such field, or
    /// the value is not one the field takes.
    virtual bool SetField(const std::string &name, const FieldValue &value, std::string &error);

    /// The names of the fields, with what each holds and what it is for.
    struct Field
    {
      std::string name;
      FieldKind kind = FieldKind::Text;
      std::string description;
      std::vector<std::string> choices;
    };

    [[nodiscard]] virtual std::vector<Field> GetFields() const;

    /// Called once per frame for an element that asked for it with
    /// Invalidate(), with the time that passed. For what moves by itself,
    /// such as a caret that blinks. Returns whether it is to be called
    /// again in the next frame.
    virtual bool Tick(const UiFrame &frame, float seconds);

    // What is the same for every kind.

    [[nodiscard]] const std::string &GetType() const;

    [[nodiscard]] const std::string &GetName() const;

    /// The line of the file the element starts at.
    [[nodiscard]] std::size_t GetLine() const;

    void SetIdentity(const std::string &type, const std::string &name, std::size_t line);

    /// What the element is called in a message: `button 'start'`.
    [[nodiscard]] std::string Describe() const;

    /// Sets the style of a state by hand, which then counts in place of
    /// what the cascade comes to for that state.
    void SetStyle(std::size_t index, const UiStyle &style);

    /// The style of the state the element is in: what the cascade came to,
    /// with what is animated. It is worked out when it is asked for and
    /// something changed.
    [[nodiscard]] const UiStyle &GetStyle() const;

    /// The style the cascade came to, without what is animated.
    [[nodiscard]] const UiStyle &GetComputedStyle() const;

    /// Whether the cascade has given the element a style before. What an
    /// element starts with is no change, and takes no time.
    [[nodiscard]] bool HasComputedStyle() const;

    /// The style of a part of the element, such as `scrollbar-thumb`.
    [[nodiscard]] const UiStyle &GetPartStyle(const std::string &part) const;

    // What a style sheet and a script see of an element.

    /// What the element belongs to. It is handed on to everything inside.
    void SetHost(UiElementHost *host);

    [[nodiscard]] UiElementHost *GetHost() const;

    /// What a handle names the element by. 0 for an element that has none.
    [[nodiscard]] std::uint64_t GetId() const;

    void SetId(std::uint64_t id);

    [[nodiscard]] const std::vector<std::string> &GetClasses() const;

    void SetClasses(const std::vector<std::string> &classes);

    [[nodiscard]] bool HasClass(const std::string &name) const;

    /// Each returns whether the classes changed.
    bool AddClass(const std::string &name);

    bool RemoveClass(const std::string &name);

    /// What is shown next to the pointer while it rests on the element.
    [[nodiscard]] const std::string &GetTitle() const;

    void SetTitle(const std::string &title);

    /// Where the element is in the order the tab key goes through. Below 0
    /// leaves it out, 0 is the order of the file, and above 0 comes first,
    /// the lowest in front.
    [[nodiscard]] int GetTabIndex() const;

    void SetTabIndex(int tab_index);

    /// What the file writes for the element, which the cascade reads the
    /// properties from, and the file it is from.
    void SetWritten(const DataValue &written, const std::string &document);

    [[nodiscard]] const DataValue &GetWritten() const;

    [[nodiscard]] const std::string &GetDocumentName() const;

    /// A property a game sets while it runs, as CSS writes it. It counts as
    /// written for the element itself, behind what the file writes. An
    /// empty value takes it away again.
    void SetProperty(const std::string &name, const std::string &value);

    [[nodiscard]] const std::vector<std::pair<std::string, std::string>> &GetSetProperties() const;

    /// Takes over what the cascade came to. Called by what works out the
    /// style. It tells what is inside to work out its style again when
    /// something it inherits changed, and asks for a new layout when a
    /// property changed that moves or resizes.
    void SetComputedStyle(
      const UiStyle &style,
      const std::shared_ptr<const Variables> &variables,
      std::size_t fingerprint);

    /// The style with what is animated, which is what is drawn. Called by
    /// what animates.
    void SetUsedStyle(const UiStyle &style);

    [[nodiscard]] const std::shared_ptr<const Variables> &GetVariables() const;

    /// Says that something about the element has to be worked out again:
    /// one or more of UiDirty, added up.
    void Invalidate(unsigned what);

    /// The same for the element and everything inside it.
    void InvalidateAll(unsigned what);

    [[nodiscard]] bool IsDirty(unsigned what) const;

    void ClearDirty(unsigned what);

    /// What the element wants the game to know of since the last call.
    [[nodiscard]] std::vector<UiNotice> TakeNotices();

    /// Tells the game that something happened to the element, named as its
    /// event, such as `changed`. For a kind of element. With `binding`,
    /// `bound_value` is written to the value of the game of that name.
    void Notify(
      const std::string &name,
      const std::string &value,
      const std::string &binding = "",
      const UiValue &bound_value = {});

    // What is inside an element, while the game runs.

    /// Puts an element inside, in front of the one at `index`, or at the
    /// end. It joins the layout when the element it is put into is part of
    /// one.
    UiElement &InsertChild(std::unique_ptr<UiElement> child, std::size_t index, LayoutEngine *engine, const UiFrame *frame);

    /// Takes an element out and hands it over. nullptr when it is not
    /// inside this one.
    std::unique_ptr<UiElement> RemoveChild(UiElement &child, LayoutEngine *engine);

    [[nodiscard]] UiElement *GetPreviousSibling() const;

    [[nodiscard]] UiElement *GetNextSibling() const;

    /// The place among the elements under the same parent, counted from 0.
    [[nodiscard]] std::size_t GetIndex() const;

    // Scrolling.

    /// How far what is inside was scrolled, in units of the file.
    [[nodiscard]] float GetScrollX() const;

    [[nodiscard]] float GetScrollY() const;

    /// Scrolls to a place, which is held to what there is to scroll.
    /// Returns whether the place changed.
    bool SetScroll(float x, float y);

    /// How far it can be scrolled at the most.
    [[nodiscard]] float GetMaxScrollX() const;

    [[nodiscard]] float GetMaxScrollY() const;

    /// The size of what is inside, which is at least that of the box it is
    /// scrolled in.
    [[nodiscard]] float GetContentWidth() const;

    [[nodiscard]] float GetContentHeight() const;

    /// Whether there is something to scroll to along a side, which asks
    /// for `overflow` to be `scroll` or `auto`.
    [[nodiscard]] bool CanScrollX() const;

    [[nodiscard]] bool CanScrollY() const;

    /// How wide the scrollbars are, in units of the file. 0 for none.
    [[nodiscard]] float GetScrollbarWidth() const;

    /// The part of the frame the element can be seen in: its border box,
    /// less what the elements above cut off. In units of the file.
    [[nodiscard]] const UiRectangle &GetVisibleBox() const;

    /// Whether nothing of the element can be seen, since what it is inside
    /// of cuts it off.
    [[nodiscard]] bool IsClippedAway() const;

    // CssElement

    [[nodiscard]] const std::string &GetCssType() const override;

    [[nodiscard]] const std::string &GetCssId() const override;

    [[nodiscard]] bool HasCssClass(const std::string &name) const override;

    [[nodiscard]] bool GetCssAttribute(const std::string &name, std::string &value) const override;

    [[nodiscard]] bool IsInCssState(const std::string &name) const override;

    [[nodiscard]] const CssElement *GetCssParent() const override;

    [[nodiscard]] const CssElement *GetCssPreviousSibling() const override;

    [[nodiscard]] std::size_t GetCssIndex() const override;

    [[nodiscard]] std::size_t GetCssSiblingCount() const override;

    [[nodiscard]] bool HasCssChildren() const override;

    void SetHidden(const UiFlag &hidden);

    UiElement &AddChild(std::unique_ptr<UiElement> child);

    [[nodiscard]] UiElement *GetParent() const;

    [[nodiscard]] const std::vector<std::unique_ptr<UiElement>> &GetChildren() const;

    /// The children in the order they are drawn in: by `z_index`, and by
    /// the order of the file where that is the same.
    [[nodiscard]] std::vector<UiElement *> GetChildrenInPaintOrder() const;

    [[nodiscard]] const UiStates &GetStates() const;

    void SetStates(const UiStates &states);

    /// Asks the element whether it can be used and whether it is chosen,
    /// which are states as well.
    void RefreshStates();

    /// Works out again where everything inside the element is, with what
    /// it was cut off at the last time. For what was scrolled.
    void Replace();

    /// Works out whether the element and what is inside it are hidden,
    /// by themselves or by one above them.
    void UpdateHidden(bool parent_is_hidden);

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

    /// Follows the values of the game: the element and everything inside
    /// it. Whatever changed says so by Invalidate().
    void Follow(const UiFrame &frame, bool parent_is_hidden);

    /// Hands the style of the element to the layout engine, as far as it
    /// decides where the element goes.
    void PushLayoutStyle(LayoutEngine &engine);

    /// Whether the element would be laid out as it was: its style for the
    /// layout engine is what it was, and its content measures the same as
    /// when the engine last asked. A text that changed to one of the same
    /// size is such an element, and needs drawing and no layout.
    [[nodiscard]] bool LaysOutTheSame(const UiFrame &frame);

    /// The style as the layout engine is given it.
    [[nodiscard]] LayoutStyle GetLayoutStyle() const;

    /// Takes over the boxes of the layout engine for everything inside the
    /// element, and for the element itself when `with_itself` says so.
    void TakeLayout(const LayoutEngine &engine, bool with_itself);

    /// Works out where everything inside the element is in the frame, from
    /// where the element is and how far it was scrolled.
    void Place(const UiRectangle &clip);

    /// The same for the element at the top.
    void PlaceFrom(float parent_left, float parent_top, const UiRectangle &clip);

    /// Whether what is inside the element depends on nothing but the size
    /// the element has: a change inside of it moves nothing outside.
    [[nodiscard]] bool IsLayoutBoundary() const;

    [[nodiscard]] const LayoutBox &GetLayoutBox() const;

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

  /// The element and everything inside it, from the top to the end.
  void CollectUiElements(UiElement &element, std::vector<UiElement *> &elements);

  /// A rectangle in units of the file as one in pixels of the frame. Every
  /// side lands on a whole pixel, so that two boxes that touch share an
  /// edge and leave no gap.
  [[nodiscard]] UiRectangle ToPixels(const UiRectangle &rectangle, float scale);
} // neon

#endif //UI_ELEMENT_HPP
