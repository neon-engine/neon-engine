#include "vk-render-system.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <format>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

#include "vk-culling.hpp"
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
        !_resolve.Initialize(&_device, _file_system_context, _frame_pass, &_samplers, max_scene_images, _logger) ||
        !CreateDescriptors())
    {
      throw std::runtime_error("Failed to set up the Vulkan renderer");
    }

    _canvas_shared = {
      .device = &_device,
      .resolve = &_resolve,
      .scene_pass = _scene_pass,
      .frame_pass = _frame_pass,
      .depth_format = _depth_format,
      .pipeline_layout = _pipelines.Layout(),
      .models = &_models,
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
        !CreateFrameBuffer(_object_buffer, sizeof(VK_ObjectData), static_cast<uint32_t>(_settings_config.max_render_objects)))
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
    constexpr std::array<VkDescriptorPoolSize, 3> sizes{{
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2 * kSets_Per_Pool},
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
    // plain white.
    // A render target that is not there is drawn as plain white as well.
    // Each is read through the shared sampler of its way, bound two
    // bindings on from the texture.
    const auto &textures = material.Textures();
    const auto usable = [this](const VK_Texture &texture) -> const VK_Texture &
    {
      return texture.View() != VK_NULL_HANDLE ? texture : _white_texture;
    };

    const VK_Texture &diffuse = textures.empty() ? _white_texture : usable(textures[0]);
    const VK_Texture &specular = textures.size() > 1 ? usable(textures[1]) : diffuse;

    const VkDescriptorBufferInfo scene{_scene_buffer.buffer, 0, sizeof(VK_SceneData)};
    const VkDescriptorBufferInfo object{_object_buffer.buffer, 0, sizeof(VK_ObjectData)};
    const VkDescriptorImageInfo diffuse_image{
      VK_NULL_HANDLE, diffuse.View(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo specular_image{
      VK_NULL_HANDLE, specular.View(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo diffuse_sampler{
      _samplers.Of(diffuse.Sampling()), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    const VkDescriptorImageInfo specular_sampler{
      _samplers.Of(specular.Sampling()), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
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
    writes[0].pBufferInfo = &scene;
    writes[1].pBufferInfo = &object;
    writes[2].pImageInfo = &diffuse_image;
    writes[3].pImageInfo = &specular_image;
    writes[4].pImageInfo = &diffuse_sampler;
    writes[5].pImageInfo = &specular_sampler;
    writes[6].pImageInfo = &shadow_map;
    writes[7].pImageInfo = &shadow_sampler;

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
      _shadow_map.End(_shadow_commands);
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

    // What the model file says about its look fills in what the scene does
    // not: its colour factor multiplies the colour of the material, its
    // textures are shown when the scene names none, and its doubleSided
    // holds unless the scene says always or never. A model with several
    // materials is drawn with its first, see docs/models.md.
    MaterialInfo material_info = render_info.material_info;
    const ModelMaterial *model_material = model.GetDrawnMaterial();
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

    // the same material as a render object that has one already, shared
    const std::string key = VK_MaterialCache::KeyOf(render_info, material_info);
    if (const int shared = _materials.Find(key); shared >= 0)
    {
      const auto shared_id = _render_object_buffer.Add(RenderObjectRef{.model_id = model_id, .material_id = shared});
      if (shared_id < 0)
      {
        const std::size_t most = _settings_config.max_render_objects;
        _logger->Error("There is no room for another render object: rendering.max_render_objects of the settings is {}", most);
        VK_Material freed;
        if (_materials.Release(shared, freed)) { freed.CleanUp(); }
        _models.Release(model_id);
        return -1;
      }
      const int holders = _materials.CountOf(shared);
      _logger->Debug("Material {} is shared, {} render objects draw with it now", shared, holders);
      _logger->Debug(
        "Created render object {} from {} with model id {} and material id {}",
        shared_id,
        render_info.mesh != nullptr ? "a mesh that was built" : render_info.model_path,
        model_id,
        shared);
      _objects_created++;
      return shared_id;
    }

    VK_Material material(
      render_info.shader_path,
      render_info.texture_paths,
      material_info,
      render_info.scale_textures,
      _file_system_context,
      &_device,
      _logger);
    material.SetTextureCache(&_textures);
    if (model_material != nullptr) { material.SetModelTextures(model_material->textures, render_info.model_path); }

    material.SetSurfaceLookup([this](const std::string &name, VK_Texture &texture)
    {
      return FindSurface(name, texture);
    });

    // The pipeline for a mirrored object, which turns its triangles round,
    // is left until one is drawn: most materials never have one.
    VkPipeline pipeline = VK_NULL_HANDLE;
    if (!material.Initialize() ||
        !_pipelines.Get(
          render_info.shader_path, material_info.alpha_mode, material.IsDoubleSided(), false, pipeline) ||
        !CreateDescriptorSet(material))
    {
      _logger->Error("Could not initialize material with shader {}", render_info.shader_path);
      material.CleanUp();
      _models.Release(model_id);
      return -1;
    }
    material.SetPipeline(pipeline);

    const auto material_id = _materials.Keep(key, material);
    if (material_id < 0)
    {
      const std::size_t most = _settings_config.max_render_objects;
        _logger->Error("There is no room for another render object: rendering.max_render_objects of the settings is {}", most);
      material.CleanUp();
      _models.Release(model_id);
      return -1;
    }

    const auto render_id = _render_object_buffer.Add(RenderObjectRef{
      .model_id = model_id,
      .material_id = material_id
    });

    _logger->Debug(
      "Created render object {} from {} with model id {} and material id {}",
      render_id,
      render_info.mesh != nullptr ? "a mesh that was built" : render_info.model_path,
      model_id,
      material_id);

    _objects_created++;
    return render_id;
  }

  void VK_RenderSystem::ReportCreated()
  {
    if (_objects_created == 0) { return; }

    const auto model_loads = _models.Loads() - _model_loads_reported;
    const auto model_shares = _models.Shares() - _model_shares_reported;
    const auto texture_loads = _textures.Loads() - _texture_loads_reported;
    const auto texture_shares = _textures.Shares() - _texture_shares_reported;

    const auto material_makes = _materials.Makes() - _material_makes_reported;
    const auto material_shares = _materials.Shares() - _material_shares_reported;

    _logger->Info(
      "Created {} render objects: {} models and {} textures were loaded, and {} models and {} textures were "
      "shared with objects that had them already; {} materials were made and {} shared",
      _objects_created, model_loads, texture_loads, model_shares, texture_shares, material_makes, material_shares);
    _material_makes_reported = _materials.Makes();
    _material_shares_reported = _materials.Shares();

    _objects_created = 0;
    _model_loads_reported = _models.Loads();
    _model_shares_reported = _models.Shares();
    _texture_loads_reported = _textures.Loads();
    _texture_shares_reported = _textures.Shares();
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

    // the shadow map is fitted around the first camera of the frame that
    // draws with a light that casts, and every later camera reads it
    const auto fit_shadow = [this, &scene](const LightSource &light)
    {
      if (!_shadow_fitted)
      {
        _shadow_view_projection = depth_correction * VK_ShadowFit::ViewProjection(
          light.direction, glm::vec3(scene.view_position), static_cast<float>(VK_ShadowMap::kSize));
        _shadow_fitted = true;
      }
      scene.direction_light.light_view_projection = _shadow_view_projection;
      scene.direction_light.shadow = {
        1.0f, 1.0f / static_cast<float>(VK_ShadowMap::kSize), VK_ShadowSettings::kBias, 0.0f};
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
          if (light.casts_shadows && glm::dot(light.direction, light.direction) > 0.0f) { fit_shadow(light); }
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
    const auto &model = _models[model_id];
    auto &material = _materials[material_id];

    // What is drawn into a render target cannot show that target, since
    // an image is not read while it is written. It is left out there.
    if (_current_target != No_Render_Target && material.Shows(_targets[_current_target].target.Name()))
    {
      return;
    }

    VK_Canvas &canvas = CurrentCanvas();
    const VkCommandBuffer commands = canvas.Commands();
    if (!canvas.EnterScene()) { return; }

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

    // A mirrored object turns its triangles round, and is drawn with the
    // opposite front. The pipeline for that is made when the first mirrored
    // object of a variant is drawn, which pays for it; the pipeline is
    // shared by every material of the variant from then on.
    const bool mirrored = VK_Culling::IsMirrored(model_matrix);
    VkPipeline pipeline = material.Pipeline(mirrored);
    if (pipeline == VK_NULL_HANDLE)
    {
      if (!_pipelines.Get(material.ShaderPath(), material.GetAlphaMode(), material.IsDoubleSided(), true, pipeline))
      {
        return;
      }
      material.SetMirroredPipeline(pipeline);
    }

    const auto object_offset = static_cast<uint32_t>(_object_buffer.used * _object_buffer.entry_size);
    std::memcpy(_object_buffer.mapped + object_offset, &object, sizeof(VK_ObjectData));
    _object_buffer.used++;

    const std::array offsets{_last_scene_offset, object_offset};
    const VkDescriptorSet set = material.DescriptorSet();

    // The shadow map holds what the first scene that casts draws, as its
    // light sees it. What is see-through casts no shadow for now.
    if (scene.direction_light.shadow.x > 0.5f && (!_shadow_open || _shadow_target == _current_target))
    {
      if ((_shadow_open || BeginShadowPass()) && material.GetAlphaMode() != AlphaMode::Blend)
      {
        DrawShadow(model, material, mirrored, offsets);
      }
    }

    // what is see-through is drawn when the scene is finished, over what
    // is opaque, from the farthest to the nearest
    if (material.GetAlphaMode() == AlphaMode::Blend)
    {
      canvas.KeepSeeThrough({
        .pipeline = pipeline,
        .set = set,
        .scene_offset = _last_scene_offset,
        .object_offset = object_offset,
        .model_id = model_id,
        .distance = VK_DrawOrder::DistanceOf(view, glm::vec3(model_matrix[3]))
      });
      return;
    }

    vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(
      commands,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      _pipelines.Layout(),
      0,
      1,
      &set,
      static_cast<uint32_t>(offsets.size()),
      offsets.data());

    model.Use();
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

    _shadow_map.Begin(_shadow_commands);
    _shadow_open = true;
    _shadow_target = _current_target;
    return true;
  }

  void VK_RenderSystem::DrawShadow(
    const VK_Model &model,
    VK_Material &material,
    const bool mirrored,
    const std::array<uint32_t, 2> &offsets)
  {
    // the pipeline of the pass is made when the first object that casts
    // is drawn with the material, culled as the material is
    VkPipeline pipeline = material.ShadowPipeline(mirrored);
    if (pipeline == VK_NULL_HANDLE)
    {
      if (!_pipelines.GetShadow(material.IsDoubleSided(), mirrored, pipeline)) { return; }
      material.SetShadowPipeline(mirrored, pipeline);
    }

    const VkDescriptorSet set = material.DescriptorSet();

    vkCmdBindPipeline(_shadow_commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(
      _shadow_commands,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      _pipelines.Layout(),
      0,
      1,
      &set,
      static_cast<uint32_t>(offsets.size()),
      offsets.data());

    model.Draw(_shadow_commands);
  }

  void VK_RenderSystem::DestroyRenderObject(const int render_object_id)
  {
    // nothing may still be drawing with what is about to be destroyed
    vkDeviceWaitIdle(_device.Device());

    const auto [model_id, material_id] = _render_object_buffer.Remove(render_object_id);

    // the material goes when the last render object that drew with it goes
    VK_Material material;
    if (_materials.Release(material_id, material))
    {
      if (const VkDescriptorSet set = material.DescriptorSet(); set != VK_NULL_HANDLE)
      {
        vkFreeDescriptorSets(_device.Device(), material.DescriptorPool(), 1, &set);
      }
      material.CleanUp();
      _logger->Debug("Material {} was freed, nothing draws with it any more", material_id);
    }
    _models.Release(model_id);
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
