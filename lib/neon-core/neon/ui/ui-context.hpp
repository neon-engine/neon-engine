#ifndef UI_CONTEXT_HPP
#define UI_CONTEXT_HPP

#include <functional>
#include <string>
#include <vector>

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

    /// Sets a value that files refer to as `{name}`. What shows it changes
    /// with it.
    virtual void SetNumber(const std::string &name, double number) = 0;

    virtual void SetText(const std::string &name, const std::string &text) = 0;

    virtual void SetFlag(const std::string &name, bool flag) = 0;

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
