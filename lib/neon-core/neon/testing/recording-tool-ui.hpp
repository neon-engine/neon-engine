#ifndef RECORDING_TOOL_UI_HPP
#define RECORDING_TOOL_UI_HPP

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include <neon/tool-ui/tool-ui-context.hpp>

namespace neon::testing
{
  /// A tool interface that keeps what is declared on it, as one line of
  /// words each, so that a test can ask what a window would have shown.
  /// Every window is open and the room in a window is what the test says.
  ///
  /// It is also a toolkit the size of a test: every item is recorded with
  /// a place of its own, one under the other, and the events fed through
  /// FeedEvents() are what the items react to in the frame NextFrame()
  /// begins. A button, a menu item, or a checkbox is pressed when the
  /// pointer is over where it was in the frame before and the button of
  /// the pointer is released; a field takes the text that is typed once it
  /// was clicked. That is how a script of the editor is tested without a
  /// window and without Dear ImGui. Menus are open unless
  /// `menus_follow_clicks` is set, when a menu opens with a click on it
  /// and closes when one of its items is chosen.
  class RecordingToolUi final : public ToolUiContext
  {
    // what was drawn since NextFrame(), and in the frame before it, which
    // is where a click of this frame lands
    std::vector<ToolUiItem> _items;
    std::vector<ToolUiItem> _items_before;
    float _next_y = 0.0f;

    // what was fed for the next frame, and what this frame reads
    ToolUiEvents _fed;
    std::optional<ToolUiPoint> _pointer;
    bool _button_down = false;
    bool _released = false;
    std::string _typed;
    std::vector<std::string> _open_menus;
    std::string _focused_field;

    [[nodiscard]] bool Hit(const std::string &label, const ToolUiItemKind kind) const
    {
      if (!_released || !_pointer.has_value()) { return false; }
      for (const ToolUiItem &item : _items_before)
      {
        if (item.kind == kind && item.label == label && item.rect.Contains(*_pointer)) { return true; }
      }
      return false;
    }

    const ToolUiItem &Record(const ToolUiItemKind kind, const std::string &label)
    {
      const ToolUiRect rect{.x = 10.0f, .y = _next_y, .width = 100.0f, .height = 20.0f};
      _next_y += 20.0f;
      return _items.emplace_back(ToolUiItem{.kind = kind, .label = label, .rect = rect});
    }

  public:
    std::vector<std::string> calls;
    ToolUiSize available{640.0f, 360.0f};
    bool windows_open = true;
    bool hovered = false;
    bool focused = false;
    bool wants_keyboard = false;
    bool menus_follow_clicks = false;

    /// Every call of FeedEvents(), in order.
    std::vector<ToolUiEvents> events_fed;

    /// Begins a frame: what was fed becomes the input of this frame, and
    /// what was drawn becomes the frame before.
    void NextFrame()
    {
      _items_before = _items;
      _items.clear();
      _next_y = 0.0f;

      if (_fed.pointer.has_value()) { _pointer = _fed.pointer; }
      _released = _fed.button_down.has_value() && !*_fed.button_down && _button_down;
      if (_fed.button_down.has_value()) { _button_down = *_fed.button_down; }
      _typed = _fed.text;
      _fed = ToolUiEvents{};
    }

    /// Where the pointer is, as the events put it.
    [[nodiscard]] const std::optional<ToolUiPoint> &GetPointer() const { return _pointer; }

    [[nodiscard]] const std::vector<std::string> &GetOpenMenus() const { return _open_menus; }

    void FeedEvents(const ToolUiEvents &events) override
    {
      events_fed.push_back(events);
      if (events.pointer.has_value()) { _fed.pointer = events.pointer; }
      if (events.button_down.has_value()) { _fed.button_down = events.button_down; }
      _fed.keys.insert(_fed.keys.end(), events.keys.begin(), events.keys.end());
      _fed.text += events.text;
      _fed.wheel_x += events.wheel_x;
      _fed.wheel_y += events.wheel_y;
    }

    [[nodiscard]] const std::vector<ToolUiItem> &GetItemsDrawn() const override { return _items; }

    void DockSpace(const ToolUiDockLayout &layout) override
    {
      calls.push_back("dock-space " + layout.center + " " + layout.bottom);
    }

    bool BeginMainMenuBar() override
    {
      calls.emplace_back("begin-main-menu-bar");
      return true;
    }

    void EndMainMenuBar() override { calls.emplace_back("end-main-menu-bar"); }

    bool BeginMenu(const std::string &label) override
    {
      calls.push_back("begin-menu " + label);
      Record(ToolUiItemKind::Menu, label);
      if (!menus_follow_clicks) { return true; }

      if (Hit(label, ToolUiItemKind::Menu))
      {
        if (std::ranges::find(_open_menus, label) == _open_menus.end()) { _open_menus.push_back(label); }
        else { std::erase(_open_menus, label); }
      }
      return std::ranges::find(_open_menus, label) != _open_menus.end();
    }

    void EndMenu() override { calls.emplace_back("end-menu"); }

    bool MenuItem(const std::string &label, const std::string &shortcut, bool selected, bool enabled) override
    {
      calls.push_back("menu-item " + label);
      Record(ToolUiItemKind::MenuItem, label);
      if (!enabled || !Hit(label, ToolUiItemKind::MenuItem)) { return false; }
      _open_menus.clear();
      return true;
    }

    bool BeginWindow(const std::string &title, bool scrolls) override
    {
      calls.push_back("begin-window " + title);
      _items.push_back({
        .kind = ToolUiItemKind::Window,
        .label = title,
        .rect = {.x = 0.0f, .y = 0.0f, .width = available.width, .height = available.height}
      });
      return windows_open;
    }

    void EndWindow() override { calls.emplace_back("end-window"); }

    [[nodiscard]] ToolUiSize GetAvailableSize() override { return available; }

    [[nodiscard]] bool IsWindowHovered() override { return hovered; }

    [[nodiscard]] bool IsWindowFocused() override { return focused; }

    void Text(const std::string &text) override
    {
      calls.push_back("text " + text);
      Record(ToolUiItemKind::Text, text);
    }

    void Separator() override { calls.emplace_back("separator"); }

    void SameLine() override { calls.emplace_back("same-line"); }

    bool Button(const std::string &label) override
    {
      calls.push_back("button " + label);
      Record(ToolUiItemKind::Button, label);
      return Hit(label, ToolUiItemKind::Button);
    }

    bool Checkbox(const std::string &label, bool &checked) override
    {
      calls.push_back("checkbox " + label);
      Record(ToolUiItemKind::Checkbox, label);
      if (!Hit(label, ToolUiItemKind::Checkbox)) { return false; }
      checked = !checked;
      return true;
    }

    bool InputText(const std::string &label, std::string &text) override
    {
      calls.push_back("input-text " + label);
      Record(ToolUiItemKind::InputText, label);
      if (Hit(label, ToolUiItemKind::InputText)) { _focused_field = label; }
      if (_focused_field != label || _typed.empty()) { return false; }
      text += _typed;
      _typed.clear();
      return true;
    }

    void ImageOfRenderTarget(const int target, const float width, const float height) override
    {
      calls.push_back(
        "image " + std::to_string(target) + " " + std::to_string(static_cast<int>(width)) + "x" +
        std::to_string(static_cast<int>(height)));
    }

    void ScrollToBottom() override { calls.emplace_back("scroll-to-bottom"); }

    [[nodiscard]] bool WantsPointer() const override { return false; }

    [[nodiscard]] bool WantsKeyboard() const override { return wants_keyboard; }
  };
} // neon::testing

#endif //RECORDING_TOOL_UI_HPP
