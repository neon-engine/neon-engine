#ifndef UI_CONTEXT_HPP
#define UI_CONTEXT_HPP

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <neon/data/data-value.hpp>
#include <neon/reflection/field-value.hpp>
#include <neon/reflection/type-info.hpp>

#include "ui-handle.hpp"

namespace neon
{
  /// Something that happened in the user interface and that the game may
  /// want to react to.
  struct UiEvent
  {
    enum class Kind
    {
      /// An element was chosen: with the pointer, the keys, or a
      /// controller.
      Click = 0
    };

    Kind kind = Kind::Click;

    /// The name of the element, as the file writes it.
    std::string element;

    /// What the file the element is from calls itself.
    std::string document;

    /// The name of the surface the file is shown on: `window`, or what a
    /// surface in the world was called.
    std::string surface;
  };

  /// The surface every application has: what the frame is drawn to, which
  /// is the window when there is one.
  constexpr int Ui_Window_Surface = 0;

  /// Stands for no surface.
  constexpr int No_Ui_Surface = -1;

  /// Where an element is: its border box, from the left top corner of what
  /// is shown.
  struct UiBox
  {
    /// In units of the file the element is from.
    float left = 0.0f;
    float top = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    /// Pixels of what is drawn to for each unit.
    float scale = 1.0f;
  };

  /// A row of a list a game hands to its user interface: what each field
  /// of the row holds, as text.
  using UiRow = std::map<std::string, std::string>;

  /// What a game sees of the user interface: it says which files are
  /// shown, hands over the values they show, and learns what was chosen.
  class UiContext
  {
  protected:
    ~UiContext() = default;

  public:
    /// Shows the user interface of a file at a virtual path, on top of
    /// what is shown already. Returns what it is known as, or -1 when the
    /// file cannot be used. Every problem of the file is logged.
    virtual int Load(const std::string &path) = 0;

    /// Stops showing what Load() returned.
    virtual void Unload(int document) = 0;

    // What follows has a body that does nothing, so that what stands in
    // for a user interface in a test has to know of none of it.

    /// Reads the style sheets of what Load() returned again, while it is
    /// shown. For an editor, and for working on a theme while the game
    /// runs. Returns false and leaves everything as it is when a sheet
    /// cannot be read. With -1, the sheets of every file are read again.
    virtual bool ReloadStyles(int document = -1)
    {
      return false;
    }

    /// What the player asked the user interface to be made larger or
    /// smaller by, such as 1.5. It multiplies into the scale of every
    /// file.
    virtual void SetUserScale(float scale) {}

    [[nodiscard]] virtual float GetUserScale() const
    {
      return 1.0f;
    }

    /// Whether the player asked for less motion. Style sheets ask for it
    /// with `@media (prefers-reduced-motion: reduce)`.
    virtual void SetReducedMotion(bool reduced) {}

    [[nodiscard]] virtual bool GetReducedMotion() const
    {
      return false;
    }

    /// How fast the time of the user interface runs: 1 as the time it is
    /// told, 0 not at all. It is the time animations run by and a caret
    /// blinks by, and has nothing to do with the time of the world: a menu
    /// that pauses the game goes on moving.
    virtual void SetTimeScale(double scale) {}

    [[nodiscard]] virtual double GetTimeScale() const
    {
      return 1.0;
    }

    // Elements, as a game and a script reach them. Every function that is
    // handed a handle of an element that is gone does nothing, and returns
    // false, nothing, or a handle that names nothing.

    /// Whether the element a handle names is still there.
    [[nodiscard]] virtual bool IsAlive(UiHandle element) const
    {
      return false;
    }

    /// The element at the top of what Load() returned, or of the topmost
    /// file with -1.
    [[nodiscard]] virtual UiHandle GetRoot(int document = -1) const
    {
      return {};
    }

    /// The first element of a name, which is its `id` in a selector. It is
    /// looked for inside `from`, or in every file, the topmost first.
    [[nodiscard]] virtual UiHandle FindByName(const std::string &name, UiHandle from = {}) const
    {
      return {};
    }

    [[nodiscard]] virtual std::vector<UiHandle> FindByClass(const std::string &name, UiHandle from = {}) const
    {
      return {};
    }

    [[nodiscard]] virtual std::vector<UiHandle> FindByType(const std::string &type, UiHandle from = {}) const
    {
      return {};
    }

