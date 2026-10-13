// Before everything else: the math.h of the macOS 27 SDK includes only the
// part of float.h that has infinity and NaN, and libc++ 20 then takes its
// float.h as read, so FLT_MIN and FLT_MAX, which imgui.h needs, are never
// defined unless float.h is read whole before any math header is.
#include <cfloat>

#include "imgui-tool-ui-system.hpp"

#include <cstdint>
#include <string>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include <imgui_stdlib.h>

namespace neon
{
  // Helpers of ImGui_ToolUiSystem, for this file alone.
  namespace
  {
    /// How many sets of images Dear ImGui may register: its font, and the
    /// render targets that are shown.
    constexpr uint32_t descriptor_pool_size = 64;

    /// Frames the Vulkan backend keeps buffers for. The renderer waits for
    /// every frame before the next, so two are plenty.
    constexpr uint32_t image_count = 2;

    /// What a frame stands for without a window when the window system
    /// says nothing.
    constexpr float fallback_delta_time = 1.0f / 60.0f;

    /// What a descriptor set is shown to Dear ImGui as.
    ImTextureID texture_id_of(const VkDescriptorSet set)
    {
      return static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(set));
    }

    /// The direction a side of the dock space is split off towards.
    ImGuiDir direction_of(const ToolUiDockSide side)
    {
      switch (side)
      {
        case ToolUiDockSide::Left:
        {
          return ImGuiDir_Left;
        }
        case ToolUiDockSide::Right:
        {
          return ImGuiDir_Right;
        }
        case ToolUiDockSide::Top:
        {
          return ImGuiDir_Up;
        }
        default:
        {
          return ImGuiDir_Down;
        }
      }
    }

    /// Reads a shortcut as a menu shows it, `Ctrl+Shift+L`, into the keys
    /// Dear ImGui asks for. Returns false when it is written otherwise.
    bool chord_of(const std::string &shortcut, ImGuiKeyChord &chord)
    {
      chord = 0;
      std::size_t start = 0;
      while (start < shortcut.size())
      {
        std::size_t end = shortcut.find('+', start);
        if (end == std::string::npos) { end = shortcut.size(); }
        const std::string part = shortcut.substr(start, end - start);
        start = end + 1;

        if (part == "Ctrl") { chord |= ImGuiMod_Ctrl; }
        else if (part == "Shift") { chord |= ImGuiMod_Shift; }
        else if (part == "Alt") { chord |= ImGuiMod_Alt; }
        else if (part == "Super") { chord |= ImGuiMod_Super; }
        else
        {
          // the key is last, and there is one
          if ((chord & ~ImGuiMod_Mask_) != 0 || start <= shortcut.size()) { return false; }

          if (part.size() == 1 && part[0] >= 'A' && part[0] <= 'Z')
          {
            chord |= ImGuiKey_A + (part[0] - 'A');
          } else if (part.size() == 1 && part[0] >= '0' && part[0] <= '9')
          {
            chord |= ImGuiKey_0 + (part[0] - '0');
          } else if (part.size() >= 2 && part[0] == 'F')
          {
            int number = 0;
            for (std::size_t i = 1; i < part.size(); i++)
            {
              if (part[i] < '0' || part[i] > '9') { return false; }
              number = number * 10 + (part[i] - '0');
            }
            if (number < 1 || number > 12) { return false; }
            chord |= ImGuiKey_F1 + (number - 1);
          } else
          {
            return false;
          }
        }
      }
      return (chord & ~ImGuiMod_Mask_) != 0;
    }

