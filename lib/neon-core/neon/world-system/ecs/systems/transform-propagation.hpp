#ifndef TRANSFORM_PROPAGATION_HPP
#define TRANSFORM_PROPAGATION_HPP

#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Places every entity in the world. The position, rotation, and scale of
  /// a Transform are relative to the parent of the entity. This works out
  /// where that puts the entity in the world, parents before children.
  class TransformPropagation final : public EntitySystem
  {
    QueryId _query = 0;

  public:
    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //TRANSFORM_PROPAGATION_HPP
