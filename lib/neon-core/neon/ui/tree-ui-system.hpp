#ifndef TREE_UI_SYSTEM_HPP
#define TREE_UI_SYSTEM_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <neon/input/clipboard-context.hpp>
#include <neon/window/window-context.hpp>

#include "ui-animator.hpp"
#include "ui-cascade.hpp"
#include "ui-draw-cache.hpp"

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/input/input-context.hpp>
#include <neon/layout/layout-engine.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/render-2d-context.hpp>
#include <neon/text/font-rasterizer.hpp>
#include <neon/text/text-shaper.hpp>

#include "ui-document.hpp"
#include "ui-element-types.hpp"
#include "ui-file.hpp"
#include "ui-input-gate.hpp"
#include "ui-painter.hpp"
#include "ui-resources.hpp"
#include "ui-scrollbars.hpp"
#include "ui-system.hpp"
#include "ui-values.hpp"

namespace neon
{
  /// What the user interface worked out, counted since it was created.
  /// It says whether a frame in which nothing changed did no work, and is
  /// what the tests of that look at.
  struct UiStatistics
  {
    /// How often the values of the game were taken to the elements.
    std::size_t follows = 0;

    /// How many styles the cascade worked out.
    std::size_t styles = 0;

    /// How often the layout engine placed boxes, and how often of those it
    /// placed everything and not a part of the tree.
    std::size_t layouts = 0;
    std::size_t full_layouts = 0;

    /// How often an element that changed measured the same as before, and
    /// was drawn again without a layout.
    std::size_t layouts_spared = 0;

    /// How many elements were in what was placed.
    std::size_t laid_out_elements = 0;

    /// How often the boxes of elements were moved without anything being
    /// measured, as after scrolling.
    std::size_t places = 0;

    /// How often the elements were drawn, and how often what was drawn
    /// the frame before was handed to the renderer again.
    std::size_t paints = 0;
    std::size_t replays = 0;
  };

  /// What an application decides about its user interface.
  struct UiSettings
  {
    /// The fonts every file can ask for without naming them under `fonts`.
    /// The family `sans-serif` is what text is drawn with when a file says
    /// nothing about its font.
    std::vector<UiFontFace> fonts;

    /// Virtual path of a file that is shown from the start. Empty shows
    /// none.
    std::string start_path;
  };

  /// The user interface as a tree of elements for every file that is
  /// shown.
  ///
  /// The elements are objects of their own and not entities of the world.
  /// The order of elements decides what is drawn on top and where the
  /// focus goes, their sizes follow from each other up and down the tree,
  /// and each kind brings behaviour of its own. An entity store keeps
  /// neither an order nor behaviour. A menu also outlives the scene it is
  /// shown over, and is not part of what a scene saves.
  ///
  /// Files that are shown lie on top of each other, the one loaded last on
  /// top. One that is `modal` takes the input away from the game and from
  /// the files below it.
  // ReSharper disable once CppInconsistentNaming
  class Tree_UiSystem final : public UiSystem, private UiElementHost
  {
    // What was added for style sheets, for what changes to be worked out
    // alone, and for displays of high density.

    WindowContext *_window = nullptr;
    Memory_Clipboard _own_clipboard;
    ClipboardContext *_clipboard = &_own_clipboard;

    UiDrawCache _draw_cache;
    UiStatistics _statistics;

    // by what a handle names an element by
    std::unordered_map<std::uint64_t, UiElement *> _elements;
    std::uint64_t _next_element_id = 1;

    // seconds of the user interface's own time
    double _time = 0.0;
    double _advance = 0.0;
    bool _was_advanced = false;
    double _time_scale = 1.0;

    float _user_scale = 1.0f;
    bool _reduced_motion = false;

    // what could not be read while styles were worked out, each said once
    std::set<std::string> _reported;
    std::vector<std::string> _problems;

