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
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/RayCast.h>
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
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

namespace neon
{
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
    struct BodyRecord
    {
      JPH::BodyID jolt;
      Entity entity = No_Entity;
      bool trigger = false;

      /// The body that a character is for the bodies around it.
      bool of_character = false;
    };

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

    // by the two numbers Jolt knows the bodies as
    std::map<std::pair<std::uint32_t, std::uint32_t>, Touch> touches;

    BodyId next_body = 1;
    CharacterId next_character = 1;
    std::size_t body_count = 0;
    std::size_t added_since_rebuild = 0;

    // kinematic bodies that are on their way to where they were moved to
    std::vector<BodyId> moved;

    [[nodiscard]] const BodyRecord *Find(const BodyId body) const
    {
      const auto it = bodies.find(body);
      return it == bodies.end() ? nullptr : &it->second;
    }

    /// Makes the shape Jolt works with. Returns nullptr and says why when
    /// it cannot be made.
    JPH::RefConst<JPH::Shape> MakeShape(const ShapeInfo &info, std::string &error) const;

    JPH::RefConst<JPH::Shape> MakeShapes(const std::vector<ShapeInfo> &shapes, std::string &error) const;

    [[nodiscard]] bool IsResting(const std::pair<std::uint32_t, std::uint32_t> &pair, const Touch &touch) const;

    std::vector<PhysicsEvent> ReadContacts();
  };

  JPH::RefConst<JPH::Shape> Jolt_PhysicsSystem::State::MakeShape(const ShapeInfo &info, std::string &error) const
  {
    const auto name = Name(info.kind);

    if (!IsFinite(info.scale) || info.scale.x == 0.0f || info.scale.y == 0.0f || info.scale.z == 0.0f)
    {
      error = std::format(
        "the {} is scaled by [{}, {}, {}], where no axis can be 0",
        name, info.scale.x, info.scale.y, info.scale.z);
      return nullptr;
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

    return shape;
  }

  JPH::RefConst<JPH::Shape> Jolt_PhysicsSystem::State::MakeShapes(
    const std::vector<ShapeInfo> &shapes,
    std::string &error) const
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

    // characters first, since each holds a body
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

    for (const auto &shape : info.shapes)
    {
      if (shape.kind == ShapeKind::Mesh && dynamic)
      {
        error = "a dynamic body cannot have a mesh. A mesh is a surface without an inside, so it has no "
                "mass. Make the body static or kinematic, or give it a convex hull";
        return false;
      }

      if (shape.kind == ShapeKind::Plane && (info.kind != BodyKind::Static || info.trigger))
      {
        error = "only a static body can have a plane, which has no end and cannot move";
        return false;
      }
    }

    if (dynamic && !(info.mass > 0.0f && std::isfinite(info.mass)))
    {
      error = std::format("a dynamic body has a mass above 0, and this one has {}", info.mass);
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
    _state->bodies[body] = BodyRecord{.jolt = created->GetID(), .entity = info.entity, .trigger = info.trigger};
    _state->body_count++;
    _state->added_since_rebuild++;
    return true;
  }

  void Jolt_PhysicsSystem::DestroyBody(const BodyId body)
  {
    if (_state == nullptr) { return; }

    const auto it = _state->bodies.find(body);
    if (it == _state->bodies.end() || it->second.of_character) { return; }

    auto &interface = _state->physics.GetBodyInterface();
    interface.RemoveBody(it->second.jolt);
    interface.DestroyBody(it->second.jolt);

    _state->bodies.erase(it);
    _state->body_count--;
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

    if (shape.kind == ShapeKind::Mesh || shape.kind == ShapeKind::Plane)
    {
      const auto name = Name(shape.kind);
      _logger->Error("An overlap cannot be looked for with a {}, which has no inside", name);
      return;
    }

    // where the shape sits is what the call says
    ShapeInfo in_place = shape;
    in_place.position = glm::vec3{0.0f};
    in_place.rotation = glm::quat{1.0f, 0.0f, 0.0f, 0.0f};

    std::string error;
    const auto made = _state->MakeShape(in_place, error);
    if (made == nullptr)
    {
      _logger->Error("An overlap cannot be looked for: {}", error);
      return;
    }

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
} // neon
