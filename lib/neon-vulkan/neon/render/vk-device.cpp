#include "vk-device.hpp"

#include <algorithm>
#include <cstring>

#include "vk-api-version.hpp"
#include "vk-capabilities.hpp"

#if defined(__APPLE__)
#  include <dlfcn.h>
#endif

namespace neon
{
  // Helpers of VK_Device, for this file alone.
  namespace
  {
    bool has_extension(const std::vector<VkExtensionProperties> &available, const char *name)
    {
      return std::ranges::any_of(available, [name](const VkExtensionProperties &extension)
      {
        return std::strcmp(extension.extensionName, name) == 0;
      });
    }

    // higher is better
    int rate_device_type(const VkPhysicalDeviceType type)
    {
      switch (type)
      {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return 4;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 3;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return 2;
        case VK_PHYSICAL_DEVICE_TYPE_CPU: return 1;
        default: return 0;
      }
    }
  }

  bool VK_Device::LoadLibrary() const
  {
    if (volkInitialize() == VK_SUCCESS) { return true; }

#if defined(__APPLE__)
    // macOS only searches a few fixed folders for libraries, and Homebrew
    // installs the Vulkan loader outside of them
    for (const char *candidate : {"/opt/homebrew/lib/libvulkan.1.dylib", "/usr/local/lib/libvulkan.1.dylib"})
    {
      void *library = dlopen(candidate, RTLD_NOW | RTLD_LOCAL);
      if (library == nullptr) { continue; }

      if (void *entry = dlsym(library, "vkGetInstanceProcAddr"); entry != nullptr)
      {
        const std::string path(candidate);
        _logger->Info("Using the Vulkan library at {}", path);
        volkInitializeCustom(reinterpret_cast<PFN_vkGetInstanceProcAddr>(entry));
        return true;
      }
      dlclose(library);
    }
#endif

    return false;
  }

  bool VK_Device::Initialize(
    WindowContext *window_context,
    const ApiVersion &requested,
    const std::shared_ptr<Logger> &logger)
  {
    _logger = logger;

    // said before anything is looked for, since no driver can help it
    _requested_version = VK_ApiVersion::FromCore(requested);
    if (!VK_ApiVersion::IsEnough(_requested_version))
    {
      const std::string asked = requested.ToString();
      const std::string minimum = VK_ApiVersion::ToCore(VK_ApiVersion::kMinimum).ToString();
      _logger->Critical("Vulkan {} was asked for, but the engine needs Vulkan {} at least", asked, minimum);
      return false;
    }

    if (!LoadLibrary())
    {
      _logger->Critical("The Vulkan library could not be found, is a Vulkan driver installed?");
      return false;
    }

    // empty when there is no window to draw to
    const std::vector<std::string> window_extensions = window_context->GetVulkanInstanceExtensions();

    if (!CreateInstance(window_extensions)) { return false; }

    if (!window_extensions.empty() && !window_context->CreateVulkanSurface(_instance, &_surface))
    {
      _logger->Critical("The window could not provide a Vulkan surface");
      return false;
    }

    return PickPhysicalDevice() && FindCapabilities() && CreateDevice();
  }

