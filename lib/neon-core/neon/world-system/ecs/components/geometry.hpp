#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <neon/reflection/type-builder.hpp>

#include "geometry-shape.hpp"

namespace neon
{
  /// A shape built by the engine when the scene loads, in metres, in place
  /// of a model from a file. The Renderable of the entity draws it with its
  /// shader and textures, and a Collider of kind `mesh` or `convex_hull`
  /// without a `model` takes its shape from it, so that a level is blocked
  /// out in a recipe alone, and an importer or a tool fills the same
  /// component. The shape is sized in metres as it is written; the
  /// Transform's scale multiplies it as it does a model.
  struct Geometry
  {
    GeometryShape shape = GeometryShape::Box;

    /// The size of a box, a ramp, a sphere, or a cylinder in x, y, and z; of
    /// a plane in x and z; of a quad in x and y.
    glm::vec3 size{1.0f};

    /// How many quads a plane is cut into along each side.
    int segments = 1;

    /// How many faces go round a sphere or a cylinder. A sphere has half as
    /// many rings from pole to pole.
    int sides = 24;

    /// The outline of a prism on the ground, as pairs of x and z, going
    /// round either way. Its height is `size.y`.
    std::vector<float> outline;

    /// How often a texture repeats over one metre of surface.
    float texels_per_metre = 1.0f;

    /// Whether the normals are smoothed over shared vertices, for a plane
    /// that is to look round rather than faceted, and for a sphere and the
    /// side of a cylinder, which are lit as round surfaces with it.
    bool smooth = false;

    /// Whether the faces point inward, for a box or a prism that is a room
    /// seen from within, or a sphere that is a dome. The outside is then left out by culling, as the
    /// inside of a crate is.
    bool inside = false;
  };

  inline void Describe(TypeBuilder<Geometry> &type)
  {
    type.Named("Geometry", "A shape the engine builds in place of a model");

    type.Choice("shape", &Geometry::shape, {"box", "plane", "ramp", "prism", "sphere", "cylinder", "quad"})
        .Describe("What is built. Which of the other values count depends on it");

    type.Field("size", &Geometry::size)
        .Unit("m")
        .Describe(
          "A box, a ramp, a sphere, or a cylinder in x, y, and z; a plane in x and z; a quad in x and y; the height "
          "of a prism in y");

    type.Field("segments", &Geometry::segments)
        .AtLeast(1)
        .OnlyWhen("shape", {"plane"})
        .Describe("How many quads a plane is cut into along each side");

    type.Field("sides", &Geometry::sides)
        .AtLeast(3)
        .OnlyWhen("shape", {"sphere", "cylinder"})
        .Describe("How many faces go round a sphere or a cylinder");

    type.Field("outline", &Geometry::outline)
        .OnlyWhen("shape", {"prism"})
        .Describe("Pairs of x and z on the ground, going round either way, at least three");

    type.Field("texels_per_metre", &Geometry::texels_per_metre)
        .Above(0)
        .Describe("How often a texture repeats over one metre of surface");

    type.Field("smooth", &Geometry::smooth)
        .Describe("Whether the normals are smoothed over shared vertices");

    type.Field("inside", &Geometry::inside)
        .OnlyWhen("shape", {"box", "prism", "sphere", "cylinder"})
        .Describe("Whether the faces point inward, for a room seen from within");

    type.Rule("outline", [](const Geometry &geometry, const std::string &where) -> std::string
    {
      if (geometry.shape != GeometryShape::Prism) { return {}; }
      if (geometry.outline.size() >= 6 && geometry.outline.size() % 2 == 0) { return {}; }
      return "'outline' of " + where + " needs at least three points, as pairs of x and z";
    });
  }
} // neon

#endif //GEOMETRY_HPP
