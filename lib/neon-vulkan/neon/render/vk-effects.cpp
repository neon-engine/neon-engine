#include "vk-effects.hpp"

#include <cstring>
#include <vector>

namespace neon
{
  bool VK_Effects::LoadModule(const std::string &path, VkShaderModule &module) const
  {
    std::vector<unsigned char> bytes;
    if (!_file_system_context->ReadBytes(path, bytes))
    {
      _logger->Error("Could not read shader {}, was it compiled by the build?", path);
      return false;
    }

    if (bytes.empty() || bytes.size() % sizeof(uint32_t) != 0)
    {
      _logger->Error("Shader {} is not valid SPIR-V", path);
      return false;
    }

    // the code is handed over as 32 bit words, which need their alignment
    std::vector<uint32_t> words(bytes.size() / sizeof(uint32_t));
    std::memcpy(words.data(), bytes.data(), bytes.size());

    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = bytes.size();
    info.pCode = words.data();

    if (vkCreateShaderModule(_device->Device(), &info, nullptr, &module) != VK_SUCCESS)
    {
      _logger->Error("Could not create a module from shader {}", path);
      return false;
    }
    return true;
  }

  bool VK_Effects::CreatePass(const VkFormat format, VkRenderPass &pass) const
  {
    // One image, every pixel of which the effect writes, and which the
    // next effect, or the resolve step, reads.
    VkAttachmentDescription attachment{};
    attachment.format = format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    constexpr VkAttachmentReference reference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &reference;

    // what read the image before has to be done before it is drawn to
    // again, and it is drawn before what comes next reads it
    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = static_cast<uint32_t>(dependencies.size());
    info.pDependencies = dependencies.data();

    return vkCreateRenderPass(_device->Device(), &info, nullptr, &pass) == VK_SUCCESS;
  }

