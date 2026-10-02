#include "vk-model-cache.hpp"

namespace neon
{
  void VK_ModelCache::Initialize(
    FileSystemContext *file_system_context,
    VK_Device *device,
    const std::shared_ptr<Logger> &logger)
  {
    _file_system_context = file_system_context;
    _device = device;
    _logger = logger;
  }

  int VK_ModelCache::Acquire(const RenderInfo &render_info)
  {
    // a mesh that was built is drawn as it is, and is nobody else's
    if (render_info.mesh != nullptr)
    {
      VK_Model model(render_info.mesh, _device, _logger);
      if (!model.Initialize())
      {
        _logger->Error("Could not initialize model the mesh that was built");
        return -1;
      }

      const int id = _models.Add(model);
      if (id < 0)
      {
        _logger->Error("There is no room for another model");
        model.CleanUp();
      }
      return id;
    }

    const Key key{render_info.model_path, render_info.fit};
    if (const auto it = _shared.find(key); it != _shared.end())
    {
      it->second.count++;
      _shares++;
      _logger->Debug("Model {} is shared, {} render objects draw it now", render_info.model_path, it->second.count);
      return it->second.id;
    }

    VK_Model model(render_info.model_path, render_info.fit, _file_system_context, _device, _logger);
    if (!model.Initialize())
    {
      _logger->Error("Could not initialize model {}", render_info.model_path);
      return -1;
    }

    const int id = _models.Add(model);
    if (id < 0)
    {
      _logger->Error("There is no room for another model");
      model.CleanUp();
      return -1;
    }

    _loads++;
    _shared[key] = Shared{.id = id, .count = 1};
    _keys[id] = key;
    return id;
  }

  void VK_ModelCache::Release(const int id)
  {
    if (!_models.Contains(id)) { return; }

    const auto key = _keys.find(id);
    if (key == _keys.end())
    {
      // a mesh that was built, which its render object alone drew
      Free(id);
      return;
    }

    auto &shared = _shared[key->second];
    if (--shared.count > 0) { return; }

    _logger->Debug("Model {} was freed, nothing draws it any more", key->second.first);
    _shared.erase(key->second);
    _keys.erase(key);
    Free(id);
  }

  void VK_ModelCache::Free(const int id)
  {
    _models.Remove(id).CleanUp();
  }

  void VK_ModelCache::CleanUp()
  {
    for (int id = 0; id < _models.Capacity(); id++)
    {
      if (_models.Contains(id)) { Free(id); }
    }
    _shared.clear();
    _keys.clear();
  }
} // neon
