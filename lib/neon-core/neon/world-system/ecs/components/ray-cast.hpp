#ifndef RAY_CAST_HPP
#define RAY_CAST_HPP

#include <cstdint>
#include <string>

#include <glm/glm.hpp>

#include <neon/physics/physics-types.hpp>
#include <neon/reflection/type-builder.hpp>
#include <neon/world-system/ecs/entity.hpp>

namespace neon
{
  /// Casts a ray through the physics from the entity every frame, along
  /// where it looks, and says what it hits: the first body in the layers
  /// of its `mask`. On a camera it finds what the player looks at: the
  /// screen in the world under the dot in the middle of the view, a door,
  /// a crate. Whoever cares reads the result from here; the ray tells
  /// nobody. The pointer on a screen comes from it, and so may a door that
  /// opens when looked at, or a hint on the HUD.
  ///
  ///     - name: camera
  ///       components:
  ///         Camera: Default
  ///         RayCast:
  ///           length: 3
  ///           owner: player
  ///
  /// `owner` is who the ray acts for, by its path from the top of the
  /// world: its bodies are not hit, so that a ray from inside a capsule
  /// sees past it, and a click on a screen the ray points at comes from
  /// that entity. Without one, the ray's own entity is both. What the ray
  /// meets is every body of the physics, and a Collider that belongs to
  /// no body, which is a shape that queries find and nothing meets: a
  /// screen in the world is one.
  struct RayCast
  {
    /// How far the ray goes, in units of the world. 0 for as far as there
    /// is anything.
    float length = 3.0f;

    /// The layers the ray looks at. Every layer unless the scene says.
    std::uint32_t mask = All_Layers;

    /// Whether triggers are hit as well.
    bool triggers = false;

    /// Who the ray acts for, by its path, or empty for the entity itself.
    std::string owner;

    /// Whether the ray is cast. Off, it hits nothing.
    bool enabled = true;

    /// Whether the ray hits something in this frame. Written by the engine.
    bool is_hitting = false;

    /// Where it hits, in the world, which way the surface faces there, and
    /// how far along the ray. Written by the engine, and left as they were
    /// when nothing is hit.
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    float distance = 0.0f;

    /// What is hit, or No_Entity. Written by the engine.
    Entity hit = No_Entity;

    /// The owner, found by its path once. Written by the engine.
    Entity owner_entity = No_Entity;
  };

  /// What the ray hits, and the owner once found, are written by the
  /// engine: a script reads them, and no file does.
  inline void Describe(TypeBuilder<RayCast> &type)
  {
    type.Named("RayCast", "Casts a ray through the physics from the entity every frame, along where it looks, and says what it hits");

    type.Field("length", &RayCast::length)
        .AtLeast(0)
        .Describe("How far the ray goes, in units of the world. 0 for as far as there is anything");

    type.Layers("mask", &RayCast::mask)
        .Describe("The layers the ray looks at");

    type.Field("triggers", &RayCast::triggers)
        .Describe("Whether triggers are hit as well");

    type.Field("owner", &RayCast::owner)
        .Describe("Who the ray acts for, by its path from the top of the world: its bodies are not hit, and a click comes from it. The entity itself when left out");

    type.Field("enabled", &RayCast::enabled)
        .Describe("Whether the ray is cast");

    type.Field("is_hitting", &RayCast::is_hitting)
        .EngineWritten()
        .Describe("Whether the ray hits something in this frame. Written by the engine");

    type.Field("point", &RayCast::point)
        .EngineWritten()
        .Describe("Where the ray hits, in the world. Written by the engine");

    type.Field("normal", &RayCast::normal)
        .EngineWritten()
        .Describe("Which way the surface faces where the ray hits. Written by the engine");

    type.Field("distance", &RayCast::distance)
        .EngineWritten()
        .Describe("How far along the ray it hits. Written by the engine");
  }
} // neon

#endif //RAY_CAST_HPP