    /// The elements a selector of CSS matches, from the top of each file to
    /// its end: `panel.inventory > button:enabled`. With `from`, those
    /// inside of it, and `:scope` is that element. A selector that cannot
    /// be read matches nothing, and is logged.
    [[nodiscard]] virtual std::vector<UiHandle> Query(const std::string &selector, UiHandle from = {}) const
    {
      return {};
    }

    /// The first of them, or a handle that names nothing.
    [[nodiscard]] virtual UiHandle QueryFirst(const std::string &selector, UiHandle from = {}) const
    {
      return {};
    }

    /// Whether a selector matches an element.
    [[nodiscard]] virtual bool Matches(UiHandle element, const std::string &selector) const
    {
      return false;
    }

    [[nodiscard]] virtual UiHandle GetParent(UiHandle element) const
    {
      return {};
    }

    [[nodiscard]] virtual std::vector<UiHandle> GetChildren(UiHandle element) const
    {
      return {};
    }

    [[nodiscard]] virtual UiHandle GetNextSibling(UiHandle element) const
    {
      return {};
    }

    [[nodiscard]] virtual UiHandle GetPreviousSibling(UiHandle element) const
    {
      return {};
    }

    /// What the element is, such as `button`, and what it is called.
    [[nodiscard]] virtual std::string GetElementType(UiHandle element) const
    {
      return "";
    }

    [[nodiscard]] virtual std::string GetElementName(UiHandle element) const
    {
      return "";
    }

    /// Sets a property of an element, as CSS writes both:
    /// `Set(element, "background-color", "#334")`. It counts as written
    /// for the element itself, on top of what its file writes. An empty
    /// value takes back what was set. Returns false, changes nothing, and
    /// logs why for a name that is not known and a value that cannot be
    /// read. A name with two hyphens in front sets a custom property.
    virtual bool Set(UiHandle element, const std::string &property, const std::string &value)
    {
      return false;
    }

    /// What a property of an element came to after the cascade, as CSS
    /// writes it: `rgb(51, 51, 68)`. With what is animated, which is what
    /// is drawn. Empty for a property that is not known.
    [[nodiscard]] virtual std::string GetComputed(UiHandle element, const std::string &property) const
    {
      return "";
    }

    /// Each returns whether the classes changed.
    virtual bool AddClass(UiHandle element, const std::string &name)
    {
      return false;
    }

    virtual bool RemoveClass(UiHandle element, const std::string &name)
    {
      return false;
    }

    /// Returns whether the element has the class afterwards.
    virtual bool ToggleClass(UiHandle element, const std::string &name)
    {
      return false;
    }

    [[nodiscard]] virtual bool HasClass(UiHandle element, const std::string &name) const
    {
      return false;
    }

    [[nodiscard]] virtual std::vector<std::string> GetClasses(UiHandle element) const
    {
      return {};
    }

    /// A field of an element next to its properties, such as `text`,
    /// `value`, and `enabled`. Which there are depends on what the element
    /// is: see DescribeElement().
    virtual bool SetField(UiHandle element, const std::string &name, const FieldValue &value)
    {
      return false;
    }

    [[nodiscard]] virtual bool GetField(UiHandle element, const std::string &name, FieldValue &value) const
    {
      return false;
    }

    /// The field `text` of an element that has one.
    virtual bool SetElementText(UiHandle element, const std::string &text)
    {
      return false;
    }

    [[nodiscard]] virtual std::string GetElementText(UiHandle element) const
    {
      return "";
    }

    /// Hides an element and shows it again, as `hidden` of its file does.
    virtual bool SetVisible(UiHandle element, bool visible)
    {
      return false;
    }

    /// Whether the element is shown: neither hidden itself nor inside of
    /// something that is.
    [[nodiscard]] virtual bool IsVisible(UiHandle element) const
    {
      return false;
    }

    /// Moves the focus to an element. Returns false when it cannot have
    /// it.
    virtual bool FocusElement(UiHandle element)
    {
      return false;
    }

    /// The element that has the focus, or a handle that names nothing.
    [[nodiscard]] virtual UiHandle GetFocusedElement() const
    {
      return {};
    }

    /// Takes the focus away from whatever has it.
    virtual void Blur() {}

    /// How far what is inside an element was scrolled, in units of its
    /// file. Returns false for an element that is gone.
    [[nodiscard]] virtual bool GetScroll(UiHandle element, float &x, float &y) const
    {
      return false;
    }

    /// Scrolls to a place, which is held to what there is to scroll. It
    /// takes a short time where the element says `scroll_behavior: smooth`.
    virtual bool SetScroll(UiHandle element, float x, float y)
    {
      return false;
    }

