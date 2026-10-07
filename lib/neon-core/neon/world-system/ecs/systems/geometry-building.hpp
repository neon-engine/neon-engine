#ifndef GEOMETRY_BUILDING_HPP
#define GEOMETRY_BUILDING_HPP

#include <cstddef>
#include <map>
#include <memory>
#include <string>

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
  /// Entities whose Geometry has the same values share one mesh, which is
  /// built for the first of them: the crates a prefab spawns, or the walls
  /// a scene places. The mesh goes with its key, see KeyOf(), so that the
  /// renderer draws them all as one model.
  ///
  /// An application adds it to the world; it registers the component:
  ///
  ///     world.AddSystem(std::make_unique<neon::GeometryBuilding>(logger));
  class GeometryBuilding final : public EntitySystem
  {
    std::shared_ptr<Logger> _logger;
    QueryId _query = 0;

    // The meshes that were built, by their keys, for as long as an entity
    // holds one. What nothing holds any more is forgotten once there are
    // twice as many as were held at the last sweep.
    std::map<std::string, std::weak_ptr<const MeshData>> _meshes;
    std::size_t _sweep_at = 64;

    /// The mesh of a Geometry: the one that was built already for the same
    /// values, or a new one.
    std::shared_ptr<const MeshData> MeshOf(const Geometry &geometry, const std::string &key);

  public:
    explicit GeometryBuilding(const std::shared_ptr<Logger> &logger);

    /// The mesh a Geometry describes, in metres, with its normals and
    /// texture coordinates. Empty, and said in the log, when the component
    /// cannot be built, such as a prism with too few points.
    static MeshData Build(const Geometry &geometry);

    /// What a Geometry is known by: every value of it, written out exactly,
    /// so that two Geometry components have the same key when, and only
    /// when, they build the same mesh. Starts with `geometry:`, which no
    /// path of a model file does, and joins its parts with underscores, so
    /// it has no spaces.
    static std::string KeyOf(const Geometry &geometry);

    void Register(EntityStore &store) override;

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //GEOMETRY_BUILDING_HPP
