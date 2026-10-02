#include "physics-simulation.hpp"

#include <format>

#include <glm/gtc/matrix_transform.hpp>

#include <neon/world-system/ecs/components/character-body.hpp>
#include <neon/world-system/ecs/components/collider.hpp>
#include <neon/world-system/ecs/components/geometry.hpp>
#include <neon/world-system/ecs/components/joint.hpp>
#include <neon/world-system/ecs/components/rigid-body.hpp>
#include <neon/world-system/ecs/components/trigger.hpp>
#include <neon/world-system/ecs/systems/geometry-building.hpp>

namespace neon
{
  // Helpers of PhysicsSimulation, for this file alone.
  namespace
  {
    glm::mat4 LocalMatrixOf(const Transform &transform)
    {
      return translate(glm::mat4{1.0f}, transform.position)
             * mat4_cast(transform.rotation.GetQuaternion())
             * scale(glm::mat4{1.0f}, transform.scale);
    }

    /// Takes a matrix apart into where it moves to, how it turns, and how
    /// it sizes. A matrix that shears, which a parent with different sizes
    /// along its axes and a child that is turned make together, comes out
    /// without the shear.
    void TakeApart(const glm::mat4 &matrix, glm::vec3 &position, glm::quat &rotation, glm::vec3 &size)
    {
      position = matrix[3];

      glm::mat3 axes{matrix};
      size = {length(axes[0]), length(axes[1]), length(axes[2])};

      for (int axis = 0; axis < 3; axis++)
      {
        if (size[axis] > 0.0f) { axes[axis] /= size[axis]; }
      }

      // a mirrored matrix is no rotation. The mirror goes into the size
      if (determinant(axes) < 0.0f)
      {
        axes[0] = -axes[0];
        size.x = -size.x;
      }

      rotation = normalize(quat_cast(axes));
    }

    bool SameRotation(const Rotation &left, const Rotation &right)
    {
      return left.pitch == right.pitch && left.yaw == right.yaw && left.roll == right.roll;
    }

    bool SameCollider(const Collider &left, const Collider &right)
    {
      return left.shape == right.shape
             && left.size == right.size
             && left.radius == right.radius
             && left.height == right.height
             && left.top_radius == right.top_radius
             && left.bottom_radius == right.bottom_radius
             && left.model == right.model
             && left.fit == right.fit
             && left.offset == right.offset
             && SameRotation(left.rotation, right.rotation);
    }

    std::string Describe(const ShapeKind shape)
    {
      return shape == ShapeKind::ConvexHull ? "convex hull" : "mesh";
    }

    /// The axes a component names, as the bits the physics takes. A word
    /// that is no axis was refused when the component was read.
    std::uint8_t AxisBits(const std::vector<std::string> &axes)
    {
      std::uint8_t bits = 0;
      for (const auto &axis : axes)
      {
        if (axis == "x") { bits |= Axis_X; }
        if (axis == "y") { bits |= Axis_Y; }
        if (axis == "z") { bits |= Axis_Z; }
      }
      return bits;
    }
  }

  PhysicsSimulation::PhysicsSimulation(
    PhysicsContext *physics,
    FileSystemContext *file_system,
    const std::shared_ptr<Logger> &logger)
  {
    _physics = physics;
    _file_system = file_system;
    _logger = logger;
  }

  void PhysicsSimulation::Register(EntityStore &store)
  {
    // what the physics holds for a component is released when the component
    // leaves its entity, which includes the entity being destroyed
    store.Register<RigidBody>("RigidBody", [this](const Entity entity, RigidBody &body)
    {
      Release(entity);
      body.body = No_Body;
    });

    store.Register<Trigger>("Trigger", [this](const Entity entity, Trigger &trigger)
    {
      Release(entity);
      trigger.body = No_Body;
    });

    store.Register<CharacterBody>("CharacterBody", [this](const Entity entity, CharacterBody &character)
    {
      Release(entity);
      character.character = No_Character;
    });

    store.Register<Joint>("Joint", [this](const Entity entity, Joint &joint)
    {
      if (const auto it = _joints.find(entity); it != _joints.end())
      {
        _physics->DestroyJoint(it->second);
        _joints.erase(it);
      }
      joint.joint = No_Joint;
    });

    store.Register<Collider>("Collider", [this](const Entity entity, Collider &)
    {
      // the body it was part of has one shape less
      if (const auto it = _claimed.find(entity); it != _claimed.end())
      {
        _reshape.insert(it->second);
        _claimed.erase(it);
      }
      _loose.erase(entity);
    });

    // parents come first, so that a body below another finds its parent
    // where this step put it
  }

