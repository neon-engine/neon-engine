#ifndef VK_DEVICE_HPP
#define VK_DEVICE_HPP

#include <memory>
#include <string>
#include <vector>
#include <volk.h>
#include <neon/logging/logger.hpp>
#include <neon/render/api-version.hpp>
#include <neon/render/render-capabilities.hpp>
#include <neon/window/window-context.hpp>

namespace neon
{
  /// The connection to Vulkan that everything else in the backend shares: the
  /// instance, the graphics card that was picked, and the logical device with
  /// its queue. It also holds the small helpers for creating buffers and
  /// images, which would otherwise be repeated by every resource.
  // ReSharper disable once CppInconsistentNaming
  class VK_Device
  {
    VkInstance _instance = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties _properties{};
    uint32_t _requested_version = 0;
    uint32_t _instance_version = 0;
    uint32_t _api_version = 0;
    VkDevice _device = VK_NULL_HANDLE;
    uint32_t _queue_family = 0;
    VkQueue _queue = VK_NULL_HANDLE;
    VkCommandPool _command_pool = VK_NULL_HANDLE;
    VkCommandBuffer _frame_commands = VK_NULL_HANDLE;
    RenderCapabilities _capabilities;
    std::shared_ptr<Logger> _logger;

    bool LoadLibrary() const;

    bool CreateInstance(const std::vector<std::string> &window_extensions);

    bool PickPhysicalDevice();

    bool CreateDevice();

    /// Asks what the graphics card can do, and logs it. Returns false when
    /// it lacks what the renderer cannot do without.
    bool FindCapabilities();

    [[nodiscard]] bool FindMemoryType(uint32_t allowed_types, VkMemoryPropertyFlags properties, uint32_t &type) const;

  public:
    /// Brings Vulkan up. With a window, its surface is created as well. With
    /// a headless window context there is no surface and nothing is presented.
    /// It renders with the highest version of Vulkan up to `requested` that
    /// the driver and the graphics card offer, and fails when that is less
    /// than the engine needs.
    bool Initialize(
      WindowContext *window_context,
      const ApiVersion &requested,
      const std::shared_ptr<Logger> &logger);

    void CleanUp();

    [[nodiscard]] VkInstance Instance() const { return _instance; }
    [[nodiscard]] VkSurfaceKHR Surface() const { return _surface; }
    [[nodiscard]] VkPhysicalDevice PhysicalDevice() const { return _physical_device; }
    [[nodiscard]] const VkPhysicalDeviceProperties &Properties() const { return _properties; }

    /// The version of Vulkan that is rendered with, see VK_ApiVersion.
    [[nodiscard]] uint32_t VulkanVersion() const { return _api_version; }

    /// What the graphics card can do, once Initialize() succeeded.
    [[nodiscard]] const RenderCapabilities &Capabilities() const { return _capabilities; }
    [[nodiscard]] VkDevice Device() const { return _device; }
    [[nodiscard]] uint32_t QueueFamily() const { return _queue_family; }
    [[nodiscard]] VkQueue Queue() const { return _queue; }
    [[nodiscard]] VkCommandPool CommandPool() const { return _command_pool; }

    /// The commands of the frame that is being drawn, or VK_NULL_HANDLE
    /// outside a frame. Meshes and materials record into it.
    [[nodiscard]] VkCommandBuffer FrameCommands() const { return _frame_commands; }
    void SetFrameCommands(const VkCommandBuffer commands) { _frame_commands = commands; }

    bool CreateBuffer(
      VkDeviceSize size,
      VkBufferUsageFlags usage,
      VkMemoryPropertyFlags properties,
      VkBuffer &buffer,
      VkDeviceMemory &memory) const;

    bool CreateImage(
      uint32_t width,
      uint32_t height,
      uint32_t mip_levels,
      VkFormat format,
      VkImageUsageFlags usage,
      VkImage &image,
      VkDeviceMemory &memory,
      VkImageCreateFlags flags = 0,
      uint32_t layers = 1) const;

    /// A view of `layers` layers of the image from `first_layer` on: one
    /// layer is a 2D view, more are an array the shaders index.
    bool CreateImageView(
      VkImage image,
      VkFormat format,
      VkImageAspectFlags aspect,
      uint32_t mip_levels,
      VkImageView &view,
      uint32_t first_layer = 0,
      uint32_t layers = 1) const;

    /// Starts commands that are run once, right away, outside a frame. Used
    /// for uploads and copies.
    [[nodiscard]] VkCommandBuffer BeginCommands() const;

    /// Runs the commands started by BeginCommands() and waits for them.
    bool EndCommands(VkCommandBuffer commands) const;

    /// Moves mip levels of an image from one layout to another.
    static void TransitionImage(
      VkCommandBuffer commands,
      VkImage image,
      VkImageAspectFlags aspect,
      uint32_t first_mip_level,
      uint32_t mip_levels,
      VkImageLayout from,
      VkImageLayout to,
      uint32_t layers = 1);
  };
} // neon

#endif //VK_DEVICE_HPP
