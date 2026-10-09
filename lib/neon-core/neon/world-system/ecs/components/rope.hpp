#ifndef ROPE_HPP
#define ROPE_HPP

#include <memory>
#include <string>

#include <glm/glm.hpp>

#include <neon/geometry/mesh-data.hpp>
#include <neon/reflection/type-builder.hpp>
#include <neon/world-system/ecs/entity.hpp>

namespace neon
{
  /// A rope drawn between two points that move: straight while they are as
  /// far apart as it is long, and hanging in a curve between them while
  /// they are nearer. The Renderable of the entity draws it, with its
  /// shader and material, in place of a model or a Geometry.
  ///
  /// It either draws the rope a Joint of type `rope` holds, by naming the
  /// entity of that joint, so that what is seen is what holds:
  ///
  ///     Rope:
  ///       joint: lamp
  ///
  /// or hangs between two places it names itself, with nothing held, as a
  /// cable between two poles does:
  ///
  ///     Rope:
  ///       from: pole-a
  ///       from_anchor: [0, 2, 0]
  ///       to: pole-b
  ///       to_anchor: [0, 2, 0]
  ///       length: 6
  ///
  /// The rope is where it comes to rest between its ends in every frame,
  /// see docs/curves.md. It collides with nothing.
  struct Rope
  {
    /// Path of the entity whose Joint of type `rope` is drawn, from the
    /// top. The ends and the length are then those of the joint, and
    /// `from`, `to`, and `length` are not read.
    std::string joint;

    /// Path of the entity one end is on, from the top. Empty for the world.
    std::string from;

    /// Where that end is on the entity, sized by the scale of its
    /// Transform, or in the world when `from` is empty.
    glm::vec3 from_anchor{0.0f};

    /// Path of the entity the other end is on. Empty for the world.
    std::string to;

    /// Where the other end is on that entity, or in the world.
    glm::vec3 to_anchor{0.0f};

    /// How long the rope is. Its ends may be further apart, and it is then
    /// drawn straight between them. 0 for a rope that is always straight.
    float length = 0.0f;

    /// How thick the rope is, across.
    float thickness = 0.03f;

    /// How many faces go round the rope.
    int sides = 8;

    /// How many straight pieces the rope is drawn as from end to end.
    int segments = 16;

    /// How often a texture repeats over one meter of the rope.
    float texels_per_meter = 1.0f;

    /// The mesh the rope is drawn as, written anew when an end moves.
    /// Filled in by the engine, as everything below is.
    std::shared_ptr<MeshData> mesh;

    /// Where the ends were, and how long the rope, when the mesh was
    /// last written.
    glm::vec3 drawn_from{0.0f};
    glm::vec3 drawn_to{0.0f};
    float drawn_length = -1.0f;

    /// The length of a joint that has none of its own: how far its ends
    /// were apart when the rope first saw them. Below 0 until then.
    float found_length = -1.0f;

    /// The entities the ends are on, once found.
    Entity from_entity = No_Entity;
    Entity to_entity = No_Entity;

    /// Set when the rope could not be drawn, which was said in the log.
    bool failed = false;
  };

  /// What the engine fills in is not described.
  inline void Describe(TypeBuilder<Rope> &type)
  {
    type.Named("Rope", "A rope drawn between two points that move, hanging while it is slack");

    type.Field("joint", &Rope::joint)
        .Describe(
          "Path of the entity whose Joint of type rope is drawn, from the top. Its ends and its length are then "
          "those of the joint");

    type.Field("from", &Rope::from)
        .Describe("Path of the entity one end is on, from the top. Empty for the world. Not read with 'joint'");

    type.Field("from_anchor", &Rope::from_anchor)
        .Describe("Where that end is on the entity, sized by its scale, or in the world when 'from' is empty");

    type.Field("to", &Rope::to)
        .Describe("Path of the entity the other end is on, from the top. Empty for the world. Not read with 'joint'");

    type.Field("to_anchor", &Rope::to_anchor)
        .Describe("Where the other end is on that entity, sized by its scale, or in the world when 'to' is empty");

    type.Field("length", &Rope::length)
        .Unit("m")
        .AtLeast(0.0f)
        .Describe("How long the rope is. 0 for a rope that is always straight. Not read with 'joint'");

    type.Field("thickness", &Rope::thickness)
        .Unit("m")
        .Above(0.0f)
        .Describe("How thick the rope is, across");

    type.Field("sides", &Rope::sides)
        .AtLeast(3)
        .Describe("How many faces go round the rope");

    type.Field("segments", &Rope::segments)
        .AtLeast(1)
        .Describe("How many straight pieces the rope is drawn as from end to end");

    type.Field("texels_per_meter", &Rope::texels_per_meter)
        .Above(0.0f)
        .Describe("How often a texture repeats over one meter of the rope");
  }
} // neon

#endif //ROPE_HPP
