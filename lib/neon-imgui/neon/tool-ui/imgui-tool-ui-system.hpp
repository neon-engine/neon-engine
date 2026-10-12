#ifndef IMGUI_TOOL_UI_SYSTEM_HPP
#define IMGUI_TOOL_UI_SYSTEM_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include <neon/input/sdl2-event-listener.hpp>
#include <neon/input/sdl2-input-system.hpp>
#include <neon/render/vk-render-system.hpp>
#include <neon/tool-ui/tool-ui-system.hpp>
#include <neon/window/sdl2-window-system.hpp>

namespace neon
{
  /// The tool interface drawn with Dear ImGui, its docking branch: its
  /// SDL2 backend reads the events of the window through the input
  /// system, and its Vulkan backend draws into the frame of the renderer,
  /// on top of everything else. Docking is on; windows torn off onto the
  /// desktop are not, see docs/editor.md.
  ///
  /// Nothing of Dear ImGui is in this header: what the editor sees is
  /// ToolUiContext.
  // ReSharper disable once CppInconsistentNaming
  class ImGui_ToolUiSystem final : public ToolUiSystem, public SDL2_EventListener
  {
    /// A render target as Dear ImGui draws it: the view it was registered
    /// with, and what it was registered as. Registered again when the
    /// target is made again with another view.
    struct RegisteredTarget
    {
      VkImageView view = VK_NULL_HANDLE;
      VkDescriptorSet set = VK_NULL_HANDLE;
    };

    SDL2_WindowSystem *_window_system;
    SDL2_InputSystem *_input_system;
    VK_RenderSystem *_render_system;

    bool _initialized = false;
    bool _frame_begun = false;

    // whether the dock space was laid out as the layout asks, which is
    // done once, the first time it is declared
    bool _dock_laid_out = false;

    std::map<int, RegisteredTarget> _targets;

    /// Lets Dear ImGui go of every target that was registered.
    void ForgetTargets();

  public:
    ImGui_ToolUiSystem(
      SDL2_WindowSystem *window_system,
      SDL2_InputSystem *input_system,
      VK_RenderSystem *render_system,
      const std::shared_ptr<Logger> &logger);

    bool Initialize() override;

    void CleanUp() override;

    void BeginFrame() override;

    void EndFrame() override;

    void HandleEvent(const SDL_Event &event) override;

    void DockSpace(const ToolUiDockLayout &layout) override;

    bool BeginMainMenuBar() override;

    void EndMainMenuBar() override;

    bool BeginMenu(const std::string &label) override;

    void EndMenu() override;

    bool MenuItem(const std::string &label, const std::string &shortcut, bool selected, bool enabled) override;

    bool BeginWindow(const std::string &title, bool scrolls) override;

    void EndWindow() override;

    [[nodiscard]] ToolUiSize GetAvailableSize() override;

    [[nodiscard]] bool IsWindowHovered() override;

    [[nodiscard]] bool IsWindowFocused() override;

    void Text(const std::string &text) override;

    void Separator() override;

    void SameLine() override;

    bool Button(const std::string &label) override;

    bool Checkbox(const std::string &label, bool &checked) override;

    bool InputText(const std::string &label, std::string &text) override;

    void ImageOfRenderTarget(int target, float width, float height) override;

    void ScrollToBottom() override;

    [[nodiscard]] bool WantsPointer() const override;

    [[nodiscard]] bool WantsKeyboard() const override;
  };
} // neon

#endif //IMGUI_TOOL_UI_SYSTEM_HPP
