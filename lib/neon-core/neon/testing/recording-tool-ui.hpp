#ifndef RECORDING_TOOL_UI_HPP
#define RECORDING_TOOL_UI_HPP

#include <string>
#include <vector>

#include <neon/tool-ui/tool-ui-context.hpp>

namespace neon::testing
{
  /// A tool interface that keeps what is declared on it, as one line of
  /// words each, so that a test can ask what a window would have shown.
  /// Every window is open, every button is up, and the room in a window
  /// is what the test says.
  class RecordingToolUi final : public ToolUiContext
  {
  public:
    std::vector<std::string> calls;
    ToolUiSize available{640.0f, 360.0f};
    bool windows_open = true;
    bool hovered = false;
    bool focused = false;
    bool wants_keyboard = false;

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
      return true;
    }

    void EndMenu() override { calls.emplace_back("end-menu"); }

    bool MenuItem(const std::string &label, const std::string &shortcut, bool selected, bool enabled) override
    {
      calls.push_back("menu-item " + label);
      return false;
    }

    bool BeginWindow(const std::string &title, bool scrolls) override
    {
      calls.push_back("begin-window " + title);
      return windows_open;
    }

    void EndWindow() override { calls.emplace_back("end-window"); }

    [[nodiscard]] ToolUiSize GetAvailableSize() override { return available; }

    [[nodiscard]] bool IsWindowHovered() override { return hovered; }

    [[nodiscard]] bool IsWindowFocused() override { return focused; }

    void Text(const std::string &text) override { calls.push_back("text " + text); }

    void Separator() override { calls.emplace_back("separator"); }

    void SameLine() override { calls.emplace_back("same-line"); }

    bool Button(const std::string &label) override
    {
      calls.push_back("button " + label);
      return false;
    }

    bool Checkbox(const std::string &label, bool &checked) override
    {
      calls.push_back("checkbox " + label);
      return false;
    }

    bool InputText(const std::string &label, std::string &text) override
    {
      calls.push_back("input-text " + label);
      return false;
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
