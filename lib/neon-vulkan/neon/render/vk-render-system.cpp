#include "vk-render-system.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

// kept private to this file, so that another library can carry its own copy
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace neon
{
  namespace
  {
    // Plain colours, with no conversion on the way in or out. Textures are
    // read the same way, so a colour in an image file is the colour drawn.
    constexpr VkFormat color_format = VK_FORMAT_R8G8B8A8_UNORM;

    // The core builds its projection with glm's defaults, where depth runs
    // from -1 to 1. Vulkan expects 0 to 1. This moves one range onto the
    // other.
    const glm::mat4 depth_correction(
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.5f, 0.0f,
      0.0f, 0.0f, 0.5f, 1.0f);

    VkDeviceSize align_up(const VkDeviceSize size, const VkDeviceSize alignment)
    {
      return alignment == 0 ? size : (size + alignment - 1) / alignment * alignment;
    }

    void append_png_bytes(void *context, void *data, const int size)
    {
      auto *out = static_cast<std::vector<unsigned char> *>(context);
      const auto *bytes = static_cast<const unsigned char *>(data);
      out->insert(out->end(), bytes, bytes + size);
    }
  }

  void VK_RenderSystem::Initialize()
  {
    _logger->Info("Initializing Vulkan");

    if (!_device.Initialize(_window_context, _logger))
    {
      throw std::runtime_error("Failed to initialize Vulkan");
    }

    const auto [width, height] = _window_context->GetDrawableSize();
    _extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    if (_device.Surface() != VK_NULL_HANDLE && !CreateSwapchain())
    {
      throw std::runtime_error("Failed to create the Vulkan swapchain");
    }

    _logger->Info("Render resolution: {}x{}", _extent.width, _extent.height);
    _render_resolution.emplace(static_cast<int>(_extent.width), static_cast<int>(_extent.height));

    if (!CreateRenderTarget() || !CreateDescriptors())
    {
      throw std::runtime_error("Failed to set up the Vulkan renderer");
    }

    _renderer_2d.Initialize(&_device, _file_system_context, _render_pass, _extent, _logger);

    _white_texture = VK_Texture("a plain white texture", _file_system_context, &_device, _logger);
    if (!_white_texture.InitializeWithColor(255, 255, 255, 255))
    {
      throw std::runtime_error("Failed to create the fallback texture");
    }

    VkCommandBufferAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocation.commandPool = _device.CommandPool();
    allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocation.commandBufferCount = 1;

    VkFenceCreateInfo fence{};
    fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if (vkAllocateCommandBuffers(_device.Device(), &allocation, &_commands) != VK_SUCCESS ||
        vkCreateFence(_device.Device(), &fence, nullptr, &_frame_done) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to set up the Vulkan frame");
    }
  }

  bool VK_RenderSystem::CreateSwapchain()
  {
    const VkPhysicalDevice physical_device = _device.PhysicalDevice();
    const VkSurfaceKHR surface = _device.Surface();

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);

    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, formats.data());

    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, nullptr);
    std::vector<VkPresentModeKHR> modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, modes.data());

    if (formats.empty() || modes.empty())
    {
      _logger->Critical("The window offers no format to present in");
      return false;
    }

    if (!(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
    {
      _logger->Critical("The window does not accept copied images");
      return false;
    }

    // plain colours, to show exactly what was drawn
    VkSurfaceFormatKHR format = formats[0];
    for (const auto &candidate : formats)
    {
      if (candidate.format == VK_FORMAT_B8G8R8A8_UNORM || candidate.format == VK_FORMAT_R8G8B8A8_UNORM)
      {
        format = candidate;
        break;
      }
    }

    // Show frames as soon as they are done. Waiting for the screen is the
    // one mode every driver has, and the fallback.
    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
    for (const VkPresentModeKHR wanted : {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR})
    {
      if (std::ranges::find(modes, wanted) != modes.end())
      {
        mode = wanted;
        break;
      }
    }

    // the window usually dictates the size
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
      _extent = capabilities.currentExtent;
    } else
    {
      _extent.width = std::clamp(
        _extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
      _extent.height = std::clamp(
        _extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    _swapchain_extent = _extent;

    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) { image_count = std::min(image_count, capabilities.maxImageCount); }

    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface;
    info.minImageCount = image_count;
    info.imageFormat = format.format;
    info.imageColorSpace = format.colorSpace;
    info.imageExtent = _swapchain_extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = mode;
    info.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(_device.Device(), &info, nullptr, &_swapchain) != VK_SUCCESS)
    {
      _logger->Critical("Could not create the Vulkan swapchain");
      return false;
    }

    vkGetSwapchainImagesKHR(_device.Device(), _swapchain, &count, nullptr);
    _swapchain_images.resize(count);
    vkGetSwapchainImagesKHR(_device.Device(), _swapchain, &count, _swapchain_images.data());

    VkSemaphoreCreateInfo semaphore{};
    semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    return vkCreateSemaphore(_device.Device(), &semaphore, nullptr, &_image_available) == VK_SUCCESS &&
           vkCreateSemaphore(_device.Device(), &semaphore, nullptr, &_render_finished) == VK_SUCCESS;
  }

  bool VK_RenderSystem::CreateRenderTarget()
  {
    for (const VkFormat candidate :
         {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT})
    {
      VkFormatProperties properties;
      vkGetPhysicalDeviceFormatProperties(_device.PhysicalDevice(), candidate, &properties);
      if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
      {
        _depth_format = candidate;
        break;
      }
    }

    if (_depth_format == VK_FORMAT_UNDEFINED)
    {
      _logger->Critical("The graphics card offers no depth format");
      return false;
    }

    if (!_device.CreateImage(
          _extent.width, _extent.height, 1, color_format,
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
          _color_image, _color_memory) ||
        !_device.CreateImageView(_color_image, color_format, VK_IMAGE_ASPECT_COLOR_BIT, 1, _color_view) ||
        !_device.CreateImage(
          _extent.width, _extent.height, 1, _depth_format,
          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
          _depth_image, _depth_memory) ||
        !_device.CreateImageView(_depth_image, _depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1, _depth_view))
    {
      return false;
    }

    std::array<VkAttachmentDescription, 2> attachments{};
    attachments[0].format = color_format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // a finished frame is only ever copied from, to the window or to a file
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    attachments[1].format = _depth_format;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    constexpr VkAttachmentReference color_reference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    constexpr VkAttachmentReference depth_reference{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pDepthStencilAttachment = &depth_reference;

    // what came before has to be done before drawing starts, and drawing has
    // to be done before the frame is copied
    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dependencies[0].dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    dependencies[0].dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    VkRenderPassCreateInfo pass{};
    pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass.attachmentCount = static_cast<uint32_t>(attachments.size());
    pass.pAttachments = attachments.data();
    pass.subpassCount = 1;
    pass.pSubpasses = &subpass;
    pass.dependencyCount = static_cast<uint32_t>(dependencies.size());
    pass.pDependencies = dependencies.data();

    if (vkCreateRenderPass(_device.Device(), &pass, nullptr, &_render_pass) != VK_SUCCESS)
    {
      _logger->Critical("Could not create the Vulkan render pass");
      return false;
    }

    const std::array views{_color_view, _depth_view};

    VkFramebufferCreateInfo framebuffer{};
    framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer.renderPass = _render_pass;
    framebuffer.attachmentCount = static_cast<uint32_t>(views.size());
    framebuffer.pAttachments = views.data();
    framebuffer.width = _extent.width;
    framebuffer.height = _extent.height;
    framebuffer.layers = 1;

    if (vkCreateFramebuffer(_device.Device(), &framebuffer, nullptr, &_framebuffer) != VK_SUCCESS)
    {
      _logger->Critical("Could not create the Vulkan framebuffer");
      return false;
    }
    return true;
  }

  bool VK_RenderSystem::CreateFrameBuffer(
    FrameBuffer &buffer,
    const VkDeviceSize entry_size,
    const uint32_t capacity) const
  {
    // entries are picked by an offset, which has to respect the alignment
    // the graphics card asks for
    buffer.entry_size = align_up(entry_size, _device.Properties().limits.minUniformBufferOffsetAlignment);
    buffer.capacity = capacity;
    buffer.used = 0;

    const VkDeviceSize size = buffer.entry_size * capacity;

    if (!_device.CreateBuffer(
      size,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      buffer.buffer,
      buffer.memory))
    {
      return false;
    }

    void *mapped = nullptr;
    if (vkMapMemory(_device.Device(), buffer.memory, 0, size, 0, &mapped) != VK_SUCCESS) { return false; }

    buffer.mapped = static_cast<unsigned char *>(mapped);
    return true;
  }

  void VK_RenderSystem::DestroyFrameBuffer(FrameBuffer &buffer) const
  {
    if (buffer.buffer != VK_NULL_HANDLE) { vkDestroyBuffer(_device.Device(), buffer.buffer, nullptr); }
    if (buffer.memory != VK_NULL_HANDLE) { vkFreeMemory(_device.Device(), buffer.memory, nullptr); }
    buffer = {};
  }

  bool VK_RenderSystem::CreateDescriptors()
  {
    if (!CreateFrameBuffer(_scene_buffer, sizeof(VK_SceneData), kMax_Scenes_Per_Frame) ||
        !CreateFrameBuffer(_object_buffer, sizeof(VK_ObjectData), kMax_Render_Objects))
    {
      _logger->Critical("Could not create the buffers for shader data");
      return false;
    }

    // has to match the bindings in scene-data.glsl and the shaders
    constexpr VkShaderStageFlags both = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    constexpr std::array<VkDescriptorSetLayoutBinding, 4> bindings{{
      {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, both, nullptr},
      {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, both, nullptr},
      {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    }};

    VkDescriptorSetLayoutCreateInfo layout{};
    layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout.bindingCount = static_cast<uint32_t>(bindings.size());
    layout.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(_device.Device(), &layout, nullptr, &_descriptor_layout) != VK_SUCCESS)
    {
      _logger->Critical("Could not create the Vulkan descriptor layout");
      return false;
    }

    VkPipelineLayoutCreateInfo pipeline_layout{};
    pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout.setLayoutCount = 1;
    pipeline_layout.pSetLayouts = &_descriptor_layout;

    if (vkCreatePipelineLayout(_device.Device(), &pipeline_layout, nullptr, &_pipeline_layout) != VK_SUCCESS)
    {
      _logger->Critical("Could not create the Vulkan pipeline layout");
      return false;
    }

    // one set for each material
    constexpr std::array<VkDescriptorPoolSize, 2> sizes{{
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2 * kMax_Render_Objects},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * kMax_Render_Objects},
    }};

    VkDescriptorPoolCreateInfo pool{};
    pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool.maxSets = kMax_Render_Objects;
    pool.poolSizeCount = static_cast<uint32_t>(sizes.size());
    pool.pPoolSizes = sizes.data();

    if (vkCreateDescriptorPool(_device.Device(), &pool, nullptr, &_descriptor_pool) != VK_SUCCESS)
    {
      _logger->Critical("Could not create the Vulkan descriptor pool");
      return false;
    }
    return true;
  }

  bool VK_RenderSystem::GetPipeline(const std::string &shader_path, VkPipeline &pipeline)
  {
    // materials that name the same shader share one pipeline
    if (const auto existing = _pipelines.find(shader_path); existing != _pipelines.end())
    {
      pipeline = existing->second.pipeline;
      return true;
    }

    PipelineEntry entry;
    entry.shader = VK_Shader(shader_path, _file_system_context, &_device, _logger);
    if (!entry.shader.Initialize()) { return false; }

    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = entry.shader.Vertex();
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = entry.shader.Fragment();
    stages[1].pName = "main";

    constexpr VkVertexInputBindingDescription binding{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
    constexpr std::array<VkVertexInputAttributeDescription, 3> attributes{{
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
      {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, tex_coords)},
    }};

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertex_input.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // set for every frame in PrepareFrame()
    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    // both sides of a triangle are drawn
    VkPipelineRasterizationStateCreateInfo rasterization{};
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = VK_CULL_MODE_NONE;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = VK_TRUE;
    depth.depthWriteEnable = VK_TRUE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blend_attachment{};
    blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blend_attachment;

    constexpr std::array dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.stageCount = static_cast<uint32_t>(stages.size());
    info.pStages = stages.data();
    info.pVertexInputState = &vertex_input;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &rasterization;
    info.pMultisampleState = &multisample;
    info.pDepthStencilState = &depth;
    info.pColorBlendState = &blend;
    info.pDynamicState = &dynamic;
    info.layout = _pipeline_layout;
    info.renderPass = _render_pass;
    info.subpass = 0;

    if (vkCreateGraphicsPipelines(
      _device.Device(), VK_NULL_HANDLE, 1, &info, nullptr, &entry.pipeline) != VK_SUCCESS)
    {
      _logger->Error("Could not create the pipeline of shader {}", shader_path);
      entry.shader.CleanUp();
      return false;
    }

    pipeline = entry.pipeline;
    _pipelines.emplace(shader_path, entry);
    return true;
  }

  bool VK_RenderSystem::CreateDescriptorSet(VK_Material &material) const
  {
    VkDescriptorSetAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocation.descriptorPool = _descriptor_pool;
    allocation.descriptorSetCount = 1;
    allocation.pSetLayouts = &_descriptor_layout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(_device.Device(), &allocation, &set) != VK_SUCCESS)
    {
      _logger->Error("Could not allocate a descriptor set for a material");
      return false;
    }

    // The first texture is the diffuse one and the second the specular one.
    // A material with a single texture uses it for both. One with none gets
    // plain white.
    const auto &textures = material.Textures();
    const VK_Texture &diffuse = textures.empty() ? _white_texture : textures[0];
    const VK_Texture &specular = textures.size() > 1 ? textures[1] : diffuse;

    const VkDescriptorBufferInfo scene{_scene_buffer.buffer, 0, sizeof(VK_SceneData)};
    const VkDescriptorBufferInfo object{_object_buffer.buffer, 0, sizeof(VK_ObjectData)};
    const VkDescriptorImageInfo diffuse_image{
      diffuse.Sampler(), diffuse.View(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo specular_image{
      specular.Sampler(), specular.View(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    std::array<VkWriteDescriptorSet, 4> writes{};
    for (uint32_t i = 0; i < writes.size(); i++)
    {
      writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writes[i].dstSet = set;
      writes[i].dstBinding = i;
      writes[i].descriptorCount = 1;
    }
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writes[0].pBufferInfo = &scene;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writes[1].pBufferInfo = &object;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].pImageInfo = &diffuse_image;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[3].pImageInfo = &specular_image;

    vkUpdateDescriptorSets(_device.Device(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    material.SetDescriptorSet(set);
    return true;
  }

  void VK_RenderSystem::CleanUp()
  {
    _logger->Info("Cleaning up Vulkan");

    const VkDevice device = _device.Device();
    if (device == VK_NULL_HANDLE) { return; }

    vkDeviceWaitIdle(device);

    // whatever the scene did not destroy itself
    for (int id = 0; id < _model_refs.Capacity(); id++)
    {
      if (_model_refs.Contains(id)) { _model_refs.Remove(id).CleanUp(); }
    }
    for (int id = 0; id < _material_refs.Capacity(); id++)
    {
      if (_material_refs.Contains(id)) { _material_refs.Remove(id).CleanUp(); }
    }

    _white_texture.CleanUp();
    _renderer_2d.CleanUp();

    for (auto &[path, entry] : _pipelines)
    {
      vkDestroyPipeline(device, entry.pipeline, nullptr);
      entry.shader.CleanUp();
    }
    _pipelines.clear();

    if (_descriptor_pool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device, _descriptor_pool, nullptr); }
    if (_pipeline_layout != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device, _pipeline_layout, nullptr); }
    if (_descriptor_layout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device, _descriptor_layout, nullptr); }

    DestroyFrameBuffer(_scene_buffer);
    DestroyFrameBuffer(_object_buffer);

    if (_frame_done != VK_NULL_HANDLE) { vkDestroyFence(device, _frame_done, nullptr); }
    if (_image_available != VK_NULL_HANDLE) { vkDestroySemaphore(device, _image_available, nullptr); }
    if (_render_finished != VK_NULL_HANDLE) { vkDestroySemaphore(device, _render_finished, nullptr); }
    if (_swapchain != VK_NULL_HANDLE) { vkDestroySwapchainKHR(device, _swapchain, nullptr); }

    if (_framebuffer != VK_NULL_HANDLE) { vkDestroyFramebuffer(device, _framebuffer, nullptr); }
    if (_render_pass != VK_NULL_HANDLE) { vkDestroyRenderPass(device, _render_pass, nullptr); }
    if (_depth_view != VK_NULL_HANDLE) { vkDestroyImageView(device, _depth_view, nullptr); }
    if (_depth_image != VK_NULL_HANDLE) { vkDestroyImage(device, _depth_image, nullptr); }
    if (_depth_memory != VK_NULL_HANDLE) { vkFreeMemory(device, _depth_memory, nullptr); }
    if (_color_view != VK_NULL_HANDLE) { vkDestroyImageView(device, _color_view, nullptr); }
    if (_color_image != VK_NULL_HANDLE) { vkDestroyImage(device, _color_image, nullptr); }
    if (_color_memory != VK_NULL_HANDLE) { vkFreeMemory(device, _color_memory, nullptr); }

    _descriptor_pool = VK_NULL_HANDLE;
    _pipeline_layout = VK_NULL_HANDLE;
    _descriptor_layout = VK_NULL_HANDLE;
    _frame_done = VK_NULL_HANDLE;
    _image_available = VK_NULL_HANDLE;
    _render_finished = VK_NULL_HANDLE;
    _swapchain = VK_NULL_HANDLE;
    _framebuffer = VK_NULL_HANDLE;
    _render_pass = VK_NULL_HANDLE;
    _depth_view = VK_NULL_HANDLE;
    _depth_image = VK_NULL_HANDLE;
    _depth_memory = VK_NULL_HANDLE;
    _color_view = VK_NULL_HANDLE;
    _color_image = VK_NULL_HANDLE;
    _color_memory = VK_NULL_HANDLE;
    _commands = VK_NULL_HANDLE;

    _device.CleanUp();
  }

  void VK_RenderSystem::PrepareFrame()
  {
    _scene_buffer.used = 0;
    _object_buffer.used = 0;
    _has_last_scene = false;
    _renderer_2d.PrepareFrame();

    vkResetCommandBuffer(_commands, 0);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(_commands, &begin);

    std::array<VkClearValue, 2> clear{};
    clear[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clear[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo pass{};
    pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass.renderPass = _render_pass;
    pass.framebuffer = _framebuffer;
    pass.renderArea = {{0, 0}, _extent};
    pass.clearValueCount = static_cast<uint32_t>(clear.size());
    pass.pClearValues = clear.data();

    vkCmdBeginRenderPass(_commands, &pass, VK_SUBPASS_CONTENTS_INLINE);

    // Vulkan counts rows from the top, while the projection of the core
    // assumes they are counted from the bottom. A viewport of negative
    // height, starting at the bottom, turns the picture the right way up.
    const VkViewport viewport{
      0.0f,
      static_cast<float>(_extent.height),
      static_cast<float>(_extent.width),
      -static_cast<float>(_extent.height),
      0.0f,
      1.0f};
    const VkRect2D scissor{{0, 0}, _extent};

    vkCmdSetViewport(_commands, 0, 1, &viewport);
    vkCmdSetScissor(_commands, 0, 1, &scissor);

    _device.SetFrameCommands(_commands);
    _frame_open = true;
  }

  void VK_RenderSystem::CopyToWindow(const VkCommandBuffer commands, const uint32_t image_index) const
  {
    const VkImage target = _swapchain_images[image_index];
    constexpr VkImageAspectFlags color = VK_IMAGE_ASPECT_COLOR_BIT;

    VK_Device::TransitionImage(
      commands, target, color, 0, 1,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // a scaling copy, which also converts between the colour orders that the
    // renderer and the window may use
    VkImageBlit blit{};
    blit.srcSubresource = {color, 0, 0, 1};
    blit.srcOffsets[1] = {static_cast<int32_t>(_extent.width), static_cast<int32_t>(_extent.height), 1};
    blit.dstSubresource = {color, 0, 0, 1};
    blit.dstOffsets[1] = {
      static_cast<int32_t>(_swapchain_extent.width),
      static_cast<int32_t>(_swapchain_extent.height),
      1};

    vkCmdBlitImage(
      commands,
      _color_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1, &blit, VK_FILTER_NEAREST);

    VK_Device::TransitionImage(
      commands, target, color, 0, 1,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  }

  void VK_RenderSystem::FinishFrame()
  {
    if (!_frame_open) { return; }

    vkCmdEndRenderPass(_commands);

    uint32_t image_index = 0;
    bool present = false;

    if (_swapchain != VK_NULL_HANDLE)
    {
      const VkResult acquired = vkAcquireNextImageKHR(
        _device.Device(), _swapchain, UINT64_MAX, _image_available, VK_NULL_HANDLE, &image_index);

      present = acquired == VK_SUCCESS || acquired == VK_SUBOPTIMAL_KHR;
      if (present)
      {
        CopyToWindow(_commands, image_index);
      } else
      {
        const int code = acquired;
        _logger->Warn("Could not get an image of the window to draw to, error {}", code);
      }
    }

    vkEndCommandBuffer(_commands);

    constexpr VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &_commands;
    if (present)
    {
      submit.waitSemaphoreCount = 1;
      submit.pWaitSemaphores = &_image_available;
      submit.pWaitDstStageMask = &wait_stage;
      submit.signalSemaphoreCount = 1;
      submit.pSignalSemaphores = &_render_finished;
    }

    if (vkQueueSubmit(_device.Queue(), 1, &submit, _frame_done) != VK_SUCCESS)
    {
      _logger->Error("Could not submit the frame");
    } else
    {
      if (present)
      {
        VkPresentInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &_render_finished;
        info.swapchainCount = 1;
        info.pSwapchains = &_swapchain;
        info.pImageIndices = &image_index;
        vkQueuePresentKHR(_device.Queue(), &info);
      }

      // One frame at a time. The frame is done before the next one starts,
      // which keeps the buffers of shader data free to be written again.
      vkWaitForFences(_device.Device(), 1, &_frame_done, VK_TRUE, UINT64_MAX);
      vkResetFences(_device.Device(), 1, &_frame_done);
      _frame_finished = true;
    }

    _device.SetFrameCommands(VK_NULL_HANDLE);
    _frame_open = false;
  }

  bool VK_RenderSystem::CaptureFrame(const std::string &path)
  {
    if (!_frame_finished)
    {
      _logger->Error("No frame was rendered yet, {} was not written", path);
      return false;
    }

    const VkDevice device = _device.Device();
    const VkDeviceSize size = static_cast<VkDeviceSize>(_extent.width) * _extent.height * 4;

    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    if (!_device.CreateBuffer(
      size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      buffer,
      memory))
    {
      return false;
    }

    // a finished frame is left ready to be copied from
    const VkCommandBuffer commands = _device.BeginCommands();

    VkBufferImageCopy region{};
    region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {_extent.width, _extent.height, 1};
    vkCmdCopyImageToBuffer(commands, _color_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

    bool written = false;
    void *pixels = nullptr;

    if (_device.EndCommands(commands) && vkMapMemory(device, memory, 0, size, 0, &pixels) == VK_SUCCESS)
    {
      std::vector<unsigned char> png;
      const int encoded = stbi_write_png_to_func(
        append_png_bytes,
        &png,
        static_cast<int>(_extent.width),
        static_cast<int>(_extent.height),
        4,
        pixels,
        static_cast<int>(_extent.width) * 4);

      vkUnmapMemory(device, memory);

      written = encoded != 0 && _file_system_context->WriteBytes(path, png);
    }

    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);

    if (written)
    {
      _logger->Info("Saved the frame to {}", path);
    } else
    {
      _logger->Error("Could not save the frame to {}", path);
    }
    return written;
  }

  const RenderResolution &VK_RenderSystem::GetRenderResolution()
  {
    return *_render_resolution;
  }

  int VK_RenderSystem::CreateRenderObject(const RenderInfo &render_info)
  {
    VK_Model model(render_info.model_path, _file_system_context, &_device, _logger);
    VK_Material material(
      render_info.shader_path,
      render_info.texture_paths,
      render_info.material_info,
      render_info.scale_textures,
      _file_system_context,
      &_device,
      _logger);

    if (!model.Initialize())
    {
      _logger->Error("Could not initialize model {}", render_info.model_path);
      return -1;
    }

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (!material.Initialize() ||
        !GetPipeline(render_info.shader_path, pipeline) ||
        !CreateDescriptorSet(material))
    {
      _logger->Error("Could not initialize material with shader {}", render_info.shader_path);
      model.CleanUp();
      material.CleanUp();
      return -1;
    }
    material.SetPipeline(pipeline);

    const auto model_id = _model_refs.Add(model);
    const auto material_id = _material_refs.Add(material);

    if (model_id < 0 || material_id < 0)
    {
      _logger->Error("There is no room for another render object");
      if (model_id >= 0) { _model_refs.Remove(model_id); }
      if (material_id >= 0) { _material_refs.Remove(material_id); }
      model.CleanUp();
      material.CleanUp();
      return -1;
    }

    const auto render_id = _render_object_buffer.Add(RenderObjectRef{
      .model_id = model_id,
      .material_id = material_id
    });

    _logger->Debug(
      "Created render object {} from {} with model id {} and material id {}",
      render_id,
      render_info.model_path,
      model_id,
      material_id);

    return render_id;
  }

  VK_SceneData VK_RenderSystem::BuildSceneData(
    const glm::mat4 &view,
    const glm::mat4 &projection,
    const std::vector<LightSource> &lights)
  {
    VK_SceneData scene;
    scene.view = view;
    scene.projection = depth_correction * projection;
    // where the camera stands, which the view matrix holds in reverse
    scene.view_position = glm::inverse(view)[3];

    int point_lights = 0;
    int spot_lights = 0;
    bool dropped = false;

    for (const auto &light : lights)
    {
      switch (light.light_type)
      {
        case LightType::Direction:
        {
          scene.direction_light.direction = glm::vec4(light.direction, 0.0f);
          scene.direction_light.ambient = glm::vec4(light.ambient, 0.0f);
          scene.direction_light.diffuse = glm::vec4(light.diffuse, 0.0f);
          scene.direction_light.specular = glm::vec4(light.specular, 0.0f);
          break;
        }
        case LightType::Point:
        {
          if (point_lights >= kMax_Point_Lights)
          {
            dropped = true;
            break;
          }
          auto &[position, ambient, diffuse, specular, attenuation] = scene.point_lights[point_lights++];
          position = glm::vec4(light.position, 1.0f);
          ambient = glm::vec4(light.ambient, 0.0f);
          diffuse = glm::vec4(light.diffuse, 0.0f);
          specular = glm::vec4(light.specular, 0.0f);
          attenuation = {light.constant, light.linear, light.quadratic, 0.0f};
          break;
        }
        case LightType::SpotLight:
        {
          if (spot_lights >= kMax_Spot_Lights)
          {
            dropped = true;
            break;
          }
          auto &spot = scene.spot_lights[spot_lights++];
          spot.position = glm::vec4(light.position, 1.0f);
          spot.direction = glm::vec4(light.direction, 0.0f);
          spot.ambient = glm::vec4(light.ambient, 0.0f);
          spot.diffuse = glm::vec4(light.diffuse, 0.0f);
          spot.specular = glm::vec4(light.specular, 0.0f);
          spot.attenuation = {light.constant, light.linear, light.quadratic, 0.0f};
          spot.cutoff = {light.cutoff, light.outer_cutoff, 0.0f, 0.0f};
          break;
        }
      }
    }

    if (dropped && !_warned_about_lights)
    {
      _warned_about_lights = true;
      constexpr int point_limit = kMax_Point_Lights;
      constexpr int spot_limit = kMax_Spot_Lights;
      _logger->Warn(
        "The scene has more lights than the shaders hold, which is {} point lights and {} spot lights. "
        "The rest are left out",
        point_limit,
        spot_limit);
    }

    scene.light_counts = {point_lights, spot_lights, 0, 0};
    return scene;
  }

  void VK_RenderSystem::DrawRenderObject(
    const int render_object_id,
    const Transform &transform,
    const glm::mat4 &view,
    const glm::mat4 &projection,
    const std::vector<LightSource> &lights)
  {
    if (!_frame_open) { return; }

    const auto [model_id, material_id] = _render_object_buffer[render_object_id];
    const auto &model = _model_refs[model_id];
    const auto &material = _material_refs[material_id];

    // The camera and the lights are handed over with every object, but they
    // rarely change within a frame. They are only stored again when they do.
    const VK_SceneData scene = BuildSceneData(view, projection, lights);
    if (!_has_last_scene || std::memcmp(&scene, &_last_scene, sizeof(VK_SceneData)) != 0)
    {
      if (_scene_buffer.used < _scene_buffer.capacity)
      {
        _last_scene_offset = static_cast<uint32_t>(_scene_buffer.used * _scene_buffer.entry_size);
        std::memcpy(_scene_buffer.mapped + _last_scene_offset, &scene, sizeof(VK_SceneData));
        _scene_buffer.used++;
        _last_scene = scene;
        _has_last_scene = true;
      } else if (!_warned_about_capacity)
      {
        _warned_about_capacity = true;
        _logger->Warn("The camera or the lights changed too often within one frame, the last ones are kept");
      }
    }

    if (_object_buffer.used >= _object_buffer.capacity)
    {
      if (!_warned_about_capacity)
      {
        _warned_about_capacity = true;
        _logger->Warn("More objects were drawn in one frame than the renderer holds, the rest are left out");
      }
      return;
    }

    const glm::mat4 model_matrix = transform.world_coordinates * model.GetNormalizedModelMatrix();
    const VK_ObjectData object = material.GetObjectData(model_matrix, transform);

    const auto object_offset = static_cast<uint32_t>(_object_buffer.used * _object_buffer.entry_size);
    std::memcpy(_object_buffer.mapped + object_offset, &object, sizeof(VK_ObjectData));
    _object_buffer.used++;

    const std::array offsets{_last_scene_offset, object_offset};
    const VkDescriptorSet set = material.DescriptorSet();

    vkCmdBindPipeline(_commands, VK_PIPELINE_BIND_POINT_GRAPHICS, material.Pipeline());
    vkCmdBindDescriptorSets(
      _commands,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      _pipeline_layout,
      0,
      1,
      &set,
      static_cast<uint32_t>(offsets.size()),
      offsets.data());

    model.Use();
  }

  void VK_RenderSystem::DestroyRenderObject(const int render_object_id)
  {
    // nothing may still be drawing with what is about to be destroyed
    vkDeviceWaitIdle(_device.Device());

    const auto [model_id, material_id] = _render_object_buffer.Remove(render_object_id);
    auto model = _model_refs.Remove(model_id);
    auto material = _material_refs.Remove(material_id);

    if (const VkDescriptorSet set = material.DescriptorSet(); set != VK_NULL_HANDLE)
    {
      vkFreeDescriptorSets(_device.Device(), _descriptor_pool, 1, &set);
    }

    model.CleanUp();
    material.CleanUp();
  }

  int VK_RenderSystem::CreateTexture(const int width, const int height, const std::vector<unsigned char> &pixels)
  {
    return _renderer_2d.CreateTexture(width, height, pixels);
  }

  int VK_RenderSystem::LoadTexture(const std::string &path)
  {
    return _renderer_2d.LoadTexture(path);
  }

  bool VK_RenderSystem::GetTextureSize(const int texture, int &width, int &height)
  {
    return _renderer_2d.GetTextureSize(texture, width, height);
  }

  void VK_RenderSystem::DestroyTexture(const int texture)
  {
    _renderer_2d.DestroyTexture(texture);
  }

  void VK_RenderSystem::DrawTriangles(const Triangles2D &triangles)
  {
    if (!_frame_open) { return; }

    _renderer_2d.Draw(triangles);
  }
} // neon