    /// The key of Dear ImGui a key of the engine is, or ImGuiKey_None for
    /// one it has no name for.
    ImGuiKey imgui_key_of(const Key key)
    {
      switch (key)
      {
        case Key::Left: return ImGuiKey_LeftArrow;
        case Key::Right: return ImGuiKey_RightArrow;
        case Key::Up: return ImGuiKey_UpArrow;
        case Key::Down: return ImGuiKey_DownArrow;
        case Key::Home: return ImGuiKey_Home;
        case Key::End: return ImGuiKey_End;
        case Key::PageUp: return ImGuiKey_PageUp;
        case Key::PageDown: return ImGuiKey_PageDown;
        case Key::Backspace: return ImGuiKey_Backspace;
        case Key::Delete: return ImGuiKey_Delete;
        case Key::Enter: return ImGuiKey_Enter;
        case Key::Tab: return ImGuiKey_Tab;
        case Key::Escape: return ImGuiKey_Escape;
        case Key::Space: return ImGuiKey_Space;
        case Key::LeftShift: return ImGuiKey_LeftShift;
        case Key::RightShift: return ImGuiKey_RightShift;
        case Key::LeftControl: return ImGuiKey_LeftCtrl;
        case Key::RightControl: return ImGuiKey_RightCtrl;
        case Key::LeftAlt: return ImGuiKey_LeftAlt;
        case Key::RightAlt: return ImGuiKey_RightAlt;
        default: break;
      }

      // the letters, the digits, and the function keys are in rows of
      // their own, in the same order in both
      if (key >= Key::A && key <= Key::Z)
      {
        return static_cast<ImGuiKey>(ImGuiKey_A + (static_cast<int>(key) - static_cast<int>(Key::A)));
      }
      if (key >= Key::Digit0 && key <= Key::Digit9)
      {
        return static_cast<ImGuiKey>(ImGuiKey_0 + (static_cast<int>(key) - static_cast<int>(Key::Digit0)));
      }
      if (key >= Key::F1 && key <= Key::F12)
      {
        return static_cast<ImGuiKey>(ImGuiKey_F1 + (static_cast<int>(key) - static_cast<int>(Key::F1)));
      }
      return ImGuiKey_None;
    }

