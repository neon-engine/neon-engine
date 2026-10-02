#ifndef GEOMETRY_BUILDING_HPP
#define GEOMETRY_BUILDING_HPP

#include <memory>

#include <neon/geometry/mesh-data.hpp>
#include <neon/logging/logger.hpp>
#include <neon/world-system/ecs/components/geometry.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Builds the mesh of every entity that carries a Geometry, once, and
  /// hands it to the entity's Renderable, which the renderer then draws in
  /// place of a model. The physics builds the same shape for a Collider
  /// without a model through Build(), so that what is drawn is what collides.
  ///
  /// An application adds it to the world; it registers the component:
  ///
  ///     world.AddSystem(std::make_unique<neon::GeometryBuilding>(logger));
  class GeometryBuilding final : public EntitySystem
  {
    std::shared_ptr<Logger> _logger;
    QueryId _query = 0;

  public:
    explicit GeometryBuilding(const std::shared_ptr<Logger> &logger);

    /// The mesh a Geometry describes, in metres, with its normals and
    /// texture coordinates. Empty, and said in the log, when the component
    /// cannot be built, such as a prism with too few points.
    static MeshData Build(const Geometry &geometry);

    void Register(EntityStore &store) override;

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //GEOMETRY_BUILDING_HPP
