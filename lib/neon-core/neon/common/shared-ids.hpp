#ifndef SHARED_IDS_HPP
#define SHARED_IDS_HPP

#include <cstddef>
#include <map>

namespace neon
{
  /// Which things are shared, by what they are known by, and how many hold
  /// each. A cache keeps its things where it likes and hands their ids to
  /// this: the first that asks for a key makes the thing and adds its id,
  /// every other takes the same id, and the last that gives it back frees
  /// it. The key is whatever tells two things apart: the path of a file,
  /// or the values a thing was built from.
  ///
  /// An id that was never added is nobody else's: giving it back frees it.
  ///
  /// Key has to be ordered with operator<.
  template<typename Key>
  class SharedIds
  {
    /// Where a shared thing is kept, and how many hold it.
    struct Shared
    {
      int id = -1;
      int count = 0;
    };

    std::map<Key, Shared> _shared;

    // the key of every shared id, for giving it back
    std::map<int, Key> _keys;

    // how many times an id was taken that was added already
    std::size_t _shares = 0;

  public:
    /// The id kept under `key`, counted as held once more, or -1 when
    /// nothing is kept under it.
    int Take(const Key &key)
    {
      const auto it = _shared.find(key);
      if (it == _shared.end()) { return -1; }

      it->second.count++;
      _shares++;
      return it->second.id;
    }

    /// Keeps `id` under `key`, held once, by whoever made it.
    void Add(const Key &key, const int id)
    {
      _shared[key] = Shared{.id = id, .count = 1};
      _keys[id] = key;
    }

    /// Gives `id` back once. Returns whether nothing holds it any more, so
    /// that it is to be freed; it is then forgotten. An id that was never
    /// added is to be freed at once.
    bool Release(const int id)
    {
      const auto key = _keys.find(id);
      if (key == _keys.end()) { return true; }

      auto &shared = _shared[key->second];
      if (--shared.count > 0) { return false; }

      _shared.erase(key->second);
      _keys.erase(key);
      return true;
    }

    /// The key `id` was added under, or nullptr when it was never added.
    [[nodiscard]] const Key *KeyOf(const int id) const
    {
      const auto key = _keys.find(id);
      return key == _keys.end() ? nullptr : &key->second;
    }

    /// Whether `id` was added under a key and is held.
    [[nodiscard]] bool Contains(const int id) const { return _keys.contains(id); }

    /// How many hold `id`: 0 when it was never added.
    [[nodiscard]] int CountOf(const int id) const
    {
      const auto key = _keys.find(id);
      return key == _keys.end() ? 0 : _shared.at(key->second).count;
    }

    /// How many ids are shared.
    [[nodiscard]] std::size_t Size() const { return _keys.size(); }

    /// How many times an id was taken that was added already.
    [[nodiscard]] std::size_t Shares() const { return _shares; }

    /// Forgets every id, whatever held it. The count of shares stays.
    void Clear()
    {
      _shared.clear();
      _keys.clear();
    }
  };
} // neon

#endif //SHARED_IDS_HPP
