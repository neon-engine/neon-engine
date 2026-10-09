#include "vk-material-cache.hpp"

#include <format>

namespace neon
{
  std::string VK_MaterialCache::KeyOf(const RenderInfo &render_info, const MaterialInfo &material_info)
  {
    // every part is set apart by a bar, and a list by its count, so that
    // two keys read the same only when they are the same
    std::string key = render_info.shader_path + '|' + std::to_string(render_info.texture_paths.size());
    for (const auto &texture : render_info.texture_paths) { key += '|' + texture; }
    key += '|' + render_info.model_path;
    key += render_info.scale_textures ? "|scaled" : "|plain";
    key += render_info.mesh != nullptr ? "|mesh" : "|file";
    key += std::format(
      "|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}",
      material_info.color.r,
      material_info.color.g,
      material_info.color.b,
      material_info.color.a,
      material_info.shininess,
      material_info.use_textures,
      static_cast<int>(material_info.alpha_mode),
      material_info.MetallicWith(std::nullopt),
      material_info.RoughnessWith(std::nullopt),
      static_cast<int>(material_info.double_sided));

    // what the surface gives off, which two materials alike in all else
    // may differ in
    key += std::format(
      "|{}|{}|{}|{}|{}",
      material_info.emissive.r,
      material_info.emissive.g,
      material_info.emissive.b,
      material_info.emissive_strength,
      material_info.emissive_texture);

    // and the light that was worked out ahead for it
    key += std::format("|{}|{}", material_info.lightmap, material_info.lightmap_strength);
    return key;
  }

  int VK_MaterialCache::Find(const std::string &key)
  {
    const auto found = _shared.find(key);
    if (found == _shared.end()) { return -1; }

    // one that nothing held is held again
    if (found->second.count == 0) { _unused--; }

    found->second.count++;
    _shares++;
    return found->second.id;
  }

  int VK_MaterialCache::Keep(const std::string &key, const VK_Material &material)
  {
    const int id = _materials.Add(material);
    if (id < 0) { return -1; }

    _shared[key] = Shared{.id = id, .count = 1};
    _keys[id] = key;
    _makes++;
    return id;
  }

  void VK_MaterialCache::Release(const int id, const std::uint64_t now)
  {
    const auto key = _keys.find(id);
    if (key == _keys.end()) { return; }

    Shared &shared = _shared[key->second];
    if (shared.count == 0) { return; }

    shared.count--;
    if (shared.count > 0) { return; }

    shared.unused_since = now;
    _unused++;
  }

  int VK_MaterialCache::CountOf(const int id) const
  {
    const auto key = _keys.find(id);
    if (key == _keys.end()) { return 0; }
    return _shared.at(key->second).count;
  }
} // neon
