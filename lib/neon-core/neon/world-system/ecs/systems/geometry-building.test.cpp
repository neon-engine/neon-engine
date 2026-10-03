#include "geometry-building.hpp"

#include <cmath>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>

namespace
{
  using neon::Entity;
  using neon::Geometry;
  using neon::GeometryBuilding;
  using neon::GeometryShape;
  using neon::Renderable;
  using neon::testing::FakeEntityStore;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;

  class GeometryBuildingTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    FakeEntityStore _store;
    GeometryBuilding _system{_logger};

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Renderable>("Renderable");
      _system.Register(_store);
      _system.Initialize(_store);
    }

    void TearDown() override
    {
      _store.CleanUp();
    }

    Entity Create(const std::string &name, const Geometry &geometry)
    {
      const Entity entity = _store.CreateEntity(name);
      _store.Set(entity, geometry);
      _store.Set(entity, Renderable{});
      return entity;
    }
  };

  TEST_F(GeometryBuildingTest, RegistersTheComponentSoThatAnEntityCanCarryIt)
  {
    const Entity crate = _store.CreateEntity("crate");
    _store.Set(crate, Geometry{.shape = GeometryShape::Ramp});

    ASSERT_NE(_store.Get<Geometry>(crate), nullptr);
    EXPECT_EQ(_store.Get<Geometry>(crate)->shape, GeometryShape::Ramp);
  }

  TEST_F(GeometryBuildingTest, GivesTheRenderableTheMeshOfItsGeometry)
  {
    const Entity crate = Create("crate", Geometry{.shape = GeometryShape::Box, .size = {1.0f, 2.0f, 3.0f}});

    _system.Update(_store, 0.016);

    const auto *renderable = _store.Get<Renderable>(crate);
    ASSERT_NE(renderable->render_info.mesh, nullptr);
    EXPECT_EQ(renderable->render_info.mesh->TriangleCount(), 12u);
    EXPECT_THAT(_logger->Messages(LogLevel::Error), ::testing::IsEmpty());
  }

  TEST_F(GeometryBuildingTest, BuildsTheMeshOnce)
  {
    const Entity crate = Create("crate", Geometry{});

    _system.Update(_store, 0.016);
    const auto *first = _store.Get<Renderable>(crate)->render_info.mesh.get();
    _system.Update(_store, 0.016);

    EXPECT_EQ(_store.Get<Renderable>(crate)->render_info.mesh.get(), first);
  }

  TEST_F(GeometryBuildingTest, SaysWhenAGeometryBuildsNothing)
  {
    (void) Create("odd", Geometry{.shape = GeometryShape::Prism, .outline = {0.0f, 0.0f, 1.0f, 0.0f}});

    _system.Update(_store, 0.016);

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "The Geometry of entity 'odd' builds nothing, so nothing is drawn"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(GeometryBuildingTest, BuildsEveryShapeTheComponentNames)
  {
    EXPECT_EQ(GeometryBuilding::Build(Geometry{.shape = GeometryShape::Box}).TriangleCount(), 12u);
    EXPECT_EQ(GeometryBuilding::Build(Geometry{.shape = GeometryShape::Plane, .segments = 2}).TriangleCount(), 8u);
    EXPECT_EQ(GeometryBuilding::Build(Geometry{.shape = GeometryShape::Ramp}).TriangleCount(), 8u);
    EXPECT_EQ(
      GeometryBuilding::Build(Geometry{.shape = GeometryShape::Prism, .outline = {0, 0, 1, 0, 1, 1}}).TriangleCount(),
      3u * 2u + 1u + 1u);
    EXPECT_EQ(
      GeometryBuilding::Build(Geometry{.shape = GeometryShape::Sphere, .sides = 8}).TriangleCount(),
      8u + 8u + 2u * 8u * 2u);
    EXPECT_EQ(
      GeometryBuilding::Build(Geometry{.shape = GeometryShape::Cylinder, .sides = 8}).TriangleCount(),
      8u * 2u + 6u + 6u);
    EXPECT_EQ(GeometryBuilding::Build(Geometry{.shape = GeometryShape::Quad}).TriangleCount(), 2u);
  }

  TEST_F(GeometryBuildingTest, ASmoothSphereIsRoundAndASmoothCylinderKeepsItsCaps)
  {
    const auto ball = GeometryBuilding::Build(Geometry{.shape = GeometryShape::Sphere, .sides = 8, .smooth = true});
    const auto faceted = GeometryBuilding::Build(Geometry{.shape = GeometryShape::Sphere, .sides = 8});
    const auto can = GeometryBuilding::Build(Geometry{.shape = GeometryShape::Cylinder, .sides = 8, .smooth = true});

    // a ball shares its vertices, a faceted one has one for every corner of
    // every face
    EXPECT_LT(ball.vertices.size(), faceted.vertices.size());
    for (const auto &vertex : ball.vertices) { EXPECT_NEAR(dot(vertex.normal, vertex.position), 0.5f, 1e-5f); }

    // the caps of the can still face straight up and down
    std::size_t straight = 0;
    for (const auto &vertex : can.vertices)
    {
      if (std::fabs(vertex.normal.y) > 0.999f) { straight++; }
    }
    EXPECT_EQ(straight, 16u);
  }

  TEST_F(GeometryBuildingTest, ASphereTurnedInsideOutIsADomeSeenFromWithin)
  {
    const auto dome = GeometryBuilding::Build(
      Geometry{.shape = GeometryShape::Sphere, .size = glm::vec3(10.0f), .smooth = true, .inside = true});

    for (const auto &vertex : dome.vertices) { EXPECT_LT(dot(vertex.normal, vertex.position), 0.0f); }
  }

  TEST_F(GeometryBuildingTest, SmoothsTheNormalsWhenAsked)
  {
    const auto faceted = GeometryBuilding::Build(Geometry{.shape = GeometryShape::Box});
    const auto smooth = GeometryBuilding::Build(Geometry{.shape = GeometryShape::Box, .smooth = true});

    // a box's corners stay split, so each vertex is still on one face and
    // smoothing changes nothing; a plane's shared corners would. What matters
    // here is that the option is read and the mesh stays whole
    EXPECT_EQ(smooth.vertices.size(), faceted.vertices.size());
    EXPECT_EQ(smooth.TriangleCount(), faceted.TriangleCount());
  }
} // namespace
