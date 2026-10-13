#ifndef POSSESSION_HPP
#define POSSESSION_HPP

#include <memory>
#include <string>
#include <vector>

#include <neon/logging/logger.hpp>
#include <neon/world-system/possession-listener.hpp>

#include "entity-store.hpp"

namespace neon
{
  /// Who drives in the world, see docs/possession.md: the one entity that
  /// is possessed, the input reaches and the view follows. The world owns
  /// one. Possessing another entity releases the one before, marks the new
  /// one with Possessed, and tells every listener of both, so that the
  /// hooks of the scripts and the extensions hear of it.
  ///
  /// When a scene is placed, Start() decides what is possessed first: the
  /// entity the scene names with `possess`, else what was possessed and
  /// stayed through the change of scene, else the first entity that
  /// carries a Possessable, in the order of the scene, else the first
  /// FirstPersonController, else the first Spectator, which is what a scene
  /// written before the tag drove. Nothing else is possessed without being
  /// asked, and nothing happens in a scene without any of them.
  class Possession final
  {
    EntityStore *_store;
    std::shared_ptr<Logger> _logger;

    Entity _possessed = No_Entity;
    std::vector<PossessionListener *> _listeners;

    /// Marks the entity and tells the listeners; releases the one before.
    void Take(Entity entity);

    /// Takes the mark from the possessed entity and tells the listeners.
    void Release();

    /// The first entity, in the order of the scene, that carries the
    /// component, or No_Entity.
    [[nodiscard]] Entity FirstWith(ComponentId component) const;

    [[nodiscard]] Entity FirstWithBelow(Entity parent, ComponentId component) const;

    void Tell(Entity entity, bool possessed) const;

  public:
    Possession(EntityStore *store, const std::shared_ptr<Logger> &logger);

    /// Whoever is told when an entity is possessed or released. The
    /// listener outlives the world.
    void AddListener(PossessionListener *listener);

    /// Possesses the entity: it is marked Possessed, the one before is
    /// released, and the listeners are told of both. An entity that is gone
    /// or carries no Possessable is refused, with the reason in the log,
    /// and nothing changes. Possessing the entity that is possessed changes
    /// nothing either. Returns whether the entity is possessed.
    bool Possess(Entity entity);

    /// Releases the possessed entity, so that nothing is possessed, and
    /// tells the listeners. Nothing happens when nothing is possessed.
    void Unpossess();

    /// The entity that is possessed, or No_Entity: when nothing is, and
    /// when the possessed entity was destroyed.
    [[nodiscard]] Entity GetPossessed() const;

    /// Decides what is possessed when a scene has been placed, see above.
    /// `named` is the path the scene names with `possess`, or empty.
    void Start(const std::string &named);
  };
} // neon

#endif //POSSESSION_HPP
