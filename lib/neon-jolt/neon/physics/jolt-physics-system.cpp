// Jolt uses FLT_MAX and its kin. With the SDK of macOS 27 and the standard
// library of LLVM 20, a header that asks for a part of <float.h> first keeps
// the rest of it from ever being read. Asking for all of it before anything
// else works everywhere.
#include <cfloat>

#include "jolt-physics-system.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>
#include <utility>

// Jolt asks for this header before any other of its own.
#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollisionGroup.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/GroupFilter.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/PlaneShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/TaperedCapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/TaperedCylinderShape.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/MotorSettings.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/SpringSettings.h>
#include <Jolt/Physics/Constraints/TwoBodyConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

namespace neon
{
  // Helpers of Jolt_PhysicsSystem, for this file alone.
  namespace
  {
    // What Jolt can hold at most. Reaching one of these is reported, and
    // nothing breaks.
    constexpr JPH::uint most_bodies = 65536;
    constexpr JPH::uint most_body_pairs = 65536;
    constexpr JPH::uint most_contacts = 65536;
    constexpr int temporary_memory = 64 * 1024 * 1024;

    // Bodies that were added since the tree Jolt finds them with was
    // rebuilt. Above this many, it is rebuilt before the next step.
    constexpr std::size_t rebuild_after = 64;

    // Jolt has one place for what it shares between every physics of a
    // process. It is set up with the first and taken down with the last.
    std::mutex shared_mutex;
    int shared_users = 0;

    /// The factory Jolt makes its types with. It is one object for the
    /// process, which Jolt is given the address of while a physics is in
    /// use, and which is emptied again when the last is gone. Made on the
    /// first call, after the allocator of Jolt is in place.
    JPH::Factory &SharedFactory()
    {
      static JPH::Factory factory;
      return factory;
    }

    void AcquireShared()
    {
      const std::scoped_lock lock(shared_mutex);
      if (shared_users++ > 0) { return; }

      JPH::RegisterDefaultAllocator();
      JPH::Factory::sInstance = &SharedFactory();
      JPH::RegisterTypes();
    }

    void ReleaseShared()
    {
      const std::scoped_lock lock(shared_mutex);
      if (--shared_users > 0) { return; }

      // unregistering empties the factory
      JPH::UnregisterTypes();
      JPH::Factory::sInstance = nullptr;
    }

    JPH::Vec3 ToJolt(const glm::vec3 &value) { return {value.x, value.y, value.z}; }

    JPH::Quat ToJolt(const glm::quat &value)
    {
      return JPH::Quat(value.x, value.y, value.z, value.w).Normalized();
    }

    glm::vec3 ToGlm(const JPH::Vec3 &value) { return {value.GetX(), value.GetY(), value.GetZ()}; }

    glm::quat ToGlm(const JPH::Quat &value) { return {value.GetW(), value.GetX(), value.GetY(), value.GetZ()}; }

    bool IsFinite(const glm::vec3 &value)
    {
      return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    }

    /// What a body is free to do, which is everything but what is locked.
    JPH::EAllowedDOFs AllowedDegrees(const std::uint8_t locked_position, const std::uint8_t locked_rotation)
    {
      auto allowed = JPH::EAllowedDOFs::None;
      if ((locked_position & Axis_X) == 0) { allowed |= JPH::EAllowedDOFs::TranslationX; }
      if ((locked_position & Axis_Y) == 0) { allowed |= JPH::EAllowedDOFs::TranslationY; }
      if ((locked_position & Axis_Z) == 0) { allowed |= JPH::EAllowedDOFs::TranslationZ; }
      if ((locked_rotation & Axis_X) == 0) { allowed |= JPH::EAllowedDOFs::RotationX; }
      if ((locked_rotation & Axis_Y) == 0) { allowed |= JPH::EAllowedDOFs::RotationY; }
      if ((locked_rotation & Axis_Z) == 0) { allowed |= JPH::EAllowedDOFs::RotationZ; }
      return allowed;
    }

    std::string Name(const ShapeKind kind)
    {
      switch (kind)
      {
        case ShapeKind::Box: return "box";
        case ShapeKind::Sphere: return "sphere";
        case ShapeKind::Capsule: return "capsule";
        case ShapeKind::Cylinder: return "cylinder";
        case ShapeKind::TaperedCapsule: return "tapered capsule";
        case ShapeKind::TaperedCylinder: return "tapered cylinder";
        case ShapeKind::Plane: return "plane";
        case ShapeKind::ConvexHull: return "convex hull";
        case ShapeKind::Mesh: return "mesh";
      }
      return "shape";
    }

    /// The shapes of a body as the log names them, such as `1 box shape`
    /// or `2 box shapes and 1 sphere shape`, each kind in the order it
    /// first comes in.
    std::string Describe(const std::vector<ShapeInfo> &shapes)
    {
      std::vector<std::pair<ShapeKind, std::size_t>> counts;
      for (const auto &shape : shapes)
      {
        const auto it = std::ranges::find(counts, shape.kind, &std::pair<ShapeKind, std::size_t>::first);
        if (it == counts.end()) { counts.emplace_back(shape.kind, 1); } else { it->second++; }
      }

      std::string said;
      for (std::size_t i = 0; i < counts.size(); i++)
      {
        if (i > 0) { said += counts.size() == 2 ? " and " : (i + 1 == counts.size() ? ", and " : ", "); }
        const auto [kind, count] = counts[i];
        said += std::format("{} {} shape{}", count, Name(kind), count == 1 ? "" : "s");
      }
      return said;
    }

    std::string Name(const BodyKind kind)
    {
      switch (kind)
      {
        case BodyKind::Static: return "static";
        case BodyKind::Kinematic: return "kinematic";
        case BodyKind::Dynamic: return "dynamic";
      }
      return "body";
    }

    /// What a body is in and looks for. Jolt keeps a number of 16 bits
    /// with every body, which is the place of one of these in a list. So
    /// all 32 layers are there, and as many different sets of them as the
    /// number has values.
    struct LayerEntry
    {
      std::uint32_t layers = 0;
      std::uint32_t mask = 0;
      bool moving = false;
      bool trigger = false;

      bool operator==(const LayerEntry &) const = default;
    };

    bool Meet(const LayerEntry &first, const LayerEntry &second)
    {
      // two triggers report nothing of each other
      if (first.trigger && second.trigger) { return false; }

      // a trigger looks, and is not looked for
      if (first.trigger) { return (first.mask & second.layers) != 0; }
      if (second.trigger) { return (second.mask & first.layers) != 0; }

      return (first.mask & second.layers) != 0 || (second.mask & first.layers) != 0;
    }

    namespace broad_phase
    {
      const JPH::BroadPhaseLayer resting(0);
      const JPH::BroadPhaseLayer moving(1);
      constexpr JPH::uint count = 2;
    }