    // the elements that are in a state, to take them out of it again
    std::vector<UiElement *> _in_state;
    UiElement *_hovered = nullptr;

    // what the player used last, as the values `input_device` and
    // `gamepad` tell every file
    bool _uses_gamepad = false;
    bool _has_device = false;

    // what the frame was when it was last drawn
    int _painted_width = 0;
    int _painted_height = 0;
    bool _needs_paint = true;
    std::size_t _always_painting = 0;

    // UiElementHost

    void ComputeStyle(UiElement &element) override;

    void ComputePartStyle(const UiElement &element, const std::string &part, UiStyle &style) override;

    void Invalidated(UiElement &element, unsigned what) override;

    void StatesChanged(UiElement &element, const UiStates &before, const UiStates &after) override;

    void ClassesChanged(UiElement &element) override;

    void Leaving(UiElement &element) override;

    void Joined(UiElement &element) override;

    /// What the cascade of a document works with.
    [[nodiscard]] UiCascadeSettings CascadeOf(const UiDocument &document);

    /// Logs what could not be read while styles were worked out.
    void ReportProblems();

    /// Takes a document that was read into the user interface.
    void Adopt(UiDocument &document);

    /// Brings everything up to what it is now: the values of the game,
    /// the styles, and where everything is. Does nothing where nothing
    /// changed.
    void Settle();

    void SettleStyles(UiDocument &document);

    void SettleLayout(UiDocument &document);

    /// The elements layout starts at for what changed: for each element
    /// that changed, the nearest above it that nothing inside decides the
    /// size of.
    [[nodiscard]] static std::vector<UiElement *> LayoutRootsOf(const UiDocument &document);

    void LayOut(UiDocument &document, UiElement &root);

    /// The window in points and in pixels. Without a window, a point is a
    /// pixel of the frame.
    [[nodiscard]] WindowMetrics GetMetrics() const;

    /// Puts the elements into the states they are in now, and takes those
    /// out that are in them no longer. What is hovered and pressed is that
    /// of the surface each element is shown on.
    void UpdateStates(bool accept_is_down);

    // Events.

    struct Subscription
    {
      int id = 0;

      // 0 listens to every element
      std::uint64_t element = 0;
      std::string event;
      UiListener listener;
    };

    std::vector<UiElementEvent> _element_events;
    std::vector<Subscription> _subscriptions;
    int _next_subscription = 1;

    // the elements that left something to tell
    std::vector<UiElement *> _noticing;

    /// Notes that something happened to an element. It is told at the end
    /// of Update().
    void Emit(UiElement &target, const std::string &name, UiElementEvent event = {});

    /// Takes what the elements left to tell.
    void CollectNotices();

    /// Tells what happened, to what listens at the element it happened to
    /// and at those above it.
    void DispatchEvents();

    [[nodiscard]] UiElement *ElementOf(UiHandle handle) const;

    /// Makes an element and puts it into another. `make` is handed what
    /// the element is said to be from in messages, and where to put them.
    UiHandle PutInto(
      UiHandle parent,
      int index,
      const std::function<std::unique_ptr<UiElement>(const std::string &where, std::vector<std::string> &errors)> &
      make);

    [[nodiscard]] static UiHandle HandleOf(const UiElement *element);

    // Scrolling.

    struct ThumbDrag
    {
      std::uint64_t element = 0;
      UiScrollbars::Axis axis = UiScrollbars::Axis::Vertical;

      // from the start of the thumb to where it was taken hold of
      float grip = 0.0f;
    };

    struct ContentDrag
    {
      std::uint64_t element = 0;

      // where the pointer went down, and how far it was scrolled then
      float start_x = 0.0f;
      float start_y = 0.0f;
      float scroll_x = 0.0f;
      float scroll_y = 0.0f;

      bool is_dragging = false;

      // how fast it is dragged, in units for each second
      float last_x = 0.0f;
      float last_y = 0.0f;
      float speed_x = 0.0f;
      float speed_y = 0.0f;
    };

