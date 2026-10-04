#include "vk-texture-cache.hpp"

namespace neon
{
  void VK_TextureCache::Initialize(
    FileSystemContext *file_system_context,
    VK_Device *device,
    const std::shared_ptr<Logger> &logger)
  {
    _file_system_context = file_system_context;
    _device = device;
    _logger = logger;
  }

  std::string VK_TextureCache::KeyOf(
    const std::string &path,
    const std::string &model_path,
    const VK_TextureOptions &options)
  {
    // the same image kept two ways, as colours and as numbers, is two
    // textures
    std::string key = model_path.empty() ? path : model_path + "#" + path;
    key += options.is_color ? "|color" : "|data";
    if (!options.mip_levels) { key += "|flat"; }
    if (!options.repeat) { key += "|clamp"; }
    if (options.premultiply_alpha) { key += "|premultiplied"; }
    return key;
  }

  bool VK_TextureCache::Acquire(
    const std::string &key,
    const std::string &path,
    const std::shared_ptr<const std::vector<unsigned char>> &file,
    const VK_TextureOptions &options,
    VK_Texture &texture)
  {
    if (const auto it = _shared.find(key); it != _shared.end())
    {
      it->second.count++;
      _shares++;
      _logger->Debug("Texture {} is shared, {} materials read it now", path, it->second.count);
      texture = it->second.texture;
      return true;
    }

    VK_Texture made(path, _file_system_context, _device, _logger);
    const bool loaded = file != nullptr ? made.InitializeWithFile(*file, options) : made.Initialize(options);
    if (!loaded) { return false; }

    _loads++;
    _shared[key] = Shared{.texture = made, .count = 1};
    texture = made;
    return true;
  }

  bool VK_TextureCache::AcquirePixels(
    const std::string &key,
    const std::string &path,
    const ImagePixels &pixels,
    const VK_TextureOptions &options,
    VK_Texture &texture)
  {
    if (const auto it = _shared.find(key); it != _shared.end())
    {
      it->second.count++;
      _shares++;
      _logger->Debug("Texture {} is shared, {} materials read it now", path, it->second.count);
      texture = it->second.texture;
      return true;
    }

    VK_Texture made(path, _file_system_context, _device, _logger);
    if (!made.InitializeWithPixels(
      pixels.pixels.data(),
      static_cast<uint32_t>(pixels.width),
      static_cast<uint32_t>(pixels.height),
      options))
    {
      return false;
    }

    _loads++;
    _shared[key] = Shared{.texture = made, .count = 1};
    texture = made;
    return true;
  }

  void VK_TextureCache::Release(const std::string &key)
  {
    const auto it = _shared.find(key);
    if (it == _shared.end()) { return; }

    if (--it->second.count > 0) { return; }

    _logger->Debug("Texture {} was freed, nothing reads it any more", key);
    it->second.texture.CleanUp();
    _shared.erase(it);
  }

  void VK_TextureCache::CleanUp()
  {
    for (auto &[key, shared] : _shared) { shared.texture.CleanUp(); }
    _shared.clear();
  }
} // neon