    /// Answers every question Jolt has about layers.
    ///
    /// It is written to when a body is created, and read by the threads of
    /// Jolt during a step. The two never happen at the same time, since
    /// nothing is created while a step is taken.
    class Layers final
      : public JPH::BroadPhaseLayerInterface,
        public JPH::ObjectVsBroadPhaseLayerFilter,
        public JPH::ObjectLayerPairFilter
    {
      std::vector<LayerEntry> _entries;

    public:
      /// The place of an entry, which is added when it is new. Returns
      /// false when there is no place left.
      bool Find(const LayerEntry &entry, JPH::ObjectLayer &layer)
      {
        for (std::size_t i = 0; i < _entries.size(); i++)
        {
          if (_entries[i] == entry)
          {
            layer = static_cast<JPH::ObjectLayer>(i);
            return true;
          }
        }

        if (_entries.size() >= static_cast<std::size_t>(JPH::cObjectLayerInvalid)) { return false; }

        _entries.push_back(entry);
        layer = static_cast<JPH::ObjectLayer>(_entries.size() - 1);
        return true;
      }

      [[nodiscard]] const LayerEntry &Get(const JPH::ObjectLayer layer) const
      {
        static const LayerEntry none{};
        return layer < _entries.size() ? _entries[layer] : none;
      }

      void Clear() { _entries.clear(); }

      [[nodiscard]] JPH::uint GetNumBroadPhaseLayers() const override { return broad_phase::count; }

      [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(const JPH::ObjectLayer layer) const override
      {
        return Get(layer).moving ? broad_phase::moving : broad_phase::resting;
      }

      [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer layer, const JPH::BroadPhaseLayer other) const override
      {
        // what does not move meets what moves, and nothing else
        return Get(layer).moving || other == broad_phase::moving;
      }

      [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer first, const JPH::ObjectLayer second) const override
      {
        return Meet(Get(first), Get(second));
      }
    };

    /// Lets a query or a character meet what its layers say.
    class LayerFilter final : public JPH::ObjectLayerFilter
    {
      const Layers *_layers;
      LayerEntry _asking;
      bool _is_query;
      bool _triggers;

    public:
      /// For a query, which looks for the layers of its mask.
      LayerFilter(const Layers *layers, const QueryFilter &filter)
      {
        _layers = layers;
        _asking.mask = filter.mask;
        _is_query = true;
        _triggers = filter.triggers;
      }

      /// For a character, which meets what a body of its layers meets.
      LayerFilter(const Layers *layers, const LayerEntry &character)
      {
        _layers = layers;
        _asking = character;
        _is_query = false;
        _triggers = false;
      }

      [[nodiscard]] bool ShouldCollide(const JPH::ObjectLayer layer) const override
      {
        const auto &entry = _layers->Get(layer);
        if (entry.trigger && !_triggers) { return false; }

        return _is_query ? (_asking.mask & entry.layers) != 0 : Meet(_asking, entry);
      }
    };

    /// What is kept of a body.
    /// What tells the shared shapes of models apart.
    struct SharedShapeKey
    {
      std::string source;
      ShapeKind kind = ShapeKind::Mesh;
      float scale_x = 1.0f;
      float scale_y = 1.0f;
      float scale_z = 1.0f;

      auto operator<=>(const SharedShapeKey &) const = default;
    };

    struct BodyRecord
    {
      JPH::BodyID jolt;
      Entity entity = No_Entity;
      BodyKind kind = BodyKind::Static;
      bool trigger = false;

      /// The body that a character is for the bodies around it.
      bool of_character = false;

      /// What a dynamic body keeps when its shape changes.
      float mass = 1.0f;
      JPH::EAllowedDOFs allowed = JPH::EAllowedDOFs::All;
    };

    /// Whether shapes can be on a body. Says why not in `error`.
    bool ShapesFit(const std::vector<ShapeInfo> &shapes, const BodyKind kind, const bool trigger, std::string &error)
    {
      const bool dynamic = kind == BodyKind::Dynamic && !trigger;

      for (const auto &shape : shapes)
      {
        if (shape.kind == ShapeKind::Mesh && dynamic)
        {
          error = "a dynamic body cannot have a mesh. A mesh is a surface without an inside, so it has no "
                  "mass. Make the body static or kinematic, or give it a convex hull";
          return false;
        }

        if (shape.kind == ShapeKind::Plane && (kind != BodyKind::Static || trigger))
        {
          error = "only a static body can have a plane, which has no end and cannot move";
          return false;
        }
      }
      return true;
    }

    /// What is kept of a joint. Jolt makes the constraint itself, and
    /// counts references to it. This is one of them.
    struct JointRecord
    {
      JPH::Ref<JPH::TwoBodyConstraint> constraint;
      JointKind kind = JointKind::Fixed;
      BodyId body = No_Body;
      BodyId other = No_Body;

      /// Whether the two bodies were kept from colliding with each other.
      bool apart = false;

      /// The axis of a hinge or a slider, on the body that holds, which is
      /// the world when there is no other.
      JPH::Vec3 axis_on_held = JPH::Vec3::sAxisY();

      /// Whether it was said that the joint has no state to read.
      bool warned = false;
    };

    /// Keeps the two bodies of a joint from colliding with each other,
    /// when the joint asks for it. Jolt asks a group filter whether two
    /// bodies may collide. Every body a joint keeps apart is given this
    /// one, with the number the physics knows the body by as its group, and
    /// the pairs that are kept apart are written down here.
    ///
    /// Jolt asks from several threads at once during a step. Pairs are
    /// added and taken away between steps alone.
    class JointFilter final : public JPH::GroupFilter
    {
      using Pair = std::pair<JPH::CollisionGroup::GroupID, JPH::CollisionGroup::GroupID>;

      // how many joints keep each pair apart
      std::map<Pair, int> _apart;

      static Pair PairOf(const BodyId first, const BodyId second)
      {
        return {std::min(first, second), std::max(first, second)};
      }

    public:
      void KeepApart(const BodyId first, const BodyId second) { _apart[PairOf(first, second)]++; }

      void LetTouch(const BodyId first, const BodyId second)
      {
        const auto it = _apart.find(PairOf(first, second));
        if (it == _apart.end()) { return; }
        if (--it->second <= 0) { _apart.erase(it); }
      }

      bool CanCollide(const JPH::CollisionGroup &first, const JPH::CollisionGroup &second) const override
      {
        const auto a = first.GetGroupID();
        const auto b = second.GetGroupID();
        if (a == JPH::CollisionGroup::cInvalidGroup || b == JPH::CollisionGroup::cInvalidGroup) { return true; }

        return !_apart.contains(PairOf(a, b));
      }
    };

    std::string Name(const JointKind kind)
    {
      switch (kind)
      {
        case JointKind::Fixed: return "fixed";
        case JointKind::Hinge: return "hinge";
        case JointKind::Slider: return "slider";
        case JointKind::Point: return "point";
        case JointKind::Rope: return "rope";
      }
      return "joint";
    }

    /// What is kept of a character. The character of Jolt is a part of the
    /// record, built in place in a map, whose nodes do not move, so that
    /// nothing is made with `new`. Jolt counts references to a character,
    /// and the record marks it as embedded so that Jolt never deletes it.
    struct CharacterRecord
    {
      JPH::CharacterVirtual character;
      Entity entity = No_Entity;
      BodyId body = No_Body;
      LayerEntry layers;
      float step_height = 0.0f;
      float cosine_of_slope = 0.0f;

      CharacterRecord(
        const JPH::CharacterVirtualSettings &settings,
        const JPH::RVec3 &position,
        const BodyId body,
        JPH::PhysicsSystem *physics)
        : character(&settings, position, JPH::Quat::sIdentity(), body, physics), body(body)
      {
        character.SetEmbedded();
      }

      // the character of Jolt stays where it was built
      CharacterRecord(const CharacterRecord &) = delete;

      CharacterRecord &operator=(const CharacterRecord &) = delete;
    };

    /// A contact as a thread of Jolt found it.
    struct Found
    {
      bool added = false;
      std::uint32_t first = 0;
      std::uint32_t second = 0;
      std::uint32_t first_part = 0;
      std::uint32_t second_part = 0;
      BodyId first_body = No_Body;
      BodyId second_body = No_Body;
      glm::vec3 point{0.0f};
      glm::vec3 normal{0.0f};
    };

    /// Two bodies that touch.
    struct Touch
    {
      /// How many of their parts touch.
      int count = 0;

      /// Both came to rest, and Jolt has stopped looking at them. They
      /// still touch.
      bool asleep = false;

      bool trigger = false;
      BodyId first_body = No_Body;
      BodyId second_body = No_Body;
      Entity first = No_Entity;
      Entity second = No_Entity;
    };

    /// Skips the bodies of an entity.
    class IgnoreEntity final : public JPH::BodyFilter
    {
      const std::unordered_map<BodyId, BodyRecord> *_bodies;
      Entity _ignore;

    public:
      IgnoreEntity(const std::unordered_map<BodyId, BodyRecord> *bodies, const Entity ignore)
      {
        _bodies = bodies;
        _ignore = ignore;
      }

      [[nodiscard]] bool ShouldCollideLocked(const JPH::Body &body) const override
      {
        if (_ignore == No_Entity) { return true; }

        const auto it = _bodies->find(static_cast<BodyId>(body.GetUserData()));
        return it == _bodies->end() || it->second.entity != _ignore;
      }
    };

    /// Walks a character up a step that lies along a wall it leans on.
    ///
    /// Jolt steps up in the direction the character asks for. One that
    /// leans on a wall and moves along it asks mostly for the wall, so the
    /// step forward is eaten by the wall, and what is left along it is too
    /// short to find the top of the step: Jolt gives up, and the character
    /// stands at the side of the step. Here, when the character came short
    /// of its way after Jolt's own try, each wall it pushes into is taken
    /// out of the way it asks for, one at a time, and the step is tried
    /// again along what is left. Head on into a step, nothing is left, and
    /// nothing is tried.
    void WalkStairsAlongAWall(
      JPH::CharacterVirtual &moved,
      const JPH::RVec3 &before,
      const JPH::Vec3 desired_velocity,
      const float time,
      const JPH::CharacterVirtual::ExtendedUpdateSettings &update,
      const JPH::BroadPhaseLayerFilter &broad_phase_filter,
      const JPH::ObjectLayerFilter &layer_filter,
      JPH::TempAllocator &temporary)
    {
      if (update.mWalkStairsStepUp.IsNearZero()) { return; }

      const auto up = moved.GetUp();
      const auto desired = desired_velocity - desired_velocity.Dot(up) * up;
      const float desired_length = desired.Length() * time;
      if (desired_length < 1.0e-6f) { return; }

      // how far it came the way it asked for. Jolt's own step, when it
      // worked, took it the whole way
      auto achieved = JPH::Vec3(moved.GetPosition() - before);
      achieved -= achieved.Dot(up) * up;
      const float achieved_length = std::max(0.0f, achieved.Dot(desired) / desired.Length());
      if (achieved_length + 1.0e-4f >= desired_length || !moved.CanWalkStairs(desired_velocity)) { return; }

      // the walls the character pushes into, as the last move found them
      std::vector<JPH::Vec3> walls;
      for (const auto &contact : moved.GetActiveContacts())
      {
        if (!contact.mHadCollision || contact.mIsSensorB || !moved.IsSlopeTooSteep(contact.mSurfaceNormal)) { continue; }

        auto normal = contact.mSurfaceNormal - contact.mSurfaceNormal.Dot(up) * up;
        if (normal.IsNearZero() || normal.Dot(desired) >= 0.0f) { continue; }

        walls.push_back(normal.Normalized());
      }

      for (const auto &wall : walls)
      {
        // what is asked for without the part that goes into this wall
        const auto along = desired - desired.Dot(wall) * wall;
        const float length = along.Length();
        if (length < 1.0e-6f) { continue; }

        const auto direction = along / length;
        const auto step_forward = direction * std::max(update.mWalkStairsMinStepForward, length * time);
        const auto step_forward_test = direction * update.mWalkStairsStepForwardTest;

        if (moved.WalkStairs(
          time,
          update.mWalkStairsStepUp,
          step_forward,
          step_forward_test,
          update.mWalkStairsStepDownExtra,
          broad_phase_filter,
          layer_filter,
          {},
          {},
          temporary))
        {
          return;
        }
      }
    }

    /// Decides what a character and a body do to each other when they
    /// touch.
    class CharacterContacts final : public JPH::CharacterContactListener
    {
      const JPH::PhysicsSystem *_physics;

    public:
      explicit CharacterContacts(const JPH::PhysicsSystem *physics)
      {
        _physics = physics;
      }

      void OnContactAdded(
        const JPH::CharacterVirtual *,
        const JPH::BodyID &body,
        const JPH::SubShapeID &,
        JPH::RVec3Arg,
        JPH::Vec3Arg,
        JPH::CharacterContactSettings &settings) override
      {
        // A character pushes what is dynamic, and is not pushed by it,
        // however heavy and fast it is. What is moved by code, such as a
        // lift, carries and pushes a character.
        const auto motion = _physics->GetBodyInterfaceNoLock().GetMotionType(body);

        settings.mCanPushCharacter = motion != JPH::EMotionType::Dynamic;
        settings.mCanReceiveImpulses = true;
      }

      /// Whether the character that is being moved may slide down ground
      /// it can stand on. Set before every move: a character that asked
      /// for no way along the ground stays where it is, as the sample of
      /// Jolt does, and one that walks, jumps, or is in the air slides.
      bool sliding_allowed = true;

      void OnContactSolve(
        const JPH::CharacterVirtual *character,
        const JPH::BodyID &,
        const JPH::SubShapeID &,
        JPH::RVec3Arg,
        JPH::Vec3Arg normal,
        JPH::Vec3Arg contact_velocity,
        const JPH::PhysicsMaterial *,
        JPH::Vec3Arg,
        JPH::Vec3 &new_velocity) override
      {
        // A character at rest is pulled into the ground by one step of
        // gravity, which Jolt would turn into a slide down any slope.
        // Against ground that stands still and is not too steep, nothing
        // is left of it. Ground that is too steep is a wall, and the
        // character slides down it as it should.
        if (!sliding_allowed && contact_velocity.IsNearZero() && !character->IsSlopeTooSteep(normal))
        {
          new_velocity = JPH::Vec3::sZero();
        }
      }
    };

    /// Writes down the contacts Jolt finds. Jolt calls it from several
    /// threads at once, in an order that differs from run to run. What is
    /// made of them is decided once the step is done.
    class Contacts final : public JPH::ContactListener
    {
      std::mutex _mutex;
      std::vector<Found> _found;

    public:
      void OnContactAdded(
        const JPH::Body &first,
        const JPH::Body &second,
        const JPH::ContactManifold &manifold,
        JPH::ContactSettings &) override
      {
        Found found;
        found.added = true;
        found.first = first.GetID().GetIndexAndSequenceNumber();
        found.second = second.GetID().GetIndexAndSequenceNumber();
        found.first_part = manifold.mSubShapeID1.GetValue();
        found.second_part = manifold.mSubShapeID2.GetValue();
        found.first_body = static_cast<BodyId>(first.GetUserData());
        found.second_body = static_cast<BodyId>(second.GetUserData());
        found.normal = ToGlm(manifold.mWorldSpaceNormal);

        if (!manifold.mRelativeContactPointsOn1.empty())
        {
          found.point = ToGlm(JPH::Vec3(manifold.GetWorldSpaceContactPointOn1(0)));
        }

        const std::scoped_lock lock(_mutex);
        _found.push_back(found);
      }

      void OnContactRemoved(const JPH::SubShapeIDPair &pair) override
      {
        Found found;
        found.added = false;
        found.first = pair.GetBody1ID().GetIndexAndSequenceNumber();
        found.second = pair.GetBody2ID().GetIndexAndSequenceNumber();

        const std::scoped_lock lock(_mutex);
        _found.push_back(found);
      }

      std::vector<Found> Take()
      {
        const std::scoped_lock lock(_mutex);
        return std::exchange(_found, {});
      }
    };
  }