    /// What goes on by itself after it was let go.
    struct Glide
    {
      std::uint64_t element = 0;
      float speed_x = 0.0f;
      float speed_y = 0.0f;
    };

    /// Scrolling to a place that takes a short time.
    struct ScrollMove
    {
      std::uint64_t element = 0;
      float from_x = 0.0f;
      float from_y = 0.0f;
      float to_x = 0.0f;
      float to_y = 0.0f;
      float elapsed = 0.0f;
    };

    ThumbDrag _thumb_drag;
    ContentDrag _content_drag;
    std::vector<Glide> _glides;
    std::vector<ScrollMove> _scroll_moves;

    /// The topmost element at a place in pixels, whether it takes the
    /// pointer or not. What is scrolled is found by where the pointer is.
    [[nodiscard]] UiElement *ElementAt(float x, float y) const;

    [[nodiscard]] static UiElement *ElementAt(UiElement &element, float scale, float x, float y);

    /// The nearest element, from `from` upwards, that is scrolled and has
    /// something left to scroll to that way.
    [[nodiscard]] static UiElement *ScrollerOf(UiElement *from, float by_x, float by_y);

    /// Pixels of what is drawn to for each unit of the file of an element.
    [[nodiscard]] float ScaleOf(const UiElement *element) const;

    /// Scrolls an element to a place, over a short time when its style
    /// asks for it and `at_once` does not say otherwise.
    bool ScrollTo(UiElement &element, float x, float y, bool at_once);

    /// Scrolls whatever the element is inside of so that it can be seen.
    void ScrollToShow(UiElement &element);

    /// Returns whether the press went to a scrollbar.
    bool PressScrollbar(float x, float y);

    void DragScrollbar(float x, float y);

    void ScrollWithWheel(const InputState &input, UiConsumed &consumed);

    void ScrollWithKeys(const InputState &input);

    void ScrollWithStick(const InputState &input, float seconds, UiConsumed &consumed);

    void DragContent(const InputState &input, bool pointer_is_down, bool pointer_went_down, float seconds);

    /// Moves what scrolls by itself: what takes a short time, and what
    /// goes on after it was let go.
    void MoveScrolling(float seconds);

    // Input.

    // what takes what the pointer does until its button goes up, as a
    // slider that is dragged
    UiElement *_captured = nullptr;

    // where and when the button of the pointer went down the last time,
    // and how often in a row
    double _press_time = -1.0;
    float _press_x = 0.0f;
    float _press_y = 0.0f;
    int _press_count = 0;
    std::uint64_t _press_element = 0;

    // what had the focus when it was last told
    std::uint64_t _told_focus = 0;
    bool _focus_came_by_keys = false;

    bool _cancel_was_down = false;

    // whether a text is typed, and where its caret was reported to be
    bool _is_typing = false;
    TextInputArea _caret_area;
    TextComposition _composition;

    // dragging an element onto another
    std::uint64_t _drag_candidate = 0;
    std::uint64_t _dragged = 0;
    std::uint64_t _drag_over = 0;

    // what is shown next to the pointer while it rests on an element
    struct Tooltip
    {
      std::uint64_t element = 0;
      std::string text;
      float rested = 0.0f;
      float x = 0.0f;
      float y = 0.0f;
      bool is_shown = false;
    };

    Tooltip _tooltip;
    float _pointer_x = 0.0f;
    float _pointer_y = 0.0f;

    CursorShape _cursor_shape = CursorShape::Default;

    /// Moves the focus, and notes whether keys did it, which scrolls what
    /// has it into view.
    void SetFocus(UiElement *element, bool by_keys);

    /// Tells what came to have the focus and what lost it, and asks the
    /// platform for text while an element has it that a text is typed
    /// into.
    void FollowFocus();

    /// Hands something to an element. Returns whether the element used it.
    bool Interact(UiElement &element, UiInteraction &interaction);

