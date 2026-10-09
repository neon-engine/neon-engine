#ifndef ENTITY_LOGGING_HPP
#define ENTITY_LOGGING_HPP

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <neon/logging/logger.hpp>
#include <neon/physics/physics-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Logs where entities are, once in every frame, so that a check of where
  /// the player went reads a line of the log in place of the pixels of a
  /// frame. It is what `--log-entity` adds to the world.
  ///
  /// Each entity is named by its path from the top of the world, as a scene
  /// names it, such as `player` or `crates/upper`, and looked for in every
  /// frame, so that one that is spawned later or comes with another scene
  /// is logged from the frame it is there. A line at Info level says the
  /// frame, counted from 1, and where the entity is drawn in the world:
  ///
  ///   Frame 20: player is at [0.000, 0.900, 3.767]
  ///
  /// When the entity is a body of the physics, through a RigidBody or a
  /// CharacterBody, the line also says where the physics has the body,
  /// which is where the last step left it and not blended between the last
  /// two steps as what is drawn is:
  ///
  ///   Frame 20: player is at [0.000, 0.900, 3.767], its body at [0.000, 0.900, 3.700]
  ///
  /// A path that names no entity is said once, as a warning, and not in
  /// every frame; once the entity is there it is logged, and if it goes away
  /// again, that is said once more. So is an entity without a Transform,
  /// which has no place.
  ///
  /// It is added after placing, so that it sees every entity where it is
  /// drawn in the frame. It moves nothing.
  class EntityLogging final : public EntitySystem
  {
    std::vector<std::string> _paths;
    PhysicsContext *_physics;
    std::shared_ptr<Logger> _logger;

    // the kinds of component that are read, by name, so that a world
    // without physics has none of the bodies and nothing is refused
    ComponentId _transform = No_Component;
    ComponentId _rigid_body = No_Component;
    ComponentId _character_body = No_Component;

    std::size_t _frame = 0;

    // the paths that named no entity, and those whose entity had no place,
    // when they were last looked for, which were said once
    std::set<std::string> _missing;
    std::set<std::string> _placeless;

    /// Where the physics has the body of the entity. False when it is no
    /// body, or there is no physics to ask.
    bool FindBody(EntityStore &store, Entity entity, glm::vec3 &position) const;

  public:
    /// `physics` may be nullptr, and then only where the entities are drawn
    /// is logged.
    EntityLogging(std::vector<std::string> paths, PhysicsContext *physics, std::shared_ptr<Logger> logger);

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //ENTITY_LOGGING_HPP