  struct Jolt_PhysicsSystem::State
  {
    JPH::TempAllocatorImpl temporary{temporary_memory};
    std::unique_ptr<JPH::JobSystemThreadPool> jobs;
    Layers layers;
    Contacts contacts;
    JPH::PhysicsSystem physics;
    CharacterContacts character_contacts{&physics};

    std::unordered_map<BodyId, BodyRecord> bodies;
    std::map<CharacterId, CharacterRecord> characters;
    std::map<JointId, JointRecord> joints;

    // what keeps joined bodies apart. Jolt counts references to it, and
    // the state marks it as embedded so that Jolt never deletes it
    JointFilter joint_filter;

    // by the two numbers Jolt knows the bodies as
    std::map<std::pair<std::uint32_t, std::uint32_t>, Touch> touches;

    // The shape of every model, by what its points were read from, its
    // kind, and its scale: held once, and given to every body whose
    // collider names the same. Jolt counts who holds a shape, so one that
    // no body holds any more is let go.
    std::map<SharedShapeKey, JPH::RefConst<JPH::Shape>> shared_shapes;

    BodyId next_body = 1;
    CharacterId next_character = 1;
    JointId next_joint = 1;
    std::size_t body_count = 0;
    std::size_t added_since_rebuild = 0;

    // kinematic bodies that are on their way to where they were moved to
    std::vector<BodyId> moved;

    [[nodiscard]] const BodyRecord *Find(const BodyId body) const
    {
      const auto it = bodies.find(body);
      return it == bodies.end() ? nullptr : &it->second;
    }

    /// Makes the shape Jolt works with, or hands over the one that was made
    /// for the same model already. Returns nullptr and says why when it
    /// cannot be made.
    JPH::RefConst<JPH::Shape> MakeShape(const ShapeInfo &info, std::string &error);

    JPH::RefConst<JPH::Shape> MakeShapes(const std::vector<ShapeInfo> &shapes, std::string &error);

    /// Lets go of every shared shape that no body holds any more.
    void ForgetUnusedShapes();

    /// Makes the shape of a query, which is held where the query says and
    /// has an inside. Returns nullptr and says why in the log when it
    /// cannot be made. `what` is the query, such as `An overlap cannot be
    /// looked for`.
    JPH::RefConst<JPH::Shape> MakeQueryShape(
      const ShapeInfo &info,
      const std::string &what,
      const std::shared_ptr<Logger> &logger);

    [[nodiscard]] bool IsResting(const std::pair<std::uint32_t, std::uint32_t> &pair, const Touch &touch) const;

    std::vector<PhysicsEvent> ReadContacts();

    State() { joint_filter.SetEmbedded(); }

    /// Takes a joint apart. What it held wakes up, since what was held
    /// up may now fall.
    void RemoveJoint(const JointRecord &record);

    /// Takes every joint apart that holds a body.
    void RemoveJointsOf(BodyId body);
  };

  void Jolt_PhysicsSystem::State::RemoveJoint(const JointRecord &record)
  {
    physics.RemoveConstraint(record.constraint);
    if (record.apart) { joint_filter.LetTouch(record.body, record.other); }

    auto &interface = physics.GetBodyInterface();
    for (const auto body : {record.body, record.other})
    {
      const auto *found = Find(body);
      if (found != nullptr && found->kind == BodyKind::Dynamic && !found->trigger)
      {
        interface.ActivateBody(found->jolt);
      }
    }
  }

  void Jolt_PhysicsSystem::State::RemoveJointsOf(const BodyId body)
  {
    for (auto it = joints.begin(); it != joints.end();)
    {
      if (it->second.body == body || it->second.other == body)
      {
        RemoveJoint(it->second);
        it = joints.erase(it);
      } else
      {
        ++it;
      }
    }
  }

  JPH::RefConst<JPH::Shape> Jolt_PhysicsSystem::State::MakeShape(const ShapeInfo &info, std::string &error)
  {
    const auto name = Name(info.kind);

    if (!IsFinite(info.scale) || info.scale.x == 0.0f || info.scale.y == 0.0f || info.scale.z == 0.0f)
    {
      error = std::format(
        "the {} is scaled by [{}, {}, {}], where no axis can be 0",
        name, info.scale.x, info.scale.y, info.scale.z);
      return nullptr;
    }

    // the shape of a model is made once for every scale it is used at
    const bool of_model = !info.source.empty() &&
                          (info.kind == ShapeKind::ConvexHull || info.kind == ShapeKind::Mesh);
    const SharedShapeKey key{info.source, info.kind, info.scale.x, info.scale.y, info.scale.z};
    if (of_model)
    {
      if (const auto it = shared_shapes.find(key); it != shared_shapes.end()) { return it->second; }
    }

    JPH::ShapeSettings::ShapeResult result;
    auto scale = ToJolt(info.scale);

    switch (info.kind)
    {
      case ShapeKind::Box:
      {
        // a box is sized as it is made, which leaves its edges as round as
        // those of every other box
        const auto half = ToJolt(abs(info.size * info.scale) * 0.5f);
        const float smallest = half.ReduceMin();

        if (!(smallest > 0.0f) || !IsFinite(info.size))
        {
          error = std::format(
            "the box has a size of [{}, {}, {}], where every side has to be above 0",
            info.size.x, info.size.y, info.size.z);
          return nullptr;
        }

        result = JPH::BoxShapeSettings(half, std::min(JPH::cDefaultConvexRadius, smallest * 0.5f)).Create();
        scale = JPH::Vec3::sReplicate(1.0f);
        break;
      }

      case ShapeKind::Sphere:
      {
        if (!(info.radius > 0.0f))
        {
          error = std::format("the sphere has a radius of {}, which has to be above 0", info.radius);
          return nullptr;
        }

        result = JPH::SphereShapeSettings(info.radius).Create();
        break;
      }

      case ShapeKind::Capsule:
      {
        const float straight = info.height * 0.5f - info.radius;

        if (!(info.radius > 0.0f) || straight < -1e-5f)
        {
          error = std::format(
            "the capsule has a height of {} and a radius of {}, where the height has to be at least twice "
            "the radius, and the radius above 0",
            info.height, info.radius);
          return nullptr;
        }

        // a capsule that is as high as it is wide is a sphere
        result = straight <= 1e-5f
                   ? JPH::SphereShapeSettings(info.radius).Create()
                   : JPH::CapsuleShapeSettings(straight, info.radius).Create();
        break;
      }

      case ShapeKind::Cylinder:
      {
        if (!(info.radius > 0.0f) || !(info.height > 0.0f))
        {
          error = std::format(
            "the cylinder has a height of {} and a radius of {}, which both have to be above 0",
            info.height, info.radius);
          return nullptr;
        }

        const float half = info.height * 0.5f;
        const float round = std::min(JPH::cDefaultConvexRadius, std::min(half, info.radius) * 0.5f);
        result = JPH::CylinderShapeSettings(half, info.radius, round).Create();
        break;
      }

      case ShapeKind::TaperedCapsule:
      {
        const float straight = (info.height - info.top_radius - info.bottom_radius) * 0.5f;

        if (!(info.top_radius > 0.0f) || !(info.bottom_radius > 0.0f) || !(straight > 0.0f))
        {
          error = std::format(
            "the tapered capsule has a height of {} and radii of {} and {}, where the radii have to be "
            "above 0 and the height above both together",
            info.height, info.top_radius, info.bottom_radius);
          return nullptr;
        }

        result = JPH::TaperedCapsuleShapeSettings(straight, info.top_radius, info.bottom_radius).Create();

        // Jolt puts the middle between the two round ends where the shape
        // is. The wider end then reaches further than the other. Here the
        // shape reaches as far up as down, as every other shape does.
        if (!result.HasError() && info.top_radius != info.bottom_radius)
        {
          const float rise = (info.bottom_radius - info.top_radius) * 0.5f;
          result = JPH::RotatedTranslatedShapeSettings(
            JPH::Vec3(0.0f, rise, 0.0f), JPH::Quat::sIdentity(), result.Get()).Create();
        }
        break;
      }

      case ShapeKind::TaperedCylinder:
      {
        const float widest = std::max(info.top_radius, info.bottom_radius);

        if (info.top_radius < 0.0f || info.bottom_radius < 0.0f || !(widest > 0.0f) || !(info.height > 0.0f))
        {
          error = std::format(
            "the tapered cylinder has a height of {} and radii of {} and {}, where the height and one "
            "radius have to be above 0, and no radius below 0",
            info.height, info.top_radius, info.bottom_radius);
          return nullptr;
        }

        const float half = info.height * 0.5f;
        const float narrowest = std::min(info.top_radius, info.bottom_radius);

        // an end that is a point has nothing to round
        const float round = std::min(JPH::cDefaultConvexRadius, std::min(half, narrowest) * 0.5f);
        result = JPH::TaperedCylinderShapeSettings(half, info.top_radius, info.bottom_radius, round).Create();
        break;
      }

      case ShapeKind::Plane:
      {
        result = JPH::PlaneShapeSettings(JPH::Plane(JPH::Vec3::sAxisY(), 0.0f)).Create();

        // a plane has no size
        scale = JPH::Vec3::sReplicate(1.0f);
        break;
      }

      case ShapeKind::ConvexHull:
      {
        if (info.points.size() < 4)
        {
          error = std::format("the convex hull has {} points, where at least 4 are needed", info.points.size());
          return nullptr;
        }

        JPH::Array<JPH::Vec3> points;
        points.reserve(info.points.size());
        for (const auto &point : info.points) { points.push_back(ToJolt(point)); }

        result = JPH::ConvexHullShapeSettings(points).Create();
        break;
      }

      case ShapeKind::Mesh:
      {
        if (info.triangles.size() < 3 || info.triangles.size() % 3 != 0)
        {
          error = std::format(
            "the mesh has {} corners of triangles, where a number that 3 divides is needed, and at least 3",
            info.triangles.size());
          return nullptr;
        }

        JPH::VertexList points;
        points.reserve(info.points.size());
        for (const auto &point : info.points) { points.emplace_back(point.x, point.y, point.z); }

        JPH::IndexedTriangleList triangles;
        triangles.reserve(info.triangles.size() / 3);
        for (std::size_t i = 0; i + 2 < info.triangles.size(); i += 3)
        {
          const auto first = info.triangles[i];
          const auto second = info.triangles[i + 1];
          const auto third = info.triangles[i + 2];

          if (first >= info.points.size() || second >= info.points.size() || third >= info.points.size())
          {
            error = std::format(
              "triangle {} of the mesh names a point that is not there. The mesh has {} points",
              i / 3 + 1, info.points.size());
            return nullptr;
          }

          triangles.emplace_back(first, second, third);
        }

        result = JPH::MeshShapeSettings(std::move(points), std::move(triangles)).Create();
        break;
      }
    }

    if (result.HasError())
    {
      error = std::format("the {} cannot be made: {}", name, std::string(result.GetError().c_str()));
      return nullptr;
    }

    JPH::RefConst<JPH::Shape> shape = result.Get();

    if (!scale.IsClose(JPH::Vec3::sReplicate(1.0f), 0.0f))
    {
      if (!shape->IsValidScale(scale))
      {
        error = std::format(
          "the {} is scaled by [{}, {}, {}], and cannot be sized differently along these axes. A sphere "
          "needs the same for all three, and a capsule or a cylinder the same for x and z",
          name, info.scale.x, info.scale.y, info.scale.z);
        return nullptr;
      }

      const auto scaled = shape->ScaleShape(scale);
      if (scaled.HasError())
      {
        error = std::format("the {} cannot be scaled: {}", name, std::string(scaled.GetError().c_str()));
        return nullptr;
      }
      shape = scaled.Get();
    }

    if (of_model) { shared_shapes[key] = shape; }
    return shape;
  }

