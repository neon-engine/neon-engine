#include "tube.hpp"

#include <cmath>

#include <gtest/gtest.h>

#include "mesh-builder.hpp"

namespace
{
  using neon::AppendTube;
  using neon::MeshData;

  /// A tube along a quarter circle of radius 2 in the plane of x and y.
  std::vector<glm::vec3> QuarterCircle(const int pieces)
  {
    std::vector<glm::vec3> centers;
    for (int i = 0; i <= pieces; i++)
    {
      const float angle = 1.5707963f * static_cast<float>(i) / static_cast<float>(pieces);
      centers.emplace_back(2.0f * std::cos(angle), 2.0f * std::sin(angle), 0.0f);
    }
    return centers;
  }
}

TEST(Tube, HasARingForEveryPointAndACapAtEachEnd)
{
  MeshData mesh;
  AppendTube({{0, 0, 0}, {0, 0, 1}, {0, 0, 2}}, 0.5f, 8, 1.0f, mesh);

  // three rings of eight and one more for the seam, two caps of a middle and eight
  EXPECT_EQ(mesh.vertices.size(), 3u * 9u + 2u * 9u);
  // two lengths of eight quads, two caps of eight triangles
  EXPECT_EQ(mesh.TriangleCount(), 2u * 8u * 2u + 2u * 8u);
}

TEST(Tube, IsAsThickAsItsRadiusAllTheWayRoundABend)
{
  const auto centers = QuarterCircle(16);
  MeshData mesh;
  AppendTube(centers, 0.25f, 12, 1.0f, mesh);

  // every vertex of the side is 0.25 from its point of the line, and its
  // normal points away from it
  for (std::size_t ring = 0; ring < centers.size(); ring++)
  {
    for (std::size_t step = 0; step <= 12; step++)
    {
      const auto &vertex = mesh.vertices[ring * 13 + step];
      EXPECT_NEAR(glm::length(vertex.position - centers[ring]), 0.25f, 1e-5f);
      EXPECT_NEAR(dot(vertex.normal, normalize(vertex.position - centers[ring])), 1.0f, 1e-5f);
    }
  }
}

TEST(Tube, FacesOutward)
{
  MeshData mesh;
  AppendTube(QuarterCircle(8), 0.25f, 12, 1.0f, mesh);

  // the side a triangle is wound to face is the side its normals point to
  for (std::size_t triangle = 0; triangle < mesh.TriangleCount(); triangle++)
  {
    const auto &a = mesh.vertices[mesh.indices[triangle * 3]];
    const auto &b = mesh.vertices[mesh.indices[triangle * 3 + 1]];
    const auto &c = mesh.vertices[mesh.indices[triangle * 3 + 2]];
    const glm::vec3 winding = normalize(cross(b.position - a.position, c.position - a.position));
    EXPECT_GT(dot(winding, a.normal), 0.7f) << "triangle " << triangle;
    EXPECT_GT(dot(winding, b.normal), 0.7f) << "triangle " << triangle;
    EXPECT_GT(dot(winding, c.normal), 0.7f) << "triangle " << triangle;
  }
}

TEST(Tube, DoesNotTwistAlongABend)
{
  const auto centers = QuarterCircle(16);
  MeshData mesh;
  AppendTube(centers, 0.25f, 12, 1.0f, mesh);

  // the first vertex of a ring stays next to the first of the ring before
  for (std::size_t ring = 1; ring < centers.size(); ring++)
  {
    const glm::vec3 before = mesh.vertices[(ring - 1) * 13].normal;
    const glm::vec3 here = mesh.vertices[ring * 13].normal;
    EXPECT_GT(dot(before, here), 0.98f) << "ring " << ring;
  }
}

TEST(Tube, LaysItsTextureRoundItAndAlongIt)
{
  MeshData mesh;
  AppendTube({{0, 0, 0}, {0, 0, 3}}, 0.5f, 8, 2.0f, mesh);

  // twice a meter: round a girth of pi, along three meters
  EXPECT_NEAR(mesh.vertices[0].tex_coords.x, 0.0f, 1e-5f);
  EXPECT_NEAR(mesh.vertices[8].tex_coords.x, 2.0f * 3.1415927f, 1e-4f);
  EXPECT_NEAR(mesh.vertices[0].tex_coords.y, 0.0f, 1e-5f);
  EXPECT_NEAR(mesh.vertices[9].tex_coords.y, 6.0f, 1e-5f);

  // and the builder keeps them where it would project its own
  const MeshData built = neon::MeshBuilder(2.0f).AddTube({{0, 0, 0}, {0, 0, 3}}, 0.5f, 8).Build();
  EXPECT_NEAR(built.vertices[9].tex_coords.y, 6.0f, 1e-5f);
}

TEST(Tube, IsWrittenOverTheOneBeforeWithoutGrowing)
{
  MeshData mesh;
  AppendTube(QuarterCircle(8), 0.25f, 12, 1.0f, mesh);
  const std::size_t vertices = mesh.vertices.size();
  const std::size_t indices = mesh.indices.size();
  const std::size_t room = mesh.vertices.capacity();

  // the same points somewhere else
  auto moved = QuarterCircle(8);
  for (auto &center : moved) { center += glm::vec3(0.0f, 1.0f, 0.5f); }
  mesh.vertices.clear();
  mesh.indices.clear();
  AppendTube(moved, 0.25f, 12, 1.0f, mesh);

  EXPECT_EQ(mesh.vertices.size(), vertices);
  EXPECT_EQ(mesh.indices.size(), indices);
  EXPECT_EQ(mesh.vertices.capacity(), room);
}

TEST(Tube, IsNothingThroughFewerThanTwoPoints)
{
  MeshData mesh;
  AppendTube({{1, 2, 3}}, 0.25f, 12, 1.0f, mesh);
  EXPECT_TRUE(mesh.IsEmpty());
}

TEST(Tube, HoldsTogetherWherePointsLieOnEachOther)
{
  MeshData mesh;
  AppendTube({{0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 1, 0}}, 0.25f, 6, 1.0f, mesh);

  for (const auto &vertex : mesh.vertices)
  {
    EXPECT_TRUE(std::isfinite(vertex.position.x) && std::isfinite(vertex.normal.x));
  }
}
