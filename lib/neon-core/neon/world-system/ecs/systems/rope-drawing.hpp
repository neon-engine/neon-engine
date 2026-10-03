#ifndef ROPE_DRAWING_HPP
#define ROPE_DRAWING_HPP

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <neon/logging/logger.hpp>
#include <neon/world-system/ecs/components/rope.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Draws every Rope: finds where its two ends are in the frame that is
  /// about to be drawn, lays the curve a rope of its length hangs as
  /// between them, and writes that as a tube into the mesh of the entity's
  /// Renderable. A rope whose ends did not move is left as it is.
  ///
  /// It works once every entity is placed where it is drawn, in
  /// Interpolate(), so that a rope ends on a body where the body is seen
  /// and not where it was a step ago. An application adds it after the
  /// physics, which places the bodies:
  ///
  ///     world.AddSystem(std::make_unique<neon::RopeDrawing>(logger));
  class RopeDrawing final : public EntitySystem
  {
    std::shared_ptr<Logger> _logger;
    QueryId _query = 0;

    /// Whether a Joint can be asked for: the physics brings the component.
    bool _joints_known = false;

    /// The curve as straight pieces, and the middle of every ring of the
    /// tube. Kept from rope to rope and frame to frame for their room.
    std::vector<glm::vec3> _flat;
    std::vector<glm::vec3> _centres;

    /// Where the two ends of the rope are in the world, and how long it
    /// is. False, and said in the log once, when an end cannot be found.
    bool FindEnds(EntityStore &store, Entity entity, Rope &rope, glm::vec3 &from, glm::vec3 &to, float &length) const;

    /// Where `anchor` on the entity at `path` is in the world, or `anchor`
    /// itself when the path is empty. `found` remembers the entity.
    bool Place(
      EntityStore &store,
      const std::string &path,
      const glm::vec3 &anchor,
      Entity &found,
      glm::vec3 &place) const;

  public:
    explicit RopeDrawing(const std::shared_ptr<Logger> &logger);

    void Register(EntityStore &store) override;

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override {}

    void Interpolate(EntityStore &store, double blend) override;
  };
} // neon

#endif //ROPE_DRAWING_HPP