  void Jolt_PhysicsSystem::State::ForgetUnusedShapes()
  {
    for (auto it = shared_shapes.begin(); it != shared_shapes.end();)
    {
      // the map holds the one reference that is left
      if (it->second->GetRefCount() == 1)
      {
        it = shared_shapes.erase(it);
      } else
      {
        ++it;
      }
    }
  }

  JPH::RefConst<JPH::Shape> Jolt_PhysicsSystem::State::MakeShapes(
    const std::vector<ShapeInfo> &shapes,
    std::string &error)
  {
    if (shapes.empty())
    {
      error = "it has no shape";
      return nullptr;
    }

    const auto is_in_place = [](const ShapeInfo &info)
    {
      return info.position == glm::vec3{0.0f} && std::abs(info.rotation.w) == 1.0f;
    };

    if (shapes.size() == 1)
    {
      const auto &info = shapes.front();
      if (!IsFinite(info.position))
      {
        error = std::format("the {} sits at a place that is no number", Name(info.kind));
        return nullptr;
      }

      auto shape = MakeShape(info, error);
      if (shape == nullptr || is_in_place(info)) { return shape; }

      const auto moved = JPH::RotatedTranslatedShapeSettings(
        ToJolt(info.position), ToJolt(info.rotation), shape).Create();
      if (moved.HasError())
      {
        error = std::format("the {} cannot be placed: {}", Name(info.kind), std::string(moved.GetError().c_str()));
        return nullptr;
      }
      return moved.Get();
    }

    JPH::StaticCompoundShapeSettings compound;
    for (const auto &info : shapes)
    {
      if (!IsFinite(info.position))
      {
        error = std::format("the {} sits at a place that is no number", Name(info.kind));
        return nullptr;
      }

      const auto shape = MakeShape(info, error);
      if (shape == nullptr) { return nullptr; }

      compound.AddShape(ToJolt(info.position), ToJolt(info.rotation), shape);
    }

    const auto result = compound.Create();
    if (result.HasError())
    {
      error = std::format("its shapes cannot be put together: {}", std::string(result.GetError().c_str()));
      return nullptr;
    }
    return result.Get();
  }

  JPH::RefConst<JPH::Shape> Jolt_PhysicsSystem::State::MakeQueryShape(
    const ShapeInfo &info,
    const std::string &what,
    const std::shared_ptr<Logger> &logger)
  {
    if (info.kind == ShapeKind::Mesh || info.kind == ShapeKind::Plane)
    {
      const auto name = Name(info.kind);
      logger->Error("{} with a {}, which has no inside", what, name);
      return nullptr;
    }

    // where the shape sits is what the call says
    ShapeInfo in_place = info;
    in_place.position = glm::vec3{0.0f};
    in_place.rotation = glm::quat{1.0f, 0.0f, 0.0f, 0.0f};

    std::string error;
    const auto made = MakeShape(in_place, error);
    if (made == nullptr) { logger->Error("{}: {}", what, error); }
    return made;
  }

  bool Jolt_PhysicsSystem::State::IsResting(
    const std::pair<std::uint32_t, std::uint32_t> &pair,
    const Touch &touch) const
  {
    // one of them is gone
    if (Find(touch.first_body) == nullptr || Find(touch.second_body) == nullptr) { return false; }

    const auto &interface = physics.GetBodyInterfaceNoLock();
    return !interface.IsActive(JPH::BodyID(pair.first)) && !interface.IsActive(JPH::BodyID(pair.second));
  }

  std::vector<PhysicsEvent> Jolt_PhysicsSystem::State::ReadContacts()
  {
    struct Change
    {
      int added = 0;
      int removed = 0;
      bool has_contact = false;
      Found contact;
    };

    // by pair, so that the order the threads found them in does not count
    std::map<std::pair<std::uint32_t, std::uint32_t>, Change> changes;

    for (const auto &found : contacts.Take())
    {
      auto &change = changes[{found.first, found.second}];

      if (!found.added)
      {
        change.removed++;
        continue;
      }

      change.added++;

      // of several parts that began to touch, the one that is named is
      // always the same
      if (!change.has_contact
          || std::pair(found.first_part, found.second_part)
          < std::pair(change.contact.first_part, change.contact.second_part))
      {
        change.has_contact = true;
        change.contact = found;
      }
    }

    std::vector<PhysicsEvent> events;

    const auto report = [&events](const PhysicsEventKind kind, const Touch &touch, const Found *contact)
    {
      PhysicsEvent event;
      event.kind = kind;
      event.trigger = touch.trigger;
      event.first = touch.first;
      event.second = touch.second;
      event.first_body = touch.first_body;
      event.second_body = touch.second_body;

      if (contact != nullptr)
      {
        // Jolt points from the first body it names to the second, which
        // are the other way around when they were put in order here
        const bool same_order = contact->first_body == touch.first_body;
        event.point = contact->point;
        event.normal = same_order ? contact->normal : -contact->normal;
      }

      events.push_back(event);
    };

    for (const auto &[pair, change] : changes)
    {
      auto it = touches.find(pair);
      const int before = it == touches.end() ? 0 : it->second.count;
      const int after = std::max(0, before + change.added - change.removed);

      if (it == touches.end())
      {
        if (after == 0 || !change.has_contact) { continue; }

        const auto *first = Find(change.contact.first_body);
        const auto *second = Find(change.contact.second_body);
        if (first == nullptr || second == nullptr) { continue; }

        Touch touch;
        touch.trigger = first->trigger || second->trigger;

        // the trigger comes first. Without one, the body that was created
        // first does
        const bool swap = touch.trigger
                            ? second->trigger
                            : change.contact.second_body < change.contact.first_body;

        touch.first_body = swap ? change.contact.second_body : change.contact.first_body;
        touch.second_body = swap ? change.contact.first_body : change.contact.second_body;
        touch.first = swap ? second->entity : first->entity;
        touch.second = swap ? first->entity : second->entity;

        it = touches.emplace(pair, touch).first;
      }

      auto &touch = it->second;
      touch.count = after;

      if (before == 0 && after > 0)
      {
        if (touch.asleep)
        {
          // they woke up and still touch, which is nothing new
          touch.asleep = false;
        } else
        {
          report(PhysicsEventKind::Began, touch, change.has_contact ? &change.contact : nullptr);
        }
      } else if (before > 0 && after == 0)
      {
        // Jolt stops looking at bodies that came to rest, and says that
        // their contact is gone. They still touch.
        if (IsResting(pair, touch))
        {
          touch.asleep = true;
        } else
        {
          report(PhysicsEventKind::Ended, touch, nullptr);
          touches.erase(it);
        }
      }
    }

    // What rested and touched, and was then moved away or destroyed, never
    // touches again, and Jolt has nothing to say about it.
    for (auto it = touches.begin(); it != touches.end();)
    {
      if (it->second.asleep && it->second.count == 0 && !IsResting(it->first, it->second))
      {
        report(PhysicsEventKind::Ended, it->second, nullptr);
        it = touches.erase(it);
      } else
      {
        ++it;
      }
    }

    return events;
  }

  Jolt_PhysicsSystem::Jolt_PhysicsSystem(
    const SettingsConfig &settings_config,
    const std::shared_ptr<Logger> &logger) : PhysicsSystem(settings_config, logger) {}

