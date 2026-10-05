#ifndef POOL_MANAGER_HPP
#define POOL_MANAGER_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "pool-entry.hpp"
#include "pool-kind.hpp"

namespace neon
{
  /// Makes an entity a pool: it holds a fixed number of instances of one or
  /// more kinds of thing, made ahead and turned off, and hands them out and
  /// takes them back, so that what comes and goes often, a projectile, a
  /// spark, is never made or destroyed while the game runs.
  ///
  /// An instance that waits has its Renderable, its Collider, and its
  /// RigidBody turned off, on itself and on what is below it. One that is
  /// handed out has them turned on. A pool never makes more than it was
  /// told to hold: when all of a kind are out it hands out nothing, and
  /// says so. See PoolSystem for what is asked of a pool.
  struct PoolManager
  {
    /// What the pool is to hold, as its recipe says. Read once, when the
    /// pool is filled. It is not a field: a recipe writes it as `_entries`,
    /// and what is in a pool is asked of PoolSystem.
    std::vector<PoolEntry> entries;

    /// What it holds. Filled in by the engine.
    std::vector<PoolKind> kinds;

    /// Whether the entries were made into instances. Filled in by the engine.
    bool is_filled = false;

    /// The kinds that were asked for and that the pool does not hold, each
    /// once, and how many of them were said. Filled in by the engine.
    std::vector<std::string> unknown;
    std::size_t unknown_said = 0;
  };
} // neon

#endif //POOL_MANAGER_HPP
