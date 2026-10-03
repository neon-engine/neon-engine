#include "vk-pipelines.hpp"

#include <array>
#include <cstddef>
#include <neon/render/vertex.hpp>

#include "vk-culling.hpp"

namespace neon
{
  bool VK_Pipelines::Initialize(
    VK_Device *device,
    FileSystemContext *file_system_context,
    const VkRenderPass scene_pass,
    const VkRenderPass shadow_pass,
    const std::shared_ptr<Logger> &logger)
  {
    _device = device;
    _file_system_context = file_system_context;
    _scene_pass = scene_pass;
    _shadow_pass = shadow_pass;
    _logger = logger;

    VkDescriptorSetLayoutCreateInfo layout{};
    layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout.bindingCount = static_cast<uint32_t>(kBindings.size());
    layout.pBindings = kBindings.data();

    if (vkCreateDescriptorSetLayout(_device->Device(), &layout, nullptr, &_descriptor_layout) != VK_SUCCESS)
    {
      _logger->Critical("Could not create the Vulkan descriptor layout");
      return false;
    }

    // the shadow pass names the cascade it draws with a push constant,
    // which the shaders of the scene leave alone
    constexpr VkPushConstantRange cascade{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t)};

    VkPipelineLayoutCreateInfo pipeline_layout{};
    pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout.setLayoutCount = 1;
    pipeline_layout.pSetLayouts = &_descriptor_layout;
    pipeline_layout.pushConstantRangeCount = 1;
    pipeline_layout.pPushConstantRanges = &cascade;

    if (vkCreatePipelineLayout(_device->Device(), &pipeline_layout, nullptr, &_pipeline_layout) != VK_SUCCESS)
    {
      _logger->Critical("Could not create the Vulkan pipeline layout");
      return false;
    }
    return true;
  }

  void VK_Pipelines::CleanUp()
  {
    if (_device == nullptr) { return; }

    const VkDevice device = _device->Device();

    for (auto &[path, entry] : _pipelines)
    {
      vkDestroyPipeline(device, entry.pipeline, nullptr);
      entry.shader.CleanUp();
    }
    _pipelines.clear();

    if (_pipeline_layout != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device, _pipeline_layout, nullptr); }
    if (_descriptor_layout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device, _descriptor_layout, nullptr); }

    _pipeline_layout = VK_NULL_HANDLE;
    _descriptor_layout = VK_NULL_HANDLE;
  }

  bool VK_Pipelines::Get(
    const std::string &shader_path,
    const AlphaMode alpha_mode,
    const bool double_sided,
    const bool mirrored,
    VkPipeline &pipeline)
  {
    // materials that name the same shader, and cover and are culled alike,
    // share a pipeline
    const std::string key = VK_Culling::PipelineKey(shader_path, alpha_mode, double_sided, mirrored);
    return Make(key, shader_path, alpha_mode, double_sided, mirrored, false, pipeline);
  }

  bool VK_Pipelines::GetShadow(const bool double_sided, const bool mirrored, VkPipeline &pipeline)
  {
    // one shader for every caster, so the culling alone tells them apart
    const std::string key = VK_Culling::PipelineKey(kShadow_Shader_Path, AlphaMode::Opaque, double_sided, mirrored);
    return Make(key, kShadow_Shader_Path, AlphaMode::Opaque, double_sided, mirrored, true, pipeline);
  }

  bool VK_Pipelines::Make(
    const std::string &key,
    const std::string &shader_path,
    const AlphaMode alpha_mode,
    const bool double_sided,
    const bool mirrored,
    const bool shadow,
    VkPipeline &pipeline)
  {
    const bool blends = alpha_mode == AlphaMode::Blend;

    if (const auto existing = _pipelines.find(key); existing != _pipelines.end())
    {
      pipeline = existing->second.pipeline;
      return true;
    }

    Entry entry;
    entry.shader = VK_Shader(shader_path, _file_system_context, _device, _logger);
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
    // one layout for every model: a model without vertex colours carries
    // white ones, see docs/models.md
    constexpr std::array<VkVertexInputAttributeDescription, 4> attributes{{
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
      {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, tex_coords)},
      {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color)},
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

    // set when the scene of a frame is begun
    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    // the back of a triangle is left out, unless the material is drawn
    // from both sides
    VkPipelineRasterizationStateCreateInfo rasterization{};
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = VK_Culling::CullModeFor(double_sided);
    rasterization.frontFace = VK_Culling::FrontFaceFor(mirrored);
    rasterization.lineWidth = 1.0f;

    // A caster is pushed back from the light along the slope of its
    // surface, so that the surface does not shadow itself where the map
    // rounds. The constant part is left to the shaders: in a map of whole
    // floats the unit of it is too small to matter.
    rasterization.depthBiasEnable = shadow ? VK_TRUE : VK_FALSE;
    rasterization.depthBiasSlopeFactor = shadow ? kShadow_Slope_Bias : 0.0f;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // What is see-through is hidden by what is opaque in front of it, but
    // hides nothing itself, so that what is drawn after it still shows.
    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = VK_TRUE;
    depth.depthWriteEnable = blends ? VK_FALSE : VK_TRUE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS;

    // A see-through colour is blended over what is behind it, in the
    // linear light of the scene image. Its alpha is multiplied into the
    // colour, and the scene image keeps alpha multiplied in.
    //
    // An opaque colour replaces what is behind it, with factors of one and
    // zero, which is the colour the shader wrote to the last bit. Its alpha
    // does not reach the scene image as the shader wrote it: the resolve
    // divides the light by the alpha, so an alpha below 1 from an opaque
    // shader would come out brighter than it is. Vulkan has no blend factor
    // that writes a constant, so the alpha is the larger of what the
    // shader wrote and what was there, for which MAX ignores the factors.
    // Over a clear that is opaque, as a frame is, that is 1 whatever the
    // shader wrote. Over a texture a camera clears to a see-through colour
    // it is at least what was there, and the shaders of the engine write 1
    // themselves through object_alpha() for that case.
    VkPipelineColorBlendAttachmentState blend_attachment{};
    blend_attachment.blendEnable = VK_TRUE;
    blend_attachment.srcColorBlendFactor = blends ? VK_BLEND_FACTOR_SRC_ALPHA : VK_BLEND_FACTOR_ONE;
    blend_attachment.dstColorBlendFactor = blends ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ZERO;
    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.alphaBlendOp = blends ? VK_BLEND_OP_ADD : VK_BLEND_OP_MAX;
    blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // the shadow map has no colour to write
    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = shadow ? 0 : 1;
    blend.pAttachments = shadow ? nullptr : &blend_attachment;

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
    info.renderPass = shadow ? _shadow_pass : _scene_pass;
    info.subpass = 0;

    if (vkCreateGraphicsPipelines(
      _device->Device(), VK_NULL_HANDLE, 1, &info, nullptr, &entry.pipeline) != VK_SUCCESS)
    {
      _logger->Error("Could not create the pipeline of shader {}", shader_path);
      entry.shader.CleanUp();
      return false;
    }

    pipeline = entry.pipeline;
    _pipelines.emplace(key, entry);
    return true;
  }
} // neon