  void PhysicsSimulation::Initialize(EntityStore &store)
  {
    _bodies = store.Query<Transform, RigidBody>(QueryOrder::ParentsFirst);
    _triggers = store.Query<Transform, Trigger>(QueryOrder::ParentsFirst);
    _characters = store.Query<Transform, CharacterBody>(QueryOrder::ParentsFirst);
    _colliders = store.Query<Collider>();
    _joint_query = store.Query<Joint>();
  }

  void PhysicsSimulation::Update(EntityStore &, const double) {}

  void PhysicsSimulation::Release(const Entity entity)
  {
    const auto it = _records.find(entity);
    if (it == _records.end()) { return; }

    if (it->second.body != No_Body) { _physics->DestroyBody(it->second.body); }
    if (it->second.character != No_Character) { _physics->DestroyCharacter(it->second.character); }

    for (const auto &read : it->second.colliders) { _claimed.erase(read.entity); }
    _reshape.erase(entity);
    _records.erase(it);
  }

  std::string PhysicsSimulation::NameOf(const RecordKind kind)
  {
    switch (kind)
    {
      case RecordKind::Body: return "RigidBody";
      case RecordKind::Trigger: return "Trigger";
      case RecordKind::Character: return "CharacterBody";
    }
    return "body";
  }

  void PhysicsSimulation::Claim(
    Record &record,
    const Entity owner,
    const std::vector<Entity> &colliders,
    EntityStore &store)
  {
    record.colliders.clear();
    for (const auto collider : colliders)
    {
      _claimed[collider] = owner;
      _loose.erase(collider);
      record.colliders.push_back({collider, *store.Get<Collider>(collider)});
    }
  }

  bool PhysicsSimulation::Knows(const Entity entity, const BodyId body, const CharacterId character) const
  {
    if (body == No_Body && character == No_Character) { return false; }

    // A component that was copied from another entity names what belongs
    // to the other. What belongs to this one is what was created for it.
    const auto it = _records.find(entity);
    return it != _records.end() && it->second.body == body && it->second.character == character;
  }

  bool PhysicsSimulation::IsOwner(EntityStore &store, const Entity entity)
  {
    return store.Has<RigidBody>(entity) || store.Has<Trigger>(entity) || store.Has<CharacterBody>(entity);
  }

  std::string PhysicsSimulation::PathOf(EntityStore &store, const Entity entity)
  {
    std::string path = store.GetName(entity);
    if (path.empty()) { path = "(without a name)"; }

    for (Entity parent = store.GetParent(entity); parent != No_Entity; parent = store.GetParent(parent))
    {
      const auto name = store.GetName(parent);
      path = (name.empty() ? std::string("(without a name)") : name) + "/" + path;
    }
    return path;
  }

  glm::mat4 PhysicsSimulation::ParentMatrixOf(EntityStore &store, const Entity entity)
  {
    auto matrix = glm::mat4{1.0f};

    for (Entity parent = store.GetParent(entity); parent != No_Entity; parent = store.GetParent(parent))
    {
      // an entity without a Transform only groups others, and places
      // nothing
      if (const auto *transform = store.Get<Transform>(parent); transform != nullptr)
      {
        matrix = LocalMatrixOf(*transform) * matrix;
      }
    }
    return matrix;
  }

  PhysicsSimulation::Pose PhysicsSimulation::WorldPoseOf(
    EntityStore &store,
    const Entity entity,
    const Transform &transform)
  {
    Pose pose;

    if (store.GetParent(entity) == No_Entity)
    {
      pose.position = transform.position;
      pose.rotation = transform.rotation.GetQuaternion();
      return pose;
    }

    glm::vec3 size;
    TakeApart(ParentMatrixOf(store, entity) * LocalMatrixOf(transform), pose.position, pose.rotation, size);
    return pose;
  }

