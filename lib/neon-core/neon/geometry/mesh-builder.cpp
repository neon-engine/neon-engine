#include "mesh-builder.hpp"

#include <algorithm>
#include <utility>

#include "mesh-uvs.hpp"

namespace neon
{
  MeshBuilder::MeshBuilder(const float texels_per_metre)
  {
    _texels_per_metre = texels_per_metre;
  }

  void MeshBuilder::AddFace(const std::vector<glm::vec3> &corners)
  {
    if (corners.size() < 3) { return; }

    // the normal of the front, which is the side the corners go round
    // anticlockwise on
    const glm::vec3 normal = normalize(cross(corners[1] - corners[0], corners[2] - corners[0]));

    const auto first = static_cast<unsigned int>(_mesh.vertices.size());
    for (const auto &corner : corners)
    {
      _mesh.vertices.push_back(Vertex{.position = corner, .normal = normal, .tex_coords = {}});
    }

    // a fan from the first corner, which is right for a convex face
    for (unsigned int i = 1; i + 1 < corners.size(); i++)
    {
      _mesh.indices.push_back(first);
      _mesh.indices.push_back(first + i);
      _mesh.indices.push_back(first + i + 1);
    }
  }

  MeshBuilder &MeshBuilder::AddQuad(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d)
  {
    AddFace({a, b, c, d});
    return *this;
  }

  MeshBuilder &MeshBuilder::AddBox(const glm::vec3 &size, const glm::vec3 &center)
  {
    const glm::vec3 h = size * 0.5f;
    const glm::vec3 c = center;

    // the eight corners: l/r is x, b/t is y, n/f is z towards the viewer
    const glm::vec3 lbn = c + glm::vec3(-h.x, -h.y, h.z);
    const glm::vec3 rbn = c + glm::vec3(h.x, -h.y, h.z);
    const glm::vec3 rtn = c + glm::vec3(h.x, h.y, h.z);
    const glm::vec3 ltn = c + glm::vec3(-h.x, h.y, h.z);
    const glm::vec3 lbf = c + glm::vec3(-h.x, -h.y, -h.z);
    const glm::vec3 rbf = c + glm::vec3(h.x, -h.y, -h.z);
    const glm::vec3 rtf = c + glm::vec3(h.x, h.y, -h.z);
    const glm::vec3 ltf = c + glm::vec3(-h.x, h.y, -h.z);

    AddQuad(lbn, rbn, rtn, ltn); // front, facing +z
    AddQuad(rbf, lbf, ltf, rtf); // back, facing -z
    AddQuad(rbn, rbf, rtf, rtn); // right, facing +x
    AddQuad(lbf, lbn, ltn, ltf); // left, facing -x
    AddQuad(ltn, rtn, rtf, ltf); // top, facing +y
    AddQuad(lbf, rbf, rbn, lbn); // bottom, facing -y
    return *this;
  }

  MeshBuilder &MeshBuilder::AddPlane(const glm::vec2 &size, const int segments, const glm::vec3 &center)
  {
    const int n = segments < 1 ? 1 : segments;
    const glm::vec2 step = size / static_cast<float>(n);
    const glm::vec2 start = -size * 0.5f;

    for (int row = 0; row < n; row++)
    {
      for (int column = 0; column < n; column++)
      {
        const float x0 = center.x + start.x + step.x * static_cast<float>(column);
        const float x1 = x0 + step.x;
        const float z0 = center.z + start.y + step.y * static_cast<float>(row);
        const float z1 = z0 + step.y;

        // anticlockwise seen from above, so that the plane faces up
        AddQuad({x0, center.y, z1}, {x1, center.y, z1}, {x1, center.y, z0}, {x0, center.y, z0});
      }
    }
    return *this;
  }

  MeshBuilder &MeshBuilder::AddRamp(const glm::vec3 &size, const glm::vec3 &center)
  {
    const float hx = size.x * 0.5f;
    const float hz = size.z * 0.5f;
    const float y0 = center.y;
    const float y1 = center.y + size.y;

    // the low edge is at the front, the high edge at the back
    const glm::vec3 lf{center.x - hx, y0, center.z + hz};
    const glm::vec3 rf{center.x + hx, y0, center.z + hz};
    const glm::vec3 lb{center.x - hx, y0, center.z - hz};
    const glm::vec3 rb{center.x + hx, y0, center.z - hz};
    const glm::vec3 ltb{center.x - hx, y1, center.z - hz};
    const glm::vec3 rtb{center.x + hx, y1, center.z - hz};

    AddQuad(lf, rf, rtb, ltb);   // the slope, facing up and forward
    AddQuad(rb, lb, ltb, rtb);   // the back, facing -z
    AddQuad(lb, rb, rf, lf);     // the bottom, facing -y
    AddFace({rf, rb, rtb});      // the right side, facing +x
    AddFace({lb, lf, ltb});      // the left side, facing -x
    return *this;
  }

  MeshBuilder &MeshBuilder::AddPrism(const std::vector<glm::vec2> &given, const float height, const float floor_y)
  {
    if (given.size() < 3) { return *this; }

    // The order the points come in must not matter, so the outline is put
    // the one way round that makes every face below point outward: the way
    // whose shoelace area in x and z is negative, which is anticlockwise seen
    // from above with z towards the viewer.
    float twice_area = 0.0f;
    for (std::size_t i = 0; i < given.size(); i++)
    {
      const glm::vec2 a = given[i];
      const glm::vec2 b = given[(i + 1) % given.size()];
      twice_area += a.x * b.y - b.x * a.y;
    }
    std::vector<glm::vec2> outline(given);
    if (twice_area > 0.0f) { std::reverse(outline.begin(), outline.end()); }

    const float y0 = floor_y;
    const float y1 = floor_y + height;

    // the walls, each a quad from one point of the outline to the next, with
    // the outside on the right of the way from a to b
    for (std::size_t i = 0; i < outline.size(); i++)
    {
      const glm::vec2 a = outline[i];
      const glm::vec2 b = outline[(i + 1) % outline.size()];
      AddQuad({a.x, y0, a.y}, {b.x, y0, b.y}, {b.x, y1, b.y}, {a.x, y1, a.y});
    }

    // the ceiling faces up, in the order of the outline; the floor faces
    // down, in the reverse
    std::vector<glm::vec3> ceiling;
    std::vector<glm::vec3> floor;
    for (const auto &point : outline) { ceiling.emplace_back(point.x, y1, point.y); }
    for (auto it = outline.rbegin(); it != outline.rend(); ++it) { floor.emplace_back(it->x, y0, it->y); }
    AddFace(ceiling);
    AddFace(floor);
    return *this;
  }

  MeshBuilder &MeshBuilder::InsideOut(const bool inside_out)
  {
    _inside_out = inside_out;
    return *this;
  }

  MeshData MeshBuilder::Build()
  {
    if (_inside_out)
    {
      // the winding decides which side is the front: swapping two corners
      // of every triangle turns it round, and the normals follow
      for (std::size_t i = 0; i + 2 < _mesh.indices.size(); i += 3) { std::swap(_mesh.indices[i + 1], _mesh.indices[i + 2]); }
      for (auto &vertex : _mesh.vertices) { vertex.normal = -vertex.normal; }
    }

    ProjectUvs(_mesh, _texels_per_metre);
    MeshData built = std::move(_mesh);
    _mesh = MeshData{};
    return built;
  }
} // neon
