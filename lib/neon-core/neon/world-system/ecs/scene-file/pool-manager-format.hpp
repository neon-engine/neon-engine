#ifndef POOL_MANAGER_FORMAT_HPP
#define POOL_MANAGER_FORMAT_HPP

#include "component-format.hpp"

namespace neon
{
  /// How a PoolManager is written in a scene file: by what it is to hold.
  ///
  ///     PoolManager:
  ///       _entries:
  ///         - source: assets://prefabs/nail.prefab.yml
  ///           count: 64
  ///
  /// `_entries` is read once, when the pool is filled, and is no field of
  /// the component, which the line in front of its name says: a script and
  /// an extension ask PoolSystem what a pool holds. The format is written by
  /// hand, since a description has no list of things with several values.
  [[nodiscard]] ComponentFormat PoolManagerFormat();
} // neon

#endif //POOL_MANAGER_FORMAT_HPP
