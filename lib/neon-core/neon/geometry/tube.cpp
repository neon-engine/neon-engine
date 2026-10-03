#include "tube.hpp"

#include <cmath>

#include <glm/gtc/constants.hpp>

namespace neon
{
  // Helpers of AppendTube().
  namespace
  {
    /// The direction the line goes at point `index`: from the point before
    /// to the point after, or along the one piece an end has. `fallback`
    /// where the points lie on each other.
    glm::vec3 DirectionAt(const std::vector<glm::vec3> &centres, const std::size_t index, const glm::vec3 &fallback)
    {
      const glm::vec3 &before = centres[index == 0 ? 0 : index - 1];
      const glm::vec3 &after = centres[index + 1 < centres.size() ? index + 1 : index];
      const glm::vec3 along = after - before;
      return dot(along, along) > 1e-12f ? normalize(along) : fallback;
    }

    /// Something of length 1 that is square to `direction`.
    glm::vec3 SquareTo(const glm::vec3 &direction)
    {
      const glm::vec3 other = std::fabs(direction.y) < 0.9f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
      return normalize(cross(other, direction));
    }
  }

  void AppendTube(
    const std::vector<glm::vec3> &centres,
    const float radius,
    const int sides,
    const float texels_per_metre,
    MeshData &mesh)
  {
    if (centres.size() < 2) { return; }

    const auto around = static_cast<unsigned int>(sides < 3 ? 3 : sides);
    const auto rings = static_cast<unsigned int>(centres.size());
    const auto first = static_cast<unsigned int>(mesh.vertices.size());
    const float girth = glm::two_pi<float>() * radius;

    // Every ring is turned as little as the bend asks against the ring
    // before it, so that the tube does not twist along its way: `side`
    // is carried from ring to ring and squared to the new direction.
    glm::vec3 direction = DirectionAt(centres, 0, {0.0f, 0.0f, 1.0f});
    glm::vec3 side = SquareTo(direction);
    const glm::vec3 first_direction = direction;
    float walked = 0.0f;

    for (unsigned int ring = 0; ring < rings; ring++)
    {
      direction = DirectionAt(centres, ring, direction);
      const glm::vec3 carried = side - direction * dot(side, direction);
      side = dot(carried, carried) > 1e-12f ? normalize(carried) : SquareTo(direction);
      const glm::vec3 other_side = cross(direction, side);
      if (ring > 0) { walked += length(centres[ring] - centres[ring - 1]); }

      // the first vertex of a ring is there twice, so that the texture
      // closes round the tube
      for (unsigned int step = 0; step <= around; step++)
      {
        const float turn = static_cast<float>(step) / static_cast<float>(around);
        const float angle = glm::two_pi<float>() * turn;
        const glm::vec3 outward = side * std::cos(angle) + other_side * std::sin(angle);
        mesh.vertices.push_back(Vertex{
          .position = centres[ring] + outward * radius,
          .normal = outward,
          .tex_coords = glm::vec2(turn * girth, walked) * texels_per_metre});
      }
    }

    const unsigned int in_ring = around + 1;
    for (unsigned int ring = 0; ring + 1 < rings; ring++)
    {
      for (unsigned int step = 0; step < around; step++)
      {
        const unsigned int here = first + ring * in_ring + step;
        const unsigned int next = here + in_ring;
        for (const unsigned int index : {here, here + 1, next + 1, here, next + 1, next})
        {
          mesh.indices.push_back(index);
        }
      }
    }

    // The caps: a fan round the middle, with vertices of their own so that
    // their edges are hard. The one at the start faces back along the line.
    const auto add_cap = [&](const unsigned int ring, const glm::vec3 &normal, const bool at_start)
    {
      const auto middle = static_cast<unsigned int>(mesh.vertices.size());
      mesh.vertices.push_back(Vertex{
        .position = centres[ring],
        .normal = normal,
        .tex_coords = glm::vec2(radius, radius) * texels_per_metre});

      for (unsigned int step = 0; step < around; step++)
      {
        const Vertex &rim = mesh.vertices[first + ring * in_ring + step];
        const float angle = glm::two_pi<float>() * static_cast<float>(step) / static_cast<float>(around);
        mesh.vertices.push_back(Vertex{
          .position = rim.position,
          .normal = normal,
          .tex_coords = glm::vec2(1.0f + std::cos(angle), 1.0f + std::sin(angle)) * radius * texels_per_metre});
      }

      for (unsigned int step = 0; step < around; step++)
      {
        const unsigned int here = middle + 1 + step;
        const unsigned int next = middle + 1 + (step + 1) % around;
        mesh.indices.push_back(middle);
        mesh.indices.push_back(at_start ? next : here);
        mesh.indices.push_back(at_start ? here : next);
      }
    };

    add_cap(0, -first_direction, true);
    add_cap(rings - 1, direction, false);
  }
} // neon