    /// Scrolls whatever the element is inside of so that it can be seen.
    virtual bool ScrollIntoView(UiHandle element)
    {
      return false;
    }

    /// Where the element is, as it was placed last.
    [[nodiscard]] virtual bool GetBox(UiHandle element, UiBox &box) const
    {
      return false;
    }

    // Making and removing elements while the game runs.

    /// Makes an element from text in the format of the files, such as
    /// `{type: label, text: Hello}`, and puts it into `parent` in front of
    /// the element at `index`, or at the end with -1. Returns a handle that
    /// names nothing, and logs why, when the text is wrong or the parent
    /// takes nothing inside.
    virtual UiHandle Create(const std::string &description, UiHandle parent, int index = -1)
    {
      return {};
    }

    /// The same from values.
    virtual UiHandle CreateFrom(const DataValue &description, UiHandle parent, int index = -1)
    {
      return {};
    }

    /// Makes a copy of a template that the file of `parent` defines under
    /// `templates`. Every `${name}` in a text of the template is replaced
    /// by what `fields` holds under that name, which is how a row of a
    /// list gets what it shows.
    virtual UiHandle CreateFromTemplate(
      const std::string &name,
      UiHandle parent,
      const UiRow &fields = {},
      int index = -1)
    {
      return {};
    }

    /// Removes an element with everything inside it. Its handles name
    /// nothing afterwards.
    virtual bool Remove(UiHandle element)
    {
      return false;
    }

    /// Moves an element into another, of the same file, in front of the
    /// element at `index`, or to the end with -1. Its handles stay.
    virtual bool Move(UiHandle element, UiHandle parent, int index = -1)
    {
      return false;
    }

    /// Hands over a list, which a file shows with `for_each: "{name}"`: an
    /// element for every row, made from a template.
    virtual void SetList(const std::string &name, const std::vector<UiRow> &rows) {}

    // Events.

    /// Calls `listener` whenever something of that name happens to the
    /// element, or to an element inside of it for an event that goes up.
    /// It is called at the end of Update(), where it is free to change
    /// anything. Returns what Off() takes it away with, or 0 for an
    /// element that is gone.
    virtual int On(UiHandle element, const std::string &event, const UiListener &listener)
    {
      return 0;
    }

    /// The same for every element, which is how something listens that is
    /// about none in particular: sound for what the pointer enters and
    /// what is clicked.
    virtual int OnAny(const std::string &event, const UiListener &listener)
    {
      return 0;
    }

    virtual void Off(int subscription) {}

    /// What happened to elements in the last Update(), in the order it
    /// happened in.
    [[nodiscard]] virtual const std::vector<UiElementEvent> &GetElementEvents() const
    {
      static const std::vector<UiElementEvent> none;
      return none;
    }

    // Animations.

    /// Starts the keyframes of a name on an element, as `animation` of CSS
    /// writes what follows the name: `"0.3s ease-out both"`. It runs next
    /// to what the style of the element runs. Returns false for keyframes
    /// no style sheet of the file defines.
    virtual bool StartAnimation(UiHandle element, const std::string &name, const std::string &options = "")
    {
      return false;
    }

    /// Stops what StartAnimation() started, or everything it started on
    /// the element with an empty name.
    virtual bool StopAnimation(UiHandle element, const std::string &name = "")
    {
      return false;
    }

    // What there is to reach.

    /// The kinds of elements, by the name a file writes as `type`.
    [[nodiscard]] virtual std::vector<std::string> GetElementTypeNames() const
    {
      return {};
    }

    /// Describes a kind of element: its fields, with what each holds and
    /// what it is for. It is what an inspector shows and a binding for
    /// scripts is made from.
    [[nodiscard]] virtual bool DescribeElement(const std::string &type, TypeInfo &description) const
    {
      return false;
    }

    /// Sets a value that files refer to as `{name}`. What shows it changes
    /// with it.
    virtual void SetNumber(const std::string &name, double number) = 0;

    virtual void SetText(const std::string &name, const std::string &text) = 0;

    virtual void SetFlag(const std::string &name, bool flag) = 0;

    /// A value as it is now, as text: what the game set, or what was typed
    /// into an element whose value follows the name. Empty for a name that
    /// has no value, which `is_set` says.
    [[nodiscard]] virtual std::string GetValue(const std::string &name, bool *is_set = nullptr) const
    {
      if (is_set != nullptr) { *is_set = false; }
      return "";
    }

