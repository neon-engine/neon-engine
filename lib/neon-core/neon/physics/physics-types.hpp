#ifndef PHYSICS_TYPES_HPP
#define PHYSICS_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <neon/world-system/ecs/entity.hpp>

// What is handed to the physics and what it hands back. Everything is in the
// space of the world: positions in units, orientations as quaternions,
// velocities in units per second, and turning in radians per second.

namespace neon
{
  /// Names a body, as handed out when it was created.
  using BodyId = std::uint32_t;

  /// Names a character, as handed out when it was created.
  using CharacterId = std::uint32_t;

  /// Stands for no body, such as that of a component whose body was not
  /// created yet.
  // ReSharper disable once CppInconsistentNaming
  inline constexpr BodyId No_Body = 0;

  // ReSharper disable once CppInconsistentNaming
  inline constexpr CharacterId No_Character = 0;

  /// Every one of the 32 layers.
  // ReSharper disable once CppInconsistentNaming
  inline constexpr std::uint32_t All_Layers = 0xffffffffu;

  /// The axes of the world, one bit each, for what is locked on a body.
  // ReSharper disable CppInconsistentNaming
  inline constexpr std::uint8_t Axis_X = 1;
  inline constexpr std::uint8_t Axis_Y = 2;
  inline constexpr std::uint8_t Axis_Z = 4;
  // ReSharper restore CppInconsistentNaming

  /// How a body moves.
  enum class BodyKind
  {
    /// It does not move. Floors and walls.
    Static = 0,

    /// It is moved by code. What is in its way is pushed, and nothing pushes
    /// it.
    Kinematic,

    /// It is moved by the simulation: gravity, forces, and what it hits.
    Dynamic
  };

  enum class ShapeKind
  {
    Box = 0,
    Sphere,

    /// A cylinder with half a sphere at each end, along the Y axis.
    Capsule,

    /// Along the Y axis.
    Cylinder,

    /// A capsule whose ends have different radii.
    TaperedCapsule,

    /// A cylinder whose ends have different radii. With one of 0 it is a
    /// cone.
    TaperedCylinder,

    /// Everything below a plane that has no end. Only a body that does not
    /// move can have it.
    Plane,

    /// The smallest shape without dents that holds a set of points.
    ConvexHull,

    /// Triangles as they are, dents and holes included. A dynamic body
    /// cannot have it.
    Mesh
  };

  /// One shape of a body. Which of the values count depends on the kind.
  struct ShapeInfo
  {
    ShapeKind kind = ShapeKind::Box;

    /// The lengths of the sides of a box.
    glm::vec3 size{1.0f};

    /// Of a sphere, a capsule, and a cylinder.
    float radius = 0.5f;

    /// Of a capsule and a cylinder, and of their tapered kinds, from end to
    /// end. That of a capsule includes its round ends.
    float height = 2.0f;

    /// Of the tapered kinds.
    float top_radius = 0.25f;
    float bottom_radius = 0.5f;

    /// The points of a convex hull, and the corners of the triangles of a
    /// mesh.
    std::vector<glm::vec3> points;

    /// Three for every triangle of a mesh, each the number of a point.
    std::vector<std::uint32_t> triangles;

    /// What the points of a convex hull or a mesh were read from: the path
    /// of the model, with its fit when that is not `none`. Two shapes with
    /// the same source and scale have the same points, so a backend may
    /// hold one shape for both. Empty for points that are nobody else's,
    /// such as those a Geometry built.
    std::string source;

    /// Where the shape sits on its body.
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

    /// A sphere, a capsule, and a cylinder keep their form only when every
    /// axis is scaled the same, or, for the latter two, X and Z are.
    glm::vec3 scale{1.0f};
  };

  struct BodyInfo
  {
    /// The entity the body belongs to. It is what events and queries name.
    Entity entity = No_Entity;

    BodyKind kind = BodyKind::Dynamic;

    /// A trigger reports what overlaps it, and neither stops nor pushes
    /// anything.
    bool trigger = false;

    /// More than one make a compound.
    std::vector<ShapeInfo> shapes;

    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

    /// Of a dynamic body, above 0.
    float mass = 1.0f;

    /// 0 slides without end, 1 grips.
    float friction = 0.5f;

    /// How much of its speed the body keeps when it bounces, from 0 to 1.
    float bounce = 0.0f;

