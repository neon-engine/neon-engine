#ifndef MESH_BUILDER_HPP
#define MESH_BUILDER_HPP

#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "mesh-data.hpp"

namespace neon
{
  /// Builds a mesh from shapes, in meters, centered where each shape says.
  /// Every face is wound counterclockwise seen from outside and carries a flat
  /// normal; texture coordinates follow from ProjectUvs(), which the builder
  /// applies at the end with the texels per meter it was given. Every
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
    float _texels_per_meter;
    bool _inside_out = false;

    /// The texture coordinates a shape laid itself, by the index of the
    /// vertex, which Build() keeps in place of the projected ones.
    std::vector<std::pair<unsigned int, glm::vec2>> _laid_uvs;

    /// Adds one face with the corners given counterclockwise seen from its
    /// front, with the normal of that front on every corner.
    void AddFace(const std::vector<glm::vec3> &corners);

  public:
    /// `texels_per_meter` is how often a texture repeats over one meter of
    /// surface, see ProjectUvs(). 1 repeats a texture once a meter.
    explicit MeshBuilder(float texels_per_meter = 1.0f);

    /// A box of `size`, centered at `center`.
    MeshBuilder &AddBox(const glm::vec3 &size, const glm::vec3 &center = {});

    /// A flat plane of `size` in x and z, facing up, centered at `center`,
    /// cut into `segments` by `segments` quads, which lets a floor be bent or
    /// painted by vertex later.
    MeshBuilder &AddPlane(const glm::vec2 &size, int segments = 1, const glm::vec3 &center = {});

    /// A ramp of `size`: a wedge that rises along z, from its low edge at
    /// the front (positive z) to its full height at the back, centered at
    /// `center` in x and z and standing on `center.y`.
    MeshBuilder &AddRamp(const glm::vec3 &size, const glm::vec3 &center = {});

    /// A prism: an outline on the ground, as points in x and z in either
    /// order round, pulled up by `height`. The outline has to be simple and
    /// convex; the floor and the ceiling are triangulated as a fan from the
    /// first point, which is right for a convex outline and for most rooms.
    /// It stands on `floor_y`.
    MeshBuilder &AddPrism(const std::vector<glm::vec2> &outline, float height, float floor_y = 0.0f);

    /// A sphere that fills `size`, an ellipsoid when its three lengths
    /// differ, centered at `center`, with `sides` faces round its equator and
    /// half as many rings from pole to pole. `smooth` gives every vertex the
    /// normal of the round surface, so that it is lit as a ball; without it
    /// every face is flat, as the faces of the other shapes are.
    MeshBuilder &AddSphere(const glm::vec3 &size, int sides = 24, bool smooth = false, const glm::vec3 &center = {});

    /// A cylinder along y that fills `size`, centered at `center`, with
    /// `sides` faces round it and a cap at each end. `smooth` rounds the
    /// side; the caps stay flat and their edges hard.
    MeshBuilder &AddCylinder(const glm::vec3 &size, int sides = 24, bool smooth = false, const glm::vec3 &center = {});

    /// An upright quad of `size` in x and y, facing +z, centered at `center`,
    /// on which a texture lies once with its top at the top, whatever the
    /// texels per meter: what shows a picture or a surface, as a screen does.
    MeshBuilder &AddUprightQuad(const glm::vec2 &size, const glm::vec3 &center = {});

    /// A tube of `radius` with `sides` faces round it, along the line
    /// through `centers` and closed at both ends, with the normals of the
    /// round surface: a curve that was flattened, drawn as a pipe or a
    /// rope. Its texture goes round it and along it, see AppendTube().
    MeshBuilder &AddTube(const std::vector<glm::vec3> &centers, float radius, int sides = 12);

    /// One quad with its corners given counterclockwise seen from the front.
    MeshBuilder &AddQuad(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d);

    /// Turns every face of the mesh round when it is built, so that the
    /// fronts point inward: a room is a box or a prism seen from within.
    /// The textures are projected for the inside, so they read the right
    /// way round from there.
    MeshBuilder &InsideOut(bool inside_out = true);

    /// Takes the mesh, with its texture coordinates projected, but for the
    /// shapes that lay their own. The builder is empty afterwards.
    [[nodiscard]] MeshData Build();
  };
} // neon

#endif //MESH_BUILDER_HPP