    /// The modifiers of Dear ImGui that are held with a key: the keys as
    /// they are on the keyboard, which Dear ImGui reads for the platform
    /// itself, so that the shortcut key is command on a Mac through the
    /// same door the window's events take.
    void add_modifiers(ImGuiIO &io, const KeyModifiers &modifiers, const bool down)
    {
#ifdef __APPLE__
      const bool control = modifiers.control || modifiers.word;
      const bool super = modifiers.super || modifiers.shortcut;
      const bool alt = modifiers.alt || modifiers.word;
#else
      const bool control = modifiers.control || modifiers.shortcut || modifiers.word;
      const bool super = modifiers.super;
      const bool alt = modifiers.alt;
#endif
      if (modifiers.shift) { io.AddKeyEvent(ImGuiMod_Shift, down); }
      if (control) { io.AddKeyEvent(ImGuiMod_Ctrl, down); }
      if (alt) { io.AddKeyEvent(ImGuiMod_Alt, down); }
      if (super) { io.AddKeyEvent(ImGuiMod_Super, down); }
    }
  }

  ImGui_ToolUiSystem::ImGui_ToolUiSystem(
    SDL2_WindowSystem *window_system,
    SDL2_InputSystem *input_system,
    VK_RenderSystem *render_system,
    const std::shared_ptr<Logger> &logger)
    : ToolUiSystem(logger)
  {
    _window_system = window_system;
    _render_system = render_system;
    _sdl2_window = window_system;
    _sdl2_input = input_system;
  }

  ImGui_ToolUiSystem::ImGui_ToolUiSystem(
    WindowSystem *window_system,
    VK_RenderSystem *render_system,
    const std::shared_ptr<Logger> &logger)
    : ToolUiSystem(logger)
  {
    _window_system = window_system;
    _render_system = render_system;
  }

  bool ImGui_ToolUiSystem::Initialize()
  {
    if (_initialized) { return true; }

    if (_sdl2_window != nullptr && _sdl2_window->GetWindow() == nullptr)
    {
      _logger->Critical("The tool interface needs a window, and there is none");
      return false;
    }

    const VK_Device &device = _render_system->GetDevice();
    if (device.Device() == VK_NULL_HANDLE || _render_system->GetFramePass() == VK_NULL_HANDLE)
    {
      _logger->Critical("The tool interface needs the renderer, and it is not up");
      return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // the layout goes through LoadLayout() and SaveLayout(), so no file is
    // written next to wherever the editor was started
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();

    if (_sdl2_window != nullptr)
    {
      if (!ImGui_ImplSDL2_InitForVulkan(_sdl2_window->GetWindow()))
      {
        _logger->Critical("Dear ImGui could not start with the window");
        ImGui::DestroyContext();
        return false;
      }
    } else
    {
      // no platform backend: the display is what the settings say, and the
      // input is what is fed
      io.BackendPlatformName = "neon-headless";
      const WindowSize size = _window_system->GetWindowSize();
      const WindowSize drawable = _window_system->GetDrawableSize();
      io.DisplaySize = ImVec2(static_cast<float>(size.width), static_cast<float>(size.height));
      if (size.width > 0 && size.height > 0)
      {
        io.DisplayFramebufferScale = ImVec2(
          static_cast<float>(drawable.width) / static_cast<float>(size.width),
          static_cast<float>(drawable.height) / static_cast<float>(size.height));
      }
#ifdef __APPLE__
      io.ConfigMacOSXBehaviors = true;
#endif
    }

    ImGui_ImplVulkan_InitInfo info{};
    info.ApiVersion = device.VulkanVersion();
    info.Instance = device.Instance();
    info.PhysicalDevice = device.PhysicalDevice();
    info.Device = device.Device();
    info.QueueFamily = device.QueueFamily();
    info.Queue = device.Queue();
    info.DescriptorPoolSize = descriptor_pool_size;
    info.MinImageCount = image_count;
    info.ImageCount = image_count;
    info.PipelineInfoMain.RenderPass = _render_system->GetFramePass();
    info.PipelineInfoMain.Subpass = 0;
    info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    if (!ImGui_ImplVulkan_Init(&info))
    {
      _logger->Critical("Dear ImGui could not start with the renderer");
      if (_sdl2_window != nullptr) { ImGui_ImplSDL2_Shutdown(); }
      ImGui::DestroyContext();
      return false;
    }

    if (_sdl2_input != nullptr) { _sdl2_input->SetEventListener(this); }
    _initialized = true;
    const std::string version = ImGui::GetVersion();
    if (_sdl2_window != nullptr) { _logger->Info("The tool interface is drawn with Dear ImGui {}", version); }
    else
    {
      _logger->Info(
        "The tool interface is drawn with Dear ImGui {} without a window, at {} by {} points",
        version, io.DisplaySize.x, io.DisplaySize.y);
    }
    return true;
  }

  void ImGui_ToolUiSystem::ForgetTargets()
  {
    for (const auto &[target, registered] : _targets)
    {
      if (registered.set != VK_NULL_HANDLE) { ImGui_ImplVulkan_RemoveTexture(registered.set); }
    }
    _targets.clear();
  }

  void ImGui_ToolUiSystem::CleanUp()
  {
    if (!_initialized) { return; }
    _initialized = false;

    if (_sdl2_input != nullptr) { _sdl2_input->SetEventListener(nullptr); }

    // a frame that was begun and never drawn is let go
    if (_frame_begun)
    {
      ImGui::EndFrame();
      _frame_begun = false;
    }

    // nothing of the frame before may still be drawn
    vkDeviceWaitIdle(_render_system->GetDevice().Device());

    ForgetTargets();
    ImGui_ImplVulkan_Shutdown();
    if (_sdl2_window != nullptr) { ImGui_ImplSDL2_Shutdown(); }
    ImGui::DestroyContext();
    _space = 0;
  }

  void ImGui_ToolUiSystem::LoadLayout(const std::string &layout)
  {
    if (!_initialized || layout.empty()) { return; }
    ImGui::LoadIniSettingsFromMemory(layout.c_str(), layout.size());
  }

  std::string ImGui_ToolUiSystem::SaveLayout()
  {
    if (!_initialized) { return ""; }

    std::size_t size = 0;
    const char *text = ImGui::SaveIniSettingsToMemory(&size);
    ImGui::GetIO().WantSaveIniSettings = false;
    return text == nullptr ? std::string() : std::string(text, size);
  }

  void ImGui_ToolUiSystem::HandleEvent(const SDL_Event &event)
  {
    if (!_initialized) { return; }
    ImGui_ImplSDL2_ProcessEvent(&event);
  }

  void ImGui_ToolUiSystem::FeedEvents(const ToolUiEvents &events)
  {
    if (events.pointer.has_value()) { _pending.pointer = events.pointer; }
    if (events.button_down.has_value()) { _pending.button_down = events.button_down; }
    _pending.keys.insert(_pending.keys.end(), events.keys.begin(), events.keys.end());
    _pending.text += events.text;
    _pending.wheel_x += events.wheel_x;
    _pending.wheel_y += events.wheel_y;
  }

  void ImGui_ToolUiSystem::ApplyPendingEvents()
  {
    if (_pending.IsEmpty()) { return; }

    // the same calls the platform backend makes for the events of a
    // window, so that Dear ImGui tells them apart from nothing
    ImGuiIO &io = ImGui::GetIO();
    io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    if (_pending.pointer.has_value()) { io.AddMousePosEvent(_pending.pointer->x, _pending.pointer->y); }
    if (_pending.button_down.has_value()) { io.AddMouseButtonEvent(ImGuiMouseButton_Left, *_pending.button_down); }
    if (_pending.wheel_x != 0.0f || _pending.wheel_y != 0.0f)
    {
      io.AddMouseWheelEvent(-_pending.wheel_x, -_pending.wheel_y);
    }

    for (const KeyEvent &event : _pending.keys)
    {
      const ImGuiKey key = imgui_key_of(event.key);
      if (event.is_down) { add_modifiers(io, event.modifiers, true); }
      if (key != ImGuiKey_None) { io.AddKeyEvent(key, event.is_down); }
      if (!event.is_down) { add_modifiers(io, event.modifiers, false); }
    }

    if (!_pending.text.empty()) { io.AddInputCharactersUTF8(_pending.text.c_str()); }

    _pending = ToolUiEvents{};
  }

  const std::vector<ToolUiItem> &ImGui_ToolUiSystem::GetItemsDrawn() const
  {
    return _items;
  }

  void ImGui_ToolUiSystem::RecordLastItem(const ToolUiItemKind kind, const std::string &label)
  {
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    _items.push_back({
      .kind = kind,
      .label = label,
      .rect = {.x = min.x, .y = min.y, .width = max.x - min.x, .height = max.y - min.y}
    });
  }

  void ImGui_ToolUiSystem::BeginFrame()
  {
    if (!_initialized || _frame_begun) { return; }

    ImGui_ImplVulkan_NewFrame();
    if (_sdl2_window != nullptr) { ImGui_ImplSDL2_NewFrame(); }
    else
    {
      ImGuiIO &io = ImGui::GetIO();
      const WindowSize size = _window_system->GetWindowSize();
      io.DisplaySize = ImVec2(static_cast<float>(size.width), static_cast<float>(size.height));
      const double delta = _window_system->GetDeltaTime();
      io.DeltaTime = delta > 0.0 ? static_cast<float>(delta) : fallback_delta_time;
    }

    ApplyPendingEvents();
    ImGui::NewFrame();
    _items.clear();
    _frame_begun = true;
  }

  void ImGui_ToolUiSystem::EndFrame()
  {
    if (!_initialized || !_frame_begun) { return; }
    _frame_begun = false;

    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();

    const bool drawn = _render_system->DrawIntoFrame([draw_data](const VkCommandBuffer commands)
    {
      ImGui_ImplVulkan_RenderDrawData(draw_data, commands);
    });

    if (!drawn) { _logger->Warn("The tool interface was not drawn, since the renderer has no frame open"); }
  }

  void ImGui_ToolUiSystem::DockSpace(const ToolUiDockLayout &layout)
  {
    _space = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    _layout = layout;
  }

  void ImGui_ToolUiSystem::PlaceNewWindow(const std::string &title)
  {
    if (_space == 0) { return; }

    // A window that was shown in this run, or that the layout loaded
    // knows, goes where it was last. Dear ImGui keeps its place while it
    // is hidden, and the place of a docked window is what it was docked
    // to.
    if (ImGui::FindWindowByName(title.c_str()) != nullptr ||
        ImGui::FindWindowSettingsByID(ImHashStr(title.c_str())) != nullptr)
    {
      return;
    }

    const ImGuiDockNode *central = ImGui::DockBuilderGetCentralNode(_space);
    const ImGuiID middle = central != nullptr ? central->ID : _space;

    if (title == _layout.center)
    {
      ImGui::SetNextWindowDockID(middle, ImGuiCond_FirstUseEver);
      return;
    }

    for (const auto &placement : _layout.sides)
    {
      if (placement.window != title) { continue; }

      // a share of a side, split off from the middle, which keeps the rest
      ImGuiID at = 0;
      ImGuiID rest = 0;
      ImGui::DockBuilderSplitNode(middle, direction_of(placement.side), placement.share, &at, &rest);
      ImGui::SetNextWindowDockID(at, ImGuiCond_FirstUseEver);
      return;
    }
  }

  bool ImGui_ToolUiSystem::BeginMainMenuBar()
  {
    return ImGui::BeginMainMenuBar();
  }

  void ImGui_ToolUiSystem::EndMainMenuBar()
  {
    ImGui::EndMainMenuBar();
  }

  bool ImGui_ToolUiSystem::BeginMenu(const std::string &label)
  {
    const bool open = ImGui::BeginMenu(label.c_str());
    RecordLastItem(ToolUiItemKind::Menu, label);
    return open;
  }

  void ImGui_ToolUiSystem::EndMenu()
  {
    ImGui::EndMenu();
  }

  bool ImGui_ToolUiSystem::MenuItem(
    const std::string &label,
    const std::string &shortcut,
    const bool selected,
    const bool enabled)
  {
    const bool chosen = ImGui::MenuItem(label.c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), selected, enabled);
    RecordLastItem(ToolUiItemKind::MenuItem, label);
    return chosen;
  }

  bool ImGui_ToolUiSystem::IsShortcutPressed(const std::string &shortcut)
  {
    if (!_initialized || !_frame_begun) { return false; }

    ImGuiKeyChord chord = 0;
    if (!chord_of(shortcut, chord)) { return false; }

    // wherever the focus is, unless a field that has it took the keys
    return ImGui::Shortcut(chord, ImGuiInputFlags_RouteGlobal);
  }

  bool ImGui_ToolUiSystem::BeginWindow(const std::string &title, const bool scrolls)
  {
    PlaceNewWindow(title);

    const ImGuiWindowFlags flags = scrolls ? 0 : ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    const bool open = ImGui::Begin(title.c_str(), nullptr, flags);

    const ImVec2 position = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    _items.push_back({
      .kind = ToolUiItemKind::Window,
      .label = title,
      .rect = {.x = position.x, .y = position.y, .width = size.x, .height = size.y}
    });
    return open;
  }

  void ImGui_ToolUiSystem::EndWindow()
  {
    ImGui::End();
  }

  ToolUiSize ImGui_ToolUiSystem::GetAvailableSize()
  {
    const ImVec2 available = ImGui::GetContentRegionAvail();
    return {available.x, available.y};
  }

  bool ImGui_ToolUiSystem::IsWindowHovered()
  {
    return ImGui::IsWindowHovered();
  }

  bool ImGui_ToolUiSystem::IsWindowFocused()
  {
    return ImGui::IsWindowFocused();
  }

  void ImGui_ToolUiSystem::Text(const std::string &text)
  {
    ImGui::TextUnformatted(text.c_str(), text.c_str() + text.size());
    RecordLastItem(ToolUiItemKind::Text, text);
  }

  void ImGui_ToolUiSystem::Separator()
  {
    ImGui::Separator();
  }

  void ImGui_ToolUiSystem::SameLine()
  {
    ImGui::SameLine();
  }

  bool ImGui_ToolUiSystem::Button(const std::string &label)
  {
    const bool pressed = ImGui::Button(label.c_str());
    RecordLastItem(ToolUiItemKind::Button, label);
    return pressed;
  }

  bool ImGui_ToolUiSystem::Checkbox(const std::string &label, bool &checked)
  {
    const bool changed = ImGui::Checkbox(label.c_str(), &checked);
    RecordLastItem(ToolUiItemKind::Checkbox, label);
    return changed;
  }

  bool ImGui_ToolUiSystem::InputText(const std::string &label, std::string &text)
  {
    const bool changed = ImGui::InputText(label.c_str(), &text);
    RecordLastItem(ToolUiItemKind::InputText, label);
    return changed;
  }

  void ImGui_ToolUiSystem::ImageOfRenderTarget(const int target, const float width, const float height)
  {
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    if (!_render_system->GetRenderTargetView(target, view, sampler))
    {
      // the room stays, so that the window keeps its size
      ImGui::Dummy(ImVec2(width, height));
      return;
    }

    // Registered once, and again when the target was made again. The
    // renderer waits for every frame before the next is begun, so the set
    // of a view that went is read by nothing any more.
    RegisteredTarget &registered = _targets[target];
    if (registered.view != view)
    {
      if (registered.set != VK_NULL_HANDLE) { ImGui_ImplVulkan_RemoveTexture(registered.set); }
      registered.view = view;
      registered.set = ImGui_ImplVulkan_AddTexture(sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    ImGui::Image(ImTextureRef(texture_id_of(registered.set)), ImVec2(width, height));
  }

  void ImGui_ToolUiSystem::ScrollToBottom()
  {
    ImGui::SetScrollHereY(1.0f);
  }

  bool ImGui_ToolUiSystem::WantsPointer() const
  {
    return _initialized && ImGui::GetIO().WantCaptureMouse;
  }

  bool ImGui_ToolUiSystem::WantsKeyboard() const
  {
    return _initialized && ImGui::GetIO().WantCaptureKeyboard;
  }
} // neon
