#include "mesh-builder.hpp"

#include <cmath>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
  using neon::MeshBuilder;
  using neon::MeshData;
  using ::testing::Each;

  /// The normal a triangle's winding gives it, seen from where the corners
  /// go round anticlockwise.
  glm::vec3 WindingNormal(const MeshData &mesh, const std::size_t triangle)
  {
    const glm::vec3 &a = mesh.vertices[mesh.indices[triangle * 3]].position;
    const glm::vec3 &b = mesh.vertices[mesh.indices[triangle * 3 + 1]].position;
    const glm::vec3 &c = mesh.vertices[mesh.indices[triangle * 3 + 2]].position;
    return normalize(cross(b - a, c - a));
  }

  /// Every triangle's stored normals agree with its winding: what the
  /// renderer culls by is what the lighting uses.
  void ExpectWindingMatchesNormals(const MeshData &mesh)
  {
    for (std::size_t t = 0; t < mesh.TriangleCount(); t++)
    {
      const glm::vec3 winding = WindingNormal(mesh, t);
      for (std::size_t corner = 0; corner < 3; corner++)
      {
        const glm::vec3 &normal = mesh.vertices[mesh.indices[t * 3 + corner]].normal;
        EXPECT_NEAR(dot(winding, normal), 1.0f, 1e-4f) << "triangle " << t << ", corner " << corner;
      }
    }
  }

  /// How many triangles face the given direction, within a little.
  std::size_t CountFacing(const MeshData &mesh, const glm::vec3 &direction)
  {
    std::size_t count = 0;
    for (std::size_t t = 0; t < mesh.TriangleCount(); t++)
    {
      if (dot(WindingNormal(mesh, t), direction) > 0.99f) { count++; }
    }
    return count;
  }

  /// The centre of all vertices.
  glm::vec3 CentreOf(const MeshData &mesh)
  {
    glm::vec3 sum{0.0f};
    for (const auto &vertex : mesh.vertices) { sum += vertex.position; }
    return sum / static_cast<float>(mesh.vertices.size());
  }

  TEST(MeshBuilder, BuildsABoxOfTwelveTrianglesFacingOutward)
  {
    const MeshData box = MeshBuilder().AddBox({2.0f, 1.0f, 3.0f}).Build();

    EXPECT_EQ(box.TriangleCount(), 12u);
    EXPECT_EQ(box.vertices.size(), 24u);
    ExpectWindingMatchesNormals(box);
    for (const glm::vec3 direction : {glm::vec3{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}})
    {
      EXPECT_EQ(CountFacing(box, direction), 2u) << direction.x << " " << direction.y << " " << direction.z;
    }
  }

  TEST(MeshBuilder, SizesAndCentresABoxWhereItIsAsked)
  {
    const MeshData box = MeshBuilder().AddBox({2.0f, 4.0f, 6.0f}, {10.0f, 20.0f, 30.0f}).Build();

    glm::vec3 least{1e9f};
    glm::vec3 most{-1e9f};
    for (const auto &vertex : box.vertices)
    {
      least = min(least, vertex.position);
      most = max(most, vertex.position);
    }
    EXPECT_EQ(least, glm::vec3(9.0f, 18.0f, 27.0f));
    EXPECT_EQ(most, glm::vec3(11.0f, 22.0f, 33.0f));
  }

  TEST(MeshBuilder, BuildsAPlaneFacingUpCutIntoSegments)
  {
    const MeshData plane = MeshBuilder().AddPlane({4.0f, 4.0f}, 3).Build();

    EXPECT_EQ(plane.TriangleCount(), 18u);
    ExpectWindingMatchesNormals(plane);
    EXPECT_EQ(CountFacing(plane, {0, 1, 0}), 18u);
    const glm::vec3 centre = CentreOf(plane);
    EXPECT_NEAR(centre.x, 0.0f, 1e-5f);
    EXPECT_NEAR(centre.y, 0.0f, 1e-5f);
    EXPECT_NEAR(centre.z, 0.0f, 1e-5f);
  }

  TEST(MeshBuilder, BuildsARampThatRisesTowardsTheBack)
  {
    const MeshData ramp = MeshBuilder().AddRamp({2.0f, 1.0f, 4.0f}).Build();

    // the slope, the back, the bottom as quads, and two triangular sides
    EXPECT_EQ(ramp.TriangleCount(), 8u);
    ExpectWindingMatchesNormals(ramp);
    EXPECT_EQ(CountFacing(ramp, {0, 0, -1}), 2u);
    EXPECT_EQ(CountFacing(ramp, {0, -1, 0}), 2u);
    EXPECT_EQ(CountFacing(ramp, {1, 0, 0}), 1u);
    EXPECT_EQ(CountFacing(ramp, {-1, 0, 0}), 1u);

    // the slope faces up and forward
    const glm::vec3 slope = normalize(glm::vec3(0.0f, 4.0f, 1.0f));
    EXPECT_EQ(CountFacing(ramp, slope), 2u);

    // the high edge is at the back, the low edge at the front
    for (const auto &vertex : ramp.vertices)
    {
      if (vertex.position.y > 0.5f) { EXPECT_LT(vertex.position.z, 0.0f); }
    }
  }

  TEST(MeshBuilder, BuildsAPrismFromAnOutlineWithFloorAndCeiling)
  {
    const MeshData room = MeshBuilder().AddPrism({{-4, -4}, {4, -4}, {4, 4}, {-4, 4}}, 3.0f).Build();

    // four walls of two triangles, a ceiling of two, a floor of two
    EXPECT_EQ(room.TriangleCount(), 12u);
    ExpectWindingMatchesNormals(room);
    EXPECT_EQ(CountFacing(room, {0, 1, 0}), 2u);
    EXPECT_EQ(CountFacing(room, {0, -1, 0}), 2u);
    EXPECT_EQ(CountFacing(room, {1, 0, 0}), 2u);
    EXPECT_EQ(CountFacing(room, {-1, 0, 0}), 2u);
    EXPECT_EQ(CountFacing(room, {0, 0, 1}), 2u);
    EXPECT_EQ(CountFacing(room, {0, 0, -1}), 2u);
  }

  /// Every face of a closed shape points away from its centre.
  void ExpectEveryFacePointsOutward(const MeshData &mesh)
  {
    const glm::vec3 centre = CentreOf(mesh);
    for (std::size_t t = 0; t < mesh.TriangleCount(); t++)
    {
      const glm::vec3 &on_face = mesh.vertices[mesh.indices[t * 3]].position;
      EXPECT_GT(dot(WindingNormal(mesh, t), on_face - centre), 0.0f) << "triangle " << t;
    }
  }

  TEST(MeshBuilder, APrismPointsOutwardWhicheverWayRoundItsOutlineGoes)
  {
    const std::vector<glm::vec2> one_way{{-4, -4}, {4, -4}, {4, 4}, {-4, 4}};
    const std::vector<glm::vec2> other_way{{-4, 4}, {4, 4}, {4, -4}, {-4, -4}};

    ExpectEveryFacePointsOutward(MeshBuilder().AddPrism(one_way, 3.0f).Build());
    ExpectEveryFacePointsOutward(MeshBuilder().AddPrism(other_way, 3.0f).Build());
    ExpectEveryFacePointsOutward(MeshBuilder().AddBox({2, 2, 2}).Build());
  }

  TEST(MeshBuilder, RefusesAPrismOfFewerThanThreePoints)
  {
    const MeshData nothing = MeshBuilder().AddPrism({{0, 0}, {1, 0}}, 3.0f).Build();

    EXPECT_TRUE(nothing.IsEmpty());
  }

  TEST(MeshBuilder, TurnsARoomInsideOut)
  {
    const MeshData room = MeshBuilder().AddPrism({{-4, -4}, {4, -4}, {4, 4}, {-4, 4}}, 3.0f).InsideOut().Build();

    ExpectWindingMatchesNormals(room);

    // every face now points at the centre of the room
    const glm::vec3 centre = CentreOf(room);
    for (std::size_t t = 0; t < room.TriangleCount(); t++)
    {
      const glm::vec3 &on_face = room.vertices[room.indices[t * 3]].position;
      EXPECT_GT(dot(WindingNormal(room, t), centre - on_face), 0.0f) << "triangle " << t;
    }
  }

  TEST(MeshBuilder, BuildsASphereOfFlatFacesThatFillsItsSize)
  {
    const MeshData sphere = MeshBuilder().AddSphere({2.0f, 2.0f, 2.0f}, 8).Build();

    // eight round and four rings: a triangle at each pole and two rings of
    // quads between them
    EXPECT_EQ(sphere.TriangleCount(), 8u + 8u + 2u * 8u * 2u);
    ExpectWindingMatchesNormals(sphere);
    ExpectEveryFacePointsOutward(sphere);
    for (const auto &vertex : sphere.vertices) { EXPECT_NEAR(length(vertex.position), 1.0f, 1e-5f); }
  }

  TEST(MeshBuilder, ASmoothSphereSharesItsVerticesAndHasTheNormalsOfABall)
  {
    const MeshData sphere = MeshBuilder().AddSphere({2.0f, 2.0f, 2.0f}, 8, true).Build();

    // two poles and three rings of eight
    EXPECT_EQ(sphere.vertices.size(), 2u + 3u * 8u);
    EXPECT_EQ(sphere.TriangleCount(), 8u + 8u + 2u * 8u * 2u);
    ExpectEveryFacePointsOutward(sphere);
    for (const auto &vertex : sphere.vertices)
    {
      EXPECT_NEAR(dot(vertex.normal, vertex.position), 1.0f, 1e-5f);
    }
  }

  TEST(MeshBuilder, ASphereOfThreeLengthsIsAnEllipsoidCentredWhereItIsAsked)
  {
    const MeshData egg = MeshBuilder().AddSphere({2.0f, 4.0f, 6.0f}, 16, true, {1.0f, 2.0f, 3.0f}).Build();

    glm::vec3 least{1e9f};
    glm::vec3 most{-1e9f};
    for (const auto &vertex : egg.vertices)
    {
      least = min(least, vertex.position);
      most = max(most, vertex.position);
      EXPECT_NEAR(length(vertex.normal), 1.0f, 1e-5f);
    }
    EXPECT_NEAR(most.x - least.x, 2.0f, 1e-4f);
    EXPECT_NEAR(most.y - least.y, 4.0f, 1e-4f);
    EXPECT_NEAR(most.z - least.z, 6.0f, 1e-4f);
    EXPECT_NEAR((most.y + least.y) * 0.5f, 2.0f, 1e-4f);
    ExpectEveryFacePointsOutward(egg);
  }

  TEST(MeshBuilder, BuildsACylinderAlongYWithACapAtEachEnd)
  {
    const MeshData cylinder = MeshBuilder().AddCylinder({1.0f, 3.0f, 1.0f}, 6).Build();

    // six quads round, and two caps of six corners, which are four
    // triangles each
    EXPECT_EQ(cylinder.TriangleCount(), 6u * 2u + 4u + 4u);
    ExpectWindingMatchesNormals(cylinder);
    ExpectEveryFacePointsOutward(cylinder);
    EXPECT_EQ(CountFacing(cylinder, {0, 1, 0}), 4u);
    EXPECT_EQ(CountFacing(cylinder, {0, -1, 0}), 4u);
    for (const auto &vertex : cylinder.vertices) { EXPECT_NEAR(std::fabs(vertex.position.y), 1.5f, 1e-5f); }
  }

  TEST(MeshBuilder, ASmoothCylinderRoundsItsSideAndKeepsItsCapsFlat)
  {
    const MeshData cylinder = MeshBuilder().AddCylinder({2.0f, 1.0f, 2.0f}, 12, true).Build();

    EXPECT_EQ(cylinder.TriangleCount(), 12u * 2u + 10u + 10u);
    ExpectEveryFacePointsOutward(cylinder);
    EXPECT_EQ(CountFacing(cylinder, {0, 1, 0}), 10u);

    std::size_t on_the_side = 0;
    std::size_t on_a_cap = 0;
    for (const auto &vertex : cylinder.vertices)
    {
      if (std::fabs(vertex.normal.y) > 0.5f)
      {
        on_a_cap++;
        EXPECT_NEAR(std::fabs(vertex.normal.y), 1.0f, 1e-5f);
        continue;
      }
      on_the_side++;
      const glm::vec3 outward = normalize(glm::vec3(vertex.position.x, 0.0f, vertex.position.z));
      EXPECT_NEAR(dot(vertex.normal, outward), 1.0f, 1e-5f);
    }
    EXPECT_EQ(on_the_side, 24u);
    EXPECT_EQ(on_a_cap, 24u);
  }

  TEST(MeshBuilder, AnUprightQuadFacesForwardAndCarriesATextureOnce)
  {
    // the texels per metre are not asked: a picture lies on it once
    const MeshData quad = MeshBuilder(4.0f).AddUprightQuad({3.0f, 2.0f}).Build();

    EXPECT_EQ(quad.TriangleCount(), 2u);
    ExpectWindingMatchesNormals(quad);
    EXPECT_EQ(CountFacing(quad, {0, 0, 1}), 2u);
    for (const auto &vertex : quad.vertices)
    {
      // left is u = 0, and the top is v = 0, as textures are uploaded
      EXPECT_FLOAT_EQ(vertex.tex_coords.x, vertex.position.x < 0.0f ? 0.0f : 1.0f);
      EXPECT_FLOAT_EQ(vertex.tex_coords.y, vertex.position.y > 0.0f ? 0.0f : 1.0f);
      EXPECT_NEAR(std::fabs(vertex.position.x), 1.5f, 1e-5f);
      EXPECT_NEAR(std::fabs(vertex.position.y), 1.0f, 1e-5f);
    }
  }

  TEST(MeshBuilder, ProjectsTexturesOnceAMetreByDefault)
  {
    const MeshData box = MeshBuilder().AddBox({2.0f, 2.0f, 2.0f}).Build();

    // every corner lies a metre from the centre on two axes, so its texture
    // coordinates are a metre apart as well
    for (const auto &vertex : box.vertices)
    {
      EXPECT_NEAR(std::fabs(vertex.tex_coords.x), 1.0f, 1e-5f);
      EXPECT_NEAR(std::fabs(vertex.tex_coords.y), 1.0f, 1e-5f);
    }
  }

  TEST(MeshBuilder, RepeatsATextureAsOftenAsItIsTold)
  {
    const MeshData box = MeshBuilder(4.0f).AddBox({2.0f, 2.0f, 2.0f}).Build();

    for (const auto &vertex : box.vertices)
    {
      EXPECT_NEAR(std::fabs(vertex.tex_coords.x), 4.0f, 1e-5f);
    }
  }

  TEST(MeshBuilder, IsEmptyAgainAfterItWasBuilt)
  {
    MeshBuilder builder;
    (void) builder.AddBox({1, 1, 1}).Build();

    EXPECT_TRUE(builder.Build().IsEmpty());
  }
} // namespace
