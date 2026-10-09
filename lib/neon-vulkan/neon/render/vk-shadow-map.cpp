#include "vk-shadow-map.hpp"

#include <array>

namespace neon
{
  bool VK_ShadowMap::Initialize(VK_Device *device, const uint32_t size, const std::shared_ptr<Logger> &logger)
  {
    _device = device;
    _logger = logger;

    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(_device->PhysicalDevice(), kFormat, &properties);
    constexpr VkFormatFeatureFlags needed =
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    if ((properties.optimalTilingFeatures & needed) != needed)
    {
      _logger->Critical("The graphics card cannot draw and compare a shadow map of D32_SFLOAT");
      return false;
    }

    if (!CreateRenderPass())
    {
      _logger->Critical("Could not create the render pass of the shadow map");
      CleanUp();
      return false;
    }

    if (!CreateImage(size))
    {
      CleanUp();
      return false;
    }
    return true;
  }

  bool VK_ShadowMap::Resize(const uint32_t size)
  {
    if (_render_pass == VK_NULL_HANDLE) { return false; }
    if (size == _size && IsReady()) { return true; }

    DestroyImage();
    if (!CreateImage(size))
    {
      DestroyImage();
      return false;
    }
    return true;
  }

  bool VK_ShadowMap::CreateImage(const uint32_t size)
  {
    _size = size;

    constexpr auto layers = static_cast<uint32_t>(kMax_Shadow_Cascades);
    if (!_device->CreateImage(
          size, size, 1, kFormat,
          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
          _image, _memory, 0, layers) ||
        !_device->CreateImageView(_image, kFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, _view, 0, layers))
    {
      _logger->Critical("Could not create the shadow map of {} by {}", size, size);
      return false;
    }

    // a layer is drawn on its own, through a view of it alone
    for (uint32_t layer = 0; layer < layers; layer++)
    {
      if (!_device->CreateImageView(_image, kFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, _layer_views[layer], layer, 1))
      {
        _logger->Critical("Could not create the view of a layer of the shadow map");
        return false;
      }

      VkFramebufferCreateInfo framebuffer{};
      framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer.renderPass = _render_pass;
      framebuffer.attachmentCount = 1;
      framebuffer.pAttachments = &_layer_views[layer];
      framebuffer.width = size;
      framebuffer.height = size;
      framebuffer.layers = 1;

      if (vkCreateFramebuffer(_device->Device(), &framebuffer, nullptr, &_framebuffers[layer]) != VK_SUCCESS)
      {
        _logger->Critical("Could not create the framebuffer of the shadow map");
        return false;
      }
    }

    if (!ClearToLit())
    {
      _logger->Critical("Could not clear the shadow map");
      return false;
    }
    return true;
  }

  bool VK_ShadowMap::CreateRenderPass()
  {
    // Depth alone, cleared to the far plane and kept, since the shaders
    // of the scene read it afterwards.
    VkAttachmentDescription depth{};
    depth.format = kFormat;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = kRead_Layout;

    constexpr VkAttachmentReference reference{0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pDepthStencilAttachment = &reference;

    // The shaders that read the map in the frame before have to be done
    // before it is drawn again, and the pass has to be done before the
    // shaders of the scene read it: those of the frame, and those of the
    // render targets, whose commands run after this pass as well.
    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask =
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo pass{};
    pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass.attachmentCount = 1;
    pass.pAttachments = &depth;
    pass.subpassCount = 1;
    pass.pSubpasses = &subpass;
    pass.dependencyCount = static_cast<uint32_t>(dependencies.size());
    pass.pDependencies = dependencies.data();

    return vkCreateRenderPass(_device->Device(), &pass, nullptr, &_render_pass) == VK_SUCCESS;
  }

  bool VK_ShadowMap::ClearToLit() const
  {
    const VkCommandBuffer commands = _device->BeginCommands();
    if (commands == VK_NULL_HANDLE) { return false; }

    // the far depth everywhere, which every point of the scene is in
    // front of, and then the layout the shaders read it in
    constexpr auto layers = static_cast<uint32_t>(kMax_Shadow_Cascades);
    VK_Device::TransitionImage(
      commands, _image, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layers);

    constexpr VkClearDepthStencilValue far{1.0f, 0};
    constexpr VkImageSubresourceRange whole{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, layers};
    vkCmdClearDepthStencilImage(commands, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &far, 1, &whole);

    VK_Device::TransitionImage(
      commands, _image, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, kRead_Layout, layers);

    return _device->EndCommands(commands);
  }

  void VK_ShadowMap::DestroyImage()
  {
    if (_device == nullptr || _device->Device() == VK_NULL_HANDLE) { return; }

    const VkDevice device = _device->Device();

    for (auto &framebuffer : _framebuffers)
    {
      if (framebuffer != VK_NULL_HANDLE) { vkDestroyFramebuffer(device, framebuffer, nullptr); }
      framebuffer = VK_NULL_HANDLE;
    }
    for (auto &view : _layer_views)
    {
      if (view != VK_NULL_HANDLE) { vkDestroyImageView(device, view, nullptr); }
      view = VK_NULL_HANDLE;
    }
    if (_view != VK_NULL_HANDLE) { vkDestroyImageView(device, _view, nullptr); }
    if (_image != VK_NULL_HANDLE) { vkDestroyImage(device, _image, nullptr); }
    if (_memory != VK_NULL_HANDLE) { vkFreeMemory(device, _memory, nullptr); }

    _view = VK_NULL_HANDLE;
    _image = VK_NULL_HANDLE;
    _memory = VK_NULL_HANDLE;
    _size = 0;
  }

  void VK_ShadowMap::CleanUp()
  {
    if (_device == nullptr || _device->Device() == VK_NULL_HANDLE) { return; }

    DestroyImage();
    if (_render_pass != VK_NULL_HANDLE) { vkDestroyRenderPass(_device->Device(), _render_pass, nullptr); }
    _render_pass = VK_NULL_HANDLE;
  }

  void VK_ShadowMap::Begin(const VkCommandBuffer commands, const uint32_t layer) const
  {
    VkClearValue clear{};
    clear.depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo pass{};
    pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass.renderPass = _render_pass;
    pass.framebuffer = _framebuffers[layer];
    pass.renderArea = {{0, 0}, {_size, _size}};
    pass.clearValueCount = 1;
    pass.pClearValues = &clear;

    vkCmdBeginRenderPass(commands, &pass, VK_SUBPASS_CONTENTS_INLINE);

    // The map is drawn the way the scene is, with a viewport of negative
    // height that turns the picture the right way up, so that the fronts
    // of the triangles go round the way the pipelines of the scene expect.
    // The shaders that read the map turn its rows round again.
    const VkViewport viewport{
      0.0f,
      static_cast<float>(_size),
      static_cast<float>(_size),
      -static_cast<float>(_size),
      0.0f,
      1.0f};
    const VkRect2D scissor{{0, 0}, {_size, _size}};

    vkCmdSetViewport(commands, 0, 1, &viewport);
    vkCmdSetScissor(commands, 0, 1, &scissor);
  }

  void VK_ShadowMap::End(const VkCommandBuffer commands) const
  {
    vkCmdEndRenderPass(commands);
  }
} // neon
