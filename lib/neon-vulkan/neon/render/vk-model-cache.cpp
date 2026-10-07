#include "vk-model-cache.hpp"

#include <string>

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
    // a model that is held already is shared, whether it is a file or a
    // mesh that was built from the same values
    const std::optional<ModelKey> key = ModelKey::Of(render_info);
    if (key.has_value())
    {
      if (const int id = _shared.Take(*key); id >= 0)
      {
        const int count = _shared.CountOf(id);
        _logger->Debug("Model {} is shared, {} render objects draw it now", key->source, count);
        return id;
      }
    }

    const bool is_built = render_info.mesh != nullptr;
    VK_Model model = is_built
                       ? VK_Model(render_info.mesh, _device, _logger)
                       : VK_Model(render_info.model_path, render_info.fit, _file_system_context, _device, _logger);
    if (!model.Initialize())
    {
      if (is_built) { _logger->Error("Could not initialize model the mesh that was built"); }
      else { _logger->Error("Could not initialize model {}", render_info.model_path); }
      return -1;
    }

    const int id = _models.Add(model);
    if (id < 0)
    {
      _logger->Error("There is no room for another model");
      model.CleanUp();
      return -1;
    }

    if (key.has_value())
    {
      _loads++;
      _shared.Add(*key, id);
    }
    return id;
  }

  void VK_ModelCache::Release(const int id)
  {
    if (!_models.Contains(id)) { return; }

    // a mesh that was built without a key is its render object's alone
    const ModelKey *key = _shared.KeyOf(id);
    const std::string source = key != nullptr ? key->source : "the mesh that was built";
    if (!_shared.Release(id)) { return; }

    _logger->Debug("Model {} was freed, nothing draws it any more", source);
    Free(id);
  }

  void VK_ModelCache::UpdateMesh(const int id, const MeshData &mesh)
  {
    if (!_models.Contains(id)) { return; }

    // what others draw as well stays as it is
    if (_shared.Contains(id))
    {
      _logger->Error("Model {} is shared by its key, and its mesh is not changed", id);
      return;
    }

    _models[id].UpdateMesh(mesh);
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
    _shared.Clear();
  }
} // neon