    void HandlePointer(const InputState &input, UiElement *hovered, UiConsumed &consumed);

    void HandleKeys(const InputState &input, UiConsumed &consumed);

    /// Moves the focus to the next element in the order of the tab key, or
    /// to the one before.
    void MoveFocusInOrder(bool backwards);

    /// What cancel does: see UiCancel.
    void Cancel();

    void FollowHover(UiElement *hovered);

    void FollowTooltip(UiElement *at, float seconds);

    void FollowCursor(const UiElement *hovered);

    void PaintTooltip();

    void TickElements(float seconds);

    /// Moves what is animated on by the time of the frame.
    void Animate(float seconds);

    UiAnimator _animator;

    [[nodiscard]] UiAnimator::Context AnimationContext();

    // the files cancel closes, which are closed where the frame ends
    std::vector<int> _documents_to_close;

    /// Makes the rows of the elements that show a list of the game.
    void FollowLists(UiDocument &document);

    // the lists of the game, with a number that goes up when one changes
    struct List
    {
      std::vector<UiRow> rows;
      std::uint64_t revision = 0;
    };

    std::map<std::string, List> _lists;
    std::uint64_t _list_revision = 0;

    // the direction that was pressed and moved the focus nowhere
    bool _direction_was_refused[4] = {false, false, false, false};

    // seconds of the frame, by the time of the user interface
    float _frame_seconds = 0.0f;

    enum class Direction
    {
      Up = 0,
      Right,
      Down,
      Left
    };

    Render2DContext *_renderer;
    LayoutEngine *_layout;
    InputContext *_input;
    UiSettings _settings;
    std::shared_ptr<Logger> _logger;

    UiElementTypes _types;
    UiValues _values;
    UiResources _resources;
    UiFile _file;
    UiPainter _painter;
    UiInputGate _gate;

    // from the bottom to the top
    std::vector<std::unique_ptr<UiDocument>> _documents;
    int _next_id = 0;

    std::vector<UiEvent> _events;
    std::map<std::string, std::function<void()>> _callbacks;

    UiElement *_focused = nullptr;
    UiElement *_pressed = nullptr;

    // what was held down in the frame before, to tell when it is pressed
    bool _pointer_was_down = false;
    bool _accept_was_down = false;

    /// Where a user interface is laid out and drawn. The window is the
    /// first, and the only one that is no render target.
    struct Surface
    {
      int id = No_Ui_Surface;
      std::string name;

      // in pixels. The window has the size of the frame, which is asked
      // for in every frame
      int width = 0;
      int height = 0;
      float scale = 1.0f;

      int target = No_Render_Target;

      // where the player points, as the game says. The window is told by
      // the input
      bool has_pointer = false;
      float pointer_x = 0.0f;
      float pointer_y = 0.0f;
      bool pointer_is_down = false;

      bool pointer_was_down = false;
      UiElement *pressed = nullptr;
      UiElement *hovered = nullptr;
      bool uses_pointer = false;
    };

    // by what they are known as. A place is empty once its surface is
    // destroyed
    std::vector<std::unique_ptr<Surface>> _surfaces;
    int _input_surface = Ui_Window_Surface;

    // names of surfaces that could not be made, each said once
    std::vector<std::string> _refused_surfaces;

    // the values of one user interface, by the name of the user interface
    std::map<std::string, std::unique_ptr<UiValues>> _values_of;

    // callbacks for an element of one user interface, by both names
    std::map<std::pair<std::string, std::string>, std::function<void()>> _callbacks_in;

    [[nodiscard]] Surface *FindSurface(int surface) const;

    [[nodiscard]] UiValues &ValuesOf(const std::string &interface);

    /// The size of a surface in pixels, which is that of the frame for the
    /// window.
    void SizeOf(const Surface &surface, int &width, int &height) const;

