#include "mesh-builder.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include <glm/gtc/constants.hpp>

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

  MeshBuilder &MeshBuilder::AddSphere(
    const glm::vec3 &size,
    const int sides,
    const bool smooth,
    const glm::vec3 &center)
  {
    const int around = sides < 3 ? 3 : sides;
    const int rings = around / 2 < 2 ? 2 : around / 2;
    const glm::vec3 radii = size * 0.5f;

    // the point of ring `ring`, counted from the top, at step `step` round
    // y. The steps go from +x towards +z
    const auto point = [&](const int ring, const int step)
    {
      const float down = glm::pi<float>() * static_cast<float>(ring) / static_cast<float>(rings);
      const float round = glm::two_pi<float>() * static_cast<float>(step) / static_cast<float>(around);
      return glm::vec3(std::sin(down) * std::cos(round), std::cos(down), std::sin(down) * std::sin(round));
    };

    if (!smooth)
    {
      for (int ring = 0; ring < rings; ring++)
      {
        for (int step = 0; step < around; step++)
        {
          const glm::vec3 upper = center + radii * point(ring, step);
          const glm::vec3 upper_next = center + radii * point(ring, step + 1);
          const glm::vec3 lower = center + radii * point(ring + 1, step);
          const glm::vec3 lower_next = center + radii * point(ring + 1, step + 1);

          // a triangle at each pole, where a ring is one point, and a quad
          // between two rings
          if (ring == 0) { AddFace({upper, lower_next, lower}); }
          else if (ring == rings - 1) { AddFace({upper, upper_next, lower}); }
          else { AddFace({upper, upper_next, lower_next, lower}); }
        }
      }
      return *this;
    }

    // Shared vertices, each with the normal of the surface where it is:
    // the top, the rings between the poles, the bottom
    const auto first = static_cast<unsigned int>(_mesh.vertices.size());
    const auto add = [&](const glm::vec3 &on_unit_sphere)
    {
      // the normal of an ellipsoid leans towards its short axes
      const glm::vec3 normal = normalize(on_unit_sphere / radii);
      _mesh.vertices.push_back(
        Vertex{.position = center + radii * on_unit_sphere, .normal = normal, .tex_coords = {}});
    };

    add(point(0, 0));
    for (int ring = 1; ring < rings; ring++)
    {
      for (int step = 0; step < around; step++) { add(point(ring, step)); }
    }
    add(point(rings, 0));

    const auto at = [&](const int ring, const int step)
    {
      return first + 1 + static_cast<unsigned int>((ring - 1) * around + step % around);
    };
    const unsigned int top = first;
    const unsigned int bottom = first + 1 + static_cast<unsigned int>((rings - 1) * around);
    const auto triangle = [&](const unsigned int a, const unsigned int b, const unsigned int c)
    {
      _mesh.indices.push_back(a);
      _mesh.indices.push_back(b);
      _mesh.indices.push_back(c);
    };

    for (int step = 0; step < around; step++)
    {
      triangle(top, at(1, step + 1), at(1, step));
      for (int ring = 1; ring + 1 < rings; ring++)
      {
        triangle(at(ring, step), at(ring, step + 1), at(ring + 1, step + 1));
        triangle(at(ring, step), at(ring + 1, step + 1), at(ring + 1, step));
      }
      triangle(at(rings - 1, step), at(rings - 1, step + 1), bottom);
    }
    return *this;
  }

  MeshBuilder &MeshBuilder::AddCylinder(
    const glm::vec3 &size,
    const int sides,
    const bool smooth,
    const glm::vec3 &center)
  {
    const int around = sides < 3 ? 3 : sides;
    const glm::vec3 radii = size * 0.5f;

    // the direction of step `step` round y, from +x towards +z
    const auto round = [&](const int step)
    {
      const float angle = glm::two_pi<float>() * static_cast<float>(step) / static_cast<float>(around);
      return glm::vec3(std::cos(angle), 0.0f, std::sin(angle));
    };
    const glm::vec3 up{0.0f, radii.y, 0.0f};
    const glm::vec3 across{radii.x, 0.0f, radii.z};

    if (smooth)
    {
      // the side shares its vertices, a top and a bottom one for every
      // step, each with the normal of the round side there
      const auto first = static_cast<unsigned int>(_mesh.vertices.size());
      for (int step = 0; step < around; step++)
      {
        const glm::vec3 outward = round(step);
        const glm::vec3 normal = normalize(glm::vec3(outward.x / radii.x, 0.0f, outward.z / radii.z));
        _mesh.vertices.push_back(Vertex{.position = center + across * outward + up, .normal = normal, .tex_coords = {}});
        _mesh.vertices.push_back(Vertex{.position = center + across * outward - up, .normal = normal, .tex_coords = {}});
      }
      for (int step = 0; step < around; step++)
      {
        const unsigned int upper = first + static_cast<unsigned int>(step) * 2;
        const unsigned int upper_next = first + static_cast<unsigned int>((step + 1) % around) * 2;
        for (const unsigned int index : {upper, upper_next, upper_next + 1, upper, upper_next + 1, upper + 1})
        {
          _mesh.indices.push_back(index);
        }
      }
    } else
    {
      for (int step = 0; step < around; step++)
      {
        const glm::vec3 here = center + across * round(step);
        const glm::vec3 next = center + across * round(step + 1);
        AddFace({here + up, next + up, next - up, here - up});
      }
    }

    // the caps: the top goes round against the steps to face up, the bottom
    // with them to face down
    std::vector<glm::vec3> top;
    std::vector<glm::vec3> bottom;
    for (int step = 0; step < around; step++)
    {
      top.push_back(center + across * round(around - step) + up);
      bottom.push_back(center + across * round(step) - up);
    }
    AddFace(top);
    AddFace(bottom);
    return *this;
  }

  MeshBuilder &MeshBuilder::AddUprightQuad(const glm::vec2 &size, const glm::vec3 &center)
  {
    const glm::vec2 h = size * 0.5f;
    const auto first = static_cast<unsigned int>(_mesh.vertices.size());

    AddQuad(
      center + glm::vec3(-h.x, -h.y, 0.0f),
      center + glm::vec3(h.x, -h.y, 0.0f),
      center + glm::vec3(h.x, h.y, 0.0f),
      center + glm::vec3(-h.x, h.y, 0.0f));

    // the texture once across, with its top, which is v = 0, at the top
    _laid_uvs.emplace_back(first, glm::vec2(0.0f, 1.0f));
    _laid_uvs.emplace_back(first + 1, glm::vec2(1.0f, 1.0f));
    _laid_uvs.emplace_back(first + 2, glm::vec2(1.0f, 0.0f));
    _laid_uvs.emplace_back(first + 3, glm::vec2(0.0f, 0.0f));
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
    for (const auto &[index, uv] : _laid_uvs) { _mesh.vertices[index].tex_coords = uv; }

    MeshData built = std::move(_mesh);
    _mesh = MeshData{};
    _laid_uvs.clear();
    return built;
  }
} // neon