  Jolt_PhysicsSystem::~Jolt_PhysicsSystem()
  {
    Jolt_PhysicsSystem::CleanUp();
  }

  void Jolt_PhysicsSystem::Initialize()
  {
    if (_state != nullptr) { return; }

    _logger->Info("Initializing the physics");

    AcquireShared();

    _state = std::make_unique<State>();

    // One thread is left to the application. The result does not depend on
    // how many there are.
    const auto threads = std::clamp(static_cast<int>(std::thread::hardware_concurrency()) - 1, 1, 16);
    _state->jobs = std::make_unique<JPH::JobSystemThreadPool>(
      JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, threads);

    _state->physics.Init(
      most_bodies,
      0,
      most_body_pairs,
      most_contacts,
      _state->layers,
      _state->layers,
      _state->layers);

    _state->physics.SetContactListener(&_state->contacts);
    _state->physics.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

    _logger->Info("Initialized the physics with {} threads!", threads);
  }

  void Jolt_PhysicsSystem::CleanUp()
  {
    if (_state == nullptr) { return; }

    _logger->Info("Cleaning up the physics");

    // joints first, since each holds bodies, then characters, since each
    // holds a body
    for (const auto &[id, record] : _state->joints) { _state->physics.RemoveConstraint(record.constraint); }
    _state->joints.clear();
    _state->characters.clear();

    auto &interface = _state->physics.GetBodyInterface();
    for (const auto &[id, record] : _state->bodies)
    {
      if (record.of_character) { continue; }

      interface.RemoveBody(record.jolt);
      interface.DestroyBody(record.jolt);
    }

    _state.reset();
    ForgetEvents();

    ReleaseShared();
  }

  bool Jolt_PhysicsSystem::CreateBody(const BodyInfo &info, BodyId &body, std::string &error)
  {
    body = No_Body;

    if (_state == nullptr)
    {
      error = "the physics is not initialized";
      return false;
    }

    const bool dynamic = info.kind == BodyKind::Dynamic && !info.trigger;

    if (!ShapesFit(info.shapes, info.kind, info.trigger, error)) { return false; }

    if (dynamic && !(info.mass > 0.0f && std::isfinite(info.mass)))
    {
      error = std::format("a dynamic body has a mass above 0, and this one has {}", info.mass);
      return false;
    }

    if (dynamic && AllowedDegrees(info.locked_position, info.locked_rotation) == JPH::EAllowedDOFs::None)
    {
      error = "a dynamic body that is locked along every axis and around every axis cannot move at all. Make "
              "it static";
      return false;
    }

    if (!IsFinite(info.position) || !IsFinite(info.linear_velocity) || !IsFinite(info.angular_velocity))
    {
      error = "where it is or how it moves is no number";
      return false;
    }

    const auto shape = _state->MakeShapes(info.shapes, error);
    if (shape == nullptr) { return false; }

    JPH::ObjectLayer layer = 0;
    const LayerEntry entry{
      .layers = info.layers,
      .mask = info.mask,
      .moving = info.kind != BodyKind::Static,
      .trigger = info.trigger
    };
    if (!_state->layers.Find(entry, layer))
    {
      error = "there are more different sets of layers and masks than the physics can tell apart";
      return false;
    }

    auto motion = JPH::EMotionType::Static;
    if (info.kind == BodyKind::Kinematic) { motion = JPH::EMotionType::Kinematic; }
    if (info.kind == BodyKind::Dynamic) { motion = JPH::EMotionType::Dynamic; }

    // A trigger that moved with the simulation would fall. It is where it
    // is put.
    if (info.trigger && motion == JPH::EMotionType::Dynamic) { motion = JPH::EMotionType::Kinematic; }

    JPH::BodyCreationSettings settings(shape, ToJolt(info.position), ToJolt(info.rotation), motion, layer);
    settings.mUserData = _state->next_body;
    settings.mFriction = std::max(0.0f, info.friction);
    settings.mRestitution = std::clamp(info.bounce, 0.0f, 1.0f);
    settings.mLinearDamping = std::max(0.0f, info.linear_damping);
    settings.mAngularDamping = std::max(0.0f, info.angular_damping);
    settings.mGravityFactor = info.gravity_scale;
    settings.mLinearVelocity = ToJolt(info.linear_velocity);
    settings.mAngularVelocity = ToJolt(info.angular_velocity);
    settings.mAllowSleeping = info.can_sleep;
    settings.mMotionQuality = info.continuous ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;

    if (info.trigger)
    {
      settings.mIsSensor = true;

      // A trigger sees what rests inside it and what does not move at all
      // only when it is awake and looks for them.
      settings.mAllowSleeping = false;
      settings.mCollideKinematicVsNonDynamic = true;
    }

    if (dynamic)
    {
      settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
      settings.mMassPropertiesOverride.mMass = info.mass;
      settings.mAllowedDOFs = AllowedDegrees(info.locked_position, info.locked_rotation);
    } else
    {
      // What is not moved by the simulation has no use for a mass. Jolt
      // asks the shape for one all the same, and a mesh has none to give.
      settings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
      settings.mMassPropertiesOverride.mMass = 1.0f;
      settings.mMassPropertiesOverride.mInertia = JPH::Mat44::sIdentity();
    }

    auto &interface = _state->physics.GetBodyInterface();

    const auto *created = interface.CreateBody(settings);
    if (created == nullptr)
    {
      error = std::format("the physics holds {} bodies, and has no room for more", most_bodies);
      return false;
    }

    interface.AddBody(
      created->GetID(),
      motion == JPH::EMotionType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);

    body = _state->next_body++;
    _state->bodies[body] = BodyRecord{
      .jolt = created->GetID(),
      .entity = info.entity,
      .kind = info.kind,
      .trigger = info.trigger,
      .mass = info.mass,
      .allowed = settings.mAllowedDOFs
    };
    _state->body_count++;
    _state->added_since_rebuild++;

    // so that a run without a window, and a test, can tell from the log
    // that a recipe got its body, and which shapes it has
    // the logger takes what it is given by reference
    const std::string what = info.trigger ? "trigger" : Name(info.kind) + " body";
    const auto shapes = Describe(info.shapes);
    _logger->Debug("Created {} for '{}' with {}", what, info.name, shapes);
    return true;
  }

  void Jolt_PhysicsSystem::DestroyBody(const BodyId body)
  {
    if (_state == nullptr) { return; }

    const auto it = _state->bodies.find(body);
    if (it == _state->bodies.end() || it->second.of_character) { return; }

    // a joint cannot hold what is gone
    _state->RemoveJointsOf(body);

    auto &interface = _state->physics.GetBodyInterface();
    interface.RemoveBody(it->second.jolt);
    interface.DestroyBody(it->second.jolt);

    _state->bodies.erase(it);
    _state->body_count--;
    _state->ForgetUnusedShapes();
  }