  void PhysicsSimulation::WriteLocal(
    EntityStore &store,
    const Entity entity,
    const Pose &world,
    Transform &transform,
    Record &record)
  {
    if (store.GetParent(entity) == No_Entity)
    {
      transform.position = world.position;
      transform.rotation = Rotation::FromQuaternion(world.rotation);
    } else
    {
      const auto parent = ParentMatrixOf(store, entity);

      glm::vec3 position;
      glm::quat rotation;
      glm::vec3 size;
      TakeApart(parent, position, rotation, size);

      transform.position = inverse(parent) * glm::vec4(world.position, 1.0f);
      transform.rotation = Rotation::FromQuaternion(conjugate(rotation) * world.rotation);
    }

    record.written_position = transform.position;
    record.written_rotation = transform.rotation;
  }

  const ModelGeometry *PhysicsSimulation::GeometryOf(const std::string &path, const ModelFit fit)
  {
    const auto key = std::make_pair(path, fit);
    auto it = _geometries.find(key);
    if (it == _geometries.end())
    {
      ModelGeometry geometry;
      // what could not be read stays without points, and is not read again
      (void) LoadModelGeometry(path, fit, _file_system, _logger, geometry);
      it = _geometries.emplace(key, std::move(geometry)).first;
    }

    return it->second.triangles.empty() ? nullptr : &it->second;
  }

  bool PhysicsSimulation::CollectShapes(
    EntityStore &store,
    const Entity entity,
    const glm::mat4 &relative,
    std::vector<ShapeInfo> &shapes,
    std::vector<Entity> &colliders,
    std::string &error)
  {
    if (const auto *collider = store.Get<Collider>(entity); collider != nullptr)
    {
      ShapeInfo shape;
      shape.kind = collider->shape;
      shape.size = collider->size;
      shape.radius = collider->radius;
      shape.height = collider->height;
      shape.top_radius = collider->top_radius;
      shape.bottom_radius = collider->bottom_radius;

      const auto matrix = relative
                          * translate(glm::mat4{1.0f}, collider->offset)
                          * mat4_cast(collider->rotation.GetQuaternion());
      TakeApart(matrix, shape.position, shape.rotation, shape.scale);

      if (collider->shape == ShapeKind::ConvexHull || collider->shape == ShapeKind::Mesh)
      {
        // a collider without a model takes the shape the entity builds, so
        // that what is drawn is what collides
        if (collider->model.empty())
        {
          const auto *built = store.Get<Geometry>(entity);
          if (built == nullptr)
          {
            error = std::format(
              "the {} of entity '{}' names no model and the entity has no Geometry to take its shape from",
              Describe(collider->shape), PathOf(store, entity));
            return false;
          }

          const MeshData mesh = GeometryBuilding::Build(*built);
          if (mesh.IsEmpty())
          {
            error = std::format(
              "the Geometry of entity '{}' builds nothing for its {}", PathOf(store, entity), Describe(collider->shape));
            return false;
          }

          for (const auto &vertex : mesh.vertices) { shape.points.push_back(vertex.position); }
          if (collider->shape == ShapeKind::Mesh) { shape.triangles.assign(mesh.indices.begin(), mesh.indices.end()); }
        } else
        {
          const auto *geometry = GeometryOf(collider->model, collider->fit);
          if (geometry == nullptr)
          {
            error = std::format(
              "the model '{}' of the {} of entity '{}' cannot be read",
              collider->model, Describe(collider->shape), PathOf(store, entity));
            return false;
          }

          shape.points = geometry->points;
          if (collider->shape == ShapeKind::Mesh) { shape.triangles = geometry->triangles; }

          // the same model with the same fit gives the same points, which
          // the physics may hold as one shape
          shape.source = collider->fit == ModelFit::Unit ? collider->model + " (unit)" : collider->model;
        }
      }

      shapes.push_back(std::move(shape));
      colliders.push_back(entity);
    }

    for (const auto child : store.GetChildren(entity))
    {
      // what is below is a body of its own
      if (IsOwner(store, child)) { continue; }

      auto below = relative;
      if (const auto *transform = store.Get<Transform>(child); transform != nullptr)
      {
        below = relative * LocalMatrixOf(*transform);
      }

      if (!CollectShapes(store, child, below, shapes, colliders, error)) { return false; }
    }

    return true;
  }

  void PhysicsSimulation::Refuse(
    EntityStore &store,
    const Entity entity,
    const std::string &what,
    const std::string &error) const
  {
    const auto path = PathOf(store, entity);
    _logger->Error("The {} of entity '{}' cannot be created: {}", what, path, error);
  }

