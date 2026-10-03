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

    /// Copies `size` bytes over what `memory` holds.
    bool Overwrite(const void *data, VkDeviceSize size, VkDeviceMemory memory) const;

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

    /// Draws the mesh into `commands` `instances` times over, the shaders
    /// told which object each is by gl_InstanceIndex, counted from
    /// `first_instance`. The shadow pass records apart from the frame.
    void Draw(VkCommandBuffer commands, uint32_t instances = 1, uint32_t first_instance = 0) const;

    /// Draws these vertices and indices from now on. As many of both as
    /// before are written over the old ones, which is what a mesh that
    /// moves every frame costs; any other number makes the buffers anew.
    /// Called between frames, when nothing is being drawn. Returns false
    /// when the buffers could not be made, and the mesh is then not drawn.
    bool Rewrite(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices);

    void CleanUp() override;
  };
} // neon

#endif //VK_MESH_HPP