  void Jolt_PhysicsSystem::SetBodyInWorld(const BodyId body, const bool in_world)
  {
    if (_state == nullptr) { return; }

    const auto it = _state->bodies.find(body);
    if (it == _state->bodies.end() || it->second.of_character) { return; }

    auto &interface = _state->physics.GetBodyInterface();
    const JPH::BodyID jolt = it->second.jolt;
    if (interface.IsAdded(jolt) == in_world) { return; }

    if (!in_world)
    {
      interface.RemoveBody(jolt);
      return;
    }

    // a body that moves is awake when it comes back
    const bool is_static = interface.GetMotionType(jolt) == JPH::EMotionType::Static;
    interface.AddBody(jolt, is_static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
  }

  bool Jolt_PhysicsSystem::SetShape(const BodyId body, const std::vector<ShapeInfo> &shapes, std::string &error)
  {
    if (_state == nullptr)
    {
      error = "the physics is not initialized";
      return false;
    }

    const auto *record = _state->Find(body);
    if (record == nullptr || record->of_character)
    {
      error = record == nullptr
                ? "the body is not known"
                : "the body is that of a character, whose shape cannot change while it lives";
      return false;
    }

    if (!ShapesFit(shapes, record->kind, record->trigger, error)) { return false; }

    const auto shape = _state->MakeShapes(shapes, error);
    if (shape == nullptr) { return false; }

    auto &interface = _state->physics.GetBodyInterface();
    interface.SetShape(record->jolt, shape, false, JPH::EActivation::Activate);
    _state->ForgetUnusedShapes();

    // Jolt would give the body the mass of its new shape. The body keeps the
    // mass it was created with, spread over the new shape.
    if (record->kind == BodyKind::Dynamic && !record->trigger)
    {
      const JPH::BodyLockWrite lock(_state->physics.GetBodyLockInterface(), record->jolt);
      if (lock.Succeeded())
      {
        auto properties = shape->GetMassProperties();
        properties.ScaleToMass(record->mass);
        lock.GetBody().GetMotionProperties()->SetMassProperties(record->allowed, properties);
      }
    }
    return true;
  }

  bool Jolt_PhysicsSystem::HasBody(const BodyId body)
  {
    if (_state == nullptr) { return false; }

    const auto *record = _state->Find(body);
    return record != nullptr && !record->of_character;
  }

  std::size_t Jolt_PhysicsSystem::GetBodyCount()
  {
    return _state == nullptr ? 0 : _state->body_count;
  }

  std::size_t Jolt_PhysicsSystem::GetSharedShapeCount() const
  {
    return _state == nullptr ? 0 : _state->shared_shapes.size();
  }

  bool Jolt_PhysicsSystem::GetBodyState(const BodyId body, BodyState &state)
  {
    if (_state == nullptr) { return false; }

    const auto *record = _state->Find(body);
    if (record == nullptr) { return false; }

    const auto &interface = _state->physics.GetBodyInterface();

    JPH::RVec3 position;
    JPH::Quat rotation;
    interface.GetPositionAndRotation(record->jolt, position, rotation);

    JPH::Vec3 linear;
    JPH::Vec3 angular;
    interface.GetLinearAndAngularVelocity(record->jolt, linear, angular);

    state.position = ToGlm(JPH::Vec3(position));
    state.rotation = ToGlm(rotation);
    state.linear_velocity = ToGlm(linear);
    state.angular_velocity = ToGlm(angular);
    state.active = interface.IsActive(record->jolt);
    return true;
  }

  void Jolt_PhysicsSystem::SetBodyPlace(const BodyId body, const glm::vec3 &position, const glm::quat &rotation)
  {
    if (_state == nullptr || !IsFinite(position)) { return; }

    const auto *record = _state->Find(body);
    if (record == nullptr || record->of_character) { return; }

    auto &interface = _state->physics.GetBodyInterface();
    const bool rests = interface.GetMotionType(record->jolt) == JPH::EMotionType::Static;

    interface.SetPositionAndRotation(
      record->jolt,
      ToJolt(position),
      ToJolt(rotation),
      rests ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
  }

  void Jolt_PhysicsSystem::MoveBody(
    const BodyId body,
    const glm::vec3 &position,
    const glm::quat &rotation,
    const double seconds)
  {
    if (_state == nullptr || !IsFinite(position) || !(seconds > 0.0)) { return; }

    const auto *record = _state->Find(body);
    if (record == nullptr || record->of_character) { return; }

    auto &interface = _state->physics.GetBodyInterface();

    // what does not move at all is put there
    if (interface.GetMotionType(record->jolt) == JPH::EMotionType::Static)
    {
      SetBodyPlace(body, position, rotation);
      return;
    }

    interface.MoveKinematic(record->jolt, ToJolt(position), ToJolt(rotation), static_cast<float>(seconds));
    _state->moved.push_back(body);
  }

  void Jolt_PhysicsSystem::SetLinearVelocity(const BodyId body, const glm::vec3 &velocity)
  {
    if (_state == nullptr || !IsFinite(velocity)) { return; }

    const auto *record = _state->Find(body);
    if (record == nullptr || record->of_character) { return; }

    auto &interface = _state->physics.GetBodyInterface();
    if (interface.GetMotionType(record->jolt) == JPH::EMotionType::Static) { return; }

    interface.SetLinearVelocity(record->jolt, ToJolt(velocity));
  }

  void Jolt_PhysicsSystem::SetAngularVelocity(const BodyId body, const glm::vec3 &velocity)
  {
    if (_state == nullptr || !IsFinite(velocity)) { return; }

    const auto *record = _state->Find(body);
    if (record == nullptr || record->of_character) { return; }

    auto &interface = _state->physics.GetBodyInterface();
    if (interface.GetMotionType(record->jolt) == JPH::EMotionType::Static) { return; }

    interface.SetAngularVelocity(record->jolt, ToJolt(velocity));
  }

  void Jolt_PhysicsSystem::AddForce(const BodyId body, const glm::vec3 &force)
  {
    if (_state == nullptr || !IsFinite(force)) { return; }

    const auto *record = _state->Find(body);
    if (record == nullptr || record->of_character) { return; }

    auto &interface = _state->physics.GetBodyInterface();
    if (interface.GetMotionType(record->jolt) != JPH::EMotionType::Dynamic) { return; }

    interface.AddForce(record->jolt, ToJolt(force));
  }

  void Jolt_PhysicsSystem::AddTorque(const BodyId body, const glm::vec3 &torque)
  {
    if (_state == nullptr || !IsFinite(torque)) { return; }

    const auto *record = _state->Find(body);
    if (record == nullptr || record->of_character) { return; }

    auto &interface = _state->physics.GetBodyInterface();
    if (interface.GetMotionType(record->jolt) != JPH::EMotionType::Dynamic) { return; }

    interface.AddTorque(record->jolt, ToJolt(torque));
  }

  void Jolt_PhysicsSystem::AddImpulse(const BodyId body, const glm::vec3 &impulse)
  {
    if (_state == nullptr || !IsFinite(impulse)) { return; }

    const auto *record = _state->Find(body);
    if (record == nullptr || record->of_character) { return; }

    auto &interface = _state->physics.GetBodyInterface();
    if (interface.GetMotionType(record->jolt) != JPH::EMotionType::Dynamic) { return; }

    interface.AddImpulse(record->jolt, ToJolt(impulse));
  }

  void Jolt_PhysicsSystem::AddImpulseAt(const BodyId body, const glm::vec3 &impulse, const glm::vec3 &point)
  {
    if (_state == nullptr || !IsFinite(impulse) || !IsFinite(point)) { return; }

    const auto *record = _state->Find(body);
    if (record == nullptr || record->of_character) { return; }

    auto &interface = _state->physics.GetBodyInterface();
    if (interface.GetMotionType(record->jolt) != JPH::EMotionType::Dynamic) { return; }

    interface.AddImpulse(record->jolt, ToJolt(impulse), ToJolt(point));
  }

  void Jolt_PhysicsSystem::SetGravity(const glm::vec3 &gravity)
  {
    if (_state == nullptr || !IsFinite(gravity)) { return; }

    _state->physics.SetGravity(ToJolt(gravity));
  }

  glm::vec3 Jolt_PhysicsSystem::GetGravity()
  {
    if (_state == nullptr) { return {0.0f, -9.81f, 0.0f}; }

    return ToGlm(_state->physics.GetGravity());
  }

  bool Jolt_PhysicsSystem::CreateCharacter(const CharacterInfo &info, CharacterId &character, std::string &error)
  {
    character = No_Character;

    if (_state == nullptr)
    {
      error = "the physics is not initialized";
      return false;
    }

    for (const auto &shape : info.shapes)
    {
      if (shape.kind == ShapeKind::Mesh || shape.kind == ShapeKind::Plane)
      {
        error = std::format(
          "a character cannot have a {}. Give it a capsule, or another shape without dents",
          Name(shape.kind));
        return false;
      }
    }

    if (!IsFinite(info.position))
    {
      error = "where it is is no number";
      return false;
    }

    if (!(info.mass > 0.0f && std::isfinite(info.mass)))
    {
      error = std::format("a character has a mass above 0, and this one has {}", info.mass);
      return false;
    }

    const auto shape = _state->MakeShapes(info.shapes, error);
    if (shape == nullptr) { return false; }

    const LayerEntry layers{.layers = info.layers, .mask = info.mask, .moving = true, .trigger = false};

    JPH::ObjectLayer layer = 0;
    if (!_state->layers.Find(layers, layer))
    {
      error = "there are more different sets of layers and masks than the physics can tell apart";
      return false;
    }

    // What it touches with its lower end carries it. That end is as high
    // as the shape is round, which is half of its narrowest side.
    const auto bounds = shape->GetLocalBounds();
    const auto extent = bounds.GetExtent();
    const float round = extent.ReduceMin();

    JPH::CharacterVirtualSettings settings;
    settings.mShape = shape;
    settings.mInnerBodyShape = shape;
    settings.mInnerBodyLayer = layer;
    settings.mUp = JPH::Vec3::sAxisY();
    settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -(bounds.mMin.GetY() + round));
    settings.mMaxSlopeAngle = glm::radians(std::clamp(info.max_slope, 0.0f, 90.0f));
    settings.mMass = info.mass;
    settings.mMaxStrength = std::max(0.0f, info.push_strength);
    settings.mEnhancedInternalEdgeRemoval = true;

    // the body that the character is for the bodies around it is known as
    // a body, so that events and queries name its entity
    const BodyId body = _state->next_body++;

    // built where it stays, under the id it gets once it is known to be
    // complete
    const auto it = _state->characters.try_emplace(
      _state->next_character, settings, ToJolt(info.position), body, &_state->physics).first;
    auto &record = it->second;
    record.entity = info.entity;
    record.layers = layers;
    record.step_height = std::max(0.0f, info.step_height);
    record.cosine_of_slope = std::cos(glm::radians(std::clamp(info.max_slope, 0.0f, 90.0f)));

    record.character.SetListener(&_state->character_contacts);

    if (record.character.GetInnerBodyID().IsInvalid())
    {
      _state->characters.erase(it);
      error = std::format("the physics holds {} bodies, and has no room for more", most_bodies);
      return false;
    }

    _state->bodies[body] = BodyRecord{
      .jolt = record.character.GetInnerBodyID(),
      .entity = info.entity,
      .trigger = false,
      .of_character = true
    };
    _state->added_since_rebuild++;

    character = _state->next_character++;
    const auto shapes = Describe(info.shapes);
    _logger->Debug("Created character for '{}' with {}", info.name, shapes);
    return true;
  }

  void Jolt_PhysicsSystem::DestroyCharacter(const CharacterId character)
  {
    if (_state == nullptr) { return; }

    const auto it = _state->characters.find(character);
    if (it == _state->characters.end()) { return; }

    _state->bodies.erase(it->second.body);

    // the character takes its body with it
    _state->characters.erase(it);
  }

  void Jolt_PhysicsSystem::SetCharacterInWorld(const CharacterId character, const bool in_world)
  {
    if (_state == nullptr) { return; }

    const auto it = _state->characters.find(character);
    if (it == _state->characters.end()) { return; }

    // What others touch and find of a character is the body it carries
    // inside. The character itself moves only when it is told to.
    const JPH::BodyID inner = it->second.character.GetInnerBodyID();
    if (inner.IsInvalid()) { return; }

    auto &interface = _state->physics.GetBodyInterface();
    if (interface.IsAdded(inner) == in_world) { return; }

    if (in_world) { interface.AddBody(inner, JPH::EActivation::Activate); }
    else { interface.RemoveBody(inner); }
  }

  std::size_t Jolt_PhysicsSystem::GetCharacterCount()
  {
    return _state == nullptr ? 0 : _state->characters.size();
  }

  void Jolt_PhysicsSystem::SetCharacterPosition(const CharacterId character, const glm::vec3 &position)
  {
    if (_state == nullptr || !IsFinite(position)) { return; }

    const auto it = _state->characters.find(character);
    if (it == _state->characters.end()) { return; }

    it->second.character.SetPosition(ToJolt(position));
  }

  bool Jolt_PhysicsSystem::GetCharacterPosition(const CharacterId character, glm::vec3 &position)
  {
    if (_state == nullptr) { return false; }

    const auto it = _state->characters.find(character);
    if (it == _state->characters.end()) { return false; }

    position = ToGlm(JPH::Vec3(it->second.character.GetPosition()));
    return true;
  }

  bool Jolt_PhysicsSystem::MoveCharacter(
    const CharacterId character,
    const glm::vec3 &velocity,
    const double seconds,
    CharacterState &state)
  {
    if (_state == nullptr) { return false; }

    const auto it = _state->characters.find(character);
    if (it == _state->characters.end()) { return false; }

    auto &record = it->second;
    auto &moved = record.character;

    const auto before = moved.GetPosition();

    if (seconds > 0.0 && IsFinite(velocity))
    {
      const auto time = static_cast<float>(seconds);

      moved.SetLinearVelocity(ToJolt(velocity));

      // what asks for no way along the ground, and does not rise, stands
      // still on a slope instead of creeping down it
      const auto along_ground = glm::length(glm::vec2(velocity.x, velocity.z));
      _state->character_contacts.sliding_allowed = along_ground > 1e-6f || velocity.y > 0.0f || !moved.IsSupported();

      JPH::CharacterVirtual::ExtendedUpdateSettings update;
      update.mWalkStairsStepUp = JPH::Vec3(0.0f, record.step_height, 0.0f);

      // what rises, as in a jump, is not pulled back to the ground
      if (velocity.y > 0.1f) { update.mStickToFloorStepDown = JPH::Vec3::sZero(); }

      const LayerFilter layer_filter(&_state->layers, record.layers);

      // a character moves, and so meets what moves and what does not
      const JPH::BroadPhaseLayerFilter everything;

      moved.ExtendedUpdate(
        time,
        _state->physics.GetGravity(),
        update,
        everything,
        layer_filter,
        {},
        {},
        _state->temporary);

      WalkStairsAlongAWall(moved, before, ToJolt(velocity), time, update, everything, layer_filter, _state->temporary);

      state.velocity = ToGlm(JPH::Vec3(moved.GetPosition() - before)) / time;
    } else
    {
      state.velocity = glm::vec3{0.0f};
    }

    state.position = ToGlm(JPH::Vec3(moved.GetPosition()));
    state.on_floor = moved.GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;
    state.on_wall = moved.GetGroundState() == JPH::CharacterBase::EGroundState::OnSteepGround;
    state.on_ceiling = false;
    state.floor_normal = glm::vec3{0.0f, 1.0f, 0.0f};
    state.floor = No_Entity;

    if (state.on_floor)
    {
      state.floor_normal = ToGlm(moved.GetGroundNormal());

      if (const auto *ground = _state->Find(static_cast<BodyId>(moved.GetGroundUserData())); ground != nullptr)
      {
        state.floor = ground->entity;
      }
    }

    for (const auto &contact : moved.GetActiveContacts())
    {
      if (!contact.mHadCollision || contact.mIsSensorB) { continue; }

      // the normal points to the character: up from a floor, down from a
      // ceiling, and to the side from a wall
      const float up = contact.mContactNormal.GetY();
      if (up < -record.cosine_of_slope)
      {
        state.on_ceiling = true;
      } else if (up < record.cosine_of_slope)
      {
        state.on_wall = true;
      }
    }

    return true;
  }

  bool Jolt_PhysicsSystem::CreateJoint(const JointInfo &info, JointId &joint, std::string &error)
  {
    joint = No_Joint;

    if (_state == nullptr)
    {
      error = "the physics is not initialized";
      return false;
    }

    const auto *first = _state->Find(info.body);
    if (first == nullptr || first->of_character)
    {
      error = first == nullptr
                ? "the body is not known"
                : "the body is that of a character, which cannot be joined to anything";
      return false;
    }

    const auto *second = info.other == No_Body ? nullptr : _state->Find(info.other);
    if (info.other != No_Body && (second == nullptr || second->of_character))
    {
      error = second == nullptr
                ? "the other body is not known"
                : "the other body is that of a character, which cannot be joined to anything";
      return false;
    }

    const bool first_dynamic = first->kind == BodyKind::Dynamic && !first->trigger;
    const bool second_dynamic = second != nullptr && second->kind == BodyKind::Dynamic && !second->trigger;
    if (!first_dynamic && !second_dynamic)
    {
      error = "neither body is dynamic, so there is nothing for the joint to hold";
      return false;
    }

    if (!IsFinite(info.anchor) || !IsFinite(info.axis))
    {
      error = "where the joint sits or what it is along is no number";
      return false;
    }

    if (info.kind == JointKind::Rope && (!IsFinite(info.other_anchor) || !std::isfinite(info.length)))
    {
      error = "where the rope is held or how long it is is no number";
      return false;
    }

    const bool has_axis = info.kind == JointKind::Hinge || info.kind == JointKind::Slider;
    if (has_axis && !(length(info.axis) > 0.0f))
    {
      const auto name = Name(info.kind);
      error = std::format("a {} needs an axis, and this one has none", name);
      return false;
    }

    if (info.has_limits && (info.limit_min > 0.0f || info.limit_max < 0.0f))
    {
      error = std::format(
        "the limits are {} and {}, where the least is 0 or below and the most 0 or above",
        info.limit_min, info.limit_max);
      return false;
    }

    if (!std::isfinite(info.motor_velocity) || !std::isfinite(info.motor_strength)
        || !std::isfinite(info.spring_stiffness) || !std::isfinite(info.spring_damping))
    {
      error = "the motor or the spring is no number";
      return false;
    }

    if (info.motor_strength < 0.0f || info.spring_stiffness < 0.0f || info.spring_damping < 0.0f)
    {
      error = "the strength of the motor, the stiffness of the spring, and its damping are 0 or above";
      return false;
    }

    const bool has_motor = has_axis && info.motor_strength > 0.0f;
    const bool has_spring = has_axis && info.spring_stiffness > 0.0f;
    if (has_motor && has_spring)
    {
      error = "the joint has a motor and a spring, where it has one or the other";
      return false;
    }

    const auto anchor = ToJolt(info.anchor);
    const auto axis = has_axis ? ToJolt(normalize(info.axis)) : JPH::Vec3::sAxisY();
    const auto normal = axis.GetNormalizedPerpendicular();

    // The world, or the other body, comes first, and the body of the joint
    // second. Jolt measures how far the second turned or moved from the
    // first, which is what the limits say.
    const JPH::BodyID ids[] = {second == nullptr ? JPH::BodyID() : second->jolt, first->jolt};
    const JPH::BodyLockMultiWrite lock(_state->physics.GetBodyLockInterface(), ids, 2);

    auto *held = second == nullptr ? &JPH::Body::sFixedToWorld : lock.GetBody(0);
    auto *holding = lock.GetBody(1);
    if (held == nullptr || holding == nullptr)
    {
      error = "a body is not known";
      return false;
    }

    JPH::Ref<JPH::TwoBodyConstraint> constraint;

    // A motor drives the joint at a velocity, with at most the strength.
    // A spring is a motor that drives it to where it was made, with the
    // torque or the force of the spring equation.
    JPH::MotorSettings motor;
    if (has_motor)
    {
      motor.SetTorqueLimit(info.motor_strength);
      motor.SetForceLimit(info.motor_strength);
    }
    if (has_spring)
    {
      motor.mSpringSettings = JPH::SpringSettings(
        JPH::ESpringMode::StiffnessAndDamping, info.spring_stiffness, info.spring_damping);
    }

    switch (info.kind)
    {
      case JointKind::Fixed:
      {
        JPH::FixedConstraintSettings settings;
        settings.mSpace = JPH::EConstraintSpace::WorldSpace;
        settings.mPoint1 = anchor;
        settings.mPoint2 = anchor;
        constraint = settings.Create(*held, *holding);
        break;
      }

      case JointKind::Point:
      {
        JPH::PointConstraintSettings settings;
        settings.mSpace = JPH::EConstraintSpace::WorldSpace;
        settings.mPoint1 = anchor;
        settings.mPoint2 = anchor;
        constraint = settings.Create(*held, *holding);
        break;
      }

      case JointKind::Rope:
      {
        // a distance that may be anything from nothing up to the length
        // of the rope, between its end on each body
        JPH::DistanceConstraintSettings settings;
        settings.mSpace = JPH::EConstraintSpace::WorldSpace;
        settings.mPoint1 = ToJolt(info.other_anchor);
        settings.mPoint2 = anchor;
        settings.mMinDistance = 0.0f;
        settings.mMaxDistance = info.length > 0.0f ? info.length : length(info.anchor - info.other_anchor);
        constraint = settings.Create(*held, *holding);
        break;
      }

      case JointKind::Hinge:
      {
        JPH::HingeConstraintSettings settings;
        settings.mSpace = JPH::EConstraintSpace::WorldSpace;
        settings.mPoint1 = anchor;
        settings.mPoint2 = anchor;
        settings.mHingeAxis1 = axis;
        settings.mHingeAxis2 = axis;
        settings.mNormalAxis1 = normal;
        settings.mNormalAxis2 = normal;
        if (info.has_limits)
        {
          settings.mLimitsMin = std::clamp(info.limit_min, -JPH::JPH_PI, 0.0f);
          settings.mLimitsMax = std::clamp(info.limit_max, 0.0f, JPH::JPH_PI);
        }
        settings.mMotorSettings = motor;
        constraint = settings.Create(*held, *holding);

        auto *hinge = static_cast<JPH::HingeConstraint *>(constraint.GetPtr());
        if (has_motor)
        {
          hinge->SetMotorState(JPH::EMotorState::Velocity);
          hinge->SetTargetAngularVelocity(info.motor_velocity);
        } else if (has_spring)
        {
          hinge->SetMotorState(JPH::EMotorState::Position);
          hinge->SetTargetAngle(0.0f);
        }
        break;
      }

      case JointKind::Slider:
      {
        JPH::SliderConstraintSettings settings;
        settings.mSpace = JPH::EConstraintSpace::WorldSpace;
        settings.mAutoDetectPoint = false;
        settings.mPoint1 = anchor;
        settings.mPoint2 = anchor;
        settings.mSliderAxis1 = axis;
        settings.mSliderAxis2 = axis;
        settings.mNormalAxis1 = normal;
        settings.mNormalAxis2 = normal;
        if (info.has_limits)
        {
          settings.mLimitsMin = std::min(info.limit_min, 0.0f);
          settings.mLimitsMax = std::max(info.limit_max, 0.0f);
        }
        settings.mMotorSettings = motor;
        constraint = settings.Create(*held, *holding);

        auto *slider = static_cast<JPH::SliderConstraint *>(constraint.GetPtr());
        if (has_motor)
        {
          slider->SetMotorState(JPH::EMotorState::Velocity);
          slider->SetTargetVelocity(info.motor_velocity);
        } else if (has_spring)
        {
          slider->SetMotorState(JPH::EMotorState::Position);
          slider->SetTargetPosition(0.0f);
        }
        break;
      }
    }

    if (constraint == nullptr)
    {
      const auto name = Name(info.kind);
      error = std::format("the {} cannot be made", name);
      return false;
    }

    _state->physics.AddConstraint(constraint);

    JointRecord record{.constraint = constraint, .kind = info.kind, .body = info.body, .other = info.other};
    record.axis_on_held = held->GetRotation().Conjugated() * axis;

    // The two bodies are kept apart by the group filter, which each is
    // given once, with its own number as its group. Nothing collides with
    // the world itself, so there is nothing to keep apart from it.
    if (!info.collide_with_other && second != nullptr)
    {
      for (const auto &[body, id] : {std::pair{holding, info.body}, std::pair{held, info.other}})
      {
        if (body->GetCollisionGroup().GetGroupFilter() == nullptr)
        {
          body->SetCollisionGroup(
            JPH::CollisionGroup(&_state->joint_filter, id, JPH::CollisionGroup::cInvalidSubGroup));
        }
      }
      _state->joint_filter.KeepApart(info.body, info.other);
      record.apart = true;
    }

    // what is driven does not sleep through it
    if (has_motor || has_spring)
    {
      auto &interface = _state->physics.GetBodyInterfaceNoLock();
      if (first_dynamic) { interface.ActivateBody(holding->GetID()); }
      if (second_dynamic) { interface.ActivateBody(held->GetID()); }
    }

    joint = _state->next_joint++;
    _state->joints[joint] = record;
    return true;
  }

  bool Jolt_PhysicsSystem::GetJointState(const JointId joint, JointState &state)
  {
    state = JointState{};
    if (_state == nullptr) { return false; }

    const auto it = _state->joints.find(joint);
    if (it == _state->joints.end()) { return false; }

    auto &record = it->second;
    if (record.kind != JointKind::Hinge && record.kind != JointKind::Slider)
    {
      if (!record.warned)
      {
        record.warned = true;
        const auto name = Name(record.kind);
        _logger->Warn(
          "The state of joint {} was asked for, and a {} joint has none. Only a hinge has an angle and a slider "
          "a position, which read as 0 here",
          joint, name);
      }
      return true;
    }

    // Jolt measures the second body against the first, which holds, as
    // the limits and the motor do
    const auto *held = record.constraint->GetBody1();
    const auto *holding = record.constraint->GetBody2();
    const auto axis = held->GetRotation() * record.axis_on_held;

    if (record.kind == JointKind::Hinge)
    {
      const auto *hinge = static_cast<const JPH::HingeConstraint *>(record.constraint.GetPtr());
      state.position = hinge->GetCurrentAngle();
      state.velocity = (holding->GetAngularVelocity() - held->GetAngularVelocity()).Dot(axis);
    } else
    {
      const auto *slider = static_cast<const JPH::SliderConstraint *>(record.constraint.GetPtr());
      const auto point = holding->GetCenterOfMassPosition();
      state.position = slider->GetCurrentPosition();
      state.velocity = (holding->GetPointVelocity(point) - held->GetPointVelocity(point)).Dot(axis);
    }
    return true;
  }

  void Jolt_PhysicsSystem::DestroyJoint(const JointId joint)
  {
    if (_state == nullptr) { return; }

    const auto it = _state->joints.find(joint);
    if (it == _state->joints.end()) { return; }

    _state->RemoveJoint(it->second);
    _state->joints.erase(it);
  }

  void Jolt_PhysicsSystem::SetJointEnabled(const JointId joint, const bool enabled)
  {
    if (_state == nullptr) { return; }

    const auto it = _state->joints.find(joint);
    if (it == _state->joints.end() || it->second.constraint->GetEnabled() == enabled) { return; }

    it->second.constraint->SetEnabled(enabled);

    // what it held, or holds again, has to be looked at anew
    auto &interface = _state->physics.GetBodyInterface();
    for (const auto body : {it->second.body, it->second.other})
    {
      const auto found = _state->bodies.find(body);
      if (found != _state->bodies.end() && interface.IsAdded(found->second.jolt)
          && interface.GetMotionType(found->second.jolt) == JPH::EMotionType::Dynamic)
      {
        interface.ActivateBody(found->second.jolt);
      }
    }
  }

  bool Jolt_PhysicsSystem::HasJoint(const JointId joint)
  {
    return _state != nullptr && _state->joints.contains(joint);
  }

  std::size_t Jolt_PhysicsSystem::GetJointCount()
  {
    return _state == nullptr ? 0 : _state->joints.size();
  }

  void Jolt_PhysicsSystem::Step(const double seconds)
  {
    if (_state == nullptr || !(seconds > 0.0) || !std::isfinite(seconds))
    {
      Report({});
      return;
    }

    // Jolt finds bodies with a tree that is good once it was rebuilt, and
    // that it only adds to by itself.
    if (_state->added_since_rebuild >= rebuild_after)
    {
      _state->physics.OptimizeBroadPhase();
      _state->added_since_rebuild = 0;
    }

    const auto problems = _state->physics.Update(
      static_cast<float>(seconds),
      1,
      &_state->temporary,
      _state->jobs.get());

    if (problems != JPH::EPhysicsUpdateError::None)
    {
      const auto code = static_cast<int>(problems);
      _logger->Error(
        "The physics has more to keep track of than it has room for, and left contacts out (code {})",
        code);
    }

    // Jolt moves a kinematic body by giving it the velocity that takes it
    // there in one step, and leaves it with that velocity. It has arrived,
    // and stays.
    auto &interface = _state->physics.GetBodyInterface();
    for (const auto body : _state->moved)
    {
      if (const auto *record = _state->Find(body); record != nullptr)
      {
        interface.SetLinearAndAngularVelocity(record->jolt, JPH::Vec3::sZero(), JPH::Vec3::sZero());
      }
    }
    _state->moved.clear();

    Report(_state->ReadContacts());
  }

  bool Jolt_PhysicsSystem::CastRay(const Ray &ray, const QueryFilter &filter, RayHit &hit)
  {
    if (_state == nullptr) { return false; }

    const float reach = length(ray.direction);
    if (!(reach > 0.0f) || !(ray.distance > 0.0f) || !IsFinite(ray.origin) || !IsFinite(ray.direction))
    {
      return false;
    }

    const auto direction = ray.direction / reach;
    const JPH::RRayCast cast(ToJolt(ray.origin), ToJolt(direction * ray.distance));

    const LayerFilter layer_filter(&_state->layers, filter);
    const IgnoreEntity body_filter(&_state->bodies, filter.ignore);

    JPH::RayCastResult result;
    if (!_state->physics.GetNarrowPhaseQuery().CastRay(cast, result, {}, layer_filter, body_filter))
    {
      return false;
    }

    const auto point = cast.GetPointOnRay(result.mFraction);

    const JPH::BodyLockRead lock(_state->physics.GetBodyLockInterface(), result.mBodyID);
    if (!lock.Succeeded()) { return false; }

    const auto &body = lock.GetBody();
    const auto id = static_cast<BodyId>(body.GetUserData());
    const auto *record = _state->Find(id);

    hit.body = id;
    hit.entity = record == nullptr ? No_Entity : record->entity;
    hit.trigger = body.IsSensor();
    hit.point = ToGlm(JPH::Vec3(point));
    hit.normal = ToGlm(body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, point));
    hit.distance = result.mFraction * ray.distance;
    return true;
  }

