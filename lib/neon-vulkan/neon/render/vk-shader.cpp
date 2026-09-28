#include "vk-shader.hpp"

#include <cstring>
#include <vector>

namespace neon
{
  VK_Shader::VK_Shader(
    const std::string &shader_path,
    FileSystemContext *file_system_context,
    VK_Device *device,
    const std::shared_ptr<Logger> &logger)
  {
    _shader_path = shader_path;
    _file_system_context = file_system_context;
    _device = device;
    _logger = logger;
  }

  bool VK_Shader::LoadModule(const std::string &path, VkShaderModule &module) const
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

  bool VK_Shader::Initialize()
  {
    _logger->Info("Initializing shader from {}", _shader_path);

    if (!LoadModule(_shader_path + ".vert.spv", _vertex) ||
        !LoadModule(_shader_path + ".frag.spv", _fragment))
    {
      CleanUp();
      return false;
    }
    return true;
  }

  void VK_Shader::CleanUp()
  {
    if (_device == nullptr) { return; }

    if (_vertex != VK_NULL_HANDLE) { vkDestroyShaderModule(_device->Device(), _vertex, nullptr); }
    if (_fragment != VK_NULL_HANDLE) { vkDestroyShaderModule(_device->Device(), _fragment, nullptr); }

    _vertex = VK_NULL_HANDLE;
    _fragment = VK_NULL_HANDLE;
  }
} // neon
