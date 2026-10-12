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

    /// What a descriptor set is shown to Dear ImGui as.
    ImTextureID texture_id_of(const VkDescriptorSet set)
    {
      return static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(set));
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
    _input_system = input_system;
    _render_system = render_system;
  }

  bool ImGui_ToolUiSystem::Initialize()
  {
    if (_initialized) { return true; }

    if (_window_system->GetWindow() == nullptr)
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
    // the layout is not kept between runs yet, so no file is written next
    // to wherever the editor was started
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL2_InitForVulkan(_window_system->GetWindow()))
    {
      _logger->Critical("Dear ImGui could not start with the window");
      ImGui::DestroyContext();
      return false;
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
      ImGui_ImplSDL2_Shutdown();
      ImGui::DestroyContext();
      return false;
    }

    _input_system->SetEventListener(this);
    _initialized = true;
    const std::string version = ImGui::GetVersion();
    _logger->Info("The tool interface is drawn with Dear ImGui {}", version);
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

    _input_system->SetEventListener(nullptr);

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
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
  }

  void ImGui_ToolUiSystem::HandleEvent(const SDL_Event &event)
  {
    if (!_initialized) { return; }
    ImGui_ImplSDL2_ProcessEvent(&event);
  }

  void ImGui_ToolUiSystem::BeginFrame()
  {
    if (!_initialized || _frame_begun) { return; }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
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
    const ImGuiID space = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    // The windows are placed once, the first time the space is declared.
    // After that whoever drags them decides.
    if (_dock_laid_out) { return; }
    _dock_laid_out = true;

    ImGui::DockBuilderRemoveNode(space);
    ImGui::DockBuilderAddNode(space, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(space, ImGui::GetMainViewport()->WorkSize);

    ImGuiID center = space;
    if (!layout.bottom.empty())
    {
      ImGuiID bottom = 0;
      ImGui::DockBuilderSplitNode(space, ImGuiDir_Down, layout.bottom_share, &bottom, &center);
      ImGui::DockBuilderDockWindow(layout.bottom.c_str(), bottom);
    }
    if (!layout.center.empty()) { ImGui::DockBuilderDockWindow(layout.center.c_str(), center); }
    ImGui::DockBuilderFinish(space);
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
    return ImGui::BeginMenu(label.c_str());
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
    return ImGui::MenuItem(label.c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), selected, enabled);
  }

  bool ImGui_ToolUiSystem::BeginWindow(const std::string &title, const bool scrolls)
  {
    const ImGuiWindowFlags flags = scrolls ? 0 : ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    return ImGui::Begin(title.c_str(), nullptr, flags);
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
    return ImGui::Button(label.c_str());
  }

  bool ImGui_ToolUiSystem::Checkbox(const std::string &label, bool &checked)
  {
    return ImGui::Checkbox(label.c_str(), &checked);
  }

  bool ImGui_ToolUiSystem::InputText(const std::string &label, std::string &text)
  {
    return ImGui::InputText(label.c_str(), &text);
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