  void Jolt_PhysicsSystem::Overlap(
    const ShapeInfo &shape,
    const glm::vec3 &position,
    const glm::quat &rotation,
    const QueryFilter &filter,
    std::vector<OverlapHit> &hits)
  {
    hits.clear();
    if (_state == nullptr || !IsFinite(position)) { return; }

    const auto made = _state->MakeQueryShape(shape, "An overlap cannot be looked for", _logger);
    if (made == nullptr) { return; }

    const auto place = JPH::RMat44::sRotationTranslation(ToJolt(rotation), ToJolt(position))
                       * JPH::RMat44::sTranslation(made->GetCenterOfMass());

    const LayerFilter layer_filter(&_state->layers, filter);
    const IgnoreEntity body_filter(&_state->bodies, filter.ignore);

    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
    _state->physics.GetNarrowPhaseQuery().CollideShape(
      made,
      JPH::Vec3::sReplicate(1.0f),
      place,
      JPH::CollideShapeSettings(),
      JPH::RVec3::sZero(),
      collector,
      {},
      layer_filter,
      body_filter);

    const auto &interface = _state->physics.GetBodyInterface();

    for (const auto &found : collector.mHits)
    {
      const auto id = static_cast<BodyId>(interface.GetUserData(found.mBodyID2));

      // a body of several shapes is found once for each
      if (std::ranges::any_of(hits, [id](const OverlapHit &hit) { return hit.body == id; })) { continue; }

      const auto *record = _state->Find(id);
      if (record == nullptr) { continue; }

      hits.push_back(OverlapHit{.entity = record->entity, .body = id, .trigger = record->trigger});
    }

    std::ranges::sort(hits, [](const OverlapHit &left, const OverlapHit &right) { return left.body < right.body; });
  }
  bool Jolt_PhysicsSystem::CastShape(
    const ShapeInfo &shape,
    const glm::vec3 &from,
    const glm::quat &rotation,
    const glm::vec3 &to,
    const QueryFilter &filter,
    ShapeCastHit &hit)
  {
    if (_state == nullptr || !IsFinite(from) || !IsFinite(to)) { return false; }

    const auto made = _state->MakeQueryShape(shape, "A shape cannot be cast", _logger);
    if (made == nullptr) { return false; }

    const auto start = JPH::RMat44::sRotationTranslation(ToJolt(rotation), ToJolt(from));
    const auto cast = JPH::RShapeCast::sFromWorldTransform(
      made, JPH::Vec3::sReplicate(1.0f), start, ToJolt(to - from));

    const LayerFilter layer_filter(&_state->layers, filter);
    const IgnoreEntity body_filter(&_state->bodies, filter.ignore);

    // what the shape touches where it starts is found as well, at 0
    JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
    _state->physics.GetNarrowPhaseQuery().CastShape(
      cast,
      JPH::ShapeCastSettings(),
      JPH::RVec3::sZero(),
      collector,
      {},
      layer_filter,
      body_filter);

    if (!collector.HadHit()) { return false; }

    const auto &found = collector.mHit;

    const JPH::BodyLockRead lock(_state->physics.GetBodyLockInterface(), found.mBodyID2);
    if (!lock.Succeeded()) { return false; }

    const auto &body = lock.GetBody();
    const auto id = static_cast<BodyId>(body.GetUserData());
    const auto *record = _state->Find(id);

    hit.body = id;
    hit.entity = record == nullptr ? No_Entity : record->entity;
    hit.trigger = body.IsSensor();
    hit.point = ToGlm(JPH::Vec3(found.mContactPointOn2));
    hit.normal = ToGlm(body.GetWorldSpaceSurfaceNormal(found.mSubShapeID2, found.mContactPointOn2));
    hit.fraction = found.mFraction;
    hit.distance = found.mFraction * length(to - from);
    return true;
  }
} // neon