    /// Calls `callback` whenever the element of that name is chosen. It is
    /// called at the end of Update() of the frame, where the game is free
    /// to load and unload files. A name has one callback, and a second
    /// replaces the first.
    virtual void OnClick(const std::string &element, const std::function<void()> &callback) = 0;

    /// What happened in the last Update(), in the order it happened in.
    /// For a game that would sooner ask than be called.
    [[nodiscard]] virtual const std::vector<UiEvent> &GetEvents() const = 0;

    /// Whether the element of that name was chosen in the last Update().
    [[nodiscard]] virtual bool WasClicked(const std::string &element) const = 0;

    /// Moves the focus to the element of that name. Returns false when
    /// there is none that can have it.
    virtual bool Focus(const std::string &element) = 0;

    /// The name of the element that has the focus, or empty.
    [[nodiscard]] virtual std::string GetFocused() const = 0;

    // What follows can be left as it is by what implements this.

    /// Moves the time of the user interface on, which is what shaders of
    /// elements move with. It is given the time a frame advances the game
    /// by, so that a run with a fixed time step gives the same frames
    /// every time.
    virtual void AdvanceTime(const double seconds) {}

    /// Seconds since the user interface was started.
    [[nodiscard]] virtual double GetTime() const
    {
      return 0.0;
    }

    // Surfaces. A user interface is laid out and drawn on a surface, which
    // has a size in pixels. The window is one, and is there from the start.
    // Every other is an image that is drawn to, and shown by whatever
    // names it: a model in the world as the texture `surface://` and its
    // name, or an image of another user interface.

    /// Makes a surface of a size in pixels. `scale` makes everything on it
    /// larger, as the density of a screen does. Returns what the surface
    /// is known as, or No_Ui_Surface when it cannot be made, which is said
    /// once for a name.
    virtual int CreateSurface(const std::string &name, const int width, const int height, const float scale = 1.0f)
    {
      return No_Ui_Surface;
    }

    /// Stops showing what is on the surface, and releases it. The window
    /// cannot be destroyed.
    virtual void DestroySurface(const int surface) {}

    /// The surface of a name, or No_Ui_Surface. The window is called
    /// `window`.
    [[nodiscard]] virtual int FindSurface(const std::string &name) const
    {
      return No_Ui_Surface;
    }

    /// As Load(), onto a surface.
    virtual int LoadOnto(const int surface, const std::string &path)
    {
      return -1;
    }

    /// Says where the player points on a surface, in pixels of the surface
    /// from its left top corner, and whether the button is held down. It
    /// stays there until it is said again or taken away. Whoever knows
    /// where the player points says so: what casts a ray at a screen in
    /// the world, for one. The window is told by the input of the
    /// application, and needs no call.
    virtual void SetPointer(const int surface, const float x, const float y, const bool is_down) {}

    /// The same in parts of the surface, as the place on a texture is
    /// written: 0 is the left and the top, 1 the right and the bottom.
    virtual void SetPointerUv(const int surface, const float u, const float v, const bool is_down) {}

    /// Says that the player points at nothing on the surface.
    virtual void ClearPointer(const int surface) {}

    /// Hands the keys and the controller to a surface. One surface has
    /// them at a time, and it is the window until the game says otherwise.
    /// What had the focus loses it. Returns false when there is no such
    /// surface.
    virtual bool SetInputSurface(const int surface)
    {
      return false;
    }

    [[nodiscard]] virtual int GetInputSurface() const
    {
      return Ui_Window_Surface;
    }

    // Values and events of one user interface, by the name the file gives
    // itself under `ui`. A value that is set for one user interface wins
    // there over the value every user interface shares.

    virtual void SetNumberOf(const std::string &interface, const std::string &name, const double number) {}

    virtual void SetTextOf(const std::string &interface, const std::string &name, const std::string &text) {}

    virtual void SetFlagOf(const std::string &interface, const std::string &name, const bool flag) {}

    /// As OnClick(), for the element of that name in one user interface.
    /// It is called in place of the callback that is known for the name
    /// alone.
    virtual void OnClickIn(
      const std::string &interface,
      const std::string &element,
      const std::function<void()> &callback) {}

    /// Whether the element of that name in one user interface was chosen
    /// in the last Update().
    [[nodiscard]] virtual bool WasClickedIn(const std::string &interface, const std::string &element) const
    {
      return false;
    }
  };
} // neon

#endif //UI_CONTEXT_HPP