    [[nodiscard]] UiElement *HitTest(int surface, float x, float y) const;

    /// What the pointer of a surface does in this frame.
    void UpdatePointer(Surface &surface, bool has_pointer, float x, float y, bool is_down);

    void Paint(const Surface &surface);
    bool _direction_was_down[4] = {false, false, false, false};

    bool _initialized = false;
    std::size_t _draw_calls = 0;

    /// The first of the documents that take input: the topmost that is
    /// modal, or the one at the bottom.
    [[nodiscard]] std::size_t FirstActive() const;

    [[nodiscard]] bool HasModal() const;

    [[nodiscard]] UiDocument *DocumentOf(const UiElement *element) const;

    [[nodiscard]] bool TakesInput(const UiElement *element) const;

    [[nodiscard]] static bool CanBeUsed(const UiElement *element);

    [[nodiscard]] UiElement *HitTest(float x, float y) const;

    [[nodiscard]] static UiElement *HitTest(UiElement &element, float scale, float x, float y);

    std::size_t _reported_draw_calls = 0;
    std::size_t _quads = 0;

    /// Returns whether the focus moved.
    bool MoveFocus(Direction direction);

    void Click(UiElement &element, bool by_pointer);

    static void Collect(UiElement &element, std::vector<UiElement *> &elements);

    [[nodiscard]] static UiElement *FindByName(UiElement &element, const std::string &name);

    void FocusAtStart(UiDocument &document);

  public:
    /// `input` is the input as it comes from the devices. `format` says
    /// which format the files have.
    Tree_UiSystem(
      Render2DContext *renderer,
      FontRasterizer *rasterizer,
      LayoutEngine *layout,
      InputContext *input,
      FileSystemContext *file_system,
      DocumentFormat *format,
      const UiSettings &settings,
      const std::shared_ptr<Logger> &logger);

    /// The kinds of elements a file can hold. Those of the engine are
    /// known. A game adds its own before it loads a file that uses them.
    [[nodiscard]] UiElementTypes &GetElementTypes();

    /// Throws when the file that is shown from the start cannot be used.
    void Initialize() override;

    void Update() override;

    void Draw() override;

    void CleanUp() override;

    [[nodiscard]] InputContext *GetGameInput() override;

    int Load(const std::string &path) override;

    void Unload(int document) override;

    [[nodiscard]] bool IsShown(int document) const override;

    void SetNumber(const std::string &name, double number) override;

    void SetText(const std::string &name, const std::string &text) override;

    void SetFlag(const std::string &name, bool flag) override;

    [[nodiscard]] std::string GetValue(const std::string &name, bool *is_set) const override;

    [[nodiscard]] bool GetNumber(const std::string &name, double &number) const override;

    [[nodiscard]] bool DescribeElement(const std::string &type, TypeInfo &description) const override;

    /// A property of the style as a field, named `style.<property>` with
    /// the name of the file: `style.background_color`. Reading gives what
    /// the cascade came to, and setting counts as set for the element.
    [[nodiscard]] static bool GetStyleField(const UiElement &element, const std::string &path, FieldValue &value);

    static bool SetStyleField(UiElement &element, const std::string &path, const FieldValue &value);

    void OnClick(const std::string &element, const std::function<void()> &callback) override;

    [[nodiscard]] const std::vector<UiEvent> &GetEvents() const override;

    [[nodiscard]] bool WasClicked(const std::string &element) const override;

    bool Focus(const std::string &element) override;

    [[nodiscard]] std::string GetFocused() const override;

    /// The element of that name in the topmost file that has one, or
    /// nullptr. For tools and tests. A game hands over values and does not
    /// reach for elements.
    [[nodiscard]] const UiElement *Find(const std::string &name) const;

    /// The window the user interface is shown in, which says how dense
    /// the display is, how much time passes, and takes the shape of the
    /// cursor. Without one, a point is a pixel and time stands still
    /// unless Advance() says otherwise.
    void SetWindow(WindowContext *window);

