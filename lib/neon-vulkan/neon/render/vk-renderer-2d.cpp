#include "vk-renderer-2d.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>

namespace neon
{
  namespace
  {
    // A user interface is drawn at the size of its images, and a text must
    // not show what is next to a character in its atlas. Smaller copies and
    // an image that starts again would do that.
    constexpr VK_TextureOptions texture_options{
      .mip_levels = false,
      .repeat = false,
      .premultiply_alpha = true
    };
  }

  void VK_Renderer2D::Initialize(
    VK_Device *device,
    FileSystemContext *file_system_context,
    const VkRenderPass render_pass,
    const VkExtent2D extent,
    const std::shared_ptr<Logger> &logger)
  {
    _device = device;
    _file_system_context = file_system_context;
    _render_pass = render_pass;
    _extent = extent;
    _logger = logger;
  }

  bool VK_Renderer2D::IsWellFormed(const Triangles2D &triangles)
  {
    if (triangles.indices.size() % 3 != 0) { return false; }

    return std::ranges::all_of(triangles.indices, [&triangles](const std::uint32_t index)
    {
      return index < triangles.vertices.size();
    });
  }

  VkRect2D VK_Renderer2D::ScissorOf(const Triangles2D &triangles, const VkExtent2D extent)
  {
    const Triangles2D &batch = triangles;
    if (!batch.clipped) { return {{0, 0}, extent}; }

    const auto frame_width = static_cast<int>(extent.width);
    const auto frame_height = static_cast<int>(extent.height);

    const int left = std::clamp(batch.clip.x, 0, frame_width);
    const int top = std::clamp(batch.clip.y, 0, frame_height);
    const int right = std::clamp(batch.clip.x + std::max(batch.clip.width, 0), left, frame_width);
    const int bottom = std::clamp(batch.clip.y + std::max(batch.clip.height, 0), top, frame_height);

    return {{left, top}, {static_cast<uint32_t>(right - left), static_cast<uint32_t>(bottom - top)}};
  }

  bool VK_Renderer2D::Prepare()
  {
    if (_state == State::NotCreated)
    {
      // said once, and not in every frame
      if (Create())
      {
        _state = State::Ready;
      } else
      {
        _state = State::Failed;
        _logger->Error("Drawing in two dimensions could not be set up, nothing of it is drawn");
      }
    }

    return _state == State::Ready;
  }

  bool VK_Renderer2D::Create()
  {
    if (_device == nullptr || _device->Device() == VK_NULL_HANDLE)
    {
      _logger->Error("Drawing in two dimensions needs a renderer that is initialized");
      return false;
    }

    const VkDevice device = _device->Device();

    constexpr VkDescriptorSetLayoutBinding binding{
      0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};

    VkDescriptorSetLayoutCreateInfo layout{};
    layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout.bindingCount = 1;
    layout.pBindings = &binding;

    if (vkCreateDescriptorSetLayout(device, &layout, nullptr, &_descriptor_layout) != VK_SUCCESS)
    {
      _logger->Error("Could not create the descriptor layout for drawing in two dimensions");
      return false;
    }

    // the size of the frame, which the vertex shader turns pixels into
    // places with, and what the corners are moved by
    constexpr VkPushConstantRange frame_size{VK_SHADER_STAGE_VERTEX_BIT, 0, 4 * sizeof(float)};

    VkPipelineLayoutCreateInfo pipeline_layout{};
    pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout.setLayoutCount = 1;
    pipeline_layout.pSetLayouts = &_descriptor_layout;
    pipeline_layout.pushConstantRangeCount = 1;
    pipeline_layout.pPushConstantRanges = &frame_size;

    if (vkCreatePipelineLayout(device, &pipeline_layout, nullptr, &_pipeline_layout) != VK_SUCCESS)
    {
      _logger->Error("Could not create the pipeline layout for drawing in two dimensions");
      return false;
    }

    // one set for each texture
    constexpr VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMax_Textures};

    VkDescriptorPoolCreateInfo pool{};
    pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool.maxSets = kMax_Textures;
    pool.poolSizeCount = 1;
    pool.pPoolSizes = &size;

