#ifndef POOL_SYSTEM_HPP
#define POOL_SYSTEM_HPP

#include <memory>
#include <string>
#include <vector>

#include <neon/logging/logger.hpp>
#include <neon/world-system/ecs/entity-system.hpp>
#include <neon/world-system/world-system.hpp>

namespace neon
{
  /// Fills the pools of the world, and is what a pool is asked through.
  ///
  /// A pool is an entity with a PoolManager. This makes the instances its recipe
  /// asks for, once, below the pool: a prefab is spawned as many times as
  /// its entry says, each instance named after the prefab and numbered, as
  /// `nail 1` to `nail 64`, since a name is an address and two entities
  /// below one parent cannot share it. An entity that is there already is
  /// taken in as it is. Everything it takes in is turned off until it is
  /// handed out.
  ///
  /// A kind is named as a file system names things: the path of its
  /// prefab, `assets://prefabs/nail.prefab.yml`, or `instance://` and a name
  /// for instances made in code, see Add().
  ///
  /// What hands out and takes back are functions of the class, so that a
  /// script and an extension reach them with the store alone.
  class PoolSystem final : public EntitySystem
  {
    WorldSystem *_world = nullptr;
    std::shared_ptr<Logger> _logger;
    QueryId _pools = 0;

    /// Makes the instances of a pool that was not filled yet.
    void Fill(EntityStore &store, Entity pool);

    /// Says what a pool was asked for in vain since it was last said.
    void Report(EntityStore &store, Entity pool) const;

  public:
    /// What a kind of instances made in code, or of an entity that is
    /// there already, starts with.
    static constexpr const char *kInstance_Scheme = "instance://";

    /// `world` spawns the prefabs. Without one, a pool holds only what it
    /// is handed with Add().
    PoolSystem(WorldSystem *world, const std::shared_ptr<Logger> &logger);

    void Register(EntityStore &store) override;

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;

    /// Hands a pool instances that are there already, under a kind, such
    /// as `instance://nail`: for a game that makes its entities in code. They
    /// are turned off and wait. A kind the pool has gets them on top of
    /// what it holds. Returns false, and takes none, when `pool` is no
    /// pool or the kind has no name; an entity that is gone, or belongs to
    /// a pool already, is left out and said.
    static bool Add(
      EntityStore &store,
      Entity pool,
      const std::string &kind,
      const std::vector<Entity> &instances,
      Logger *logger = nullptr);

    /// Hands out an instance of a kind, turned on: the one that has waited
    /// longest. No_Entity when the pool has no such kind, and when all of
    /// the kind are out. Either is said by the system in its next update,
    /// whoever asked: once for a kind the pool does not hold, and once for
    /// a kind that ran out until one comes back. Whoever takes an instance
    /// places it and gives it back with Release().
    static Entity Acquire(EntityStore &store, Entity pool, const std::string &kind);

    /// Takes an instance back: it is turned off and waits at the end of
    /// the queue of its kind. Returns false for an entity that belongs to
    /// no pool, and for one that waits already.
    static bool Release(EntityStore &store, Entity instance);

    /// The kinds a pool holds, in the order it was told of them.
    [[nodiscard]] static std::vector<std::string> GetKinds(EntityStore &store, Entity pool);

    /// How many instances of a kind a pool holds, and how many of them
    /// wait. 0 for what is no pool, and for a kind it does not have.
    [[nodiscard]] static std::size_t GetCount(EntityStore &store, Entity pool, const std::string &kind);

    [[nodiscard]] static std::size_t GetFreeCount(EntityStore &store, Entity pool, const std::string &kind);

    /// Turns the Renderable, the Collider, and the RigidBody of an entity
    /// and of everything below it off or on: what a pool does with an
    /// instance that waits, and with one it hands out.
    static void SetInUse(EntityStore &store, Entity instance, bool in_use);
  };
} // neon

#endif //POOL_SYSTEM_HPP
