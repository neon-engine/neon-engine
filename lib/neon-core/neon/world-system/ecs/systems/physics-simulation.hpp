#ifndef PHYSICS_SIMULATION_HPP
#define PHYSICS_SIMULATION_HPP

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <neon/common/rotation.hpp>
#include <neon/common/transform.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/physics/model-geometry.hpp>
#include <neon/physics/physics-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Brings the entities and the physics together.
  ///
  /// It creates what belongs to a RigidBody, a Trigger, and a CharacterBody,
  /// with the shapes their Colliders say, and releases it when the component
  /// leaves its entity. In every step of the world it hands what a game
  /// changed to the physics, lets the physics take a step, and writes what
  /// that led to into the components. In every frame it places what is drawn
  /// between the last two steps.
  ///
  /// Everything that moves is moved in FixedUpdate, by the length of a step.
  /// Nothing here reads the time of a frame, so the physics does the same
  /// whatever the frame rate is.
  ///
  /// The physics works in the space of the world, and a Transform is
  /// relative to the parent of its entity. A body can sit on an entity with
  /// a parent. What the physics says is turned into what is relative to the
  /// parent, as the parent is at that moment. A dynamic body is not moved by
  /// its parent. A static one, a kinematic one, and a trigger are.
  ///
  /// It registers the components of the physics, and so has to be added to
  /// the world before the world is initialized. The store has to be cleaned
  /// up while this system is alive, as EntityWorld does it, since the store
  /// tells it when a component is removed.
  class PhysicsSimulation final : public EntitySystem
  {
    struct Pose
    {
      glm::vec3 position{0.0f};
      glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

      bool operator==(const Pose &) const = default;
    };

    enum class RecordKind
    {
      Body = 0,
      Trigger,
      Character
    };

    /// What is kept of an entity that the physics knows.
    struct Record
    {
      RecordKind kind = RecordKind::Body;
      BodyKind body_kind = BodyKind::Static;
      BodyId body = No_Body;
      CharacterId character = No_Character;

      /// The size of the entity in the world when it was created.
      glm::vec3 scale{1.0f};

      /// Where the last two steps led to, in the world. What is drawn lies
      /// between them.
      Pose previous;
      Pose current;

      /// What was handed to the physics last, in the world.
      Pose pushed;

      /// A kinematic body that was moved keeps the velocity it was moved
      /// with until it is told otherwise.
      bool was_moved = false;

      /// What was written into the components last. What differs from it was
      /// changed by a game.
      glm::vec3 written_position{0.0f};
      Rotation written_rotation{};
      glm::vec3 written_linear_velocity{0.0f};
      glm::vec3 written_angular_velocity{0.0f};
    };

    PhysicsContext *_physics;
    FileSystemContext *_file_system;
    std::shared_ptr<Logger> _logger;

    // ordered, so that the same run does the same in the same order
    std::map<Entity, Record> _records;

    // by the path of the model. One that could not be read is kept as well,
    // without points, so that it is not read again
    std::map<std::string, ModelGeometry> _geometries;

    // colliders that are part of something, and those that were found to be
    // part of nothing, which was said once
    std::set<Entity> _claimed;
    std::set<Entity> _loose;

    QueryId _bodies = 0;
    QueryId _triggers = 0;
    QueryId _characters = 0;
    QueryId _colliders = 0;

    void Release(Entity entity);

    /// Whether what a component names was created for its entity.
    [[nodiscard]] bool Knows(Entity entity, BodyId body, CharacterId character) const;

    [[nodiscard]] static bool IsOwner(EntityStore &store, Entity entity);

    [[nodiscard]] static std::string PathOf(EntityStore &store, Entity entity);

    /// Where the parent of an entity places what is below it.
    [[nodiscard]] static glm::mat4 ParentMatrixOf(EntityStore &store, Entity entity);

    [[nodiscard]] static Pose WorldPoseOf(EntityStore &store, Entity entity, const Transform &transform);

    /// Writes where the physics put an entity into its Transform.
    static void WriteLocal(EntityStore &store, Entity entity, const Pose &world, Transform &transform, Record &record);

    [[nodiscard]] const ModelGeometry *GeometryOf(const std::string &path);

    bool CollectShapes(
      EntityStore &store,
      Entity entity,
      const glm::mat4 &relative,
      std::vector<ShapeInfo> &shapes,
      std::vector<Entity> &colliders,
      std::string &error);

    /// Collects the shapes of an entity and what is the same for everything
    /// that is created. Returns false when it cannot be created, which was
    /// logged.
    bool Prepare(
      EntityStore &store,
      Entity entity,
      const Transform &transform,
      const std::string &what,
      Record &record,
      std::vector<ShapeInfo> &shapes);

    void Refuse(EntityStore &store, Entity entity, const std::string &what, const std::string &error) const;

    void CreateMissing(EntityStore &store);

    void FindLooseColliders(EntityStore &store);

    void HandOver(EntityStore &store, double fixed_delta_time);

    void TakeBack(EntityStore &store);

    void CountInside(EntityStore &store) const;

    void PlaceBelow(EntityStore &store, Entity parent, const glm::mat4 &parent_matrix, float blend);

    [[nodiscard]] static bool MovesByItself(const Record &record);

    /// Where an entity is drawn between the last two steps. `placed` is
    /// where its Transform puts it.
    [[nodiscard]] static glm::mat4 Blend(const Record &record, const glm::mat4 &placed, float blend);

  public:
    /// `file_system` is what the models of convex hulls and meshes are read
    /// through.
    PhysicsSimulation(
      PhysicsContext *physics,
      FileSystemContext *file_system,
      const std::shared_ptr<Logger> &logger);

    void Initialize(EntityStore &store) override;

    /// Does nothing. The physics has nothing that belongs to a frame.
    void Update(EntityStore &store, double delta_time) override;

    void FixedUpdate(EntityStore &store, double fixed_delta_time) override;

    void Interpolate(EntityStore &store, double blend) override;
  };
} // neon

#endif //PHYSICS_SIMULATION_HPP
