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
  };

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
  };
} // neon

#endif //UI_CONTEXT_HPP
