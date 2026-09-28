#ifndef VK_MESH_HPP
#define VK_MESH_HPP

#include <neon/render/mesh.hpp>

#include "vk-device.hpp"

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class VK_Mesh final : public Mesh
  {
    VK_Device *_device;
    VkBuffer _vertex_buffer = VK_NULL_HANDLE;
    VkDeviceMemory _vertex_memory = VK_NULL_HANDLE;
    VkBuffer _index_buffer = VK_NULL_HANDLE;
    VkDeviceMemory _index_memory = VK_NULL_HANDLE;

    bool Upload(const void *data, VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &memory) const;

  public:
    VK_Mesh(
      const std::vector<Vertex> &vertices,
      const std::vector<unsigned int> &indices,
      const std::vector<TextureInfo> &textures,
      VK_Device *device,
      const std::shared_ptr<Logger> &logger);

    bool Initialize() override;

    /// Draws the mesh as part of the frame that is being recorded.
    void Use() const override;

    void CleanUp() override;
  };
} // neon

#endif //VK_MESH_HPP
