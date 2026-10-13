#ifndef TOOL_UI_CONTEXT_HPP
#define TOOL_UI_CONTEXT_HPP

#include <optional>
#include <string>
#include <vector>

#include "tool-ui-dock-layout.hpp"
#include "tool-ui-events.hpp"
#include "tool-ui-item.hpp"
#include "tool-ui-size.hpp"

namespace neon
{
  /// What the editor draws its own interface with: windows, menus, and
  /// widgets, declared anew in every frame between BeginFrame() and
  /// EndFrame() of the ToolUiSystem, in the order they are shown. It is
  /// the interface of the tools, not of a game: a game's interface is the
  /// UiSystem, read from recipes and styled with CSS. Dear ImGui is behind
  /// this today, in neon-imgui (#82), and another toolkit can be (#103),
  /// so it holds what the editor uses and nothing of the toolkit.
  ///
  /// Every Begin has its End. A window that is collapsed or hidden says so
  /// through the return value of BeginWindow(), and what it would hold is
  /// left out, but EndWindow() is called all the same. A menu that is not
  /// open is left out with its End as well: EndMenu() is called only when
  /// BeginMenu() returned true, and so for the menu bar.
  class ToolUiContext
  {
  protected:
    ~ToolUiContext() = default;

  public:
    /// A space over the whole window that windows dock into, under the
    /// menu bar. Called once in a frame, before the windows. `layout` says
    /// where a window goes the first time it is shown.
    virtual void DockSpace(const ToolUiDockLayout &layout) = 0;

    /// The bar along the top of the window, which the menus go into.
    virtual bool BeginMainMenuBar() = 0;

    virtual void EndMainMenuBar() = 0;

    /// A menu of the bar, or of the menu that is open, by its label.
    /// Returns true while it is open, and its items are declared then.
    virtual bool BeginMenu(const std::string &label) = 0;

    virtual void EndMenu() = 0;

    /// An item of the menu that is open, which returns true in the frame
    /// it is chosen. `shortcut` is shown next to it and binds nothing: what
    /// it names is asked for with IsShortcutPressed(). An item that is
    /// `selected` shows a mark, as one that switches a window on and off
    /// does while the window is shown.
    virtual bool MenuItem(
      const std::string &label,
      const std::string &shortcut = "",
      bool selected = false,
      bool enabled = true) = 0;

    /// Whether the keys of a shortcut were pressed in this frame, wherever
    /// the focus is, unless a field that has it took the same keys. The
    /// shortcut is written as it is shown in a menu, `Ctrl+Shift+L`: the
    /// modifiers Ctrl, Shift, Alt, and Super in any order, with a plus
    /// between them, and one key last: a letter, a digit, or F1 to F12.
    /// Ctrl is the command key on macOS, as the platform expects. A
    /// shortcut written otherwise is never pressed.
    [[nodiscard]] virtual bool IsShortcutPressed(const std::string &shortcut) = 0;

    /// A window with that title, which is also what names it in a layout.
    /// Returns false while it is collapsed, when what it holds is left
    /// out. A window that `scrolls` shows a scrollbar for what does not
    /// fit; one that does not clips it, as the viewport does.
    virtual bool BeginWindow(const std::string &title, bool scrolls = true) = 0;

    virtual void EndWindow() = 0;

    /// The room left in the window from where the next widget goes, in
    /// points.
    [[nodiscard]] virtual ToolUiSize GetAvailableSize() = 0;

    /// Whether the pointer is over the window that is begun, and nothing
    /// of the tools is in front of it.
    [[nodiscard]] virtual bool IsWindowHovered() = 0;

    /// Whether the window that is begun has the focus, as the one clicked
    /// last has.
    [[nodiscard]] virtual bool IsWindowFocused() = 0;

    virtual void Text(const std::string &text) = 0;

    virtual void Separator() = 0;

    /// Puts the next widget on the line of the last one.
    virtual void SameLine() = 0;

    /// Returns true in the frame the button is pressed.
    virtual bool Button(const std::string &label) = 0;

    /// Returns true in the frame the box is switched, with `checked`
    /// changed.
    virtual bool Checkbox(const std::string &label, bool &checked) = 0;

    /// A field that edits `text`. Returns true in a frame it changed.
    virtual bool InputText(const std::string &label, std::string &text) = 0;

    /// Shows a render target of the renderer, by what RenderContext knows
    /// it as, at that size in points. The target is drawn into earlier in
    /// the frame, before the tool interface is drawn on top of the frame,
    /// so it shows what was drawn in this frame.
    virtual void ImageOfRenderTarget(int target, float width, float height) = 0;

    /// Scrolls the window to its end, where the last line of a log is.
    virtual void ScrollToBottom() = 0;

    /// Whether the tool interface is using the pointer, because it is over
    /// a window of it, or the keyboard, because a field has the focus. A
    /// game under the tools is then not given that input.
    [[nodiscard]] virtual bool WantsPointer() const = 0;

    [[nodiscard]] virtual bool WantsKeyboard() const = 0;

    /// Gives the tools input for the frame that is begun next, as a script
    /// of the editor does, see docs/editor.md#scripted-runs. The backend
    /// takes it in as it takes the events of its window, so that it works
    /// with a window and without one. Called before BeginFrame().
    virtual void FeedEvents(const ToolUiEvents &events) = 0;

    /// Everything that was drawn since the frame began, each under its
    /// label and where it went, see ToolUiItem; after EndFrame(), the whole
    /// frame, which is what a script reads to find what it clicks next.
    [[nodiscard]] virtual const std::vector<ToolUiItem> &GetItemsDrawn() const = 0;

    /// The first item drawn under that label, of that kind or of any kind.
    /// A text is found by the text itself.
    [[nodiscard]] std::optional<ToolUiItem> FindItem(
      const std::string &label,
      const std::optional<ToolUiItemKind> kind = std::nullopt) const
    {
      for (const ToolUiItem &item : GetItemsDrawn())
      {
        if (item.label == label && (!kind.has_value() || item.kind == *kind)) { return item; }
      }
      return std::nullopt;
    }
  };
} // neon

#endif //TOOL_UI_CONTEXT_HPP
