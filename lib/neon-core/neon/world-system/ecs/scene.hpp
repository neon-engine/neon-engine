#ifndef WORLD_SCENE_HPP
#define WORLD_SCENE_HPP

#include <string>

#include <neon/data/data-value.hpp>

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
    /// world goes on with it. A problem in what a scene holds never ends the
    /// game; a first scene that cannot be read at all leaves nothing to run,
    /// see CouldNotBeRead().
    virtual bool Populate(EntityStore &store) = 0;

    /// Reads the file of another scene, named by its virtual path, before
    /// the world empties the store for it, so that a scene that cannot be
    /// read at all is known while the one that is there still is. Returns
    /// false, and says why, when the file is not there, cannot be opened, or
    /// holds no document; the world then stays where it is. What was read
    /// is what Load() of the same path places. A scene that is written in
    /// code reads nothing ahead and returns true.
    virtual bool ReadAhead(const std::string &path)
    {
      return true;
    }

    /// Creates the entities of another scene, named by its virtual path,
    /// into the store, which the world has emptied of what does not stay.
    /// What Populate() says about problems holds here too. A scene that
    /// is written in code and knows no paths returns false and says so.
    virtual bool Load(EntityStore &store, const std::string &path) = 0;

    /// Whether the last Populate() or Load() read nothing at all: the file
    /// is not there, cannot be opened, or holds no document. A scene that
    /// was read and has problems in what it holds was read. A scene that is
    /// written in code is always read.
    [[nodiscard]] virtual bool CouldNotBeRead() const
    {
      return false;
    }

    /// Creates the entity of a prefab recipe, named by its virtual path,
    /// below `parent`, which is No_Entity for the top, while the game
    /// plays. `overrides` is read on top of it, in the form of the
    /// `components` of an entity in a scene file: a map of components, each
    /// with the values that differ, or nothing to override none. Returns
    /// the entity, which holds what could be read when something in the
    /// recipe or the overrides is wrong, with every problem in the log; or
    /// No_Entity when the recipe cannot be read at all. A scene that is
    /// written in code and knows no recipes returns No_Entity and says so.
    virtual Entity Spawn(
      EntityStore &store,
      const std::string &path,
      Entity parent,
      const DataValue &overrides) = 0;
  };
} // neon

#endif //WORLD_SCENE_HPP
