#include "vk-sky.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace neon
{
  bool VK_Sky::Initialize(
    VK_Device *device,
    FileSystemContext *file_system_context,
    const VkRenderPass render_pass,
    const VK_Samplers *samplers,
    const std::shared_ptr<Logger> &logger)
  {
    _device = device;
    _file_system_context = file_system_context;
    _samplers = samplers;
    _logger = logger;

    const VkDevice vk_device = _device->Device();

    _box_shader = VK_Shader(kBox_Shader_Path, file_system_context, _device, _logger);
    _sphere_shader = VK_Shader(kSphere_Shader_Path, file_system_context, _device, _logger);
    if (!_box_shader.Initialize() || !_sphere_shader.Initialize()) { return false; }

    VkDescriptorSetLayoutCreateInfo layout{};
    layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout.bindingCount = static_cast<uint32_t>(kBindings.size());
    layout.pBindings = kBindings.data();

    // what the fragment half is told about a draw, see sky.glsl
    constexpr VkPushConstantRange values{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(VK_SkyValues)};

    VkPipelineLayoutCreateInfo pipeline_layout{};
    pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout.setLayoutCount = 1;
    pipeline_layout.pSetLayouts = &_descriptor_layout;
    pipeline_layout.pushConstantRangeCount = 1;
    pipeline_layout.pPushConstantRanges = &values;

    // the images of every sky, and the sampler bound next to them
    constexpr std::array<VkDescriptorPoolSize, 2> sizes{{
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kMax_Skies},
      {VK_DESCRIPTOR_TYPE_SAMPLER, kMax_Skies},
    }};

    VkDescriptorPoolCreateInfo pool{};
    pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool.maxSets = kMax_Skies;
    pool.poolSizeCount = static_cast<uint32_t>(sizes.size());
    pool.pPoolSizes = sizes.data();

    if (vkCreateDescriptorSetLayout(vk_device, &layout, nullptr, &_descriptor_layout) != VK_SUCCESS ||
        vkCreatePipelineLayout(vk_device, &pipeline_layout, nullptr, &_pipeline_layout) != VK_SUCCESS ||
        vkCreateDescriptorPool(vk_device, &pool, nullptr, &_descriptor_pool) != VK_SUCCESS)
    {
      _logger->Critical("Could not set up the sky");
      return false;
    }

    return MakePipeline(_box_shader, render_pass, _box_pipeline) &&
           MakePipeline(_sphere_shader, render_pass, _sphere_pipeline);
  }

  bool VK_Sky::MakePipeline(const VK_Shader &shader, const VkRenderPass render_pass, VkPipeline &pipeline) const
  {
    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = shader.Vertex();
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = shader.Fragment();
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

    // The triangle lies at the far end of the depth, which is what the
    // depth is cleared to: it passes where nothing was drawn and nowhere
    // else, and writes no depth of its own, so that what is see-through is
    // tested against the models alone.
    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = VK_TRUE;
    depth.depthWriteEnable = VK_FALSE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    // the sky replaces what the image was cleared to, and nothing is
    // blended
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
    info.renderPass = render_pass;
    info.subpass = 0;

    if (vkCreateGraphicsPipelines(_device->Device(), VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS)
    {
      _logger->Critical("Could not create a pipeline of the sky");
      return false;
    }
    return true;
  }

  void VK_Sky::Free(Held &held) const
  {
    if (held.set != VK_NULL_HANDLE && _descriptor_pool != VK_NULL_HANDLE)
    {
      vkFreeDescriptorSets(_device->Device(), _descriptor_pool, 1, &held.set);
    }
    held.set = VK_NULL_HANDLE;
    held.texture.CleanUp();
  }

  void VK_Sky::CleanUp()
  {
    if (_device == nullptr || _device->Device() == VK_NULL_HANDLE) { return; }

    const VkDevice device = _device->Device();

    for (auto &[key, held] : _held) { Free(held); }
    _held.clear();
    _failed.clear();

    if (_box_pipeline != VK_NULL_HANDLE) { vkDestroyPipeline(device, _box_pipeline, nullptr); }
    if (_sphere_pipeline != VK_NULL_HANDLE) { vkDestroyPipeline(device, _sphere_pipeline, nullptr); }
    if (_descriptor_pool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device, _descriptor_pool, nullptr); }
    if (_pipeline_layout != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device, _pipeline_layout, nullptr); }
    if (_descriptor_layout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device, _descriptor_layout, nullptr); }
    _box_shader.CleanUp();
    _sphere_shader.CleanUp();

    _box_pipeline = VK_NULL_HANDLE;
    _sphere_pipeline = VK_NULL_HANDLE;
    _descriptor_pool = VK_NULL_HANDLE;
    _pipeline_layout = VK_NULL_HANDLE;
    _descriptor_layout = VK_NULL_HANDLE;
  }

  std::string VK_Sky::KeyOf(const SkyInfo &sky)
  {
    if (sky.type == SkyType::Sphere) { return "sphere|" + sky.texture; }

    std::string key = "box";
    for (const std::string &face : FacesOf(sky)) { key += "|" + face; }
    return key;
  }

  std::array<std::string, VK_Texture::kCube_Faces> VK_Sky::FacesOf(const SkyInfo &sky)
  {
    // positive x, negative x, positive y, negative y, positive z, negative z
    return {sky.right, sky.left, sky.top, sky.bottom, sky.front, sky.back};
  }

  VK_SkyValues VK_Sky::ValuesOf(const SkyInfo &sky, const glm::mat4 &view, const glm::mat4 &projection)
  {
    // The camera turns in the sky and never moves through it: where it
    // stands is left out of the view, and what is left is undone, which
    // takes a place at the far end of the screen to the direction it is
    // seen in. A sky that is turned is read by the direction turned back.
    const glm::mat4 turning(glm::mat3{view});
    const glm::mat4 to_world = glm::inverse(projection * turning);
    const glm::mat4 turned_back = glm::rotate(glm::mat4(1.0f), glm::radians(-sky.rotation), glm::vec3(0.0f, 1.0f, 0.0f));

    VK_SkyValues values;
    values.to_sky = turned_back * to_world;
    values.settings = {sky.brightness, 0.0f, 0.0f, 0.0f};
    return values;
  }

  bool VK_Sky::Load(const SkyInfo &sky, const std::string &key, Held &held)
  {
    if (sky.type == SkyType::Sphere)
    {
      if (sky.texture.empty())
      {
        _logger->Error("A Sky of type sphere names no texture, so no sky is drawn");
        return false;
      }

      // one image, which starts again past its left and right edge as the
      // panorama does, without smaller copies: see sky-sphere.frag
      held.texture = VK_Texture(sky.texture, _file_system_context, _device, _logger);
      if (!held.texture.Initialize(VK_TextureOptions{.mip_levels = false, .repeat = true})) { return false; }
    } else
    {
      const auto faces = FacesOf(sky);
      constexpr std::array names{"right", "left", "top", "bottom", "front", "back"};
      for (std::size_t i = 0; i < faces.size(); i++)
      {
        if (!faces[i].empty()) { continue; }

        const std::string name = names[i];
        _logger->Error("A Sky of type box names no image for its face '{}', so no sky is drawn", name);
        return false;
      }

      held.texture = VK_Texture("the faces of a sky", _file_system_context, _device, _logger);
      if (!held.texture.InitializeWithFaces(faces)) { return false; }
    }

    VkDescriptorSetAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocation.descriptorPool = _descriptor_pool;
    allocation.descriptorSetCount = 1;
    allocation.pSetLayouts = &_descriptor_layout;

    if (vkAllocateDescriptorSets(_device->Device(), &allocation, &held.set) != VK_SUCCESS)
    {
      const uint32_t most = kMax_Skies;
      _logger->Error("There is no room for another sky, {} are held at once", most);
      held.set = VK_NULL_HANDLE;
      held.texture.CleanUp();
      return false;
    }

    // read smoothly: a cube has no edge, and a panorama starts again past
    // its left and right edge
    const VK_Sampling sampling = sky.type == SkyType::Sphere ? VK_Sampling::LinearRepeat : VK_Sampling::LinearClamp;
    const VkDescriptorImageInfo image{VK_NULL_HANDLE, held.texture.View(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo sampler{_samplers->Of(sampling), VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};

    std::array<VkWriteDescriptorSet, kBindings.size()> writes{};
    for (std::size_t i = 0; i < writes.size(); i++)
    {
      writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writes[i].dstSet = held.set;
      writes[i].dstBinding = kBindings[i].binding;
      writes[i].descriptorType = kBindings[i].descriptorType;
      writes[i].descriptorCount = 1;
    }
    writes[0].pImageInfo = &image;
    writes[1].pImageInfo = &sampler;

    vkUpdateDescriptorSets(_device->Device(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    _logger->Debug("Loaded the sky {}", key);
    return true;
  }

  bool VK_Sky::Prepare(const SkyInfo &sky, const glm::mat4 &view, const glm::mat4 &projection, VK_SkyDraw &draw)
  {
    if (_box_pipeline == VK_NULL_HANDLE || _sphere_pipeline == VK_NULL_HANDLE) { return false; }

    const std::string key = KeyOf(sky);
    if (_failed.contains(key)) { return false; }

    auto found = _held.find(key);
    if (found == _held.end())
    {
      Held held;
      if (!Load(sky, key, held))
      {
        // said once, and not tried again in every frame
        _failed.insert(key);
        return false;
      }
      found = _held.emplace(key, held).first;
    }

    found->second.is_drawn = true;

    draw.pipeline = sky.type == SkyType::Sphere ? _sphere_pipeline : _box_pipeline;
    draw.set = found->second.set;
    draw.values = ValuesOf(sky, view, projection);
    return true;
  }

  void VK_Sky::Draw(const VkCommandBuffer commands, const VK_SkyDraw &draw) const
  {
    vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.pipeline);
    vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline_layout, 0, 1, &draw.set, 0, nullptr);
    vkCmdPushConstants(
      commands, _pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(VK_SkyValues), &draw.values);
    vkCmdDraw(commands, 3, 1, 0, 0);
  }

  void VK_Sky::ReleaseUnused()
  {
    for (auto each = _held.begin(); each != _held.end();)
    {
      if (each->second.is_drawn)
      {
        each->second.is_drawn = false;
        ++each;
        continue;
      }

      _logger->Debug("The sky {} was freed, no scene draws it any more", each->first);
      Free(each->second);
      each = _held.erase(each);
    }
  }
} // neon
