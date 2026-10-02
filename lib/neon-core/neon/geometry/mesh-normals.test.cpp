#include "mesh-normals.hpp"

#include <cmath>

#include <gtest/gtest.h>

#include "mesh-builder.hpp"

namespace
{
  using neon::MeshNormals;
  using neon::MeshData;
  using neon::Vertex;

  /// Two triangles that share an edge and bend at it: a roof.
  MeshData Roof()
  {
    MeshData roof;
    roof.vertices = {
      Vertex{.position = {-1, 0, 1}, .normal = {}, .tex_coords = {}},
      Vertex{.position = {1, 0, 1}, .normal = {}, .tex_coords = {}},
      Vertex{.position = {0, 1, 0}, .normal = {}, .tex_coords = {}},
      Vertex{.position = {-1, 0, -1}, .normal = {}, .tex_coords = {}},
      Vertex{.position = {1, 0, -1}, .normal = {}, .tex_coords = {}},
    };
    // the front slope, seen from the front, and the back slope, seen from
    // the back, both anticlockwise
    roof.indices = {0, 1, 2, 4, 3, 2};
    return roof;
  }

  TEST(MeshNormals, FlatNormalsSplitSharedVerticesSoThatEveryEdgeIsHard)
  {
    MeshData roof = Roof();

    MeshNormals::ComputeFlat(roof);

    EXPECT_EQ(roof.vertices.size(), 6u);
    EXPECT_EQ(roof.TriangleCount(), 2u);

    // the ridge vertex has one normal on the front slope and another on the back
    const glm::vec3 front = roof.vertices[2].normal;
    const glm::vec3 back = roof.vertices[5].normal;
    EXPECT_GT(front.z, 0.0f);
    EXPECT_LT(back.z, 0.0f);
    EXPECT_NEAR(length(front), 1.0f, 1e-5f);
    EXPECT_NEAR(length(back), 1.0f, 1e-5f);
  }

  TEST(MeshNormals, SmoothNormalsAverageTheTrianglesThatShareAVertex)
  {
    MeshData roof = Roof();

    MeshNormals::ComputeSmooth(roof);

    EXPECT_EQ(roof.vertices.size(), 5u);

    // the ridge points straight up, between the two slopes
    const glm::vec3 ridge = roof.vertices[2].normal;
    EXPECT_NEAR(ridge.x, 0.0f, 1e-5f);
    EXPECT_NEAR(ridge.y, 1.0f, 1e-5f);
    EXPECT_NEAR(ridge.z, 0.0f, 1e-5f);

    // an eave keeps the lean of its one slope
    EXPECT_GT(roof.vertices[0].normal.z, 0.0f);
    EXPECT_LT(roof.vertices[3].normal.z, 0.0f);
  }

  TEST(MeshNormals, SmoothNormalsWeighTrianglesByTheirArea)
  {
    MeshData mesh;
    mesh.vertices = {
      Vertex{.position = {0, 0, 0}, .normal = {}, .tex_coords = {}},
      Vertex{.position = {1, 0, 0}, .normal = {}, .tex_coords = {}},
      Vertex{.position = {0, 0, -1}, .normal = {}, .tex_coords = {}}, // a small triangle facing up
      Vertex{.position = {0, 10, 0}, .normal = {}, .tex_coords = {}}, // a large one facing +z, sharing the first edge
    };
    mesh.indices = {0, 1, 2, 0, 3, 1};

    MeshNormals::ComputeSmooth(mesh);

    // the shared vertices lean far towards the large triangle's normal
    const glm::vec3 shared = mesh.vertices[0].normal;
    EXPECT_GT(std::fabs(shared.z), std::fabs(shared.y) * 5.0f);
  }

  TEST(MeshNormals, ADegenerateTriangleGetsUpInsteadOfNothing)
  {
    MeshData mesh;
    mesh.vertices = {
      Vertex{.position = {0, 0, 0}, .normal = {}, .tex_coords = {}},
      Vertex{.position = {1, 0, 0}, .normal = {}, .tex_coords = {}},
      Vertex{.position = {2, 0, 0}, .normal = {}, .tex_coords = {}},
    };
    mesh.indices = {0, 1, 2};

    MeshNormals::ComputeFlat(mesh);
    EXPECT_EQ(mesh.vertices[0].normal, glm::vec3(0.0f, 1.0f, 0.0f));

    MeshNormals::ComputeSmooth(mesh);
    EXPECT_EQ(mesh.vertices[0].normal, glm::vec3(0.0f, 1.0f, 0.0f));
  }
} // namespace
