#ifndef MESH_BUILDER_HPP
#define MESH_BUILDER_HPP

#include <vector>

#include <glm/glm.hpp>

#include "mesh-data.hpp"

namespace neon
{
  /// Builds a mesh from shapes, in metres, centred where each shape says.
  /// Every face is wound anticlockwise seen from outside and carries a flat
  /// normal; texture coordinates follow from ProjectUvs(), which the builder
  /// applies at the end with the texels per metre it was given. Every
  /// vertex is white: a mesh painted by vertex sets `Vertex::color` on the
  /// mesh it is handed.
  ///
  ///     const MeshData floor = MeshBuilder(2.0f).AddPlane({10, 10}, 4).Build();
  ///     const MeshData room = MeshBuilder().AddPrism({{-2, -2}, {2, -2}, {2, 2}, {-2, 2}}, 3.0f).Build();
  ///
  /// It is what blocks out a level, and what an importer of a map format
  /// fills before it hands the mesh to the renderer and the physics.
  class MeshBuilder
  {
    MeshData _mesh;
    float _texels_per_metre;
    bool _inside_out = false;

    /// Adds one face with the corners given anticlockwise seen from its
    /// front, with the normal of that front on every corner.
    void AddFace(const std::vector<glm::vec3> &corners);

  public:
    /// `texels_per_metre` is how often a texture repeats over one metre of
    /// surface, see ProjectUvs(). 1 repeats a texture once a metre.
    explicit MeshBuilder(float texels_per_metre = 1.0f);

    /// A box of `size`, centred at `center`.
    MeshBuilder &AddBox(const glm::vec3 &size, const glm::vec3 &center = {});

    /// A flat plane of `size` in x and z, facing up, centred at `center`,
    /// cut into `segments` by `segments` quads, which lets a floor be bent or
    /// painted by vertex later.
    MeshBuilder &AddPlane(const glm::vec2 &size, int segments = 1, const glm::vec3 &center = {});

    /// A ramp of `size`: a wedge that rises along z, from its low edge at
    /// the front (positive z) to its full height at the back, centred at
    /// `center` in x and z and standing on `center.y`.
    MeshBuilder &AddRamp(const glm::vec3 &size, const glm::vec3 &center = {});

    /// A prism: an outline on the ground, as points in x and z in either
    /// order round, pulled up by `height`. The outline has to be simple and
    /// convex; the floor and the ceiling are triangulated as a fan from the
    /// first point, which is right for a convex outline and for most rooms.
    /// It stands on `floor_y`.
    MeshBuilder &AddPrism(const std::vector<glm::vec2> &outline, float height, float floor_y = 0.0f);

    /// One quad with its corners given anticlockwise seen from the front.
    MeshBuilder &AddQuad(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d);

    /// Turns every face of the mesh round when it is built, so that the
    /// fronts point inward: a room is a box or a prism seen from within.
    /// The textures are projected for the inside, so they read the right
    /// way round from there.
    MeshBuilder &InsideOut(bool inside_out = true);

    /// Takes the mesh, with its texture coordinates projected. The builder
    /// is empty afterwards.
    [[nodiscard]] MeshData Build();
  };
} // neon

#endif //MESH_BUILDER_HPP
