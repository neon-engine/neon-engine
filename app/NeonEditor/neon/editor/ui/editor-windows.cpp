#include "editor-windows.hpp"

namespace neon
{
  EditorWindow *EditorWindows::Find(const std::string &title)
  {
    for (auto &window : _windows)
    {
      if (window.title == title) { return &window; }
    }
    return nullptr;
  }

  const EditorWindow *EditorWindows::Find(const std::string &title) const
  {
    for (const auto &window : _windows)
    {
      if (window.title == title) { return &window; }
    }
    return nullptr;
  }

  bool EditorWindows::Add(const EditorWindow &window)
  {
    if (window.title.empty() || Has(window.title)) { return false; }

    // hidden until it is shown, whatever was handed in
    EditorWindow &added = _windows.emplace_back(window);
    added.shown = false;
    return true;
  }

  bool EditorWindows::Has(const std::string &title) const
  {
    return Find(title) != nullptr;
  }

  bool EditorWindows::IsShown(const std::string &title) const
  {
    const EditorWindow *window = Find(title);
    return window != nullptr && window->shown;
  }

  bool EditorWindows::Show(const std::string &title, const bool shown)
  {
    EditorWindow *window = Find(title);
    if (window == nullptr) { return false; }

    window->shown = shown;
    return true;
  }

  bool EditorWindows::Toggle(const std::string &title)
  {
    EditorWindow *window = Find(title);
    if (window == nullptr) { return false; }

    window->shown = !window->shown;
    return true;
  }

  std::vector<std::string> EditorWindows::GetShownTitles() const
  {
    std::vector<std::string> titles;
    for (const auto &window : _windows)
    {
      if (window.shown) { titles.push_back(window.title); }
    }
    return titles;
  }

  void EditorWindows::DrawViewMenu(ToolUiContext &ui)
  {
    if (!ui.BeginMenu(std::string(menu))) { return; }

    for (auto &window : _windows)
    {
      if (ui.MenuItem(window.title, window.shortcut, window.shown)) { window.shown = !window.shown; }
    }

    ui.EndMenu();
  }

  void EditorWindows::HandleShortcuts(ToolUiContext &ui)
  {
    for (auto &window : _windows)
    {
      if (!window.shortcut.empty() && ui.IsShortcutPressed(window.shortcut)) { window.shown = !window.shown; }
    }
  }

  void EditorWindows::DrawShown(ToolUiContext &ui)
  {
    for (const auto &window : _windows)
    {
      if (window.shown && window.draw) { window.draw(ui); }
    }
  }

  ToolUiDockLayout EditorWindows::LayoutAround(const std::string &center) const
  {
    ToolUiDockLayout layout{.center = center};
    for (const auto &window : _windows)
    {
      layout.sides.push_back({.window = window.title, .side = window.side, .share = window.share});
    }
    return layout;
  }
} // neon
