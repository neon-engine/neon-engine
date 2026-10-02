#include "vk-mesh.hpp"

#include <cstring>

namespace neon
{
  VK_Mesh::VK_Mesh(
    const std::vector<Vertex> &vertices,
    const std::vector<unsigned int> &indices,
    const std::vector<TextureInfo> &textures,
    VK_Device *device,
    const std::shared_ptr<Logger> &logger) : Mesh(vertices, indices, textures, logger)
  {
    _device = device;
  }

  bool VK_Mesh::Upload(
    const void *data,
    const VkDeviceSize size,
    const VkBufferUsageFlags usage,
    VkBuffer &buffer,
    VkDeviceMemory &memory) const
  {
    // Memory the processor can write to directly. Simple, and fast enough for
    // data that is written once. Copying it on to memory owned by the
    // graphics card would be the next step if it ever shows in a profile.
    if (!_device->CreateBuffer(
      size,
      usage,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      buffer,
      memory))
    {
      return false;
    }

    void *mapped = nullptr;
    if (vkMapMemory(_device->Device(), memory, 0, size, 0, &mapped) != VK_SUCCESS) { return false; }

    std::memcpy(mapped, data, size);
    vkUnmapMemory(_device->Device(), memory);
    return true;
  }

  bool VK_Mesh::Initialize()
  {
    if (_initialized)
    {
      _logger->Warn("Mesh was already initialized");
      return true;
    }

    if (_vertices.empty() || _indices.empty())
    {
      _logger->Error("Mesh has no vertices or no indices");
      return false;
    }

    if (!Upload(
          _vertices.data(),
          _vertices.size() * sizeof(Vertex),
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
          _vertex_buffer,
          _vertex_memory) ||
        !Upload(
          _indices.data(),
          _indices.size() * sizeof(unsigned int),
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
          _index_buffer,
          _index_memory))
    {
      _logger->Error("Could not upload a mesh");
      CleanUp();
      return false;
    }

    _initialized = true;
    return true;
  }

  void VK_Mesh::Use() const
  {
    Draw(_device->FrameCommands());
  }

  void VK_Mesh::Draw(const VkCommandBuffer commands) const
  {
    if (commands == VK_NULL_HANDLE || !_initialized) { return; }

    constexpr VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commands, 0, 1, &_vertex_buffer, &offset);
    vkCmdBindIndexBuffer(commands, _index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commands, static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);
  }

  void VK_Mesh::CleanUp()
  {
    const VkDevice device = _device->Device();

    if (_vertex_buffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, _vertex_buffer, nullptr); }
    if (_vertex_memory != VK_NULL_HANDLE) { vkFreeMemory(device, _vertex_memory, nullptr); }
    if (_index_buffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, _index_buffer, nullptr); }
    if (_index_memory != VK_NULL_HANDLE) { vkFreeMemory(device, _index_memory, nullptr); }

    _vertex_buffer = VK_NULL_HANDLE;
    _vertex_memory = VK_NULL_HANDLE;
    _index_buffer = VK_NULL_HANDLE;
    _index_memory = VK_NULL_HANDLE;
    _initialized = false;
  }
} // neon
