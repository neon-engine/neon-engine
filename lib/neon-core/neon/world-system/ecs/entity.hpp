#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <cstddef>
#include <cstdint>

namespace neon
{
  /// Names one thing in the world. An entity holds no data and has no
  /// behavior. What it is follows from the components it carries.
  using Entity = std::uint64_t;

  /// Names a kind of component, as returned when it was registered.
  using ComponentId = std::uint64_t;

  /// Names a query, as returned when it was created.
  using QueryId = std::size_t;

  /// Stands for no entity, such as the parent of an entity that has none.
  // ReSharper disable once CppInconsistentNaming
  inline constexpr Entity No_Entity = 0;

  /// Stands for a component that was never registered.
  // ReSharper disable once CppInconsistentNaming
  inline constexpr ComponentId No_Component = 0;

  /// Stands for a query that could not be made. The reason was logged when
  /// it was asked for, and EntityStore::Each() hands over nothing for it.
  // ReSharper disable once CppInconsistentNaming
  inline constexpr QueryId No_Query = static_cast<QueryId>(-1);
} // neon

#endif //ENTITY_HPP