    /// What is cut and copied goes to it, and what is pasted comes from
    /// it. Without one, what is copied stays in the user interface.
    void SetClipboard(ClipboardContext *clipboard);

    void Advance(double seconds) override;

    bool ReloadStyles(int document = -1) override;

    // elements, as a game and a script reach them

    [[nodiscard]] bool IsAlive(UiHandle element) const override;

    [[nodiscard]] UiHandle GetRoot(int document = -1) const override;

    [[nodiscard]] UiHandle FindByName(const std::string &name, UiHandle from = {}) const override;

    [[nodiscard]] std::vector<UiHandle> FindByClass(const std::string &name, UiHandle from = {}) const override;

    [[nodiscard]] std::vector<UiHandle> FindByType(const std::string &type, UiHandle from = {}) const override;

    [[nodiscard]] std::vector<UiHandle> Query(const std::string &selector, UiHandle from = {}) const override;

    [[nodiscard]] UiHandle QueryFirst(const std::string &selector, UiHandle from = {}) const override;

    [[nodiscard]] bool Matches(UiHandle element, const std::string &selector) const override;

    [[nodiscard]] UiHandle GetParent(UiHandle element) const override;

    [[nodiscard]] std::vector<UiHandle> GetChildren(UiHandle element) const override;

    [[nodiscard]] UiHandle GetNextSibling(UiHandle element) const override;

    [[nodiscard]] UiHandle GetPreviousSibling(UiHandle element) const override;

    [[nodiscard]] std::string GetElementType(UiHandle element) const override;

    [[nodiscard]] std::string GetElementName(UiHandle element) const override;

    bool Set(UiHandle element, const std::string &property, const std::string &value) override;

    [[nodiscard]] std::string GetComputed(UiHandle element, const std::string &property) const override;

    bool AddClass(UiHandle element, const std::string &name) override;

    bool RemoveClass(UiHandle element, const std::string &name) override;

    bool ToggleClass(UiHandle element, const std::string &name) override;

    [[nodiscard]] bool HasClass(UiHandle element, const std::string &name) const override;

    [[nodiscard]] std::vector<std::string> GetClasses(UiHandle element) const override;

    bool SetField(UiHandle element, const std::string &name, const FieldValue &value) override;

    [[nodiscard]] bool GetField(UiHandle element, const std::string &name, FieldValue &value) const override;

    bool SetElementText(UiHandle element, const std::string &text) override;

    [[nodiscard]] std::string GetElementText(UiHandle element) const override;

    bool SetVisible(UiHandle element, bool visible) override;

    [[nodiscard]] bool IsVisible(UiHandle element) const override;

    bool FocusElement(UiHandle element) override;

    [[nodiscard]] UiHandle GetFocusedElement() const override;

    void Blur() override;

    [[nodiscard]] bool GetScroll(UiHandle element, float &x, float &y) const override;

    bool SetScroll(UiHandle element, float x, float y) override;

    bool ScrollIntoView(UiHandle element) override;

    [[nodiscard]] bool GetBox(UiHandle element, UiBox &box) const override;

    UiHandle Create(const std::string &description, UiHandle parent, int index = -1) override;

    UiHandle CreateFrom(const DataValue &description, UiHandle parent, int index = -1) override;

    UiHandle CreateFromTemplate(
      const std::string &name,
      UiHandle parent,
      const UiRow &fields = {},
      int index = -1) override;

    bool Remove(UiHandle element) override;

    bool Move(UiHandle element, UiHandle parent, int index = -1) override;

    void SetList(const std::string &name, const std::vector<UiRow> &rows) override;

    bool StartAnimation(UiHandle element, const std::string &name, const std::string &options = "") override;

    bool StopAnimation(UiHandle element, const std::string &name = "") override;