    if (vkCreateDescriptorPool(device, &pool, nullptr, &_descriptor_pool) != VK_SUCCESS)
    {
      _logger->Error("Could not create the descriptor pool for drawing in two dimensions");
      return false;
    }

    if (!CreatePipeline() || !CreateBuffers()) { return false; }

    // what a batch without a texture is drawn with, so that the shader
    // always has something to read
    VK_Texture white("a plain white texture", _file_system_context, _device, _logger);
    constexpr unsigned char pixel[4] = {255, 255, 255, 255};
    if (!white.InitializeWithPixels(pixel, 1, 1, texture_options)) { return false; }

    // Prepare() is called again by Keep(), and has to find the state
    // settled by then
    _state = State::Ready;
    _white_texture = Keep(white);
    return _white_texture != No_Texture;
  }

  bool VK_Renderer2D::CreatePipeline()
  {
    _shader = VK_Shader(kShader_Path, _file_system_context, _device, _logger);
    if (!_shader.Initialize()) { return false; }

    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = _shader.Vertex();
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = _shader.Fragment();
    stages[1].pName = "main";

    // a corner is handed to the shader the way the core declares it
    static_assert(sizeof(Vertex2D) == 9 * sizeof(float));

    constexpr VkVertexInputBindingDescription binding{0, sizeof(Vertex2D), VK_VERTEX_INPUT_RATE_VERTEX};
    constexpr std::array<VkVertexInputAttributeDescription, 4> attributes{{
      {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex2D, x)},
      {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex2D, u)},
      {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex2D, color)},
      {3, 0, VK_FORMAT_R32_SFLOAT, offsetof(Vertex2D, textured)},
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

    // set for every batch in Draw()
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

    // what is drawn later is on top, whatever the scene left in the depth
    // image
    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = VK_FALSE;
    depth.depthWriteEnable = VK_FALSE;
    depth.depthCompareOp = VK_COMPARE_OP_ALWAYS;

    // Alpha is multiplied into the colours by the time they are blended,
    // in the textures and by the shader. The alpha of the frame stays 1
    // where it was 1.
    VkPipelineColorBlendAttachmentState blend_attachment{};
    blend_attachment.blendEnable = VK_TRUE;
    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
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

    if (vkCreateGraphicsPipelines(_device->Device(), VK_NULL_HANDLE, 1, &info, nullptr, &_pipeline) != VK_SUCCESS)
    {
      const std::string shader_path = kShader_Path;
      _logger->Error("Could not create the pipeline of shader {}", shader_path);
      return false;
    }
    return true;
  }

  bool VK_Renderer2D::CreateBuffers()
  {
    const VkDevice device = _device->Device();
    constexpr VkMemoryPropertyFlags writable =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    constexpr VkDeviceSize vertex_size = static_cast<VkDeviceSize>(kMax_Vertices) * sizeof(Vertex2D);
    constexpr VkDeviceSize index_size = static_cast<VkDeviceSize>(kMax_Indices) * sizeof(uint32_t);

    void *mapped = nullptr;

    if (!_device->CreateBuffer(
          vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, writable, _vertex_buffer, _vertex_memory) ||
        vkMapMemory(device, _vertex_memory, 0, vertex_size, 0, &mapped) != VK_SUCCESS)
    {
      _logger->Error("Could not create the vertex buffer for drawing in two dimensions");
      return false;
    }
    _vertices = static_cast<Vertex2D *>(mapped);

    if (!_device->CreateBuffer(
          index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, writable, _index_buffer, _index_memory) ||
        vkMapMemory(device, _index_memory, 0, index_size, 0, &mapped) != VK_SUCCESS)
    {
      _logger->Error("Could not create the index buffer for drawing in two dimensions");
      return false;
    }
    _indices = static_cast<uint32_t *>(mapped);

    return true;
  }

  int VK_Renderer2D::Keep(VK_Texture &texture)
  {
    if (!Prepare())
    {
      texture.CleanUp();
      return No_Texture;
    }

    VkDescriptorSetAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocation.descriptorPool = _descriptor_pool;
    allocation.descriptorSetCount = 1;
    allocation.pSetLayouts = &_descriptor_layout;

    Texture kept;
    kept.texture = texture;

    if (vkAllocateDescriptorSets(_device->Device(), &allocation, &kept.descriptor_set) != VK_SUCCESS)
    {
      _logger->Error("Could not allocate a descriptor set for a texture");
      texture.CleanUp();
      return No_Texture;
    }

    const VkDescriptorImageInfo image{
      texture.Sampler(), texture.View(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = kept.descriptor_set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;

    vkUpdateDescriptorSets(_device->Device(), 1, &write, 0, nullptr);

    const int id = _textures.Add(kept);
    if (id < 0)
    {
      _logger->Error("There is no room for another texture");
      vkFreeDescriptorSets(_device->Device(), _descriptor_pool, 1, &kept.descriptor_set);
      texture.CleanUp();
      return No_Texture;
    }

    return id;
  }

  int VK_Renderer2D::CreateTexture(const int width, const int height, const std::vector<unsigned char> &pixels)
  {
    if (width <= 0 || height <= 0 || pixels.size() != static_cast<std::size_t>(width) * height * 4)
    {
      const std::size_t given = pixels.size();
      _logger->Error(
        "A texture of {} by {} needs four bytes for each pixel, and was given {} bytes",
        width, height, given);
      return No_Texture;
    }

    if (!Prepare()) { return No_Texture; }

    VK_Texture texture("a texture from memory", _file_system_context, _device, _logger);
    if (!texture.InitializeWithPixels(
      pixels.data(), static_cast<uint32_t>(width), static_cast<uint32_t>(height), texture_options))
    {
      return No_Texture;
    }

    return Keep(texture);
  }

  int VK_Renderer2D::LoadTexture(const std::string &path)
  {
    if (_state == State::Failed) { return No_Texture; }

    // what is wrong with the file is found before Vulkan is asked anything
    VK_Texture texture(path, _file_system_context, _device, _logger);
    if (!texture.Initialize(texture_options)) { return No_Texture; }

    return Keep(texture);
  }

  bool VK_Renderer2D::GetTextureSize(const int texture, int &width, int &height) const
  {
    if (!_textures.Contains(texture)) { return false; }

    width = static_cast<int>(_textures[texture].texture.Width());
    height = static_cast<int>(_textures[texture].texture.Height());
    return true;
  }

  void VK_Renderer2D::DestroyTexture(const int texture)
  {
    if (!_textures.Contains(texture) || texture == _white_texture) { return; }

    // nothing may still be drawing with what is about to be destroyed
    vkDeviceWaitIdle(_device->Device());

    Texture removed = _textures.Remove(texture);
    vkFreeDescriptorSets(_device->Device(), _descriptor_pool, 1, &removed.descriptor_set);
    removed.texture.CleanUp();
  }

  void VK_Renderer2D::PrepareFrame()
  {
    _used_vertices = 0;
    _used_indices = 0;
  }

  void VK_Renderer2D::Draw(const Triangles2D &triangles)
  {
    if (triangles.indices.empty() || triangles.vertices.empty() || _device == nullptr) { return; }

    const VkCommandBuffer commands = _device->FrameCommands();
    if (commands == VK_NULL_HANDLE || !Prepare()) { return; }

    const VkRect2D scissor = ScissorOf(triangles, _extent);
    if (scissor.extent.width == 0 || scissor.extent.height == 0) { return; }

    if (!IsWellFormed(triangles))
    {
      if (!_warned_about_shape)
      {
        _warned_about_shape = true;
        _logger->Warn(
          "Triangles were handed over whose corners do not exist or do not come in threes. "
          "They are left out");
      }
      return;
    }

    const auto vertex_count = static_cast<uint32_t>(triangles.vertices.size());
    const auto index_count = static_cast<uint32_t>(triangles.indices.size());

    // Triangles that do not fit are left out as a whole. A part of them
    // would be a shape nobody asked for.
    if (vertex_count > kMax_Vertices - _used_vertices || index_count > kMax_Indices - _used_indices)
    {
      if (!_warned_about_capacity)
      {
        _warned_about_capacity = true;
        constexpr uint32_t vertex_limit = kMax_Vertices;
        constexpr uint32_t index_limit = kMax_Indices;
        _logger->Warn(
          "More was drawn in two dimensions in one frame than {} corners or {} corners of triangles, "
          "the rest is left out",
          vertex_limit, index_limit);
      }
      return;
    }

    std::memcpy(_vertices + _used_vertices, triangles.vertices.data(), vertex_count * sizeof(Vertex2D));
    std::memcpy(_indices + _used_indices, triangles.indices.data(), index_count * sizeof(uint32_t));

    // a texture that does not exist is drawn as plain white, which shows
    // where it is missing
    const int texture = _textures.Contains(triangles.texture) ? triangles.texture : _white_texture;
    const VkDescriptorSet set = _textures[texture].descriptor_set;

    // rows are counted from the top here, as the places of the corners are
    const VkViewport viewport{
      0.0f, 0.0f, static_cast<float>(_extent.width), static_cast<float>(_extent.height), 0.0f, 1.0f};
    const float frame[4] = {
      static_cast<float>(_extent.width),
      static_cast<float>(_extent.height),
      triangles.translate_x,
      triangles.translate_y
    };
    constexpr VkDeviceSize offset = 0;

    vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    vkCmdSetViewport(commands, 0, 1, &viewport);
    vkCmdSetScissor(commands, 0, 1, &scissor);
    vkCmdPushConstants(commands, _pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(frame), frame);
    vkCmdBindDescriptorSets(
      commands, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline_layout, 0, 1, &set, 0, nullptr);
    vkCmdBindVertexBuffers(commands, 0, 1, &_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(commands, _index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commands, index_count, 1, _used_indices, static_cast<int32_t>(_used_vertices), 0);

    _used_vertices += vertex_count;
    _used_indices += index_count;

    // what the models of a scene are drawn with, in case one follows
    const VkViewport scene_viewport{
      0.0f,
      static_cast<float>(_extent.height),
      static_cast<float>(_extent.width),
      -static_cast<float>(_extent.height),
      0.0f,
      1.0f};
    const VkRect2D whole_frame{{0, 0}, _extent};

    vkCmdSetViewport(commands, 0, 1, &scene_viewport);
    vkCmdSetScissor(commands, 0, 1, &whole_frame);
  }

  void VK_Renderer2D::CleanUp()
  {
    if (_device == nullptr || _device->Device() == VK_NULL_HANDLE) { return; }

    const VkDevice device = _device->Device();

    for (int id = 0; id < _textures.Capacity(); id++)
    {
      if (_textures.Contains(id)) { _textures.Remove(id).texture.CleanUp(); }
    }
    _white_texture = No_Texture;

    if (_vertex_memory != VK_NULL_HANDLE && _vertices != nullptr) { vkUnmapMemory(device, _vertex_memory); }
    if (_vertex_buffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, _vertex_buffer, nullptr); }
    if (_vertex_memory != VK_NULL_HANDLE) { vkFreeMemory(device, _vertex_memory, nullptr); }
    if (_index_memory != VK_NULL_HANDLE && _indices != nullptr) { vkUnmapMemory(device, _index_memory); }
    if (_index_buffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, _index_buffer, nullptr); }
    if (_index_memory != VK_NULL_HANDLE) { vkFreeMemory(device, _index_memory, nullptr); }

    if (_pipeline != VK_NULL_HANDLE) { vkDestroyPipeline(device, _pipeline, nullptr); }
    _shader.CleanUp();

    // the sets of the textures go with their pool
    if (_descriptor_pool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device, _descriptor_pool, nullptr); }
    if (_pipeline_layout != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device, _pipeline_layout, nullptr); }
    if (_descriptor_layout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device, _descriptor_layout, nullptr); }

    _vertices = nullptr;
    _indices = nullptr;
    _vertex_buffer = VK_NULL_HANDLE;
    _vertex_memory = VK_NULL_HANDLE;
    _index_buffer = VK_NULL_HANDLE;
    _index_memory = VK_NULL_HANDLE;
    _pipeline = VK_NULL_HANDLE;
    _descriptor_pool = VK_NULL_HANDLE;
    _pipeline_layout = VK_NULL_HANDLE;
    _descriptor_layout = VK_NULL_HANDLE;
    _state = State::NotCreated;
    _used_vertices = 0;
    _used_indices = 0;
  }
} // neon
