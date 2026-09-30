#ifndef ENTITY_QUERY_HPP
#define ENTITY_QUERY_HPP

#include <array>
#include <cstddef>
#include <vector>

#include "entity.hpp"

namespace neon
{
  /// The order a query hands its entities over in.
  enum class QueryOrder
  {
    /// Whatever is fastest for the store.
    Any = 0,

    /// An entity comes after its parent, and after the parent of that. For
    /// work that needs the result of the parent, such as placing an entity in
    /// the world.
    ParentsFirst
  };

  /// What a query looks for.
  struct QueryInfo
  {
    /// An entity matches when it carries all of these.
    std::vector<ComponentId> components;

    QueryOrder order = QueryOrder::Any;
  };

  /// A run of entities that matched a query, with their components.
  ///
  /// The components of one kind lie next to each other in memory, so a system
  /// loops over plain arrays. A query hands over as many blocks as the store
  /// keeps its entities in. The pointers are valid during the call only.
  struct EntityBlock
  {
    /// The most components one query can ask for.
    // ReSharper disable once CppInconsistentNaming
    static constexpr std::size_t Max_Components = 8;

    /// Number of entities in the block.
    std::size_t count = 0;

    const Entity *entities = nullptr;

    /// The parent that all entities in the block share. No_Entity when they
    /// have none. Only filled in for QueryOrder::ParentsFirst.
    Entity parent = No_Entity;

    /// One array for each component of the query, in the order the query
    /// named them. Each holds `count` components.
    std::array<void *, Max_Components> columns{};

    template<typename T>
    [[nodiscard]] T *Column(const std::size_t index) const
    {
      return static_cast<T *>(columns[index]);
    }
  };
} // neon

#endif //ENTITY_QUERY_HPP
