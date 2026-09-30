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

    // the shapes of a frame, and the values of the shaders of elements
    constexpr VkDescriptorSetLayoutBinding shapes_binding{
      0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    constexpr VkDescriptorSetLayoutBinding values_binding{
      0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};

    layout.pBindings = &shapes_binding;
    if (vkCreateDescriptorSetLayout(device, &layout, nullptr, &_shapes_layout) != VK_SUCCESS)
    {
      _logger->Error("Could not create the descriptor layout for the shapes of drawing in two dimensions");
      return false;
    }

    layout.pBindings = &values_binding;
    if (vkCreateDescriptorSetLayout(device, &layout, nullptr, &_values_layout) != VK_SUCCESS)
    {
      _logger->Error("Could not create the descriptor layout for the values of shaders of elements");
      return false;
    }

    // what both halves of the shader are told about a call
    constexpr VkPushConstantRange frame_range{
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Frame)};

    const std::array set_layouts{_descriptor_layout, _shapes_layout, _values_layout};

    VkPipelineLayoutCreateInfo pipeline_layout{};
    pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout.setLayoutCount = static_cast<uint32_t>(set_layouts.size());
    pipeline_layout.pSetLayouts = set_layouts.data();
    pipeline_layout.pushConstantRangeCount = 1;
    pipeline_layout.pPushConstantRanges = &frame_range;

    if (vkCreatePipelineLayout(device, &pipeline_layout, nullptr, &_pipeline_layout) != VK_SUCCESS)
    {
      _logger->Error("Could not create the pipeline layout for drawing in two dimensions");
      return false;
    }

    // two sets for each texture: one that blends its pixels, and one that
    // reads them as they are
    constexpr std::array<VkDescriptorPoolSize, 3> sizes{{
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * kMax_Textures},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1},
    }};

    VkDescriptorPoolCreateInfo pool{};
    pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool.maxSets = 2 * kMax_Textures + 2;
    pool.poolSizeCount = static_cast<uint32_t>(sizes.size());
    pool.pPoolSizes = sizes.data();

    if (vkCreateDescriptorPool(device, &pool, nullptr, &_descriptor_pool) != VK_SUCCESS)
    {
      _logger->Error("Could not create the descriptor pool for drawing in two dimensions");
      return false;
    }

    if (!CreatePipeline() || !CreateBuffers()) { return false; }

    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = VK_FILTER_NEAREST;
    sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.maxAnisotropy = 1.0f;

    if (vkCreateSampler(device, &sampler, nullptr, &_pixelated_sampler) != VK_SUCCESS)
    {
      _logger->Error("Could not create the sampler for images that are drawn pixel by pixel");
      return false;
    }

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

    return CreatePipelineFor(_shader.Fragment(), kShader_Path, _pipeline);
  }

  bool VK_Renderer2D::CreatePipelineFor(
    const VkShaderModule fragment,
    const std::string &path,
    VkPipeline &pipeline) const
  {
    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = _shader.Vertex();
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragment;
    stages[1].pName = "main";

    // a corner is handed to the shader the way the core declares it
    static_assert(sizeof(Vertex2D) == 12 * sizeof(float));
    static_assert(sizeof(Shape2D) == 72 * sizeof(float));
    static_assert(sizeof(Frame) == 20 * sizeof(float));

    constexpr VkVertexInputBindingDescription binding{0, sizeof(Vertex2D), VK_VERTEX_INPUT_RATE_VERTEX};
    constexpr std::array<VkVertexInputAttributeDescription, 6> attributes{{
      {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex2D, x)},
      {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex2D, u)},
      {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex2D, color)},
      {3, 0, VK_FORMAT_R32_SFLOAT, offsetof(Vertex2D, textured)},
      {4, 0, VK_FORMAT_R32_SFLOAT, offsetof(Vertex2D, shape)},
      {5, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex2D, local_x)},
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

    if (vkCreateGraphicsPipelines(_device->Device(), VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS)
    {
      _logger->Error("Could not create the pipeline of shader {}", path);
      return false;
    }
    return true;
  }

  bool VK_Renderer2D::CreateFrameBuffer(
    FrameBuffer &buffer,
    const VkDeviceSize size,
    const VkBufferUsageFlags usage) const
  {
    constexpr VkMemoryPropertyFlags writable =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    void *mapped = nullptr;

    if (!_device->CreateBuffer(size, usage, writable, buffer.buffer, buffer.memory) ||
        vkMapMemory(_device->Device(), buffer.memory, 0, size, 0, &mapped) != VK_SUCCESS)
    {
      return false;
    }

    buffer.mapped = static_cast<unsigned char *>(mapped);
    return true;
  }

  void VK_Renderer2D::DestroyFrameBuffer(FrameBuffer &buffer) const
  {
    const VkDevice device = _device->Device();

    if (buffer.memory != VK_NULL_HANDLE && buffer.mapped != nullptr) { vkUnmapMemory(device, buffer.memory); }
    if (buffer.buffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, buffer.buffer, nullptr); }
    if (buffer.memory != VK_NULL_HANDLE) { vkFreeMemory(device, buffer.memory, nullptr); }

    buffer = {};
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

    // values are picked by an offset, which has to respect the alignment
    // the graphics card asks for
    const VkDeviceSize alignment = std::max<VkDeviceSize>(
      _device->Properties().limits.minUniformBufferOffsetAlignment, 1);
    _values_entry_size = (kMax_Values_Size + alignment - 1) / alignment * alignment;

    constexpr VkDeviceSize shapes_size = static_cast<VkDeviceSize>(kMax_Shapes) * sizeof(Shape2D);
    const VkDeviceSize values_size = _values_entry_size * kMax_Material_Draws;

    if (!CreateFrameBuffer(_shape_buffer, shapes_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) ||
        !CreateFrameBuffer(_values_buffer, values_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
    {
      _logger->Error("Could not create the buffers for shapes and values of drawing in two dimensions");
      return false;
    }

    const std::array layouts{_shapes_layout, _values_layout};
    std::array<VkDescriptorSet, 2> sets{};

    VkDescriptorSetAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocation.descriptorPool = _descriptor_pool;
    allocation.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocation.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(device, &allocation, sets.data()) != VK_SUCCESS)
    {
      _logger->Error("Could not allocate the descriptor sets for shapes and values");
      return false;
    }

    _shapes_set = sets[0];
    _values_set = sets[1];

    const VkDescriptorBufferInfo shapes{_shape_buffer.buffer, 0, shapes_size};
    const VkDescriptorBufferInfo values{_values_buffer.buffer, 0, kMax_Values_Size};

    std::array<VkWriteDescriptorSet, 2> writes{};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = _shapes_set;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &shapes;
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = _values_set;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writes[1].pBufferInfo = &values;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

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
    if (removed.pixelated_set != VK_NULL_HANDLE)
    {
      vkFreeDescriptorSets(_device->Device(), _descriptor_pool, 1, &removed.pixelated_set);
    }
    removed.texture.CleanUp();
  }

  bool VK_Renderer2D::UpdateTexture(
    const int texture,
    const int x,
    const int y,
    const int width,
    const int height,
    const std::vector<unsigned char> &pixels)
  {
    if (!_textures.Contains(texture) || texture == _white_texture) { return false; }

    if (x < 0 || y < 0 || width <= 0 || height <= 0 ||
        pixels.size() != static_cast<std::size_t>(width) * height * 4)
    {
      return false;
    }

    return _textures[texture].texture.Update(
      pixels.data(),
      static_cast<uint32_t>(x),
      static_cast<uint32_t>(y),
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height),
      true);
  }

  int VK_Renderer2D::CreateTextureWith(
    const int width,
    const int height,
    const std::vector<unsigned char> &pixels,
    const TextureOptions2D &options)
  {
    if (!options.has_smaller_copies && !options.repeats) { return CreateTexture(width, height, pixels); }

    if (width <= 0 || height <= 0 || pixels.size() != static_cast<std::size_t>(width) * height * 4)
    {
      const std::size_t given = pixels.size();
      _logger->Error(
        "A texture of {} by {} needs four bytes for each pixel, and was given {} bytes",
        width, height, given);
      return No_Texture;
    }

    if (!Prepare()) { return No_Texture; }

    // Alpha is multiplied in before the smaller copies are made, so that
    // what is see-through does not darken what is next to it.
    ImagePixels image;
    image.width = width;
    image.height = height;
    image.pixels = pixels;
    PremultiplyAlpha(image.pixels);

    const std::vector<ImagePixels> levels = options.has_smaller_copies
      ? MakeSmallerCopies(image)
      : std::vector<ImagePixels>{image};

    VK_TextureOptions kept;
    kept.mip_levels = options.has_smaller_copies;
    kept.repeat = options.repeats;
    kept.premultiply_alpha = false;

    VK_Texture texture("a texture from memory", _file_system_context, _device, _logger);
    if (!texture.InitializeWithLevels(levels, kept)) { return No_Texture; }

    return Keep(texture);
  }

  int VK_Renderer2D::KeepBorrowed(
    const VkImageView view,
    const VkSampler sampler,
    const uint32_t width,
    const uint32_t height)
  {
    VK_Texture texture = VK_Texture::Borrowed(view, sampler, width, height);
    return Keep(texture);
  }

  VkDescriptorSet VK_Renderer2D::SetOf(const int texture, const TextureFilter2D filter)
  {
    Texture &kept = _textures[texture];
    if (filter == TextureFilter2D::Smooth) { return kept.descriptor_set; }

    if (kept.pixelated_set == VK_NULL_HANDLE)
    {
      VkDescriptorSetAllocateInfo allocation{};
      allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      allocation.descriptorPool = _descriptor_pool;
      allocation.descriptorSetCount = 1;
      allocation.pSetLayouts = &_descriptor_layout;

      if (vkAllocateDescriptorSets(_device->Device(), &allocation, &kept.pixelated_set) != VK_SUCCESS)
      {
        kept.pixelated_set = VK_NULL_HANDLE;
        return kept.descriptor_set;
      }

      const VkDescriptorImageInfo image{
        _pixelated_sampler, kept.texture.View(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

      VkWriteDescriptorSet write{};
      write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet = kept.pixelated_set;
      write.dstBinding = 0;
      write.descriptorCount = 1;
      write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      write.pImageInfo = &image;

      vkUpdateDescriptorSets(_device->Device(), 1, &write, 0, nullptr);
    }

    return kept.pixelated_set;
  }

  int VK_Renderer2D::CreateMaterial(const std::string &shader_path)
  {
    for (std::size_t i = 0; i < _materials.size(); i++)
    {
      if (_materials[i].path != shader_path) { continue; }

      // what could not be made is not tried again
      return _materials[i].pipeline != VK_NULL_HANDLE ? static_cast<int>(i) : No_Material;
    }

    Material material;
    material.path = shader_path;

    const auto keep = [this, &material]
    {
      _materials.push_back(material);
      return material.pipeline != VK_NULL_HANDLE ? static_cast<int>(_materials.size()) - 1 : No_Material;
    };

    if (!Prepare()) { return keep(); }

    // A shader of an element is the half that colours pixels. Where the
    // corners go is the work of the shader of the engine.
    const std::string file = shader_path + ".frag.spv";

    std::vector<unsigned char> bytes;
    if (!_file_system_context->ReadBytes(file, bytes))
    {
      _logger->Error("Could not read shader {}, was it compiled by the build?", file);
      return keep();
    }

    if (bytes.empty() || bytes.size() % sizeof(uint32_t) != 0)
    {
      _logger->Error("Shader {} is not valid SPIR-V", file);
      return keep();
    }

    std::vector<uint32_t> words(bytes.size() / sizeof(uint32_t));
    std::memcpy(words.data(), bytes.data(), bytes.size());

    if (!VK_ShaderValues::Read(words, material.values))
    {
      _logger->Error("Shader {} is not valid SPIR-V", file);
      return keep();
    }

    if (material.values.size > kMax_Values_Size)
    {
      const uint32_t size = material.values.size;
      constexpr uint32_t limit = kMax_Values_Size;
      _logger->Error("The values of shader {} take {} bytes, where up to {} fit", file, size, limit);
      return keep();
    }

    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = bytes.size();
    info.pCode = words.data();

    if (vkCreateShaderModule(_device->Device(), &info, nullptr, &material.fragment) != VK_SUCCESS)
    {
      _logger->Error("Could not create a module from shader {}", file);
      return keep();
    }

    if (!CreatePipelineFor(material.fragment, file, material.pipeline))
    {
      vkDestroyShaderModule(_device->Device(), material.fragment, nullptr);
      material.fragment = VK_NULL_HANDLE;
      material.pipeline = VK_NULL_HANDLE;
      return keep();
    }

    _logger->Info("Loaded the shader {} for elements", file);
    return keep();
  }

  void VK_Renderer2D::DestroyMaterial(const int material)
  {
    if (material < 0 || static_cast<std::size_t>(material) >= _materials.size()) { return; }
    if (_device == nullptr || _device->Device() == VK_NULL_HANDLE) { return; }

    Material &kept = _materials[material];
    if (kept.pipeline == VK_NULL_HANDLE) { return; }

    // nothing may still be drawing with what is about to be destroyed
    vkDeviceWaitIdle(_device->Device());

    vkDestroyPipeline(_device->Device(), kept.pipeline, nullptr);
    vkDestroyShaderModule(_device->Device(), kept.fragment, nullptr);

    // the place stays taken, so that the numbers of the others stay true
    kept.pipeline = VK_NULL_HANDLE;
    kept.fragment = VK_NULL_HANDLE;
    kept.path.clear();
  }

  void VK_Renderer2D::FillValues(
    const VK_ShaderValues &declared,
    const std::vector<MaterialValue2D> &values,
    std::vector<unsigned char> &bytes,
    std::vector<std::string> &unknown)
  {
    bytes.assign(declared.size, 0);

    for (const auto &value : values)
    {
      const VK_ShaderValue *found = declared.Find(value.name);
      if (found == nullptr)
      {
        unknown.push_back(value.name);
        continue;
      }

      for (uint32_t i = 0; i < found->count; i++)
      {
        // a number for a vector is the number in every part of it, and a
        // colour for three numbers loses its alpha
        const float number = value.count == 1
          ? value.numbers[0]
          : (i < static_cast<uint32_t>(value.count) ? value.numbers[i] : 0.0f);

        const std::size_t at = found->offset + static_cast<std::size_t>(i) * 4;
        if (at + 4 > bytes.size()) { break; }

        if (found->is_integer)
        {
          const auto whole = static_cast<int32_t>(number);
          std::memcpy(bytes.data() + at, &whole, sizeof(whole));
        } else
        {
          std::memcpy(bytes.data() + at, &number, sizeof(number));
        }
      }
    }
  }

  void VK_Renderer2D::SetTarget(const VkCommandBuffer commands, const VkExtent2D extent)
  {
    _has_target = commands != VK_NULL_HANDLE;
    _target_commands = commands;
    _target_extent = extent;
  }

  void VK_Renderer2D::PrepareFrame()
  {
    _used_vertices = 0;
    _used_indices = 0;
    _used_shapes = 0;
    _used_values = 0;
    _has_target = false;
    _target_commands = VK_NULL_HANDLE;
  }

  void VK_Renderer2D::Draw(const Triangles2D &triangles)
  {
    if (triangles.indices.empty() || triangles.vertices.empty() || _device == nullptr) { return; }

    const VkCommandBuffer commands = _has_target ? _target_commands : _device->FrameCommands();
    if (commands == VK_NULL_HANDLE || !Prepare()) { return; }

    const VkExtent2D extent = _has_target ? _target_extent : _extent;

    const VkRect2D scissor = ScissorOf(triangles, extent);
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

    const auto shape_count = static_cast<uint32_t>(triangles.shapes.size());
    if (shape_count > kMax_Shapes - _used_shapes)
    {
      if (!_warned_about_shapes)
      {
        _warned_about_shapes = true;
        constexpr uint32_t limit = kMax_Shapes;
        _logger->Warn(
          "More than {} shapes were drawn in two dimensions in one frame, the rest is left out", limit);
      }
      return;
    }

    // the shader of an element, when it has one that can be used
    Material *material = nullptr;
    if (triangles.material >= 0 && static_cast<std::size_t>(triangles.material) < _materials.size() &&
        _materials[triangles.material].pipeline != VK_NULL_HANDLE)
    {
      material = &_materials[triangles.material];
    }

    uint32_t values_offset = 0;

    if (material != nullptr && material->values.size > 0)
    {
      if (_used_values >= kMax_Material_Draws)
      {
        if (!_warned_about_values)
        {
          _warned_about_values = true;
          constexpr uint32_t limit = kMax_Material_Draws;
          _logger->Warn(
            "More than {} calls were drawn with shaders of elements in one frame, the rest is drawn "
            "without them",
            limit);
        }
        material = nullptr;
      } else
      {
        std::vector<unsigned char> bytes;
        std::vector<std::string> unknown;
        FillValues(material->values, triangles.material_values, bytes, unknown);

        for (const auto &name : unknown)
        {
          if (std::ranges::find(material->unknown, name) != material->unknown.end()) { continue; }

          material->unknown.push_back(name);
          _logger->Warn(
            "The shader {} declares no value '{}' in its block Values, the value is left out",
            material->path, name);
        }

        values_offset = static_cast<uint32_t>(_used_values * _values_entry_size);
        std::memcpy(_values_buffer.mapped + values_offset, bytes.data(), bytes.size());
        _used_values++;
      }
    } else if (material != nullptr)
    {
      for (const auto &value : triangles.material_values)
      {
        if (std::ranges::find(material->unknown, value.name) != material->unknown.end()) { continue; }

        material->unknown.push_back(value.name);
        _logger->Warn(
          "The shader {} declares no value '{}' in its block Values, the value is left out",
          material->path, value.name);
      }
    }

    std::memcpy(_vertices + _used_vertices, triangles.vertices.data(), vertex_count * sizeof(Vertex2D));
    std::memcpy(_indices + _used_indices, triangles.indices.data(), index_count * sizeof(uint32_t));

    if (shape_count > 0)
    {
      std::memcpy(
        _shape_buffer.mapped + static_cast<std::size_t>(_used_shapes) * sizeof(Shape2D),
        triangles.shapes.data(),
        shape_count * sizeof(Shape2D));
    }

    // a texture that does not exist is drawn as plain white, which shows
    // where it is missing
    const int texture = _textures.Contains(triangles.texture) ? triangles.texture : _white_texture;
    const VkDescriptorSet set = SetOf(texture, triangles.filter);

    // rows are counted from the top here, as the places of the corners are
    const VkViewport viewport{
      0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};

    Frame frame{};
    frame.size[0] = static_cast<float>(extent.width);
    frame.size[1] = static_cast<float>(extent.height);
    frame.translation[0] = triangles.translate_x;
    frame.translation[1] = triangles.translate_y;
    frame.clip_box[0] = triangles.rounded_clip.center_x;
    frame.clip_box[1] = triangles.rounded_clip.center_y;
    frame.clip_box[2] = triangles.rounded_clip.half_width;
    frame.clip_box[3] = triangles.rounded_clip.half_height;
    std::memcpy(frame.clip_radii, triangles.rounded_clip.radii, sizeof(frame.clip_radii));
    frame.state[0] = triangles.time;
    frame.state[1] = static_cast<float>(_used_shapes);
    frame.state[2] = triangles.has_rounded_clip ? 1.0f : 0.0f;
    std::memcpy(frame.element, triangles.material_box, sizeof(frame.element));

    constexpr VkDeviceSize offset = 0;
    constexpr VkShaderStageFlags both = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    const std::array sets{set, _shapes_set, _values_set};

    vkCmdBindPipeline(
      commands, VK_PIPELINE_BIND_POINT_GRAPHICS, material != nullptr ? material->pipeline : _pipeline);
    vkCmdSetViewport(commands, 0, 1, &viewport);
    vkCmdSetScissor(commands, 0, 1, &scissor);
    vkCmdPushConstants(commands, _pipeline_layout, both, 0, sizeof(frame), &frame);
    vkCmdBindDescriptorSets(
      commands,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      _pipeline_layout,
      0,
      static_cast<uint32_t>(sets.size()),
      sets.data(),
      1,
      &values_offset);
    vkCmdBindVertexBuffers(commands, 0, 1, &_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(commands, _index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commands, index_count, 1, _used_indices, static_cast<int32_t>(_used_vertices), 0);

    _used_vertices += vertex_count;
    _used_indices += index_count;
    _used_shapes += shape_count;

    // what the models of a scene are drawn with, in case one follows
    const VkViewport scene_viewport{
      0.0f,
      static_cast<float>(extent.height),
      static_cast<float>(extent.width),
      -static_cast<float>(extent.height),
      0.0f,
      1.0f};
    const VkRect2D whole_frame{{0, 0}, extent};

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

    for (auto &material : _materials)
    {
      if (material.pipeline != VK_NULL_HANDLE) { vkDestroyPipeline(device, material.pipeline, nullptr); }
      if (material.fragment != VK_NULL_HANDLE) { vkDestroyShaderModule(device, material.fragment, nullptr); }
    }
    _materials.clear();

    DestroyFrameBuffer(_shape_buffer);
    DestroyFrameBuffer(_values_buffer);

    if (_pixelated_sampler != VK_NULL_HANDLE) { vkDestroySampler(device, _pixelated_sampler, nullptr); }
    if (_shapes_layout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device, _shapes_layout, nullptr); }
    if (_values_layout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device, _values_layout, nullptr); }

    _pixelated_sampler = VK_NULL_HANDLE;
    _shapes_layout = VK_NULL_HANDLE;
    _values_layout = VK_NULL_HANDLE;
    _shapes_set = VK_NULL_HANDLE;
    _values_set = VK_NULL_HANDLE;
    _used_shapes = 0;
    _used_values = 0;
    _has_target = false;
    _target_commands = VK_NULL_HANDLE;

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