  bool VK_Effects::Initialize(
    VK_Device *device,
    FileSystemContext *file_system_context,
    const VK_Samplers *samplers,
    const VkFormat scene_format,
    const VkFormat screen_format,
    const uint32_t max_images,
    const std::shared_ptr<Logger> &logger)
  {
    _device = device;
    _file_system_context = file_system_context;
    _samplers = samplers;
    _logger = logger;

    const VkDevice vk_device = _device->Device();

    if (!LoadModule(kVertex_Path, _vertex)) { return false; }

    if (!CreatePass(scene_format, _passes[static_cast<std::size_t>(VK_EffectKind::Light)]) ||
        !CreatePass(screen_format, _passes[static_cast<std::size_t>(VK_EffectKind::Screen)]))
    {
      _logger->Critical("Could not create the render passes of the effects of the cameras");
      return false;
    }

    // what the effects are told of the game, written anew in every frame
    if (!_device->CreateBuffer(
          sizeof(Data),
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          _data_buffer,
          _data_memory) ||
        vkMapMemory(vk_device, _data_memory, 0, sizeof(Data), 0, &_data_mapped) != VK_SUCCESS)
    {
      _logger->Critical("Could not make room for what the effects of the cameras are told");
      return false;
    }
    SetData(Data{});

    VkDescriptorSetLayoutCreateInfo layout{};
    layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout.bindingCount = static_cast<uint32_t>(kBindings.size());
    layout.pBindings = kBindings.data();

    VkPipelineLayoutCreateInfo pipeline_layout{};
    pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout.setLayoutCount = 1;
    pipeline_layout.pSetLayouts = &_descriptor_layout;

    const std::array<VkDescriptorPoolSize, 3> sizes{{
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, max_images},
      {VK_DESCRIPTOR_TYPE_SAMPLER, max_images},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, max_images},
    }};

    VkDescriptorPoolCreateInfo pool{};
    pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool.maxSets = max_images;
    pool.poolSizeCount = static_cast<uint32_t>(sizes.size());
    pool.pPoolSizes = sizes.data();

    if (vkCreateDescriptorSetLayout(vk_device, &layout, nullptr, &_descriptor_layout) != VK_SUCCESS ||
        vkCreatePipelineLayout(vk_device, &pipeline_layout, nullptr, &_pipeline_layout) != VK_SUCCESS ||
        vkCreateDescriptorPool(vk_device, &pool, nullptr, &_descriptor_pool) != VK_SUCCESS)
    {
      _logger->Critical("Could not set up the effects of the cameras");
      return false;
    }
    return true;
  }

  void VK_Effects::CleanUp()
  {
    if (_device == nullptr || _device->Device() == VK_NULL_HANDLE) { return; }

    const VkDevice device = _device->Device();

    for (const auto &[name, effect] : _effects)
    {
      if (effect.pipeline != VK_NULL_HANDLE) { vkDestroyPipeline(device, effect.pipeline, nullptr); }
      if (effect.fragment != VK_NULL_HANDLE) { vkDestroyShaderModule(device, effect.fragment, nullptr); }
    }
    _effects.clear();

    if (_descriptor_pool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device, _descriptor_pool, nullptr); }
    if (_pipeline_layout != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device, _pipeline_layout, nullptr); }
    if (_descriptor_layout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device, _descriptor_layout, nullptr); }
    if (_data_buffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, _data_buffer, nullptr); }
    if (_data_memory != VK_NULL_HANDLE) { vkFreeMemory(device, _data_memory, nullptr); }
    for (VkRenderPass &pass : _passes)
    {
      if (pass != VK_NULL_HANDLE) { vkDestroyRenderPass(device, pass, nullptr); }
      pass = VK_NULL_HANDLE;
    }
    if (_vertex != VK_NULL_HANDLE) { vkDestroyShaderModule(device, _vertex, nullptr); }

    _descriptor_pool = VK_NULL_HANDLE;
    _pipeline_layout = VK_NULL_HANDLE;
    _descriptor_layout = VK_NULL_HANDLE;
    _data_buffer = VK_NULL_HANDLE;
    _data_memory = VK_NULL_HANDLE;
    _data_mapped = nullptr;
    _vertex = VK_NULL_HANDLE;
  }

  void VK_Effects::SetData(const Data &data) const
  {
    if (_data_mapped != nullptr) { std::memcpy(_data_mapped, &data, sizeof(Data)); }
  }

  VkRenderPass VK_Effects::PassOf(const VK_EffectKind kind) const
  {
    return _passes[static_cast<std::size_t>(kind)];
  }

  bool VK_Effects::CreatePipeline(const VkRenderPass pass, const VkShaderModule fragment, VkPipeline &pipeline) const
  {
    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = _vertex;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragment;
    stages[1].pName = "main";

    // one triangle that covers the image, made by the vertex shader
    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo assembly{};
    assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

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
    depth.depthCompareOp = VK_COMPARE_OP_ALWAYS;

    // every pixel is written, and nothing is blended
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
    info.renderPass = pass;
    info.subpass = 0;

    return vkCreateGraphicsPipelines(_device->Device(), VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) == VK_SUCCESS;
  }

  VkPipeline VK_Effects::Find(const VK_EffectKind kind, const std::string &path)
  {
    const std::pair key(kind, path);
    if (const auto known = _effects.find(key); known != _effects.end()) { return known->second.pipeline; }

    // kept whether it can be made or not, so that it is tried once
    Effect &effect = _effects[key];
    if (!LoadModule(path + ".frag.spv", effect.fragment))
    {
      _logger->Error("The effect {} of a camera is left out", path);
      return VK_NULL_HANDLE;
    }
    if (!CreatePipeline(PassOf(kind), effect.fragment, effect.pipeline))
    {
      _logger->Error("Could not create the pipeline of the effect {} of a camera, which is left out", path);
      effect.pipeline = VK_NULL_HANDLE;
    }
    return effect.pipeline;
  }

  VkDescriptorSet VK_Effects::Keep(const VkImageView view) const
  {
    VkDescriptorSetAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocation.descriptorPool = _descriptor_pool;
    allocation.descriptorSetCount = 1;
    allocation.pSetLayouts = &_descriptor_layout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(_device->Device(), &allocation, &set) != VK_SUCCESS)
    {
      _logger->Error("There is no room for the effects of another camera");
      return VK_NULL_HANDLE;
    }

    // an effect may read the picture anywhere, so between its pixels
    const VkDescriptorImageInfo image{VK_NULL_HANDLE, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo sampler{
      _samplers->Of(VK_Sampling::LinearClamp), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    const VkDescriptorBufferInfo data{_data_buffer, 0, sizeof(Data)};

    std::array<VkWriteDescriptorSet, kBindings.size()> writes{};
    for (std::size_t i = 0; i < writes.size(); i++)
    {
      writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writes[i].dstSet = set;
      writes[i].dstBinding = kBindings[i].binding;
      writes[i].descriptorType = kBindings[i].descriptorType;
      writes[i].descriptorCount = 1;
    }
    writes[0].pImageInfo = &image;
    writes[1].pImageInfo = &sampler;
    writes[2].pBufferInfo = &data;

    vkUpdateDescriptorSets(_device->Device(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    return set;
  }

  void VK_Effects::Release(const VkDescriptorSet set) const
  {
    if (set == VK_NULL_HANDLE || _descriptor_pool == VK_NULL_HANDLE) { return; }
    vkFreeDescriptorSets(_device->Device(), _descriptor_pool, 1, &set);
  }

  void VK_Effects::Draw(
    const VkCommandBuffer commands,
    const VkPipeline pipeline,
    const VkDescriptorSet set,
    const VkExtent2D extent) const
  {
    const VkViewport viewport{
      0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
    const VkRect2D scissor{{0, 0}, extent};

    vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdSetViewport(commands, 0, 1, &viewport);
    vkCmdSetScissor(commands, 0, 1, &scissor);
    vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline_layout, 0, 1, &set, 0, nullptr);
    vkCmdDraw(commands, 3, 1, 0, 0);
  }
} // neon
