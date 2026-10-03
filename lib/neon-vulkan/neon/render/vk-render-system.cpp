#include "vk-render-system.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <format>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

#include "vk-culling.hpp"
#include "vk-shadow-casting.hpp"
#include "vk-shadow-fit.hpp"

namespace neon
{
  // Helpers of VK_RenderSystem, for this file alone.
  namespace
  {
    // The image that is shown holds sRGB colours as bytes: what the resolve
    // step writes, and what is drawn on top of it. It is copied as it is to
    // the window and to a file.
    constexpr VkFormat color_format = VK_FORMAT_R8G8B8A8_UNORM;

    // a frame has a scene image, and so may every render target
    constexpr uint32_t max_scene_images = 1 + 64;

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
  }

  void VK_RenderSystem::Initialize()
  {
    _logger->Info("Initializing Vulkan");

    if (!_device.Initialize(_window_context, _settings_config.vulkan_version, _logger))
    {
      throw std::runtime_error("Failed to initialize Vulkan");
    }

    const auto [width, height] = _window_context->GetDrawableSize();
    _extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    if (_device.Surface() != VK_NULL_HANDLE)
    {
      if (!_swapchain.Initialize(&_device, {width, height}, _logger))
      {
        throw std::runtime_error("Failed to create the Vulkan swapchain");
      }

      // the frame is drawn at the size of the window
      _extent = _swapchain.Extent();
    }

    _logger->Info("Render resolution: {}x{}", _extent.width, _extent.height);

    _models.Initialize(_file_system_context, &_device, _logger);
    _textures.Initialize(_file_system_context, &_device, _logger);
    _render_resolution.emplace(static_cast<int>(_extent.width), static_cast<int>(_extent.height));

    VkCommandBufferAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocation.commandPool = _device.CommandPool();
    allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocation.commandBufferCount = 1;

    VkFenceCreateInfo fence{};
    fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if (vkAllocateCommandBuffers(_device.Device(), &allocation, &_commands) != VK_SUCCESS ||
        vkAllocateCommandBuffers(_device.Device(), &allocation, &_target_commands) != VK_SUCCESS ||
        vkAllocateCommandBuffers(_device.Device(), &allocation, &_shadow_commands) != VK_SUCCESS ||
        vkCreateFence(_device.Device(), &fence, nullptr, &_frame_done) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to set up the Vulkan frame");
    }

    if (!_samplers.Initialize(&_device, _logger) ||
        !CreateRenderPasses() ||
        !_shadow_map.Initialize(&_device, _logger) ||
        !_resolve.Initialize(
          &_device,
          _file_system_context,
          _frame_pass,
          &_samplers,
          max_scene_images,
          _settings_config.tonemapper,
          static_cast<float>(_settings_config.exposure),
          _logger) ||
        !_sky.Initialize(&_device, _file_system_context, _scene_pass, &_samplers, _logger) ||
        !CreateDescriptors())
    {
      throw std::runtime_error("Failed to set up the Vulkan renderer");
    }

    _canvas_shared = {
      .device = &_device,
      .resolve = &_resolve,
      .sky = &_sky,
      .scene_pass = _scene_pass,
      .frame_pass = _frame_pass,
      .depth_format = _depth_format,
      .pipeline_layout = _pipelines.Layout(),
      .models = &_models,
      .draw_opaque = [this](VK_Canvas &canvas) { FlushDraws(canvas); },
      .logger = _logger,
    };
    _frame = VK_Canvas("the frame", &_canvas_shared);
    _capture = VK_Capture(&_device, _file_system_context, _logger);

    if (!CreateFrameImages()) { throw std::runtime_error("Failed to set up the Vulkan renderer"); }

    // drawn on top of the resolved scene, in the sRGB colours CSS blends in
    _renderer_2d.Initialize(&_device, _file_system_context, _frame_pass, &_samplers, _extent, _logger);

    _white_texture = VK_Texture("a plain white texture", _file_system_context, &_device, _logger);
    if (!_white_texture.InitializeWithColor(255, 255, 255, 255))
    {
      throw std::runtime_error("Failed to create the fallback texture");
    }
  }

  bool VK_RenderSystem::CreateRenderPasses()
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

    // The scene: linear light and depth. The light is read by the resolve
    // step afterwards, and the depth is needed no longer.
    std::array<VkAttachmentDescription, 2> attachments{};
    attachments[0].format = VK_SceneImage::kFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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

    // the resolve of the frame before has read the image before it is
    // drawn to again, and the scene is drawn before the resolve reads it
    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask =
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo pass{};
    pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass.attachmentCount = static_cast<uint32_t>(attachments.size());
    pass.pAttachments = attachments.data();
    pass.subpassCount = 1;
    pass.pSubpasses = &subpass;
    pass.dependencyCount = static_cast<uint32_t>(dependencies.size());
    pass.pDependencies = dependencies.data();

    if (vkCreateRenderPass(_device.Device(), &pass, nullptr, &_scene_pass) != VK_SUCCESS)
    {
      _logger->Critical("Could not create the render pass of the scene");
      return false;
    }

    // The image that is shown: the resolve step covers it, or it is cleared
    // where no scene was drawn. A finished one is only ever copied from: to
    // the window, to a file, or into the smaller copies of a render target.
    VkAttachmentDescription shown{};
    shown.format = color_format;
    shown.samples = VK_SAMPLE_COUNT_1_BIT;
    shown.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    shown.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    shown.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    shown.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    shown.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    shown.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    VkSubpassDescription shown_subpass{};
    shown_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    shown_subpass.colorAttachmentCount = 1;
    shown_subpass.pColorAttachments = &color_reference;

    // What came before has to be done before drawing starts: the copy of
    // the frame before, and what read a render target. Drawing has to be
    // done before the image is copied.
    std::array<VkSubpassDependency, 2> shown_dependencies{};
    shown_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    shown_dependencies[0].dstSubpass = 0;
    shown_dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    shown_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    shown_dependencies[0].srcAccessMask = 0;
    shown_dependencies[0].dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    shown_dependencies[1].srcSubpass = 0;
    shown_dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    shown_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    shown_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    shown_dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    shown_dependencies[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    pass.attachmentCount = 1;
    pass.pAttachments = &shown;
    pass.pSubpasses = &shown_subpass;
    pass.dependencyCount = static_cast<uint32_t>(shown_dependencies.size());
    pass.pDependencies = shown_dependencies.data();

    if (vkCreateRenderPass(_device.Device(), &pass, nullptr, &_frame_pass) != VK_SUCCESS)
    {
      _logger->Critical("Could not create the render pass of the frame");
      return false;
    }
    return true;
  }

