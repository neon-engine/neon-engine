#ifndef POOL_INSTANCE_HPP
#define POOL_INSTANCE_HPP

#include <cstddef>

#include <neon/world-system/ecs/entity.hpp>

namespace neon
{
  /// Marks an entity as an instance of a pool: which pool it belongs to,
  /// which kind of it, and whether it is handed out. Given by the engine
  /// when a pool takes an instance in.
  struct PoolInstance
  {
    Entity pool = No_Entity;
    std::size_t kind = 0;
    bool is_out = false;
  };
} // neon

#endif //POOL_INSTANCE_HPP
