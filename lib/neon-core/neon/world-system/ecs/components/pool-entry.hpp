#ifndef POOL_ENTRY_HPP
#define POOL_ENTRY_HPP

#include <string>

namespace neon
{
  /// One kind of thing a PoolManager is asked to hold, as a recipe writes it under
  /// `_entries`: where its instances come from, and how many of them.
  struct PoolEntry
  {
    /// A prefab, as `assets://prefabs/nail.prefab.yml`, of which `count`
    /// instances are spawned; or an entity that is there already, as
    /// `instance://weapons/nail`, which is taken as the one instance.
    std::string source;

    int count = 0;
  };
} // neon

#endif //POOL_ENTRY_HPP