    /// How fast motion and turning die down by themselves.
    float linear_damping = 0.05f;
    float angular_damping = 0.05f;

    /// 0 floats, 1 falls as everything does.
    float gravity_scale = 1.0f;

    glm::vec3 linear_velocity{0.0f};

    /// Radians per second around each axis.
    glm::vec3 angular_velocity{0.0f};

    /// Looks for hits along the whole way of a step. For what is small and
    /// fast, such as a bullet, which would otherwise pass through a wall
    /// between two steps.
    bool continuous = false;

    /// A body that came to rest stops being simulated until something
    /// touches it.
    bool can_sleep = true;

    /// The axes of the world a dynamic body cannot move along, and those it
    /// cannot turn around, one bit each: Axis_X, Axis_Y, and Axis_Z. A
    /// crate that cannot tip over has its rotation locked around all three.
    /// A body that is locked on all six cannot be created, it is static
    /// instead.
    std::uint8_t locked_position = 0;
    std::uint8_t locked_rotation = 0;

    /// The layers the body is in, one bit each.
    std::uint32_t layers = 1;

    /// The layers the body looks for. See PhysicsContext.
    std::uint32_t mask = 1;
  };

  struct BodyState
  {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 linear_velocity{0.0f};
    glm::vec3 angular_velocity{0.0f};

    /// False when the body came to rest and sleeps.
    bool active = false;
  };

  struct CharacterInfo
  {
    Entity entity = No_Entity;

    std::vector<ShapeInfo> shapes;

    glm::vec3 position{0.0f};

    /// Steeper ground than this, in degrees, is a wall: the character slides
    /// down it and cannot walk up.
    float max_slope = 45.0f;

    /// A step up to this height is walked up as if it were a ramp.
    float step_height = 0.25f;

    /// How hard the character pushes dynamic bodies. It is the mass they
    /// feel standing on them, and the limit of the force it pushes with is
    /// `push_strength`.
    float mass = 70.0f;
    float push_strength = 100.0f;

    std::uint32_t layers = 1;
    std::uint32_t mask = 1;
  };

  struct CharacterState
  {
    glm::vec3 position{0.0f};

    /// What the character moved with, which is less than what was asked for
    /// when something was in the way.
    glm::vec3 velocity{0.0f};

    bool on_floor = false;

    /// Touches ground that is too steep to stand on.
    bool on_wall = false;

    bool on_ceiling = false;

    glm::vec3 floor_normal{0.0f, 1.0f, 0.0f};

    /// What it stands on, or No_Entity.
    Entity floor = No_Entity;
  };

  enum class PhysicsEventKind
  {
    /// Two bodies that did not touch now do.
    Began = 0,

    /// Two bodies that touched no longer do.
    Ended
  };

  /// Two bodies began or ended to touch.
  struct PhysicsEvent
  {
    PhysicsEventKind kind = PhysicsEventKind::Began;

    /// Whether one of the two is a trigger. Then `first` is the trigger and
    /// `second` what entered or left it.
    bool trigger = false;

    /// Without a trigger, the first is the one with the lower body id.
    Entity first = No_Entity;
    Entity second = No_Entity;
    BodyId first_body = No_Body;
    BodyId second_body = No_Body;

    /// Where they touch, and the direction from the first to the second.
    /// Both are 0 when the touch ended.
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
  };

  /// What a query looks at.
  struct QueryFilter
  {
    /// A body is looked at when it is in one of these layers.
    std::uint32_t mask = All_Layers;

    /// Whether triggers are looked at.
    bool triggers = false;

    /// The bodies of this entity are skipped, such as the one that asks.
    Entity ignore = No_Entity;
  };

  struct Ray
  {
    glm::vec3 origin{0.0f};

    /// Of any length but 0.
    glm::vec3 direction{0.0f, 0.0f, -1.0f};

    /// How far the ray reaches.
    float distance = 1000.0f;
  };

  struct RayHit
  {
    Entity entity = No_Entity;
    BodyId body = No_Body;
    bool trigger = false;

    glm::vec3 point{0.0f};

    /// Points away from what was hit.
    glm::vec3 normal{0.0f};

    /// From the origin of the ray to the point.
    float distance = 0.0f;
  };

  struct OverlapHit
  {
    Entity entity = No_Entity;
    BodyId body = No_Body;
    bool trigger = false;
  };
} // neon

#endif //PHYSICS_TYPES_HPP