  bool VK_RenderSystem::CreateFrameImages()
  {
    if (!_device.CreateImage(
          _extent.width, _extent.height, 1, color_format,
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
          _color_image, _color_memory) ||
        !_device.CreateImageView(_color_image, color_format, VK_IMAGE_ASPECT_COLOR_BIT, 1, _color_view))
    {
      return false;
    }

    VkFramebufferCreateInfo framebuffer{};
    framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer.renderPass = _frame_pass;
    framebuffer.attachmentCount = 1;
    framebuffer.pAttachments = &_color_view;
    framebuffer.width = _extent.width;
    framebuffer.height = _extent.height;
    framebuffer.layers = 1;

    if (vkCreateFramebuffer(_device.Device(), &framebuffer, nullptr, &_framebuffer) != VK_SUCCESS)
    {
      _logger->Critical("Could not create the Vulkan framebuffer");
      return false;
    }

    // the frame always lights a scene, and has its scene image from the
    // start
    _frame.SetImages(_commands, _framebuffer, _extent);
    return _frame.PrepareScene();
  }

  void VK_RenderSystem::DestroyFrameImages()
  {
    const VkDevice device = _device.Device();

    if (_framebuffer != VK_NULL_HANDLE) { vkDestroyFramebuffer(device, _framebuffer, nullptr); }
    _frame.CleanUp();
    if (_color_view != VK_NULL_HANDLE) { vkDestroyImageView(device, _color_view, nullptr); }
    if (_color_image != VK_NULL_HANDLE) { vkDestroyImage(device, _color_image, nullptr); }
    if (_color_memory != VK_NULL_HANDLE) { vkFreeMemory(device, _color_memory, nullptr); }

    _framebuffer = VK_NULL_HANDLE;
    _color_view = VK_NULL_HANDLE;
    _color_image = VK_NULL_HANDLE;
    _color_memory = VK_NULL_HANDLE;
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

    // the scene buffer is read by an offset, the object buffer whole
    if (!_device.CreateBuffer(
      size,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
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
        // every object once for the scene, and once more for the shadow
        // pass, which has its casters in an order of its own
        !CreateFrameBuffer(_object_buffer, sizeof(VK_ObjectData), 2 * static_cast<uint32_t>(_settings_config.max_render_objects)))
    {
      _logger->Critical("Could not create the buffers for shader data");
      return false;
    }

    if (!_pipelines.Initialize(&_device, _file_system_context, _scene_pass, _shadow_map.RenderPass(), _logger))
    {
      return false;
    }

    return CreateDescriptorPool();
  }

  bool VK_RenderSystem::CreateDescriptorPool()
  {
    // one set for each material, with its textures and their samplers,
    // and the shadow map with its sampler
    constexpr uint32_t textures = (VK_Pipelines::kTexture_Count + 1) * kSets_Per_Pool;
    constexpr std::array<VkDescriptorPoolSize, 4> sizes{{
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, kSets_Per_Pool},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kSets_Per_Pool},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, textures},
      {VK_DESCRIPTOR_TYPE_SAMPLER, textures},
    }};

    VkDescriptorPoolCreateInfo pool{};
    pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool.maxSets = kSets_Per_Pool;
    pool.poolSizeCount = static_cast<uint32_t>(sizes.size());
    pool.pPoolSizes = sizes.data();

    VkDescriptorPool made = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(_device.Device(), &pool, nullptr, &made) != VK_SUCCESS)
    {
      _logger->Error("Could not create a Vulkan descriptor pool");
      return false;
    }
    _descriptor_pools.push_back(made);
    const std::size_t pools = _descriptor_pools.size();
    const uint32_t sets = kSets_Per_Pool;
    _logger->Debug("Made descriptor pool {} for {} materials", pools, sets);
    return true;
  }

  bool VK_RenderSystem::CreateDescriptorSet(VK_Material &material)
  {
    const VkDescriptorSetLayout layout = _pipelines.DescriptorLayout();
    if (_descriptor_pools.empty() && !CreateDescriptorPool()) { return false; }

    VkDescriptorSetAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocation.descriptorPool = _descriptor_pools.back();
    allocation.descriptorSetCount = 1;
    allocation.pSetLayouts = &layout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    VkResult result = vkAllocateDescriptorSets(_device.Device(), &allocation, &set);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
      // the pool is full: another, and once more
      if (!CreateDescriptorPool()) { return false; }
      allocation.descriptorPool = _descriptor_pools.back();
      result = vkAllocateDescriptorSets(_device.Device(), &allocation, &set);
    }
    if (result != VK_SUCCESS)
    {
      _logger->Error("Could not allocate a descriptor set for a material");
      return false;
    }

    WriteDescriptorSet(material, set);

    material.SetDescriptorSet(set);
    material.SetDescriptorPool(_descriptor_pools.back());
    return true;
  }

  bool VK_RenderSystem::WriteDescriptorSet(const VK_Material &material, const VkDescriptorSet set) const
  {
    // The first texture is the diffuse one and the second the specular one.
    // A material with a single texture uses it for both. One with none gets
    // plain white. The third is what the surface gives off, plain white
    // when there is none, which the shaders then leave out.
    // A render target that is not there is drawn as plain white as well.
    // Each is read through the shared sampler of its way, bound three
    // bindings on from the texture.
    const auto &textures = material.Textures();
    const auto usable = [this](const VK_Texture &texture) -> const VK_Texture &
    {
      return texture.View() != VK_NULL_HANDLE ? texture : _white_texture;
    };

    const VK_Texture &diffuse = textures.empty() ? _white_texture : usable(textures[0]);
    const VK_Texture &specular = textures.size() > 1 ? usable(textures[1]) : diffuse;
    const VK_Texture &emissive = material.HasEmissiveTexture() ? usable(material.EmissiveTexture()) : _white_texture;

    const VkDescriptorBufferInfo scene{_scene_buffer.buffer, 0, sizeof(VK_SceneData)};
    const VkDescriptorBufferInfo object{_object_buffer.buffer, 0, VK_WHOLE_SIZE};
    const std::array<VkDescriptorImageInfo, VK_Pipelines::kTexture_Count> images{{
      {VK_NULL_HANDLE, diffuse.View(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
      {VK_NULL_HANDLE, specular.View(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
      {VK_NULL_HANDLE, emissive.View(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
    }};
    const std::array<VkDescriptorImageInfo, VK_Pipelines::kTexture_Count> samplers{{
      {_samplers.Of(diffuse.Sampling()), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED},
      {_samplers.Of(specular.Sampling()), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED},
      {_samplers.Of(emissive.Sampling()), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED},
    }};
    const VkDescriptorImageInfo shadow_map{VK_NULL_HANDLE, _shadow_map.View(), VK_ShadowMap::kRead_Layout};
    const VkDescriptorImageInfo shadow_sampler{
      _samplers.Of(VK_Sampling::ShadowCompare), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};

    std::array<VkWriteDescriptorSet, VK_Pipelines::kBindings.size()> writes{};
    for (std::size_t i = 0; i < writes.size(); i++)
    {
      writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writes[i].dstSet = set;
      writes[i].dstBinding = VK_Pipelines::kBindings[i].binding;
      writes[i].descriptorType = VK_Pipelines::kBindings[i].descriptorType;
      writes[i].descriptorCount = 1;
    }
    writes[VK_Pipelines::kScene_Binding].pBufferInfo = &scene;
    writes[VK_Pipelines::kObject_Binding].pBufferInfo = &object;
    for (std::size_t i = 0; i < VK_Pipelines::kTexture_Count; i++)
    {
      writes[VK_Pipelines::kFirst_Texture_Binding + i].pImageInfo = &images[i];
      writes[VK_Pipelines::kFirst_Sampler_Binding + i].pImageInfo = &samplers[i];
    }
    writes[VK_Pipelines::kShadow_Map_Binding].pImageInfo = &shadow_map;
    writes[VK_Pipelines::kShadow_Sampler_Binding].pImageInfo = &shadow_sampler;

    vkUpdateDescriptorSets(_device.Device(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    return true;
  }

  void VK_RenderSystem::CleanUp()
  {
    _logger->Info("Cleaning up Vulkan");

    const VkDevice device = _device.Device();
    if (device == VK_NULL_HANDLE) { return; }

    vkDeviceWaitIdle(device);

    // whatever the scene did not destroy itself: the materials first, since
    // they give their textures back to the cache
    _materials.RemoveAll([](VK_Material material) { material.CleanUp(); });
    _models.CleanUp();
    _textures.CleanUp();

    for (int id = 0; id < _targets.Capacity(); id++)
    {
      if (!_targets.Contains(id)) { continue; }

      Target removed = _targets.Remove(id);
      ReleaseTarget(removed);
    }
    for (auto &target : _targets_to_release) { ReleaseTarget(target); }
    _targets_to_release.clear();
    _refused_targets.clear();
    _current_target = No_Render_Target;
    _target_commands_open = false;
    _target_commands = VK_NULL_HANDLE;

    _white_texture.CleanUp();
    _renderer_2d.CleanUp();

    _pipelines.CleanUp();
    for (const VkDescriptorPool pool : _descriptor_pools) { vkDestroyDescriptorPool(device, pool, nullptr); }

    DestroyFrameBuffer(_scene_buffer);
    DestroyFrameBuffer(_object_buffer);

    if (_frame_done != VK_NULL_HANDLE) { vkDestroyFence(device, _frame_done, nullptr); }
    _swapchain.CleanUp();

    DestroyFrameImages();
    _resolve.CleanUp();
    _sky.CleanUp();
    _shadow_map.CleanUp();
    _samplers.CleanUp();
    if (_scene_pass != VK_NULL_HANDLE) { vkDestroyRenderPass(device, _scene_pass, nullptr); }
    if (_frame_pass != VK_NULL_HANDLE) { vkDestroyRenderPass(device, _frame_pass, nullptr); }

    _descriptor_pools.clear();
    _frame_done = VK_NULL_HANDLE;
    _scene_pass = VK_NULL_HANDLE;
    _frame_pass = VK_NULL_HANDLE;
    _commands = VK_NULL_HANDLE;
    _shadow_commands = VK_NULL_HANDLE;
    _shadow_open = false;

    _device.CleanUp();
  }

  bool VK_RenderSystem::FindSurface(const std::string &name, VK_Texture &texture) const
  {
    for (int id = 0; id < _targets.Capacity(); id++)
    {
      if (!_targets.Contains(id) || _targets[id].target.Name() != name) { continue; }

      const VK_RenderTarget &target = _targets[id].target;
      texture = VK_Texture::Borrowed(
        target.View(), VK_RenderTarget::kSampling, target.Extent().width, target.Extent().height);
      return true;
    }
    return false;
  }

  void VK_RenderSystem::SettleRenderTargets()
  {
    if (!_surfaces_changed && _targets_to_release.empty()) { return; }

    // the frame before is finished, and nothing refers to what changes
    vkDeviceWaitIdle(_device.Device());

    if (_surfaces_changed)
    {
      for (int id = 0; id < _materials.Capacity(); id++)
      {
        if (!_materials.Contains(id) || !_materials[id].ShowsSurfaces()) { continue; }

        VK_Material &material = _materials[id];
        material.ResolveSurfaces(VK_Texture());

        if (material.DescriptorSet() != VK_NULL_HANDLE) { WriteDescriptorSet(material, material.DescriptorSet()); }
      }
    }

    for (auto &target : _targets_to_release)
    {
      if (target.texture != No_Texture) { _renderer_2d.DestroyTexture(target.texture); }
      ReleaseTarget(target);
    }

    _targets_to_release.clear();
    _surfaces_changed = false;
  }

  void VK_RenderSystem::ReleaseTarget(Target &target) const
  {
    target.canvas.CleanUp();
    target.target.CleanUp();
  }

  VK_Canvas &VK_RenderSystem::CurrentCanvas()
  {
    return _current_target != No_Render_Target ? _targets[_current_target].canvas : _frame;
  }

  bool VK_RenderSystem::FitWindow()
  {
    if (!_swapchain.Fit(_window_context->GetDrawableSize())) { return false; }

    // The frame is drawn at the size of the window, so that the projection
    // of the camera and what is drawn in two dimensions follow it.
    const VkExtent2D window = _swapchain.Extent();
    if (_framebuffer == VK_NULL_HANDLE || window.width != _extent.width || window.height != _extent.height)
    {
      DestroyFrameImages();
      _extent = window;

      // the frame that was finished went with its image
      _frame_finished = false;

      if (!CreateFrameImages())
      {
        _logger->Critical("Could not resize the frame to {}x{}", _extent.width, _extent.height);
        DestroyFrameImages();
        _swapchain.MakeAgain();
        return false;
      }

      _logger->Info("Render resolution: {}x{}", _extent.width, _extent.height);
      _render_resolution.emplace(static_cast<int>(_extent.width), static_cast<int>(_extent.height));
      _renderer_2d.Resize(_extent);
    }
    return true;
  }

  void VK_RenderSystem::PrepareFrame()
  {
    SettleRenderTargets();

    // a window that changed its size, or has no area to draw to
    if (_device.Surface() != VK_NULL_HANDLE && !FitWindow()) { return; }

    // The frame before is finished, and nothing refers to the sky of a
    // scene that was left. A frame that is not drawn, as while the window
    // is minimized, does not get here, and frees nothing.
    _sky.ReleaseUnused();

    for (int id = 0; id < _targets.Capacity(); id++)
    {
      if (_targets.Contains(id)) { _targets[id].is_drawn = false; }
    }
    _target_commands_open = false;
    _current_target = No_Render_Target;

    _scene_buffer.used = 0;
    _object_buffer.used = 0;
    _has_last_scene = false;
    _renderer_2d.PrepareFrame();

    // the shadow map is fitted and drawn again by the first scene that
    // casts
    _shadow_open = false;
    _shadow_fitted = false;
    _shadow_target = No_Render_Target;

    _draws.Clear();
    _draws_canvas = nullptr;
    _frame_objects = 0;
    _frame_draws = 0;
    _frame_pipeline_binds = 0;
    _frame_set_binds = 0;

    vkResetCommandBuffer(_commands, 0);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(_commands, &begin);

    // a render pass is begun by what is drawn first, and the frame is
    // cleared to black
    _frame.Begin({0.0f, 0.0f, 0.0f, 1.0f});

    _device.SetFrameCommands(_commands);
    _frame_open = true;
  }

  void VK_RenderSystem::FinishFrame()
  {
    // the objects of a scene are created in its first frame, so this is
    // said once a scene
    ReportCreated();

    if (!_frame_open) { return; }

    // a target that was left open is closed, so that its commands can run
    if (_current_target != No_Render_Target) { EndRenderTarget(); }

    _frame.Leave();

    // what the frame cost in calls, every few seconds, for the bench
    if (++_frames_since_said >= 300 && _frame_objects > 0)
    {
      _frames_since_said = 0;
      _logger->Debug(
        "Drew {} objects in {} draws, with {} pipelines and {} descriptor sets bound",
        _frame_objects, _frame_draws, _frame_pipeline_binds, _frame_set_binds);
    }

    uint32_t image_index = 0;
    bool present = false;

    if (_swapchain.IsReady())
    {
      present = _swapchain.Acquire(image_index);
      if (present) { _swapchain.CopyFrom(_commands, _color_image, _extent, image_index); }
    }

    vkEndCommandBuffer(_commands);

    // The shadow map is drawn first, since every scene reads it. Then
    // what draws into render targets, and what shows them after.
    std::array<VkCommandBuffer, 3> buffers{};
    uint32_t buffer_count = 0;

    if (_shadow_open)
    {
      vkEndCommandBuffer(_shadow_commands);
      buffers[buffer_count++] = _shadow_commands;
      _shadow_open = false;
    }

    if (_target_commands_open)
    {
      vkEndCommandBuffer(_target_commands);
      buffers[buffer_count++] = _target_commands;
      _target_commands_open = false;
    }
    buffers[buffer_count++] = _commands;

    constexpr VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    const VkSemaphore image_available = _swapchain.ImageAvailable();
    const VkSemaphore render_finished = _swapchain.RenderFinished();

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = buffer_count;
    submit.pCommandBuffers = buffers.data();
    if (present)
    {
      submit.waitSemaphoreCount = 1;
      submit.pWaitSemaphores = &image_available;
      submit.pWaitDstStageMask = &wait_stage;
      submit.signalSemaphoreCount = 1;
      submit.pSignalSemaphores = &render_finished;
    }

    if (vkQueueSubmit(_device.Queue(), 1, &submit, _frame_done) != VK_SUCCESS)
    {
      _logger->Error("Could not submit the frame");
    } else
    {
      if (present) { _swapchain.Present(image_index); }

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

    return _capture.Write(_color_image, _extent, path);
  }

  const RenderCapabilities &VK_RenderSystem::GetCapabilities() const
  {
    return _device.Capabilities();
  }

  const RenderResolution &VK_RenderSystem::GetRenderResolution()
  {
    return *_render_resolution;
  }

  int VK_RenderSystem::CreateRenderObject(const RenderInfo &render_info)
  {
    if (render_info.mesh == nullptr && render_info.model_path.empty())
    {
      _logger->Error("A render object has neither a model nor a mesh that was built, so nothing is drawn");
      return -1;
    }

    // a mesh that was built is drawn as it is; a file is read and fitted
    // once, and shared by every object that draws it
    const int model_id = _models.Acquire(render_info);
    if (model_id < 0) { return -1; }
    const VK_Model &model = _models[model_id];

    // One material for each material of the file that a mesh uses, so that
    // every mesh is drawn with its own, see docs/models.md. A mesh that
    // was built, and a file without a mesh, get one that takes nothing
    // from a file. Each is shared with the render objects that draw with
    // the same one already.
    std::vector<int> model_materials = model.GetUsedMaterials();
    if (model_materials.empty()) { model_materials.push_back(-1); }

    RenderObjectRef object{.model_id = model_id};
    const auto give_back = [this, &object, model_id]
    {
      for (const int held : object.material_ids) { ReleaseObjectMaterial(held); }
      _models.Release(model_id);
    };

    for (const int model_material : model_materials)
    {
      const int material_id = AcquireObjectMaterial(render_info, model, model_material, object.material_ids.empty());
      if (material_id < 0)
      {
        give_back();
        return -1;
      }
      object.material_ids.push_back(material_id);
    }

    const auto render_id = _render_object_buffer.Add(object);
    if (render_id < 0)
    {
      const std::size_t most = _settings_config.max_render_objects;
      _logger->Error("There is no room for another render object: rendering.max_render_objects of the settings is {}", most);
      give_back();
      return -1;
    }

    // the logger takes what it is given by reference
    const std::string from = render_info.mesh != nullptr ? "a mesh that was built" : render_info.model_path;
    std::string material_ids;
    for (const int material_id : object.material_ids)
    {
      material_ids += (material_ids.empty() ? "" : ", ") + std::to_string(material_id);
    }
    _logger->Debug(
      "Created render object {} from {} with model id {} and material ids {}", render_id, from, model_id, material_ids);

    _objects_created++;
    return render_id;
  }

  int VK_RenderSystem::AcquireObjectMaterial(
    const RenderInfo &render_info,
    const VK_Model &model,
    const int material,
    const bool first)
  {
    const auto &file_materials = model.GetMaterials();
    const ModelMaterial *model_material =
      material >= 0 && static_cast<std::size_t>(material) < file_materials.size()
        ? &file_materials[static_cast<std::size_t>(material)]
        : nullptr;

    // What the model file says about its look fills in what the scene does
    // not: its colour factor multiplies the colour of the material, its
    // textures are shown when the scene names none, and its doubleSided
    // holds unless the scene says always or never. The textures the scene
    // names replace those of the first material of the model alone.
    MaterialInfo material_info = render_info.material_info;
    if (material_info.double_sided == DoubleSided::Model)
    {
      const bool from_file = model_material != nullptr && model_material->double_sided;
      material_info.double_sided = from_file ? DoubleSided::Always : DoubleSided::Never;
    }
    if (model_material != nullptr)
    {
      const Color &factor = model_material->color;
      material_info.color = {
        material_info.color.r * factor.r,
        material_info.color.g * factor.g,
        material_info.color.b * factor.b,
        material_info.color.a * factor.a};
    }

    // What the surface gives off, see docs/scenes.md. The scene's emissive
    // colour replaces the file's factor, and black, the default, leaves it
    // to the file; a texture the scene names without a colour glows as the
    // texture is. The strengths multiply, so that 0 turns a file's glow off.
    const Color &glow = material_info.emissive;
    const bool scene_names_colour = glow.r > 0.0f || glow.g > 0.0f || glow.b > 0.0f;
    const bool scene_names_texture = !material_info.emissive_texture.empty();
    if (!scene_names_colour)
    {
      if (scene_names_texture)
      {
        material_info.emissive = Color{1.0f, 1.0f, 1.0f, 1.0f};
      } else if (model_material != nullptr)
      {
        material_info.emissive = model_material->emissive;
      }
    }
    if (model_material != nullptr) { material_info.emissive_strength *= model_material->emissive_strength; }

    // What the material is made from, which is what tells two apart: the
    // scene's part, without its textures for a material that is not the
    // first, and which material of the file it is.
    RenderInfo made_from = render_info;
    if (!first) { made_from.texture_paths.clear(); }
    const std::string key = VK_MaterialCache::KeyOf(made_from, material_info) + "|material " + std::to_string(material);

    // the same material as a render object that has one already, shared
    if (const int shared = _materials.Find(key); shared >= 0)
    {
      const int holders = _materials.CountOf(shared);
      _logger->Debug("Material {} is shared, {} render objects draw with it now", shared, holders);
      return shared;
    }

    VK_Material made(
      render_info.shader_path,
      made_from.texture_paths,
      material_info,
      render_info.scale_textures,
      _file_system_context,
      &_device,
      _logger);
    made.SetTextureCache(&_textures);
    if (model_material != nullptr)
    {
      made.SetModelTextures(model_material->textures, render_info.model_path, model_material->emissive_texture);
    }

    made.SetSurfaceLookup([this](const std::string &name, VK_Texture &texture)
    {
      return FindSurface(name, texture);
    });

    // The pipeline for a mirrored object, which turns its triangles round,
    // is left until one is drawn: most materials never have one.
    VkPipeline pipeline = VK_NULL_HANDLE;
    if (!made.Initialize() ||
        !_pipelines.Get(
          render_info.shader_path, material_info.alpha_mode, made.IsDoubleSided(), false, pipeline) ||
        !CreateDescriptorSet(made))
    {
      _logger->Error("Could not initialize material with shader {}", render_info.shader_path);
      made.CleanUp();
      return -1;
    }
    made.SetPipeline(pipeline);

    const int material_id = _materials.Keep(key, made);
    if (material_id < 0)
    {
      const std::size_t most = _settings_config.max_render_objects;
      _logger->Error("There is no room for another material: rendering.max_render_objects of the settings is {}", most);
      if (const VkDescriptorSet set = made.DescriptorSet(); set != VK_NULL_HANDLE)
      {
        vkFreeDescriptorSets(_device.Device(), made.DescriptorPool(), 1, &set);
      }
      made.CleanUp();
    }
    return material_id;
  }

  void VK_RenderSystem::ReleaseObjectMaterial(const int material_id)
  {
    // the material goes when the last render object that drew with it goes
    VK_Material material;
    if (!_materials.Release(material_id, material)) { return; }

    if (const VkDescriptorSet set = material.DescriptorSet(); set != VK_NULL_HANDLE)
    {
      vkFreeDescriptorSets(_device.Device(), material.DescriptorPool(), 1, &set);
    }
    material.CleanUp();
    _logger->Debug("Material {} was freed, nothing draws with it any more", material_id);
  }

  void VK_RenderSystem::ReportCreated()
  {
    if (_objects_created == 0) { return; }

    const auto model_loads = _models.Loads() - _model_loads_reported;
    const auto model_shares = _models.Shares() - _model_shares_reported;
    const auto material_makes = _materials.Makes() - _material_makes_reported;
    const auto material_shares = _materials.Shares() - _material_shares_reported;

    // Textures are not counted: a texture is only ever read through a
    // material, so a shared material says it all, and a material that is
    // made names the textures it loads or shares at the level of debugging.
    _logger->Info(
      "Created {} render objects: {} models were loaded and {} shared with objects that had them already; "
      "{} materials were made and {} shared",
      _objects_created, model_loads, model_shares, material_makes, material_shares);
    _material_makes_reported = _materials.Makes();
    _material_shares_reported = _materials.Shares();

    _objects_created = 0;
    _model_loads_reported = _models.Loads();
    _model_shares_reported = _models.Shares();
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

    // The shadow map is fitted around the camera of the frame, the one the
    // window shows. A camera that draws into a render target is drawn
    // before it and picks a cascade by its own depth, so it cannot read a
    // map fitted to another camera: what it draws is left unshadowed, and
    // the map is kept for the frame (#350)
    const auto fit_shadow = [this, &scene, &view, &projection](const LightSource &light)
    {
      if (!_shadow_fitted)
      {
        // the cascades of this camera: slices of what it sees, as far as
        // the setting says, as many as it says
        _shadow_cascades = VK_ShadowFit::Cascades(
          view, projection, light.direction, static_cast<float>(_settings_config.shadow_distance),
          static_cast<int>(_settings_config.shadow_cascades), static_cast<float>(VK_ShadowMap::kSize));
        for (auto &cascade : _shadow_cascades.view_projections) { cascade = depth_correction * cascade; }
        _shadow_fitted = true;
      }
      for (int i = 0; i < kMax_Shadow_Cascades; i++)
      {
        scene.direction_light.cascades[i] = _shadow_cascades.view_projections[static_cast<std::size_t>(i)];
        scene.direction_light.splits[i] = _shadow_cascades.splits[static_cast<std::size_t>(i)];
      }
      scene.direction_light.shadow = {
        1.0f, 1.0f / static_cast<float>(VK_ShadowMap::kSize), VK_ShadowSettings::kBias,
        static_cast<float>(_shadow_cascades.count)};
    };

    int point_lights = 0;
    int spot_lights = 0;
    bool dropped = false;

    // The parts of a light are amounts of light that the shaders add up,
    // and are handed over as they are. A colour of a material is written
    // as a screen shows it and is turned into light, but 0.5 here means
    // half the light, as it says.
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

          // a light with no direction lights nothing, and shadows nothing
          const bool is_frame = _current_target == No_Render_Target;
          if (is_frame && light.casts_shadows && glm::dot(light.direction, light.direction) > 0.0f)
          {
            fit_shadow(light);
          }
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

  bool VK_RenderSystem::SameScene(
    const glm::mat4 &view,
    const glm::mat4 &projection,
    const std::vector<LightSource> &lights) const
  {
    if (!_has_last_scene || view != _last_view || projection != _last_projection || lights.size() != _last_lights.size())
    {
      return false;
    }
    for (std::size_t i = 0; i < lights.size(); i++)
    {
      const LightSource &a = lights[i];
      const LightSource &b = _last_lights[i];
      if (a.light_type != b.light_type || a.position != b.position || a.direction != b.direction ||
          a.ambient != b.ambient || a.diffuse != b.diffuse || a.specular != b.specular ||
          a.constant != b.constant || a.linear != b.linear || a.quadratic != b.quadratic ||
          a.cutoff != b.cutoff || a.outer_cutoff != b.outer_cutoff || a.casts_shadows != b.casts_shadows)
      {
        return false;
      }
    }
    return true;
  }

  void VK_RenderSystem::DrawRenderObject(
    const int render_object_id,
    const Transform &transform,
    const glm::mat4 &view,
    const glm::mat4 &projection,
    const std::vector<LightSource> &lights)
  {
    if (!_frame_open) { return; }

    const RenderObjectRef &object = _render_object_buffer[render_object_id];
    const int model_id = object.model_id;
    const auto &model = _models[model_id];

    // What is drawn into a render target cannot show that target, since
    // an image is not read while it is written. It is left out there.
    if (_current_target != No_Render_Target)
    {
      const std::string &target = _targets[_current_target].target.Name();
      for (const int material_id : object.material_ids)
      {
        if (_materials[material_id].Shows(target)) { return; }
      }
    }

    VK_Canvas &canvas = CurrentCanvas();
    if (!canvas.EnterScene()) { return; }

    // what was kept for another canvas is drawn there first
    if (_draws_canvas != nullptr && _draws_canvas != &canvas && !_draws.Empty()) { FlushDraws(*_draws_canvas); }
    _draws_canvas = &canvas;

    // The camera and the lights are handed over with every object, but they
    // rarely change within a frame. The scene data is only built and stored
    // again when they do.
    if (!SameScene(view, projection, lights))
    {
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
      _last_view = view;
      _last_projection = projection;
      _last_lights = lights;
    }

    // twice each, since a draw that casts is written once more for the pass
    if (_object_buffer.used + 2 * (_draws.Size() + object.material_ids.size()) > _object_buffer.capacity)
    {
      if (!_warned_about_capacity)
      {
        _warned_about_capacity = true;
        _logger->Warn(
          "More objects were drawn in one frame than rendering.max_render_objects allows, the rest are left out");
      }
      return;
    }

    const glm::mat4 model_matrix = transform.world_coordinates * model.GetNormalizedModelMatrix();

    // A mirrored object turns its triangles round, and is drawn with the
    // opposite front. The pipeline for that is made when the first mirrored
    // object of a variant is drawn, which pays for it; the pipeline is
    // shared by every material of the variant from then on.
    const bool mirrored = VK_Culling::IsMirrored(model_matrix);
    const float distance = VK_DrawOrder::DistanceOf(view, glm::vec3(model_matrix[3]));

    // one draw for each material of the object: the meshes of the model
    // that use the material of the file it was made from
    const auto &model_materials = model.GetUsedMaterials();
    const std::size_t count = object.material_ids.size();

    // what each material is drawn with, and what it casts with
    std::vector<VkPipeline> pipelines(count, VK_NULL_HANDLE);
    std::vector<VK_ShadowCasting::Caster> casters(count);
    for (std::size_t i = 0; i < count; i++)
    {
      auto &material = _materials[object.material_ids[i]];
      casters[i].model_material = i < model_materials.size() ? model_materials[i] : -1;

      pipelines[i] = material.Pipeline(mirrored);
      if (pipelines[i] == VK_NULL_HANDLE)
      {
        if (!_pipelines.Get(
              material.ShaderPath(), material.GetAlphaMode(), material.IsDoubleSided(), true, pipelines[i]))
        {
          pipelines[i] = VK_NULL_HANDLE;
          continue;
        }
        material.SetMirroredPipeline(pipelines[i]);
      }

      // what is see-through casts no shadow for now
      const bool see_through = material.GetAlphaMode() == AlphaMode::Blend;
      casters[i].casts = _last_scene.direction_light.shadow.x > 0.5f && !see_through;
      if (casters[i].casts)
      {
        // the pipeline of the pass is made when the first object that casts
        // is drawn with the material, culled as the material is
        casters[i].pipeline = material.ShadowPipeline(mirrored);
        if (casters[i].pipeline == VK_NULL_HANDLE &&
            _pipelines.GetShadow(material.IsDoubleSided(), mirrored, casters[i].pipeline))
        {
          material.SetShadowPipeline(mirrored, casters[i].pipeline);
        }
      }
    }

    // The pass that draws the shadow map writes depth alone: when every
    // material of the object casts alike, one of its draws casts the whole
    // model and the others nothing, see VK_ShadowCasting.
    const std::vector<VK_ShadowCasting::Cast> casts = VK_ShadowCasting::Plan(casters);

    for (std::size_t i = 0; i < count; i++)
    {
      if (pipelines[i] == VK_NULL_HANDLE) { continue; }

      const int material_id = object.material_ids[i];
      const auto &material = _materials[material_id];

      // kept until the scene ends, see FlushDraws(). What is see-through is
      // drawn when the scene is finished, over what is opaque, from the
      // farthest to the nearest.
      _draws.Add({
        .pipeline = pipelines[i],
        .shadow_pipeline = casters[i].pipeline,
        .set = material.DescriptorSet(),
        .scene_offset = _last_scene_offset,
        .model_id = model_id,
        .material_id = material_id,
        .model_material = casters[i].model_material,
        .distance = distance,
        .see_through = material.GetAlphaMode() == AlphaMode::Blend,
        .casts_shadow = casts[i].casts,
        .shadow_material = casts[i].model_material,
        .data = material.GetObjectData(model_matrix, transform)
      });
    }
    _frame_objects++;
  }

  void VK_RenderSystem::FlushDraws(VK_Canvas &canvas)
  {
    if (_draws.Empty() || &canvas != _draws_canvas) { return; }

    _draws.Settle();
    const auto &draws = _draws.Draws();
    const auto &batches = _draws.Batches();

    // the objects' data, in the order the draws ended up in, from where the
    // buffer of the frame was filled to; a batch names its first object
    // by its index in the buffer, which the shaders read it by
    const uint32_t base = _object_buffer.used;
    for (std::size_t i = 0; i < draws.size(); i++)
    {
      std::memcpy(_object_buffer.mapped + (base + i) * _object_buffer.entry_size, &draws[i].data, sizeof(VK_ObjectData));
    }
    _object_buffer.used += static_cast<uint32_t>(draws.size());

    // and the casters once more, in the order of the shadow pass, so that
    // a batch of the pass has its objects side by side
    const auto &shadow_order = _draws.ShadowOrder();
    const uint32_t shadow_base = _object_buffer.used;
    for (std::size_t i = 0; i < shadow_order.size(); i++)
    {
      std::memcpy(
        _object_buffer.mapped + (shadow_base + i) * _object_buffer.entry_size, &draws[shadow_order[i]].data,
        sizeof(VK_ObjectData));
    }
    _object_buffer.used += static_cast<uint32_t>(shadow_order.size());

    const VkCommandBuffer commands = canvas.Commands();
    VkPipeline bound_pipeline = VK_NULL_HANDLE;
    VkDescriptorSet bound_set = VK_NULL_HANDLE;
    uint32_t bound_offset = 0;

    for (const VK_DrawBatch &batch : batches)
    {
      // a model that was destroyed since is left out
      if (!_models.Contains(batch.model_id)) { continue; }

      if (batch.pipeline != bound_pipeline)
      {
        vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, batch.pipeline);
        bound_pipeline = batch.pipeline;
        _frame_pipeline_binds++;
      }
      if (batch.set != bound_set || batch.scene_offset != bound_offset)
      {
        vkCmdBindDescriptorSets(
          commands, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.Layout(), 0, 1, &batch.set, 1, &batch.scene_offset);
        bound_set = batch.set;
        bound_offset = batch.scene_offset;
        _frame_set_binds++;
      }

      const VK_Model &model = _models[batch.model_id];
      model.Draw(commands, batch.instances, base + batch.first_instance, batch.model_material);
      _frame_draws += model.MeshCount(batch.model_material);
    }

    // The shadow map holds what the scene of the frame that casts draws, as
    // its light sees it, and nothing of a render target, see
    // BuildSceneData(): the same batches, with the pipelines of the pass,
    // once into every cascade, which a push constant names for the shader.
    const auto &shadow_batches = _draws.ShadowBatches();
    const bool is_frame = _current_target == No_Render_Target;
    const bool is_shadow_scene = !_shadow_open || _shadow_target == _current_target;
    if (!shadow_batches.empty() && is_frame && is_shadow_scene && (_shadow_open || BeginShadowPass()))
    {
      for (int cascade = 0; cascade < _shadow_cascades.count; cascade++)
      {
        const auto layer = static_cast<uint32_t>(cascade);
        _shadow_map.Begin(_shadow_commands, layer);
        vkCmdPushConstants(_shadow_commands, _pipelines.Layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(layer), &layer);

        // The pass reads the scene and the objects alone, which every set
        // binds the same, so one set serves until the camera or the lights
        // change; and it draws the batches of its own, where the material
        // an object has in the scene parts nothing.
        bound_pipeline = VK_NULL_HANDLE;
        bool set_bound = false;
        for (const VK_ShadowBatch &batch : shadow_batches)
        {
          if (!_models.Contains(batch.model_id)) { continue; }

          if (batch.pipeline != bound_pipeline)
          {
            vkCmdBindPipeline(_shadow_commands, VK_PIPELINE_BIND_POINT_GRAPHICS, batch.pipeline);
            bound_pipeline = batch.pipeline;
            _frame_pipeline_binds++;
          }
          if (!set_bound || batch.scene_offset != bound_offset)
          {
            vkCmdBindDescriptorSets(
              _shadow_commands, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.Layout(), 0, 1, &batch.set, 1,
              &batch.scene_offset);
            set_bound = true;
            bound_offset = batch.scene_offset;
            _frame_set_binds++;
          }

          const VK_Model &model = _models[batch.model_id];
          model.Draw(_shadow_commands, batch.instances, shadow_base + batch.first_instance, batch.model_material);
          _frame_draws += model.MeshCount(batch.model_material);
        }
        _shadow_map.End(_shadow_commands);
      }
    }

    // what is see-through is drawn when the scene is finished, over what
    // is opaque, from the farthest to the nearest
    for (std::size_t i = _draws.SeeThroughStart(); i < draws.size(); i++)
    {
      const VK_Draw &draw = draws[i];
      canvas.KeepSeeThrough({
        .pipeline = draw.pipeline,
        .set = draw.set,
        .scene_offset = draw.scene_offset,
        .object_index = base + static_cast<uint32_t>(i),
        .model_id = draw.model_id,
        .material = draw.model_material,
        .distance = draw.distance
      });
    }

    _draws.Clear();
    _draws_canvas = nullptr;
  }

  bool VK_RenderSystem::BeginShadowPass()
  {
    vkResetCommandBuffer(_shadow_commands, 0);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(_shadow_commands, &begin) != VK_SUCCESS)
    {
      _logger->Error("Could not record the shadow pass");
      return false;
    }

    _shadow_open = true;
    _shadow_target = _current_target;
    return true;
  }

  void VK_RenderSystem::DrawSky(const SkyInfo &sky, const glm::mat4 &view, const glm::mat4 &projection)
  {
    if (!_frame_open) { return; }

    // its images are loaded the first time it is drawn; a sky that cannot
    // be drawn has said why, and the canvas shows what it is cleared to
    VK_SkyDraw draw;
    if (!_sky.Prepare(sky, view, depth_correction * projection, draw)) { return; }

    // a scene may have a sky and no model, so the sky begins it as well
    VK_Canvas &canvas = CurrentCanvas();
    if (!canvas.EnterScene()) { return; }

    // kept until the opaque models of the canvas are drawn, see VK_Canvas
    canvas.KeepSky(draw);
  }

  void VK_RenderSystem::DestroyRenderObject(const int render_object_id)
  {
    // nothing may still be drawing with what is about to be destroyed
    vkDeviceWaitIdle(_device.Device());

    const RenderObjectRef object = _render_object_buffer.Remove(render_object_id);
    for (const int material_id : object.material_ids) { ReleaseObjectMaterial(material_id); }
    _models.Release(object.model_id);
  }

  void VK_RenderSystem::UpdateRenderObjectMesh(const int render_object_id, const MeshData &mesh)
  {
    if (!_render_object_buffer.Contains(render_object_id)) { return; }

    // a frame is done before the next one starts, see FinishFrame(), so
    // nothing draws with the buffers that are written here
    _models.UpdateMesh(_render_object_buffer[render_object_id].model_id, mesh);
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

    CurrentCanvas().EnterOverlay();
    _renderer_2d.Draw(triangles);
  }

  bool VK_RenderSystem::UpdateTexture(
    const int texture,
    const int x,
    const int y,
    const int width,
    const int height,
    const std::vector<unsigned char> &pixels)
  {
    return _renderer_2d.UpdateTexture(texture, x, y, width, height, pixels);
  }

  int VK_RenderSystem::CreateTextureWith(
    const int width,
    const int height,
    const std::vector<unsigned char> &pixels,
    const TextureOptions2D &options)
  {
    return _renderer_2d.CreateTextureWith(width, height, pixels, options);
  }

  int VK_RenderSystem::CreateMaterial(const std::string &shader_path)
  {
    return _renderer_2d.CreateMaterial(shader_path);
  }

  void VK_RenderSystem::DestroyMaterial(const int material)
  {
    _renderer_2d.DestroyMaterial(material);
  }

  int VK_RenderSystem::FindRenderTarget(const std::string &name)
  {
    for (int id = 0; id < _targets.Capacity(); id++)
    {
      if (_targets.Contains(id) && _targets[id].target.Name() == name) { return id; }
    }
    return No_Render_Target;
  }

  int VK_RenderSystem::CreateRenderTarget(const std::string &name, const int width, const int height)
  {
    // what is wrong is said once for a name, and not in every frame
    const auto refuse = [this, &name](const std::string &reason)
    {
      if (std::ranges::find(_refused_targets, name) == _refused_targets.end())
      {
        _refused_targets.push_back(name);
        _logger->Error("The render target '{}' cannot be created: {}", name, reason);
      }
      return No_Render_Target;
    };

    constexpr int limit = VK_RenderTarget::kMax_Size;

    if (name.empty()) { return refuse("it has no name"); }
    if (width <= 0 || height <= 0 || width > limit || height > limit)
    {
      return refuse(std::format(
        "its size is {} by {}, where each side is from 1 to {} pixels", width, height, limit));
    }
    if (FindRenderTarget(name) != No_Render_Target) { return refuse("there is one of that name"); }
    if (_device.Device() == VK_NULL_HANDLE) { return refuse("the renderer is not initialized"); }

    Target kept;
    kept.target = VK_RenderTarget(name, &_device, _logger);

    if (!kept.target.Initialize(
      static_cast<uint32_t>(width), static_cast<uint32_t>(height), _frame_pass, color_format))
    {
      return refuse("its images could not be made");
    }

    kept.canvas = VK_Canvas(std::format("the render target '{}'", name), &_canvas_shared);
    kept.canvas.SetImages(_target_commands, kept.target.Framebuffer(), kept.target.Extent());

    const int id = _targets.Add(kept);
    if (id < 0)
    {
      kept.target.CleanUp();
      return refuse("there is no room for another render target");
    }

    _logger->Info("Created the render target '{}' of {} by {}", name, width, height);

    // models that were waiting for it show it from the next frame on
    _surfaces_changed = true;
    return id;
  }

  void VK_RenderSystem::DestroyRenderTarget(const int target)
  {
    if (!_targets.Contains(target)) { return; }
    if (_current_target == target) { EndRenderTarget(); }

    const std::string name = _targets[target].target.Name();
    _logger->Info("Destroying the render target '{}'", name);

    // The frame that is being drawn may show it. It is released when that
    // frame is finished, and models that show it are told then.
    _targets_to_release.push_back(_targets.Remove(target));
    _surfaces_changed = true;

    std::erase(_refused_targets, name);
  }

  bool VK_RenderSystem::BeginRenderTarget(const int target, const Color &clear)
  {
    if (!_frame_open || !_targets.Contains(target) || _current_target != No_Render_Target) { return false; }
    if (_targets[target].is_drawn) { return false; }

    if (!_target_commands_open)
    {
      vkResetCommandBuffer(_target_commands, 0);

      VkCommandBufferBeginInfo begin{};
      begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

      if (vkBeginCommandBuffer(_target_commands, &begin) != VK_SUCCESS) { return false; }
      _target_commands_open = true;
    }

    // what the frame's canvas kept is drawn there before the target's own
    if (_draws_canvas != nullptr && !_draws.Empty()) { FlushDraws(*_draws_canvas); }

    Target &kept = _targets[target];
    const VkExtent2D extent = kept.target.Extent();

    kept.canvas.Begin(clear);
    kept.is_drawn = true;
    _current_target = target;

    // the camera and the lights are stored again for what is drawn next
    _has_last_scene = false;

    _device.SetFrameCommands(_target_commands);
    _renderer_2d.SetTarget(_target_commands, extent);
    return true;
  }

  void VK_RenderSystem::EndRenderTarget()
  {
    if (_current_target == No_Render_Target) { return; }

    Target &kept = _targets[_current_target];
    kept.canvas.Leave();
    kept.target.Finish(_target_commands);

    _current_target = No_Render_Target;
    _has_last_scene = false;

    _device.SetFrameCommands(_frame_open ? _commands : VK_NULL_HANDLE);
    _renderer_2d.SetTarget(VK_NULL_HANDLE, _extent);
  }

  int VK_RenderSystem::GetRenderTargetTexture(const int target)
  {
    if (!_targets.Contains(target)) { return No_Texture; }

    Target &kept = _targets[target];

    if (kept.texture == No_Texture)
    {
      kept.texture = _renderer_2d.KeepBorrowed(
        kept.target.BytesView(), VK_RenderTarget::kSampling, kept.target.Extent().width, kept.target.Extent().height);
    }

    return kept.texture;
  }

  bool VK_RenderSystem::GetRenderTargetSize(const int target, int &width, int &height)
  {
    if (!_targets.Contains(target)) { return false; }

    width = static_cast<int>(_targets[target].target.Extent().width);
    height = static_cast<int>(_targets[target].target.Extent().height);
    return true;
  }
} // neon
