#ifndef WORLD_SCENE_HPP
#define WORLD_SCENE_HPP

#include "entity-store.hpp"

namespace neon
{
  /// Where the entities of a world come from.
  ///
  /// A scene fills an empty store. One that is written in code and one that
  /// is read from a file look the same to the world.
  class Scene
  {
  protected:
    ~Scene() = default;

  public:
    /// Creates the entities of the scene. The components of the engine are
    /// registered by the time this is called. A scene registers its own.
    ///
    /// Returns false when something of the scene could not be made: the
    /// store then holds what could be, every problem is in the log, and the
    /// world goes on with it. A scene never ends the game.
    virtual bool Populate(EntityStore &store) = 0;
  };
} // neon

#endif //WORLD_SCENE_HPP