  bool VK_Device::FindCapabilities()
  {
    VK_DeviceAnswers answers;
    answers.properties = _properties;
    answers.api_version = _api_version;
    vkGetPhysicalDeviceFeatures(_physical_device, &answers.features);

    // without a window nothing is presented, and there is nothing to choose
    if (_surface != VK_NULL_HANDLE)
    {
      uint32_t count = 0;
      vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &count, nullptr);
      answers.present_modes.resize(count);
      vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &count, answers.present_modes.data());
    }

    vkGetPhysicalDeviceFormatProperties(_physical_device, VK_Capabilities::kSceneFormat, &answers.scene_format);
    vkGetPhysicalDeviceFormatProperties(_physical_device, VK_Capabilities::kSrgbFormat, &answers.srgb_format);

    // made as a render target makes its image, see VK_RenderTarget
    VkImageFormatProperties image;
    answers.has_mutable_format_views = vkGetPhysicalDeviceImageFormatProperties(
      _physical_device, VK_Capabilities::kSrgbFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT, &image) == VK_SUCCESS;

    _capabilities = VK_Capabilities::Fill(answers);
    for (const auto &line : VK_Capabilities::Describe(_capabilities)) { _logger->Info("{}", line); }

    // Every frame is lit in the scene image, and every texture is read in
    // sRGB. Without either nothing could be drawn.
    if (!_capabilities.has_scene_format)
    {
      _logger->Critical("The graphics card cannot light the scene in an image of R16G16B16A16_SFLOAT");
      return false;
    }
    if (!_capabilities.has_srgb_textures)
    {
      _logger->Critical("The graphics card cannot read and filter textures in R8G8B8A8_SRGB");
      return false;
    }

    // only render targets need it, and a scene without them is drawn
    if (!_capabilities.has_mutable_format_views)
    {
      _logger->Warn("The graphics card cannot see an image in two formats, render targets cannot be made");
    }

    return true;
  }

  bool VK_Device::CreateInstance(const std::vector<std::string> &window_extensions)
  {
    const std::string minimum = VK_ApiVersion::ToCore(VK_ApiVersion::kMinimum).ToString();

    // A loader of Vulkan 1.0 does not have the function that says which
    // version it offers.
    uint32_t loader = VK_API_VERSION_1_0;
    if (vkEnumerateInstanceVersion != nullptr) { vkEnumerateInstanceVersion(&loader); }

    if (!VK_ApiVersion::IsEnough(loader))
    {
      const std::string offered = VK_ApiVersion::ToCore(loader).ToString();
      _logger->Critical("The Vulkan driver offers Vulkan {}, but the engine needs Vulkan {} at least. "
                        "Is the graphics driver up to date?", offered, minimum);
      return false;
    }
    _instance_version = VK_ApiVersion::ForInstance(_requested_version, loader);

    uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, available.data());

    std::vector<const char *> extensions;
    for (const auto &extension : window_extensions) { extensions.push_back(extension.c_str()); }

    // Drivers that translate Vulkan to another API, which is what MoltenVK
    // does on macOS, are hidden unless they are asked for
    VkInstanceCreateFlags flags = 0;
    if (has_extension(available, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
    {
      extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
      flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    VkApplicationInfo application{};
    application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application.pApplicationName = "Neon Engine";
    application.pEngineName = "Neon Engine";
    application.apiVersion = _instance_version;

    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.flags = flags;
    info.pApplicationInfo = &application;
    info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();

    if (const VkResult result = vkCreateInstance(&info, nullptr, &_instance); result != VK_SUCCESS)
    {
      const int code = result;
      _logger->Critical("Could not create the Vulkan instance, error {}", code);
      return false;
    }

    volkLoadInstance(_instance);
    return true;
  }

  bool VK_Device::PickPhysicalDevice()
  {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(_instance, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(_instance, &count, devices.data());

    int best_rating = -1;

    // what was found that offers too old a version, to say so when it is
    // all there is
    std::string too_old;

    for (const auto device : devices)
    {
      uint32_t family_count = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);
      std::vector<VkQueueFamilyProperties> families(family_count);
      vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families.data());

      // one queue that can draw and, when there is a window, show the result
      for (uint32_t family = 0; family < family_count; family++)
      {
        if (!(families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT)) { continue; }

        if (_surface != VK_NULL_HANDLE)
        {
          VkBool32 can_present = VK_FALSE;
          vkGetPhysicalDeviceSurfaceSupportKHR(device, family, _surface, &can_present);
          if (!can_present) { continue; }
        }

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        if (!VK_ApiVersion::IsEnough(properties.apiVersion))
        {
          too_old = std::string(properties.deviceName) + " offers Vulkan " +
                    VK_ApiVersion::ToCore(properties.apiVersion).ToString();
          break;
        }

        if (const int rating = rate_device_type(properties.deviceType); rating > best_rating)
        {
          best_rating = rating;
          _physical_device = device;
          _properties = properties;
          _queue_family = family;
        }
        break;
      }
    }

    if (_physical_device == VK_NULL_HANDLE && !too_old.empty())
    {
      const std::string minimum = VK_ApiVersion::ToCore(VK_ApiVersion::kMinimum).ToString();
      _logger->Critical("No graphics card offers Vulkan {}, which the engine needs at least. {}", minimum, too_old);
      return false;
    }

    if (_physical_device == VK_NULL_HANDLE)
    {
      _logger->Critical("No graphics card that supports Vulkan was found");
      return false;
    }

    _api_version = VK_ApiVersion::ForDevice(_instance_version, _properties.apiVersion);

    const std::string name(_properties.deviceName);
    const std::string chosen = VK_ApiVersion::ToCore(_api_version).ToString();
    const std::string asked = VK_ApiVersion::ToCore(_requested_version).ToString();
    const std::string offered = VK_ApiVersion::ToCore(_properties.apiVersion).ToString();
    _logger->Info("Rendering with {} in Vulkan {}. Vulkan {} was asked for, and the graphics card offers {}",
                  name, chosen, asked, offered);
    return true;
  }

  bool VK_Device::CreateDevice()
  {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(_physical_device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateDeviceExtensionProperties(_physical_device, nullptr, &count, available.data());

    std::vector<const char *> extensions;

    // a driver that offers this extension requires it to be enabled
    if (has_extension(available, "VK_KHR_portability_subset"))
    {
      extensions.push_back("VK_KHR_portability_subset");
    }

    if (_surface != VK_NULL_HANDLE)
    {
      if (!has_extension(available, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
      {
        _logger->Critical("The graphics card cannot present to a window");
        return false;
      }
      extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    constexpr float priority = 1.0f;
    VkDeviceQueueCreateInfo queue{};
    queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue.queueFamilyIndex = _queue_family;
    queue.queueCount = 1;
    queue.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures supported;
    vkGetPhysicalDeviceFeatures(_physical_device, &supported);

    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = supported.samplerAnisotropy;

    VkDeviceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = 1;
    info.pQueueCreateInfos = &queue;
    info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();
    info.pEnabledFeatures = &features;

    if (const VkResult result = vkCreateDevice(_physical_device, &info, nullptr, &_device); result != VK_SUCCESS)
    {
      const int code = result;
      _logger->Critical("Could not create the Vulkan device, error {}", code);
      return false;
    }

    volkLoadDevice(_device);
    vkGetDeviceQueue(_device, _queue_family, 0, &_queue);

    VkCommandPoolCreateInfo pool{};
    pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool.queueFamilyIndex = _queue_family;

    if (vkCreateCommandPool(_device, &pool, nullptr, &_command_pool) != VK_SUCCESS)
    {
      _logger->Critical("Could not create the Vulkan command pool");
      return false;
    }

    return true;
  }

  void VK_Device::CleanUp()
  {
    if (_device != VK_NULL_HANDLE)
    {
      vkDeviceWaitIdle(_device);
      ReleaseBatchStaging();
      if (_command_pool != VK_NULL_HANDLE) { vkDestroyCommandPool(_device, _command_pool, nullptr); }
      vkDestroyDevice(_device, nullptr);
    }

    if (_instance != VK_NULL_HANDLE)
    {
      if (_surface != VK_NULL_HANDLE) { vkDestroySurfaceKHR(_instance, _surface, nullptr); }
      vkDestroyInstance(_instance, nullptr);
    }

    _capabilities = {};
    _command_pool = VK_NULL_HANDLE;
    _device = VK_NULL_HANDLE;
    _surface = VK_NULL_HANDLE;
    _instance = VK_NULL_HANDLE;
    _physical_device = VK_NULL_HANDLE;
  }

  bool VK_Device::FindMemoryType(
    const uint32_t allowed_types,
    const VkMemoryPropertyFlags properties,
    uint32_t &type) const
  {
    VkPhysicalDeviceMemoryProperties memory;
    vkGetPhysicalDeviceMemoryProperties(_physical_device, &memory);

    for (uint32_t i = 0; i < memory.memoryTypeCount; i++)
    {
      if ((allowed_types & (1u << i)) && (memory.memoryTypes[i].propertyFlags & properties) == properties)
      {
        type = i;
        return true;
      }
    }
    return false;
  }

  bool VK_Device::CreateBuffer(
    const VkDeviceSize size,
    const VkBufferUsageFlags usage,
    const VkMemoryPropertyFlags properties,
    VkBuffer &buffer,
    VkDeviceMemory &memory) const
  {
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &info, nullptr, &buffer) != VK_SUCCESS)
    {
      _logger->Error("Could not create a Vulkan buffer");
      return false;
    }

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(_device, buffer, &requirements);

    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.allocationSize = requirements.size;

    if (!FindMemoryType(requirements.memoryTypeBits, properties, allocation.memoryTypeIndex) ||
        vkAllocateMemory(_device, &allocation, nullptr, &memory) != VK_SUCCESS)
    {
      _logger->Error("Could not allocate memory for a Vulkan buffer");
      vkDestroyBuffer(_device, buffer, nullptr);
      buffer = VK_NULL_HANDLE;
      return false;
    }

    vkBindBufferMemory(_device, buffer, memory, 0);
    return true;
  }

  bool VK_Device::CreateImage(
    const uint32_t width,
    const uint32_t height,
    const uint32_t mip_levels,
    const VkFormat format,
    const VkImageUsageFlags usage,
    VkImage &image,
    VkDeviceMemory &memory,
    const VkImageCreateFlags flags,
    const uint32_t layers) const
  {
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.flags = flags;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent = {width, height, 1};
    info.mipLevels = mip_levels;
    info.arrayLayers = layers;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(_device, &info, nullptr, &image) != VK_SUCCESS)
    {
      _logger->Error("Could not create a Vulkan image");
      return false;
    }

    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(_device, image, &requirements);

    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.allocationSize = requirements.size;

    if (!FindMemoryType(
          requirements.memoryTypeBits,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
          allocation.memoryTypeIndex) ||
        vkAllocateMemory(_device, &allocation, nullptr, &memory) != VK_SUCCESS)
    {
      _logger->Error("Could not allocate memory for a Vulkan image");
      vkDestroyImage(_device, image, nullptr);
      image = VK_NULL_HANDLE;
      return false;
    }

    vkBindImageMemory(_device, image, memory, 0);
    return true;
  }

  bool VK_Device::CreateImageView(
    const VkImage image,
    const VkFormat format,
    const VkImageAspectFlags aspect,
    const uint32_t mip_levels,
    VkImageView &view,
    const uint32_t first_layer,
    const uint32_t layers) const
  {
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image;
    info.viewType = layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
    info.subresourceRange = {aspect, 0, mip_levels, first_layer, layers};

    if (vkCreateImageView(_device, &info, nullptr, &view) != VK_SUCCESS)
    {
      _logger->Error("Could not create a Vulkan image view");
      return false;
    }
    return true;
  }

  VkCommandBuffer VK_Device::BeginCommands() const
  {
    // one run of commands for all of a batch
    if (_batch_commands != VK_NULL_HANDLE) { return _batch_commands; }

    VkCommandBufferAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocation.commandPool = _command_pool;
    allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocation.commandBufferCount = 1;

    VkCommandBuffer commands = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(_device, &allocation, &commands) != VK_SUCCESS) { return VK_NULL_HANDLE; }

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commands, &begin);

    return commands;
  }

  bool VK_Device::EndCommands(const VkCommandBuffer commands) const
  {
    if (commands == VK_NULL_HANDLE) { return false; }

    // run with the rest of the batch, by EndBatch()
    if (commands == _batch_commands) { return true; }

    vkEndCommandBuffer(commands);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commands;

    const bool submitted =
      vkQueueSubmit(_queue, 1, &submit, VK_NULL_HANDLE) == VK_SUCCESS &&
      vkQueueWaitIdle(_queue) == VK_SUCCESS;

    vkFreeCommandBuffers(_device, _command_pool, 1, &commands);
    return submitted;
  }

  void VK_Device::BeginBatch()
  {
    if (_batch_commands != VK_NULL_HANDLE) { return; }

    _batch_commands = BeginCommands();
  }

  bool VK_Device::EndBatch()
  {
    const VkCommandBuffer commands = _batch_commands;
    _batch_commands = VK_NULL_HANDLE;

    // as the commands of no batch are run, now that none is open
    const bool ran = EndCommands(commands);

    for (const auto &[buffer, memory] : _batch_buffers)
    {
      vkDestroyBuffer(_device, buffer, nullptr);
      vkFreeMemory(_device, memory, nullptr);
    }
    _batch_buffers.clear();

    // A batch that asked for more than the shared buffer holds finds it
    // large enough the next time, with some to spare. Now, when nothing
    // reads it.
    if (_batch_staging_wanted > _batch_staging_size)
    {
      ReleaseBatchStaging();

      const VkDeviceSize size = _batch_staging_wanted + _batch_staging_wanted / 4;
      if (CreateBuffer(
            size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            _batch_staging,
            _batch_staging_memory))
      {
        if (vkMapMemory(_device, _batch_staging_memory, 0, size, 0, &_batch_staging_mapped) == VK_SUCCESS)
        {
          _batch_staging_size = size;
        } else
        {
          ReleaseBatchStaging();
        }
      }
    }
    _batch_staging_used = 0;
    _batch_staging_wanted = 0;
    return ran;
  }

  bool VK_Device::TakeBatchStaging(
    const VkDeviceSize size,
    VkBuffer &buffer,
    VkDeviceSize &offset,
    void *&mapped) const
  {
    if (_batch_commands == VK_NULL_HANDLE) { return false; }

    // a part starts at a multiple of four bytes, as a copy to an image asks
    const VkDeviceSize taken = (size + 3) & ~static_cast<VkDeviceSize>(3);
    _batch_staging_wanted += taken;
    if (_batch_staging == VK_NULL_HANDLE || taken > _batch_staging_size - _batch_staging_used) { return false; }

    buffer = _batch_staging;
    offset = _batch_staging_used;
    mapped = static_cast<unsigned char *>(_batch_staging_mapped) + offset;
    _batch_staging_used += taken;
    return true;
  }

  void VK_Device::ReleaseBatchStaging() const
  {
    if (_batch_staging != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(_device, _batch_staging, nullptr);
      vkFreeMemory(_device, _batch_staging_memory, nullptr);
    }
    _batch_staging = VK_NULL_HANDLE;
    _batch_staging_memory = VK_NULL_HANDLE;
    _batch_staging_mapped = nullptr;
    _batch_staging_size = 0;
  }

  void VK_Device::ReleaseAfterCommands(const VkBuffer buffer, const VkDeviceMemory memory) const
  {
    if (_batch_commands != VK_NULL_HANDLE)
    {
      _batch_buffers.emplace_back(buffer, memory);
      return;
    }

    vkDestroyBuffer(_device, buffer, nullptr);
    vkFreeMemory(_device, memory, nullptr);
  }

  void VK_Device::TransitionImage(
    const VkCommandBuffer commands,
    const VkImage image,
    const VkImageAspectFlags aspect,
    const uint32_t first_mip_level,
    const uint32_t mip_levels,
    const VkImageLayout from,
    const VkImageLayout to,
    const uint32_t layers)
  {
    // what has to be finished before the change, and what waits for it
    const auto access_of = [](const VkImageLayout layout) -> VkAccessFlags
    {
      switch (layout)
      {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return VK_ACCESS_SHADER_READ_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        default: return 0;
      }
    };
    const auto stage_of = [](const VkImageLayout layout) -> VkPipelineStageFlags
    {
      switch (layout)
      {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        default: return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      }
    };

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = access_of(from);
    barrier.dstAccessMask = access_of(to);
    barrier.oldLayout = from;
    barrier.newLayout = to;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = {aspect, first_mip_level, mip_levels, 0, layers};

    vkCmdPipelineBarrier(commands, stage_of(from), stage_of(to), 0, 0, nullptr, 0, nullptr, 1, &barrier);
  }
} // neon
