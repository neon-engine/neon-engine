#ifndef POOL_KIND_HPP
#define POOL_KIND_HPP

#include <cstddef>
#include <deque>
#include <string>
#include <vector>

#include <neon/world-system/ecs/entity.hpp>

namespace neon
{
  /// One kind of thing a PoolManager holds while the game runs: every instance of
  /// it, and those that wait to be handed out, in the order they came back.
  struct PoolKind
  {
    /// What the kind is asked for by: the path of its prefab, or
    /// `instance://` and a name for instances that were made in code.
    std::string name;

    std::vector<Entity> instances;

    /// A queue: what was given back first is handed out first.
    std::deque<Entity> free;

    /// How often one was asked for in vain since that was last said, and
    /// whether it was said: once, until one comes back.
    std::size_t missed = 0;
    bool said_empty = false;
  };
} // neon

#endif //POOL_KIND_HPP
