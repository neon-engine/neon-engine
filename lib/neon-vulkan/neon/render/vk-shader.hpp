#ifndef VK_SHADER_HPP
#define VK_SHADER_HPP

#include <string>
#include <neon/filesystem/file-system-context.hpp>

#include "vk-device.hpp"

namespace neon
{
  /// The two compiled halves of a shader. A shader is named without an
  /// extension, such as `assets://shaders/basic-lit`, and read from the files
  /// with `.vert.spv` and `.frag.spv` added, which the build produces.
  // ReSharper disable once CppInconsistentNaming
  class VK_Shader
  {
    std::string _shader_path;
    VK_Device *_device = nullptr;
    FileSystemContext *_file_system_context = nullptr;
    std::shared_ptr<Logger> _logger;
    VkShaderModule _vertex = VK_NULL_HANDLE;
    VkShaderModule _fragment = VK_NULL_HANDLE;

    bool LoadModule(const std::string &path, VkShaderModule &module) const;

  public:
    VK_Shader() = default;

    VK_Shader(
      const std::string &shader_path,
      FileSystemContext *file_system_context,
      VK_Device *device,
      const std::shared_ptr<Logger> &logger);

    bool Initialize();

    void CleanUp();

    [[nodiscard]] VkShaderModule Vertex() const { return _vertex; }
    [[nodiscard]] VkShaderModule Fragment() const { return _fragment; }
  };
} // neon

#endif //VK_SHADER_HPP