    /// How many elements have something on its way: a transition, or an
    /// animation that has not ended.
    [[nodiscard]] std::size_t GetMovingCount() const;

    int On(UiHandle element, const std::string &event, const UiListener &listener) override;

    int OnAny(const std::string &event, const UiListener &listener) override;

    void Off(int subscription) override;

    [[nodiscard]] const std::vector<UiElementEvent> &GetElementEvents() const override;

    [[nodiscard]] std::vector<std::string> GetElementTypeNames() const override;

    /// The element a handle names, or nullptr. For tools and tests.
    [[nodiscard]] const UiElement *GetElement(UiHandle handle) const;

    void SetUserScale(float scale) override;

    [[nodiscard]] float GetUserScale() const override;

    void SetReducedMotion(bool reduced) override;

    [[nodiscard]] bool GetReducedMotion() const override;

    void SetTimeScale(double scale) override;

    [[nodiscard]] double GetTimeScale() const override;

    [[nodiscard]] const UiStatistics &GetStatistics() const;

    /// The documents that are shown, from the bottom to the top.
    [[nodiscard]] const std::vector<std::unique_ptr<UiDocument>> &GetDocuments() const;

    /// The pixels of the frame for each unit of the topmost file, or 1.
    [[nodiscard]] float GetScale() const;

    /// The fonts that are kept at a size. For a test of what a change of
    /// the scale leaves behind.
    [[nodiscard]] std::size_t GetFontCount() const;

    /// Draw calls of the last frame.
    [[nodiscard]] std::size_t GetDrawCalls() const;

    /// What shapes the text from now on: kerning, ligatures, and the
    /// scripts whose letters are joined or run from right to left.
    /// Without one, text is drawn character by character. Call it before
    /// a file is shown.
    void SetTextShaper(TextShaper *shaper);

    /// The fonts and the images, for an application that adds to them.
    [[nodiscard]] UiResources &GetResources();

    /// What reads image files. With one, images have smaller copies and
    /// are smooth when they are drawn smaller than they are. Without one,
    /// the renderer is asked to load them.
    void SetImageDecoder(ImageDecoder *decoder);

    /// What draws images that are made of shapes, such as SVG.
    void SetVectorImageRasterizer(VectorImageRasterizer *rasterizer);

    void AdvanceTime(double seconds) override;

    /// Seconds the user interface has been running for, by its own time.
    [[nodiscard]] double GetTime() const override;

    int CreateSurface(const std::string &name, int width, int height, float scale = 1.0f) override;

    void DestroySurface(int surface) override;

    [[nodiscard]] int FindSurface(const std::string &name) const override;

    int LoadOnto(int surface, const std::string &path) override;

    void SetPointer(int surface, float x, float y, bool is_down) override;

    void SetPointerUv(int surface, float u, float v, bool is_down) override;

    void ClearPointer(int surface) override;

    bool SetInputSurface(int surface) override;

    [[nodiscard]] int GetInputSurface() const override;

    void SetNumberOf(const std::string &interface, const std::string &name, double number) override;

    void SetTextOf(const std::string &interface, const std::string &name, const std::string &text) override;

    void SetFlagOf(const std::string &interface, const std::string &name, bool flag) override;

    void OnClickIn(
      const std::string &interface,
      const std::string &element,
      const std::function<void()> &callback) override;

    [[nodiscard]] bool WasClickedIn(const std::string &interface, const std::string &element) const override;

    /// The element of that name in one user interface, or nullptr. For
    /// tools and tests.
    [[nodiscard]] const UiElement *FindIn(const std::string &interface, const std::string &name) const;

    /// The texture a surface is shown with where it is drawn in two
    /// dimensions, or No_Texture. The window has none.
    [[nodiscard]] int GetSurfaceTexture(int surface) const;

    /// What the window is called as a surface.
    static constexpr const char *kWindow_Name = "window";
  };
} // neon

#endif //TREE_UI_SYSTEM_HPP