  bool PhysicsSimulation::Prepare(
    EntityStore &store,
    const Entity entity,
    const Transform &transform,
    const std::string &what,
    Record &record,
    std::vector<ShapeInfo> &shapes)
  {
    // the component was replaced by another, and what belonged to the one
    // before is still there
    Release(entity);

    glm::vec3 position;
    glm::quat rotation;
    TakeApart(ParentMatrixOf(store, entity) * LocalMatrixOf(transform), position, rotation, record.scale);

    record.current = {position, rotation};
    record.previous = record.current;
    record.pushed = record.current;
    record.written_position = transform.position;
    record.written_rotation = transform.rotation;

    std::vector<Entity> colliders;
    std::string error;
    if (!CollectShapes(store, entity, scale(glm::mat4{1.0f}, record.scale), shapes, colliders, error))
    {
      Refuse(store, entity, what, error);
      return false;
    }

    if (shapes.empty())
    {
      Refuse(store, entity, what, "it has no Collider, on itself or on an entity below it");
      return false;
    }

    Claim(record, entity, colliders, store);
    return true;
  }

  void PhysicsSimulation::CreateMissing(EntityStore &store)
  {
    store.Each(_bodies, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      auto *bodies = block.Column<RigidBody>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &body = bodies[i];
        const auto entity = block.entities[i];
        if (body.failed || Knows(entity, body.body, No_Character)) { continue; }

        // what could not be created is not tried again, since it would
        // fail, and say so, in every step
        body.body = No_Body;
        body.failed = true;

        if (store.Has<Trigger>(entity) || store.Has<CharacterBody>(entity))
        {
          Refuse(store, entity, "RigidBody",
                 "the entity carries more than one of RigidBody, Trigger, and CharacterBody");
          continue;
        }

        Record record;
        record.kind = RecordKind::Body;
        record.body_kind = body.kind;

        BodyInfo info;
        if (!Prepare(store, entity, transforms[i], "RigidBody", record, info.shapes)) { continue; }

        info.entity = entity;
        info.kind = body.kind;
        info.position = record.current.position;
        info.rotation = record.current.rotation;
        info.mass = body.mass;
        info.friction = body.friction;
        info.bounce = body.bounce;
        info.linear_damping = body.linear_damping;
        info.angular_damping = body.angular_damping;
        info.gravity_scale = body.gravity_scale;
        info.linear_velocity = body.linear_velocity;
        info.angular_velocity = radians(body.angular_velocity);
        info.continuous = body.continuous;
        info.can_sleep = body.can_sleep;
        info.locked_position = AxisBits(body.lock_position);
        info.locked_rotation = AxisBits(body.lock_rotation);
        info.layers = body.layers;
        info.mask = body.mask;

        std::string error;
        if (!_physics->CreateBody(info, record.body, error))
        {
          Refuse(store, entity, "RigidBody", error);
          continue;
        }

        record.written_linear_velocity = body.linear_velocity;
        record.written_angular_velocity = body.angular_velocity;

        body.body = record.body;
        body.failed = false;
        _records[entity] = record;
      }
    });

    store.Each(_triggers, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      auto *triggers = block.Column<Trigger>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &trigger = triggers[i];
        const auto entity = block.entities[i];
        if (trigger.failed || Knows(entity, trigger.body, No_Character)) { continue; }

        trigger.body = No_Body;
        trigger.failed = true;
        trigger.inside = 0;

        // said by the RigidBody already
        if (store.Has<RigidBody>(entity)) { continue; }

        if (store.Has<CharacterBody>(entity))
        {
          Refuse(store, entity, "Trigger",
                 "the entity carries more than one of RigidBody, Trigger, and CharacterBody");
          continue;
        }

        Record record;
        record.kind = RecordKind::Trigger;
        record.body_kind = BodyKind::Kinematic;

        BodyInfo info;
        if (!Prepare(store, entity, transforms[i], "Trigger", record, info.shapes)) { continue; }

        info.entity = entity;
        info.kind = BodyKind::Kinematic;
        info.trigger = true;
        info.position = record.current.position;
        info.rotation = record.current.rotation;
        info.layers = trigger.layers;
        info.mask = trigger.mask;

        std::string error;
        if (!_physics->CreateBody(info, record.body, error))
        {
          Refuse(store, entity, "Trigger", error);
          continue;
        }

        trigger.body = record.body;
        trigger.failed = false;
        _records[entity] = record;
      }
    });

    store.Each(_characters, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      auto *characters = block.Column<CharacterBody>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &character = characters[i];
        const auto entity = block.entities[i];
        if (character.failed || Knows(entity, No_Body, character.character)) { continue; }

        character.character = No_Character;
        character.failed = true;

        // said by the RigidBody or the Trigger already
        if (store.Has<RigidBody>(entity) || store.Has<Trigger>(entity)) { continue; }

        Record record;
        record.kind = RecordKind::Character;
        record.body_kind = BodyKind::Kinematic;

        CharacterInfo info;
        if (!Prepare(store, entity, transforms[i], "CharacterBody", record, info.shapes)) { continue; }

        // a character stays upright, however its entity is turned
        record.current.rotation = glm::quat{1.0f, 0.0f, 0.0f, 0.0f};
        record.previous = record.current;
        record.pushed = record.current;

        info.entity = entity;
        info.position = record.current.position;
        info.max_slope = character.max_slope;
        info.step_height = character.step_height;
        info.mass = character.mass;
        info.push_strength = character.push_strength;
        info.layers = character.layers;
        info.mask = character.mask;

        std::string error;
        if (!_physics->CreateCharacter(info, record.character, error))
        {
          Refuse(store, entity, "CharacterBody", error);
          continue;
        }

        character.character = record.character;
        character.failed = false;
        _records[entity] = record;
      }
    });
  }

  void PhysicsSimulation::CreateJoints(EntityStore &store)
  {
    store.Each(_joint_query, [&](const EntityBlock &block)
    {
      auto *joints = block.Column<Joint>(0);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &joint = joints[i];
        const auto entity = block.entities[i];
        if (joint.failed) { continue; }

        if (joint.joint != No_Joint)
        {
          // it holds. Or one of its bodies is gone, and it is made again
          // once both are back
          const auto it = _joints.find(entity);
          if (it != _joints.end() && it->second == joint.joint && _physics->HasJoint(joint.joint)) { continue; }

          if (it != _joints.end()) { _joints.erase(it); }
          joint.joint = No_Joint;
        }

        const auto refuse = [&](const std::string &why)
        {
          joint.failed = true;
          Refuse(store, entity, "Joint", why);
        };

        // the body of its own entity
        const auto *body = store.Get<RigidBody>(entity);
        if (body == nullptr)
        {
          refuse("the entity has no RigidBody. Only a RigidBody can be joined to something");
          continue;
        }
        if (body->failed)
        {
          refuse("the RigidBody of the entity could not be created");
          continue;
        }

        const auto own = _records.find(entity);
        if (own == _records.end() || own->second.kind != RecordKind::Body) { continue; }

        // the body it is joined to, or the world
        BodyId other = No_Body;
        if (!joint.other.empty())
        {
          const Entity named = store.FindEntity(joint.other);
          if (named == No_Entity)
          {
            refuse(std::format("it names '{}', which is no entity", joint.other));
            continue;
          }

          const auto *other_body = store.Get<RigidBody>(named);
          if (other_body == nullptr)
          {
            refuse(std::format("it names '{}', which has no RigidBody", joint.other));
            continue;
          }
          if (other_body->failed)
          {
            refuse(std::format("the RigidBody of '{}' could not be created", joint.other));
            continue;
          }

          const auto record = _records.find(named);
          if (record == _records.end() || record->second.kind != RecordKind::Body) { continue; }
          other = record->second.body;
        }

        // The anchor and the axis go with the entity, as it is now. The
        // anchor is sized by the scale of the entity, as the offset of a
        // Collider is, so that [-0.5, 0, 0] is the left edge of a scaled
        // cube. The axis is a direction, and is not.
        const auto &pose = own->second.current;

        JointInfo info;
        info.kind = joint.type;
        info.body = own->second.body;
        info.other = other;
        info.anchor = pose.position + pose.rotation * (own->second.scale * joint.anchor);
        info.axis = pose.rotation * joint.axis;
        info.has_limits = joint.limits.size() == 2;
        if (info.has_limits)
        {
          const bool in_degrees = joint.type == JointKind::Hinge;
          info.limit_min = in_degrees ? glm::radians(joint.limits[0]) : joint.limits[0];
          info.limit_max = in_degrees ? glm::radians(joint.limits[1]) : joint.limits[1];
        }

        JointId made = No_Joint;
        std::string error;
        if (!_physics->CreateJoint(info, made, error))
        {
          refuse(error);
          continue;
        }

        joint.joint = made;
        _joints[entity] = made;
      }
    });
  }

  void PhysicsSimulation::FindLooseColliders(EntityStore &store)
  {
    store.Each(_colliders, [&](const EntityBlock &block)
    {
      for (std::size_t i = 0; i < block.count; i++)
      {
        const auto entity = block.entities[i];
        if (_claimed.contains(entity) || _loose.contains(entity)) { continue; }

        Entity owner = entity;
        while (owner != No_Entity && !IsOwner(store, owner)) { owner = store.GetParent(owner); }

        // it joins a body that is there already, which gets its shapes anew
        if (owner != No_Entity && _records.contains(owner))
        {
          _reshape.insert(owner);
          continue;
        }

        // said once, and not in every step
        _loose.insert(entity);

        if (owner == No_Entity)
        {
          const auto path = PathOf(store, entity);
          _logger->Warn(
            "The Collider of entity '{}' belongs to nothing. It needs a RigidBody, a Trigger, or a "
            "CharacterBody on its entity or on one above it",
            path);
        }
      }
    });
  }

  void PhysicsSimulation::WatchColliders(EntityStore &store)
  {
    for (const auto &[entity, record] : _records)
    {
      for (const auto &read : record.colliders)
      {
        const auto *collider = store.Get<Collider>(read.entity);
        if (collider == nullptr || !SameCollider(*collider, read.collider))
        {
          _reshape.insert(entity);
          break;
        }
      }
    }
  }

  void PhysicsSimulation::Reshape(EntityStore &store)
  {
    for (const auto entity : _reshape)
    {
      const auto it = _records.find(entity);
      if (it == _records.end()) { continue; }

      auto &record = it->second;
      const auto what = NameOf(record.kind);
      const auto path = PathOf(store, entity);

      // the colliders as they are now are what was read, so that what
      // cannot be done is said once
      std::vector<ShapeInfo> shapes;
      std::vector<Entity> colliders;
      std::string error;
      const bool collected = CollectShapes(
        store, entity, scale(glm::mat4{1.0f}, record.scale), shapes, colliders, error);
      Claim(record, entity, colliders, store);

      if (record.kind == RecordKind::Character)
      {
        _logger->Error(
          "The shape of the {} of entity '{}' cannot change while it lives. Replace the component to give it "
          "another shape",
          what, path);
        continue;
      }

      if (!collected || shapes.empty() || !_physics->SetShape(record.body, shapes, error))
      {
        if (shapes.empty() && collected) { error = "it has no Collider, on itself or on an entity below it"; }

        _logger->Error(
          "The shape of the {} of entity '{}' cannot be changed, and stays what it was: {}",
          what, path, error);
      }
    }
    _reshape.clear();
  }

  void PhysicsSimulation::HandOver(EntityStore &store, const double fixed_delta_time)
  {
    const auto time = static_cast<float>(fixed_delta_time);

    store.Each(_bodies, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      auto *bodies = block.Column<RigidBody>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        const auto &transform = transforms[i];
        auto &body = bodies[i];
        if (!Knows(block.entities[i], body.body, No_Character)) { continue; }

        auto &record = _records.find(block.entities[i])->second;

        if (record.body_kind == BodyKind::Dynamic)
        {
          // a game wrote the Transform, which puts the body there at once
          if (transform.position != record.written_position
              || !SameRotation(transform.rotation, record.written_rotation))
          {
            const auto pose = WorldPoseOf(store, block.entities[i], transform);
            _physics->SetBodyPlace(record.body, pose.position, pose.rotation);

            // nothing is drawn on the way
            record.previous = pose;
            record.current = pose;
            record.written_position = transform.position;
            record.written_rotation = transform.rotation;
          }

          if (body.linear_velocity != record.written_linear_velocity)
          {
            _physics->SetLinearVelocity(record.body, body.linear_velocity);
            record.written_linear_velocity = body.linear_velocity;
          }

          if (body.angular_velocity != record.written_angular_velocity)
          {
            _physics->SetAngularVelocity(record.body, radians(body.angular_velocity));
            record.written_angular_velocity = body.angular_velocity;
          }

          continue;
        }

        const auto pose = WorldPoseOf(store, block.entities[i], transform);

        if (record.body_kind == BodyKind::Static)
        {
          if (pose != record.pushed)
          {
            _physics->SetBodyPlace(record.body, pose.position, pose.rotation);
            record.pushed = pose;
            record.current = pose;
            record.previous = pose;
          }
          continue;
        }

        // kinematic
        if (pose != record.pushed)
        {
          _physics->MoveBody(record.body, pose.position, pose.rotation, fixed_delta_time);
          record.pushed = pose;
          record.was_moved = true;
        } else if (record.was_moved
                   || body.linear_velocity != record.written_linear_velocity
                   || body.angular_velocity != record.written_angular_velocity)
        {
          // it arrived. From here on it moves with what its component says,
          // which is not at all for most
          _physics->SetLinearVelocity(record.body, body.linear_velocity);
          _physics->SetAngularVelocity(record.body, radians(body.angular_velocity));
          record.written_linear_velocity = body.linear_velocity;
          record.written_angular_velocity = body.angular_velocity;
          record.was_moved = false;
        }
      }
    });

    store.Each(_triggers, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      const auto *triggers = block.Column<Trigger>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        if (!Knows(block.entities[i], triggers[i].body, No_Character)) { continue; }

        auto &record = _records.find(block.entities[i])->second;

        const auto pose = WorldPoseOf(store, block.entities[i], transforms[i]);
        if (pose == record.pushed) { continue; }

        _physics->SetBodyPlace(record.body, pose.position, pose.rotation);
        record.pushed = pose;
        record.current = pose;
        record.previous = pose;
      }
    });

    // Characters are moved here, before the step, so that what they push
    // is pushed in the step that follows.
    const auto gravity = _physics->GetGravity();

    store.Each(_characters, [&](const EntityBlock &block)
    {
      auto *transforms = block.Column<Transform>(0);
      auto *characters = block.Column<CharacterBody>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &transform = transforms[i];
        auto &character = characters[i];
        const auto entity = block.entities[i];
        if (!Knows(entity, No_Body, character.character)) { continue; }

        auto &record = _records.find(entity)->second;

        if (transform.position != record.written_position)
        {
          const auto pose = WorldPoseOf(store, entity, transform);
          _physics->SetCharacterPosition(record.character, pose.position);

          record.current.position = pose.position;
          record.previous = record.current;
        }

        // Falling adds to the velocity while the entity is in the air. On
        // the ground what is left is the pull of one step, which keeps the
        // entity on the ground. A jump, which points against the pull, is
        // left alone.
        const auto pull = gravity * character.gravity_scale;
        if (character.on_floor && dot(character.fall_velocity, pull) >= 0.0f)
        {
          character.fall_velocity = pull * time;
        } else
        {
          character.fall_velocity += pull * time;
        }

        CharacterState state;
        if (!_physics->MoveCharacter(
          record.character,
          character.velocity + character.fall_velocity,
          fixed_delta_time,
          state))
        {
          continue;
        }

        // what hit the ceiling stops rising
        if (state.on_ceiling && dot(character.fall_velocity, pull) < 0.0f)
        {
          character.fall_velocity = glm::vec3{0.0f};
        }

        character.on_floor = state.on_floor;
        character.on_wall = state.on_wall;
        character.on_ceiling = state.on_ceiling;
        character.floor_normal = state.floor_normal;
        character.floor = state.floor;
        character.real_velocity = state.velocity;

        record.previous = record.current;
        record.current.position = state.position;

        const auto rotation = transform.rotation;
        WriteLocal(store, entity, record.current, transform, record);

        // the entity turns the way its game turns it
        transform.rotation = rotation;
        record.written_rotation = rotation;
      }
    });
  }

  void PhysicsSimulation::TakeBack(EntityStore &store)
  {
    store.Each(_bodies, [&](const EntityBlock &block)
    {
      auto *transforms = block.Column<Transform>(0);
      auto *bodies = block.Column<RigidBody>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &body = bodies[i];
        const auto entity = block.entities[i];
        if (!Knows(entity, body.body, No_Character)) { continue; }

        auto &record = _records.find(entity)->second;

        if (record.body_kind == BodyKind::Static) { continue; }

        BodyState state;
        if (!_physics->GetBodyState(record.body, state)) { continue; }

        const Pose pose{state.position, state.rotation};
        const bool moved = pose != record.current;

        record.previous = record.current;
        record.current = pose;

        if (record.body_kind == BodyKind::Kinematic)
        {
          // Where a game moved it to is where it is. Its Transform is left
          // as the game wrote it. What moved by the velocity of its
          // component is written as a dynamic body is.
          if (record.was_moved) { continue; }

          record.pushed = pose;
          if (moved) { WriteLocal(store, entity, pose, transforms[i], record); }
          continue;
        }

        body.linear_velocity = state.linear_velocity;
        body.angular_velocity = degrees(state.angular_velocity);
        record.written_linear_velocity = body.linear_velocity;
        record.written_angular_velocity = body.angular_velocity;

        // A body below a parent is written even when it rests, since the
        // parent may have moved, and the body is not moved by it.
        if (moved || store.GetParent(entity) != No_Entity)
        {
          WriteLocal(store, entity, pose, transforms[i], record);
        }
      }
    });
  }

  void PhysicsSimulation::CountInside(EntityStore &store) const
  {
    for (const auto &event : _physics->GetStepEvents())
    {
      if (!event.trigger || !store.IsAlive(event.first) || !store.Has<Trigger>(event.first)) { continue; }

      auto *trigger = store.Get<Trigger>(event.first);
      if (trigger->body != event.first_body) { continue; }

      if (event.kind == PhysicsEventKind::Began)
      {
        trigger->inside++;
      } else if (trigger->inside > 0)
      {
        trigger->inside--;
      }
    }
  }

  void PhysicsSimulation::FixedUpdate(EntityStore &store, const double fixed_delta_time)
  {
    CreateMissing(store);
    CreateJoints(store);
    FindLooseColliders(store);
    WatchColliders(store);
    Reshape(store);
    HandOver(store, fixed_delta_time);

    _physics->Step(fixed_delta_time);

    TakeBack(store);
    CountInside(store);
  }

  bool PhysicsSimulation::MovesByItself(const Record &record)
  {
    if (record.kind == RecordKind::Trigger || record.body_kind == BodyKind::Static) { return false; }
    if (record.kind == RecordKind::Character || record.body_kind == BodyKind::Dynamic) { return true; }

    // a kinematic body is carried by its parent until it moves
    return record.previous != record.current;
  }

  glm::mat4 PhysicsSimulation::Blend(const Record &record, const glm::mat4 &placed, const float blend)
  {
    const auto position = mix(record.previous.position, record.current.position, blend);

    if (record.kind == RecordKind::Character)
    {
      // only where it is lies between two steps. How it is turned and
      // sized is up to its Transform
      auto matrix = placed;
      matrix[3] = glm::vec4(position, 1.0f);
      return matrix;
    }

    const auto rotation = slerp(record.previous.rotation, record.current.rotation, blend);

    return translate(glm::mat4{1.0f}, position)
           * mat4_cast(rotation)
           * scale(glm::mat4{1.0f}, record.scale);
  }

  void PhysicsSimulation::PlaceBelow(
    EntityStore &store,
    const Entity parent,
    const glm::mat4 &parent_matrix,
    const float blend)
  {
    for (const auto child : store.GetChildren(parent))
    {
      auto matrix = parent_matrix;

      if (auto *transform = store.Get<Transform>(child); transform != nullptr)
      {
        matrix = parent_matrix * LocalMatrixOf(*transform);

        // what the physics moves is where the physics put it, whatever its
        // parent does. Everything else is carried by its parent
        if (const auto it = _records.find(child); it != _records.end() && MovesByItself(it->second))
        {
          matrix = Blend(it->second, matrix, blend);
        }

        transform->world_coordinates = matrix;
      }

      PlaceBelow(store, child, matrix, blend);
    }
  }

  void PhysicsSimulation::Interpolate(EntityStore &store, const double blend)
  {
    const auto between = static_cast<float>(blend);

    for (const auto &[entity, record] : _records)
    {
      if (!MovesByItself(record)) { continue; }

      // What rests is where its Transform says already, unless it has a
      // parent, which may have moved without it.
      if (record.previous == record.current && store.GetParent(entity) == No_Entity) { continue; }

      auto *transform = store.Get<Transform>(entity);
      if (transform == nullptr) { continue; }

      transform->world_coordinates = Blend(record, transform->world_coordinates, between);

      PlaceBelow(store, entity, transform->world_coordinates, between);
    }

    // every system has seen the events of this frame
    _physics->EndFrame();
  }
} // neon
