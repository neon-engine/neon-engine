#ifndef VK_MATERIAL_CACHE_HPP
#define VK_MATERIAL_CACHE_HPP

#include <cstdint>
#include <cstddef>
#include <map>
#include <string>

#include <neon/common/data-buffer.hpp>
#include <neon/render/material-info.hpp>
#include <neon/render/render-info.hpp>

#include "vk-material.hpp"

namespace neon
{
  /// The materials the render objects draw with, held once each.
  ///
  /// Two render objects whose material is the same, the same shader, the
  /// same textures, the same color and the rest of the material, over the
  /// same model, draw with one material: one descriptor set, one set of
  /// textures. The first one makes it, the others take a reference.
  ///
  /// A material that nothing holds any more is kept, with its textures,
  /// until FreeUnused() is asked for: when a scene is over, when a game
  /// says so, or when there is no room. An entity that shows one texture
  /// after another, a face that blinks, a button that lights up, goes back
  /// and forth between materials that are held already: nothing is read
  /// from a file and nothing is sent to the graphics card for a texture it
  /// showed before. It is how a game with levels loads: what a level
  /// showed stays until the level is over.
  ///
  /// The cache keeps the books and the materials; making a material needs
  /// the device, and stays with the render system. A material is known by
  /// an id that stays the same while it is held, which a render object
  /// keeps as its material id.
  // ReSharper disable once CppInconsistentNaming
  class VK_MaterialCache
  {
    /// What is known about a material: where it is held, and how many
    /// render objects hold it.
    struct Shared
    {
      int id = -1;
      int count = 0;

      /// For one that nothing holds: the frame it was last released in.
      std::uint64_t unused_since = 0;
    };

    DataBuffer<VK_Material> _materials;
    std::map<std::string, Shared> _shared;

    // the key of every material by its id, for releasing
    std::map<int, std::string> _keys;

    // how many materials were made, and how many times one that was made
    // already was taken instead
    std::size_t _makes = 0;
    std::size_t _shares = 0;

    // how many of the materials nothing holds
    int _unused = 0;

  public:
    explicit VK_MaterialCache(int capacity) : _materials(capacity) {}

    /// What tells two materials apart: everything a render object's
    /// material is made from. Two render objects with the same key draw
    /// with one material.
    [[nodiscard]] static std::string KeyOf(const RenderInfo &render_info, const MaterialInfo &material_info);

    /// Takes a reference to the material of the key when one is held, and
    /// gives its id; -1 when none is held yet, which is when the render
    /// system makes one and keeps it.
    [[nodiscard]] int Find(const std::string &key);

    /// Keeps a material that was made for the key, held once. Returns its
    /// id, or -1 when there is no room.
    int Keep(const std::string &key, const VK_Material &material);

    /// Gives a material back, in the frame `now`. When nothing holds it any
    /// more it stays where it is, to be taken again by Find() or freed by
    /// FreeUnused().
    void Release(int id, std::uint64_t now);

    /// Takes out every material that nothing has held since the frame
    /// `before`, each handed to `clean_up`. Returns how many.
    template<typename CleanUp>
    int FreeUnused(const std::uint64_t before, CleanUp clean_up)
    {
      if (_unused == 0) { return 0; }

      int freed = 0;
      for (auto shared = _shared.begin(); shared != _shared.end();)
      {
        if (shared->second.count > 0 || shared->second.unused_since >= before)
        {
          ++shared;
          continue;
        }

        const int id = shared->second.id;
        _keys.erase(id);
        shared = _shared.erase(shared);
        clean_up(_materials.Remove(id));
        _unused--;
        freed++;
      }
      return freed;
    }

    /// How many materials nothing holds, which are kept for now.
    [[nodiscard]] int UnusedCount() const { return _unused; }

    /// Takes every material out, whatever still held one, each handed to
    /// `clean_up`.
    template<typename CleanUp>
    void RemoveAll(CleanUp clean_up)
    {
      for (int id = 0; id < _materials.Capacity(); id++)
      {
        if (_materials.Contains(id)) { clean_up(_materials.Remove(id)); }
      }
      _shared.clear();
      _keys.clear();
      _unused = 0;
    }

    [[nodiscard]] bool Contains(const int id) const { return _materials.Contains(id); }

    [[nodiscard]] VK_Material &operator[](const int id) { return _materials[id]; }

    [[nodiscard]] const VK_Material &operator[](const int id) const { return _materials[id]; }

    [[nodiscard]] int Capacity() const { return _materials.Capacity(); }

    /// How many materials are held.
    [[nodiscard]] int Size() const { return _materials.Size(); }

    /// How many render objects hold the material, or 0 for an id that
    /// holds none.
    [[nodiscard]] int CountOf(int id) const;

    /// How many materials were made since the cache was made.
    [[nodiscard]] std::size_t Makes() const { return _makes; }

    /// How many times a render object took a material that was made already.
    [[nodiscard]] std::size_t Shares() const { return _shares; }
  };
} // neon

#endif //VK_MATERIAL_CACHE_HPP
