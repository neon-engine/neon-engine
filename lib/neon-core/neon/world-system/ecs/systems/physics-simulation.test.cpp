#include "physics-simulation.hpp"

#include <cmath>
#include <memory>
#include <string>

#include <glm/gtc/matrix_transform.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/world-system/ecs/components/renderable.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/fake-physics-context.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/character-body.hpp>
#include <neon/world-system/ecs/components/collider.hpp>
#include <neon/world-system/ecs/components/geometry.hpp>
#include <neon/world-system/ecs/components/joint.hpp>
#include <neon/world-system/ecs/components/rigid-body.hpp>
#include <neon/world-system/ecs/components/trigger.hpp>

namespace
{
  using neon::BodyKind;
  using neon::CharacterBody;
  using neon::Collider;
  using neon::ModelFit;
  using neon::Geometry;
  using neon::Entity;
  using neon::Joint;
  using neon::JointKind;
  using neon::No_Body;
  using neon::No_Joint;
  using neon::No_Character;
  using neon::No_Entity;
  using neon::PhysicsEvent;
  using neon::PhysicsEventKind;
  using neon::PhysicsSimulation;
  using neon::RigidBody;
  using neon::Rotation;
  using neon::ShapeKind;
  using neon::Transform;
  using neon::Trigger;
  using neon::testing::FakeEntityStore;
  using neon::testing::FakePhysicsContext;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;

  constexpr float tolerance = 1e-4f;
  constexpr double step = 1.0 / 60.0;

  void ExpectVector(const glm::vec3 &actual, const float x, const float y, const float z)
  {
    EXPECT_NEAR(actual.x, x, tolerance);
    EXPECT_NEAR(actual.y, y, tolerance);
    EXPECT_NEAR(actual.z, z, tolerance);
  }

  void ExpectSameOrientation(const glm::quat &actual, const glm::quat &expected)
  {
    EXPECT_NEAR(std::abs(dot(actual, expected)), 1.0f, tolerance);
  }

  Transform At(const float x, const float y, const float z)
  {
    Transform transform;
    transform.position = {x, y, z};
    return transform;
  }

  // a wedge that is 4 long, 2 high, and 1 wide
  const std::string wedge =
    "v 0 0 0\nv 4 0 0\nv 4 2 0\nv 0 0 1\nv 4 0 1\nv 4 2 1\n"
    "f 1 2 3\nf 4 6 5\nf 1 4 5 2\nf 2 5 6 3\nf 1 3 6 4\n";

  class PhysicsSimulationTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    FakePhysicsContext _physics;
    PhysicsSimulation _system{&_physics, &_files, _logger};

    // the store comes last, so that it goes away first. It tells the system
    // when a component is removed
    FakeEntityStore _store;

    void SetUp() override
    {
      _files.Initialize();
      _files.AddNativeFile("/assets/models/wedge.obj", wedge);

      _store.Initialize();
      _store.Register<Transform>("Transform");
      _store.Register<Geometry>("Geometry");
      _system.Register(_store);
      _system.Initialize(_store);
    }

    void TearDown() override
    {
      _store.CleanUp();
    }

    Entity Create(
      const std::string &name,
      const Transform &transform,
      const RigidBody &body,
      const Collider &collider = {},
      const Entity parent = No_Entity)
    {
      const Entity entity = _store.CreateEntity(name, parent);
      _store.Set(entity, transform);
      _store.Set(entity, body);
      _store.Set(entity, collider);
      return entity;
    }

    Entity CreateCharacter(const std::string &name, const Transform &transform, const CharacterBody &character = {})
    {
      const Entity entity = _store.CreateEntity(name);
      _store.Set(entity, transform);
      _store.Set(entity, character);
      _store.Set(entity, Collider{.shape = ShapeKind::Capsule});
      return entity;
    }

    void Step(const int count = 1)
    {
      for (int i = 0; i < count; i++) { _system.FixedUpdate(_store, step); }
    }

    Transform &TransformOf(const Entity entity) { return *_store.Get<Transform>(entity); }

    RigidBody &BodyOf(const Entity entity) { return *_store.Get<RigidBody>(entity); }

    FakePhysicsContext::Body &FakeOf(const Entity entity) { return *_physics.BodyOf(entity); }
  };

  // what is registered

  TEST_F(PhysicsSimulationTest, RegistersTheComponentsOfThePhysics)
  {
    EXPECT_NE(_store.FindComponent("RigidBody"), neon::No_Component);
    EXPECT_NE(_store.FindComponent("Trigger"), neon::No_Component);
    EXPECT_NE(_store.FindComponent("CharacterBody"), neon::No_Component);
    EXPECT_NE(_store.FindComponent("Collider"), neon::No_Component);
    EXPECT_NE(_store.FindComponent("Joint"), neon::No_Component);
  }

  // creating

  TEST_F(PhysicsSimulationTest, CreatesNothingBeforeTheFirstStep)
  {
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});

    _system.Update(_store, 0.5);
    _system.Interpolate(_store, 0.5);

    EXPECT_EQ(_physics.GetBodyCount(), 0u);
    EXPECT_EQ(BodyOf(crate).body, No_Body);
  }

  TEST_F(PhysicsSimulationTest, CreatesTheBodyOfARigidBodyWithEveryValue)
  {
    Transform transform = At(1.0f, 5.0f, -2.0f);
    transform.rotation = {.yaw = 90.0f};
    RigidBody body;
    body.kind = BodyKind::Dynamic;
    body.mass = 10.0f;
    body.friction = 0.25f;
    body.bounce = 0.75f;
    body.linear_damping = 0.5f;
    body.angular_damping = 2.0f;
    body.gravity_scale = 0.5f;
    body.linear_velocity = {1.0f, 2.0f, 3.0f};
    body.angular_velocity = {180.0f, 0.0f, -90.0f};
    body.continuous = true;
    body.can_sleep = false;
    body.lock_position = {"x", "z"};
    body.lock_rotation = {"y"};
    body.layers = 0b10;
    body.mask = 0b101;
    const Entity crate = Create("crate", transform, body);

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    const auto &info = _physics.created[0];
    EXPECT_EQ(info.entity, crate);
    EXPECT_EQ(info.kind, BodyKind::Dynamic);
    EXPECT_FALSE(info.trigger);
    ExpectVector(info.position, 1.0f, 5.0f, -2.0f);
    ExpectSameOrientation(info.rotation, Rotation{.yaw = 90.0f}.GetQuaternion());
    EXPECT_EQ(info.mass, 10.0f);
    EXPECT_EQ(info.friction, 0.25f);
    EXPECT_EQ(info.bounce, 0.75f);
    EXPECT_EQ(info.linear_damping, 0.5f);
    EXPECT_EQ(info.angular_damping, 2.0f);
    EXPECT_EQ(info.gravity_scale, 0.5f);
    EXPECT_EQ(info.linear_velocity, glm::vec3(1.0f, 2.0f, 3.0f));
    ExpectVector(info.angular_velocity, glm::pi<float>(), 0.0f, -glm::half_pi<float>());
    EXPECT_TRUE(info.continuous);
    EXPECT_FALSE(info.can_sleep);
    EXPECT_EQ(info.locked_position, neon::Axis_X | neon::Axis_Z);
    EXPECT_EQ(info.locked_rotation, neon::Axis_Y);
    EXPECT_EQ(info.layers, 0b10u);
    EXPECT_EQ(info.mask, 0b101u);

    EXPECT_NE(BodyOf(crate).body, No_Body);
    EXPECT_TRUE(_physics.HasBody(BodyOf(crate).body));
    EXPECT_FALSE(BodyOf(crate).failed);
  }

  TEST_F(PhysicsSimulationTest, CreatesEveryKindOfBody)
  {
    Create("floor", At(0.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Static});
    Create("lift", At(0.0f, 1.0f, 0.0f), RigidBody{.kind = BodyKind::Kinematic});
    Create("crate", At(0.0f, 2.0f, 0.0f), RigidBody{.kind = BodyKind::Dynamic});

    Step();

    ASSERT_EQ(_physics.created.size(), 3u);
    EXPECT_EQ(_physics.created[0].kind, BodyKind::Static);
    EXPECT_EQ(_physics.created[1].kind, BodyKind::Kinematic);
    EXPECT_EQ(_physics.created[2].kind, BodyKind::Dynamic);
  }

  TEST_F(PhysicsSimulationTest, CreatesABodyOnce)
  {
    Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});

    Step(5);

    EXPECT_EQ(_physics.created.size(), 1u);
    EXPECT_EQ(_physics.create_attempts, 1);
  }

  TEST_F(PhysicsSimulationTest, CreatesTheBodyOfAnEntityThatJoinsLater)
  {
    Create("first", At(0.0f, 5.0f, 0.0f), RigidBody{});
    Step(3);

    const Entity second = Create("second", At(3.0f, 5.0f, 0.0f), RigidBody{});
    Step();

    EXPECT_EQ(_physics.created.size(), 2u);
    EXPECT_NE(BodyOf(second).body, No_Body);
  }

  TEST_F(PhysicsSimulationTest, HandsOverTheValuesOfTheShape)
  {
    Collider collider;
    collider.shape = ShapeKind::TaperedCapsule;
    collider.size = {1.0f, 2.0f, 3.0f};
    collider.radius = 0.7f;
    collider.height = 3.0f;
    collider.top_radius = 0.1f;
    collider.bottom_radius = 0.2f;
    collider.offset = {0.0f, 1.0f, 0.0f};
    collider.rotation = {.roll = 90.0f};
    Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{}, collider);

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    ASSERT_EQ(_physics.created[0].shapes.size(), 1u);
    const auto &shape = _physics.created[0].shapes[0];
    EXPECT_EQ(shape.kind, ShapeKind::TaperedCapsule);
    EXPECT_EQ(shape.size, glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(shape.radius, 0.7f);
    EXPECT_EQ(shape.height, 3.0f);
    EXPECT_EQ(shape.top_radius, 0.1f);
    EXPECT_EQ(shape.bottom_radius, 0.2f);
    ExpectVector(shape.position, 0.0f, 1.0f, 0.0f);
    ExpectSameOrientation(shape.rotation, Rotation{.roll = 90.0f}.GetQuaternion());
    ExpectVector(shape.scale, 1.0f, 1.0f, 1.0f);
  }

  TEST_F(PhysicsSimulationTest, SizesTheShapeByTheScaleOfTheEntity)
  {
    Transform transform = At(0.0f, -0.55f, 0.0f);
    transform.scale = {100.0f, 0.1f, 100.0f};
    Create("floor", transform, RigidBody{.kind = BodyKind::Static});

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    ExpectVector(_physics.created[0].shapes[0].scale, 100.0f, 0.1f, 100.0f);
    ExpectVector(_physics.created[0].shapes[0].position, 0.0f, 0.0f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, SizesTheOffsetOfTheShapeByTheScaleOfTheEntity)
  {
    Transform transform;
    transform.scale = {2.0f, 2.0f, 2.0f};
    Create("crate", transform, RigidBody{}, Collider{.offset = {0.0f, 1.0f, 0.0f}});

    Step();

    ExpectVector(_physics.created[0].shapes[0].position, 0.0f, 2.0f, 0.0f);
  }

  // several shapes

  TEST_F(PhysicsSimulationTest, MakesOneBodyOfTheCollidersBelowAnEntity)
  {
    const Entity table = _store.CreateEntity("table");
    _store.Set(table, At(0.0f, 1.0f, 0.0f));
    _store.Set(table, RigidBody{});
    const Entity top = _store.CreateEntity("top", table);
    _store.Set(top, At(0.0f, 0.5f, 0.0f));
    _store.Set(top, Collider{.shape = ShapeKind::Box, .size = {2.0f, 0.1f, 1.0f}});
    Transform of_leg = At(-0.9f, 0.0f, 0.0f);
    of_leg.rotation = {.yaw = 90.0f};
    const Entity leg = _store.CreateEntity("leg", table);
    _store.Set(leg, of_leg);
    _store.Set(leg, Collider{.shape = ShapeKind::Cylinder, .radius = 0.05f, .height = 1.0f});

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    const auto &info = _physics.created[0];
    EXPECT_EQ(info.entity, table);
    ASSERT_EQ(info.shapes.size(), 2u);
    EXPECT_EQ(info.shapes[0].kind, ShapeKind::Box);
    ExpectVector(info.shapes[0].position, 0.0f, 0.5f, 0.0f);
    EXPECT_EQ(info.shapes[1].kind, ShapeKind::Cylinder);
    ExpectVector(info.shapes[1].position, -0.9f, 0.0f, 0.0f);
    ExpectSameOrientation(info.shapes[1].rotation, Rotation{.yaw = 90.0f}.GetQuaternion());
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 0u);
  }

  TEST_F(PhysicsSimulationTest, TakesTheColliderOfTheEntityTogetherWithThoseBelowIt)
  {
    const Entity hammer = Create("hammer", At(0.0f, 1.0f, 0.0f), RigidBody{}, Collider{.shape = ShapeKind::Cylinder});
    const Entity head = _store.CreateEntity("head", hammer);
    _store.Set(head, At(0.0f, 1.0f, 0.0f));
    _store.Set(head, Collider{});

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    ASSERT_EQ(_physics.created[0].shapes.size(), 2u);
    EXPECT_EQ(_physics.created[0].shapes[0].kind, ShapeKind::Cylinder);
    EXPECT_EQ(_physics.created[0].shapes[1].kind, ShapeKind::Box);
  }

  TEST_F(PhysicsSimulationTest, FindsCollidersSeveralLevelsBelow)
  {
    const Entity body = _store.CreateEntity("body");
    _store.Set(body, At(10.0f, 0.0f, 0.0f));
    _store.Set(body, RigidBody{});
    const Entity arm = _store.CreateEntity("arm", body);
    _store.Set(arm, At(0.0f, 2.0f, 0.0f));
    // groups others, and places nothing
    const Entity folder = _store.CreateEntity("folder", arm);
    const Entity hand = _store.CreateEntity("hand", folder);
    _store.Set(hand, At(1.0f, 0.0f, 0.0f));
    _store.Set(hand, Collider{.shape = ShapeKind::Sphere});

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    ASSERT_EQ(_physics.created[0].shapes.size(), 1u);
    ExpectVector(_physics.created[0].shapes[0].position, 1.0f, 2.0f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, LeavesTheCollidersOfABodyBelowToThatBody)
  {
    const Entity car = Create("car", At(0.0f, 1.0f, 0.0f), RigidBody{});
    const Entity trailer = Create("trailer", At(0.0f, 0.0f, 3.0f), RigidBody{}, Collider{.shape = ShapeKind::Sphere}, car);

    Step();

    ASSERT_EQ(_physics.created.size(), 2u);
    EXPECT_EQ(_physics.created[0].entity, car);
    ASSERT_EQ(_physics.created[0].shapes.size(), 1u);
    EXPECT_EQ(_physics.created[0].shapes[0].kind, ShapeKind::Box);
    EXPECT_EQ(_physics.created[1].entity, trailer);
    ASSERT_EQ(_physics.created[1].shapes.size(), 1u);
    EXPECT_EQ(_physics.created[1].shapes[0].kind, ShapeKind::Sphere);
  }

  // shapes from a model

  TEST_F(PhysicsSimulationTest, MakesAHullFromThePointsOfAModel)
  {
    Create("rock", At(0.0f, 5.0f, 0.0f), RigidBody{},
           Collider{.shape = ShapeKind::ConvexHull, .model = "assets://models/wedge.obj", .fit = ModelFit::Unit});

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    const auto &shape = _physics.created[0].shapes[0];
    EXPECT_EQ(shape.kind, ShapeKind::ConvexHull);
    EXPECT_FALSE(shape.points.empty());
    EXPECT_TRUE(shape.triangles.empty());

    // sized as the renderer sizes the model with a unit fit: the longest
    // side has length 1
    float lowest = 1000.0f;
    float highest = -1000.0f;
    for (const auto &point : shape.points)
    {
      lowest = std::min(lowest, point.x);
      highest = std::max(highest, point.x);
    }
    EXPECT_NEAR(lowest, -0.5f, tolerance);
    EXPECT_NEAR(highest, 0.5f, tolerance);
  }

  TEST_F(PhysicsSimulationTest, TakesThePointsOfAModelAsTheFileSaysThemWithoutAFit)
  {
    Create("rock", At(0.0f, 5.0f, 0.0f), RigidBody{},
           Collider{.shape = ShapeKind::ConvexHull, .model = "assets://models/wedge.obj"});

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    const auto &shape = _physics.created[0].shapes[0];

    // the wedge is 4 long from the origin, as the renderer draws it
    float lowest = 1000.0f;
    float highest = -1000.0f;
    for (const auto &point : shape.points)
    {
      lowest = std::min(lowest, point.x);
      highest = std::max(highest, point.x);
    }
    EXPECT_NEAR(lowest, 0.0f, tolerance);
    EXPECT_NEAR(highest, 4.0f, tolerance);
  }

  TEST_F(PhysicsSimulationTest, MakesAMeshFromTheTrianglesOfAModel)
  {
    Create("ramp", At(0.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Static},
           Collider{.shape = ShapeKind::Mesh, .model = "assets://models/wedge.obj"});

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    const auto &shape = _physics.created[0].shapes[0];
    EXPECT_EQ(shape.kind, ShapeKind::Mesh);
    EXPECT_FALSE(shape.points.empty());
    EXPECT_EQ(shape.triangles.size(), 8u * 3u);
  }

  TEST_F(PhysicsSimulationTest, MakesAMeshFromTheGeometryOfTheEntityWhenTheColliderNamesNoModel)
  {
    const Entity room = Create("room", At(0.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Static},
                               Collider{.shape = ShapeKind::Mesh});
    _store.Set(room, Geometry{.shape = neon::GeometryShape::Box, .size = {2.0f, 2.0f, 2.0f}});

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    const auto &shape = _physics.created[0].shapes[0];
    EXPECT_EQ(shape.kind, ShapeKind::Mesh);
    EXPECT_EQ(shape.points.size(), 24u);
    EXPECT_EQ(shape.triangles.size(), 12u * 3u);
  }

  TEST_F(PhysicsSimulationTest, MakesAHullFromTheGeometryOfTheEntity)
  {
    const Entity ramp = Create("ramp", At(0.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Static},
                               Collider{.shape = ShapeKind::ConvexHull});
    _store.Set(ramp, Geometry{.shape = neon::GeometryShape::Ramp, .size = {2.0f, 1.0f, 4.0f}});

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    const auto &shape = _physics.created[0].shapes[0];
    EXPECT_EQ(shape.kind, ShapeKind::ConvexHull);
    EXPECT_FALSE(shape.points.empty());
    EXPECT_TRUE(shape.triangles.empty());
  }

  TEST_F(PhysicsSimulationTest, MakesAMeshFromTheMeshThatWasHandedToWhatDrawsTheEntity)
  {
    _store.Register<neon::Renderable>("Renderable");

    const Entity wall = Create("wall", At(0.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Static},
                               Collider{.shape = ShapeKind::Mesh});

    // a square of two triangles, as an importer or an extension hands one over
    auto mesh = std::make_shared<neon::MeshData>();
    mesh->vertices.resize(4);
    mesh->vertices[0].position = {0.0f, 0.0f, 0.0f};
    mesh->vertices[1].position = {1.0f, 0.0f, 0.0f};
    mesh->vertices[2].position = {1.0f, 1.0f, 0.0f};
    mesh->vertices[3].position = {0.0f, 1.0f, 0.0f};
    mesh->indices = {0, 1, 2, 0, 2, 3};

    neon::Renderable renderable;
    renderable.render_info.mesh = mesh;
    _store.Set(wall, renderable);

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    const auto &shape = _physics.created[0].shapes[0];
    EXPECT_EQ(shape.kind, ShapeKind::Mesh);
    ASSERT_EQ(shape.points.size(), 4u);
    EXPECT_EQ(shape.points[2], glm::vec3(1.0f, 1.0f, 0.0f));
    EXPECT_EQ(shape.triangles.size(), 2u * 3u);
  }

  TEST_F(PhysicsSimulationTest, SaysWhenTheMeshThatWasHandedOverIsEmpty)
  {
    _store.Register<neon::Renderable>("Renderable");

    const Entity wall = Create("wall", At(0.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Static},
                               Collider{.shape = ShapeKind::Mesh});
    neon::Renderable renderable;
    renderable.render_info.mesh = std::make_shared<neon::MeshData>();
    _store.Set(wall, renderable);

    Step(2);

    EXPECT_TRUE(BodyOf(wall).failed);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "the mesh of entity 'wall' has nothing in it for its mesh"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(PhysicsSimulationTest, SaysWhenAColliderHasNeitherAModelNorAGeometry)
  {
    const Entity rock = Create("rock", At(0.0f, 5.0f, 0.0f), RigidBody{}, Collider{.shape = ShapeKind::Mesh});

    Step(2);

    EXPECT_TRUE(BodyOf(rock).failed);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "the mesh of entity 'rock' names no model, and the entity has neither a Geometry nor a mesh of its own "
      "to take its shape from"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(PhysicsSimulationTest, SaysThatAModelCannotBeReadAndDoesNotTryAgain)
  {
    const Entity rock = Create("rock", At(0.0f, 5.0f, 0.0f), RigidBody{},
                               Collider{.shape = ShapeKind::ConvexHull, .model = "assets://models/missing.obj"});

    Step(5);

    EXPECT_EQ(_physics.create_attempts, 0);
    EXPECT_TRUE(BodyOf(rock).failed);
    EXPECT_EQ(BodyOf(rock).body, No_Body);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The RigidBody of entity 'rock' cannot be created: the model 'assets://models/missing.obj' of the "
      "convex hull of entity 'rock' cannot be read")) << _logger->Messages(LogLevel::Error);

    const auto said = _logger->Count(LogLevel::Error);
    Step(5);
    EXPECT_EQ(_logger->Count(LogLevel::Error), said);
  }

  TEST_F(PhysicsSimulationTest, ReadsAModelOnceForEveryBodyThatUsesIt)
  {
    const Collider hull{.shape = ShapeKind::ConvexHull, .model = "assets://models/wedge.obj"};
    Create("first", At(0.0f, 5.0f, 0.0f), RigidBody{}, hull);
    Step();

    // the file is gone, and what was read is still there
    _files.CleanUp();
    _files.Initialize();
    Create("second", At(3.0f, 5.0f, 0.0f), RigidBody{}, hull);
    Step();

    ASSERT_EQ(_physics.created.size(), 2u);
    EXPECT_EQ(_physics.created[0].shapes[0].points, _physics.created[1].shapes[0].points);

    // and the physics is told where the points came from, so that it may
    // hold one shape for both
    EXPECT_EQ(_physics.created[0].shapes[0].source, "assets://models/wedge.obj");
    EXPECT_EQ(_physics.created[1].shapes[0].source, "assets://models/wedge.obj");
  }

  TEST_F(PhysicsSimulationTest, ReadsAModelForEachFitItIsUsedWith)
  {
    Create("first", At(0.0f, 5.0f, 0.0f), RigidBody{},
           Collider{.shape = ShapeKind::ConvexHull, .model = "assets://models/wedge.obj", .fit = ModelFit::Unit});
    Create("second", At(3.0f, 5.0f, 0.0f), RigidBody{},
           Collider{.shape = ShapeKind::ConvexHull, .model = "assets://models/wedge.obj", .fit = ModelFit::None});

    Step();

    ASSERT_EQ(_physics.created.size(), 2u);
    EXPECT_NE(_physics.created[0].shapes[0].points, _physics.created[1].shapes[0].points);
    EXPECT_EQ(_physics.created[0].shapes[0].source, "assets://models/wedge.obj (unit)");
    EXPECT_EQ(_physics.created[1].shapes[0].source, "assets://models/wedge.obj");
  }

  // what cannot be created

  TEST_F(PhysicsSimulationTest, SaysThatABodyHasNoCollider)
  {
    const Entity ghost = _store.CreateEntity("ghost");
    _store.Set(ghost, Transform{});
    _store.Set(ghost, RigidBody{});

    Step(3);

    EXPECT_EQ(_physics.create_attempts, 0);
    EXPECT_TRUE(BodyOf(ghost).failed);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The RigidBody of entity 'ghost' cannot be created: it has no Collider, on itself or on an entity "
      "below it")) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(PhysicsSimulationTest, SaysWhyThePhysicsRefusedAndDoesNotTryAgain)
  {
    _physics.fail_with = "a dynamic body cannot have a mesh";
    const Entity parent = _store.CreateEntity("level");
    const Entity ramp = Create("ramp", At(0.0f, 0.0f, 0.0f), RigidBody{}, Collider{}, parent);

    Step(10);

    EXPECT_EQ(_physics.create_attempts, 1);
    EXPECT_TRUE(BodyOf(ramp).failed);
    EXPECT_EQ(BodyOf(ramp).body, No_Body);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The RigidBody of entity 'level/ramp' cannot be created: a dynamic body cannot have a mesh"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(PhysicsSimulationTest, TriesAgainWhenTheComponentIsReplaced)
  {
    _physics.fail_with = "not now";
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});
    Step();

    _physics.fail_with.clear();
    _store.Set(crate, RigidBody{.mass = 3.0f});
    Step();

    EXPECT_EQ(_physics.create_attempts, 2);
    EXPECT_NE(BodyOf(crate).body, No_Body);
    EXPECT_FALSE(BodyOf(crate).failed);
  }

  TEST_F(PhysicsSimulationTest, RefusesAnEntityThatIsMoreThanOneThing)
  {
    const Entity both = Create("both", At(0.0f, 0.0f, 0.0f), RigidBody{});
    _store.Set(both, Trigger{});

    Step(3);

    EXPECT_EQ(_physics.create_attempts, 0);
    EXPECT_TRUE(BodyOf(both).failed);
    EXPECT_TRUE(_store.Get<Trigger>(both)->failed);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The RigidBody of entity 'both' cannot be created: the entity carries more than one of RigidBody, "
      "Trigger, and CharacterBody")) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(PhysicsSimulationTest, SaysOnceThatAColliderBelongsToNothing)
  {
    const Entity wall = _store.CreateEntity("wall");
    _store.Set(wall, Transform{});
    _store.Set(wall, Collider{});

    Step(5);

    EXPECT_EQ(_physics.create_attempts, 0);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn,
      "The Collider of entity 'wall' belongs to nothing. It needs a RigidBody, a Trigger, or a "
      "CharacterBody on its entity or on one above it")) << _logger->Messages(LogLevel::Warn);
  }

  // shapes that change while a body lives

  TEST_F(PhysicsSimulationTest, GivesABodyItsShapesAnewWhenItsColliderChanges)
  {
    const Entity crate = Create("crate", At(0.0f, 1.0f, 0.0f), RigidBody{}, Collider{.size = glm::vec3{1.0f}});
    Step(2);
    const auto body = BodyOf(crate).body;

    _store.Get<Collider>(crate)->size = {1.0f, 3.0f, 1.0f};
    Step();

    ASSERT_EQ(_physics.reshaped.size(), 1u);
    EXPECT_EQ(_physics.reshaped[0].body, body);
    ASSERT_EQ(_physics.reshaped[0].shapes.size(), 1u);
    EXPECT_EQ(_physics.reshaped[0].shapes[0].size, glm::vec3(1.0f, 3.0f, 1.0f));

    // the body is the same, and not created anew
    EXPECT_EQ(_physics.created.size(), 1u);
    EXPECT_EQ(BodyOf(crate).body, body);

    // and once is enough
    Step(3);
    EXPECT_EQ(_physics.reshaped.size(), 1u);
  }

  TEST_F(PhysicsSimulationTest, GivesABodyItsShapesAnewWhenAColliderBelowItChanges)
  {
    const Entity table = _store.CreateEntity("table");
    _store.Set(table, At(0.0f, 1.0f, 0.0f));
    _store.Set(table, RigidBody{});
    const Entity top = _store.CreateEntity("top", table);
    _store.Set(top, At(0.0f, 0.9f, 0.0f));
    _store.Set(top, Collider{.size = glm::vec3{2.0f, 0.2f, 2.0f}});
    Step();

    _store.Get<Collider>(top)->shape = ShapeKind::Sphere;
    Step();

    ASSERT_EQ(_physics.reshaped.size(), 1u);
    ASSERT_EQ(_physics.reshaped[0].shapes.size(), 1u);
    EXPECT_EQ(_physics.reshaped[0].shapes[0].kind, ShapeKind::Sphere);
    ExpectVector(_physics.reshaped[0].shapes[0].position, 0.0f, 0.9f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, TakesAColliderThatJoinsLaterIntoItsBody)
  {
    const Entity table = Create("table", At(0.0f, 1.0f, 0.0f), RigidBody{});
    Step();

    const Entity leg = _store.CreateEntity("leg", table);
    _store.Set(leg, At(-0.9f, 0.4f, -0.9f));
    _store.Set(leg, Collider{.size = glm::vec3{0.2f, 0.8f, 0.2f}});
    Step(5);

    EXPECT_EQ(_physics.created.size(), 1u);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 0u) << _logger->Messages(LogLevel::Warn);
    ASSERT_EQ(_physics.reshaped.size(), 1u);
    ASSERT_EQ(_physics.reshaped[0].shapes.size(), 2u);
    EXPECT_EQ(_physics.reshaped[0].shapes[1].size, glm::vec3(0.2f, 0.8f, 0.2f));
    ExpectVector(_physics.reshaped[0].shapes[1].position, -0.9f, 0.4f, -0.9f);

    // the leg is part of the table now, and a change to it is noticed
    _store.Get<Collider>(leg)->radius = 0.3f;
    Step();
    EXPECT_EQ(_physics.reshaped.size(), 2u);
  }

  TEST_F(PhysicsSimulationTest, GivesABodyItsShapesAnewWhenAColliderLeaves)
  {
    const Entity table = Create("table", At(0.0f, 1.0f, 0.0f), RigidBody{});
    const Entity leg = _store.CreateEntity("leg", table);
    _store.Set(leg, Transform{});
    _store.Set(leg, Collider{});
    Step();

    _store.Remove<Collider>(leg);
    Step();

    ASSERT_EQ(_physics.reshaped.size(), 1u);
    EXPECT_EQ(_physics.reshaped[0].shapes.size(), 1u);
  }

  TEST_F(PhysicsSimulationTest, KeepsTheShapesOfABodyWhoseLastColliderLeaves)
  {
    const Entity crate = Create("crate", At(0.0f, 1.0f, 0.0f), RigidBody{});
    Step();

    _store.Remove<Collider>(crate);
    Step(3);

    EXPECT_TRUE(_physics.reshaped.empty());
    EXPECT_TRUE(_physics.HasBody(BodyOf(crate).body));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The shape of the RigidBody of entity 'crate' cannot be changed, and stays what it was: it has no "
      "Collider, on itself or on an entity below it")) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(PhysicsSimulationTest, SaysOnceWhenTheNewShapesOfABodyCannotBeMade)
  {
    const Entity crate = Create("crate", At(0.0f, 1.0f, 0.0f), RigidBody{});
    Step();

    _physics.fail_with = "the box is too round";
    _store.Get<Collider>(crate)->size = glm::vec3{2.0f};
    Step(3);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The shape of the RigidBody of entity 'crate' cannot be changed, and stays what it was: the box is too "
      "round")) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(PhysicsSimulationTest, GivesATriggerItsShapesAnew)
  {
    const Entity gate = _store.CreateEntity("gate");
    _store.Set(gate, Transform{});
    _store.Set(gate, Trigger{});
    _store.Set(gate, Collider{});
    Step();

    _store.Get<Collider>(gate)->shape = ShapeKind::Sphere;
    Step();

    ASSERT_EQ(_physics.reshaped.size(), 1u);
    EXPECT_EQ(_physics.reshaped[0].body, _store.Get<Trigger>(gate)->body);
  }

  TEST_F(PhysicsSimulationTest, SaysThatTheShapeOfACharacterCannotChange)
  {
    const Entity player = CreateCharacter("player", At(0.0f, 1.0f, 0.0f));
    Step();

    _store.Get<Collider>(player)->radius = 0.7f;
    Step(3);

    EXPECT_TRUE(_physics.reshaped.empty());
    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The shape of the CharacterBody of entity 'player' cannot change while it lives. Replace the component "
      "to give it another shape")) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(PhysicsSimulationTest, StopsWatchingTheCollidersOfABodyThatIsReleased)
  {
    const Entity crate = Create("crate", At(0.0f, 1.0f, 0.0f), RigidBody{});
    Step();

    _store.Remove<RigidBody>(crate);
    _store.Get<Collider>(crate)->size = glm::vec3{2.0f};
    Step(2);

    EXPECT_TRUE(_physics.reshaped.empty());
  }

  // joints

  TEST_F(PhysicsSimulationTest, MakesAJointOnceBothBodiesExistWithTheAnchorAndTheAxisInTheWorld)
  {
    Create("frame", At(3.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Static});
    Transform turned = At(4.0f, 1.0f, 0.0f);
    turned.rotation = {.yaw = 90.0f};
    const Entity door = Create("door", turned, RigidBody{});
    Joint joint;
    joint.type = JointKind::Hinge;
    joint.other = "frame";
    joint.anchor = {-0.8f, 0.0f, 0.0f};
    joint.axis = {0.0f, 1.0f, 0.0f};
    joint.limits = {-90.0f, 45.0f};
    _store.Set(door, joint);

    Step();

    ASSERT_EQ(_physics.created_joints.size(), 1u);
    const auto &info = _physics.created_joints[0];
    EXPECT_EQ(info.kind, JointKind::Hinge);
    EXPECT_EQ(info.body, BodyOf(door).body);
    EXPECT_EQ(info.other, BodyOf(_store.FindEntity("frame")).body);

    // turned by 90 degrees of yaw, -x on the door points to -z in the world
    ExpectVector(info.anchor, 4.0f, 1.0f, 0.8f);
    ExpectVector(info.axis, 0.0f, 1.0f, 0.0f);
    EXPECT_TRUE(info.has_limits);
    EXPECT_NEAR(info.limit_min, -glm::half_pi<float>(), tolerance);
    EXPECT_NEAR(info.limit_max, glm::quarter_pi<float>(), tolerance);

    EXPECT_NE(_store.Get<Joint>(door)->joint, No_Joint);
    EXPECT_FALSE(_store.Get<Joint>(door)->failed);
    EXPECT_EQ(_physics.GetJointCount(), 1u);

    // once is enough
    Step(3);
    EXPECT_EQ(_physics.created_joints.size(), 1u);
  }

  TEST_F(PhysicsSimulationTest, SizesTheAnchorOfAJointByTheScaleOfItsEntity)
  {
    Transform scaled = At(0.0f, 1.0f, 0.0f);
    scaled.scale = {1.6f, 2.0f, 0.1f};
    const Entity door = Create("door", scaled, RigidBody{});
    _store.Set(door, Joint{.type = JointKind::Hinge, .anchor = {-0.5f, 0.25f, 0.0f}, .axis = {0.0f, 1.0f, 0.0f}});

    Step();

    ASSERT_EQ(_physics.created_joints.size(), 1u);
    ExpectVector(_physics.created_joints[0].anchor, -0.8f, 1.5f, 0.0f);
    ExpectVector(_physics.created_joints[0].axis, 0.0f, 1.0f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, HoldsARopeOnTheOtherEntityOrAtAPlaceInTheWorld)
  {
    Transform wide = At(3.0f, 4.0f, 0.0f);
    wide.scale = {2.0f, 1.0f, 1.0f};
    Create("beam", wide, RigidBody{.kind = BodyKind::Static});
    const Entity lamp = Create("lamp", At(4.0f, 2.0f, 0.0f), RigidBody{});
    const Entity bob = Create("bob", At(0.0f, 1.0f, 0.0f), RigidBody{});

    // the end of the beam, which its scale makes twice as far out
    _store.Set(lamp, Joint{
                 .type = JointKind::Rope,
                 .other = "beam",
                 .anchor = {0.0f, 0.5f, 0.0f},
                 .other_anchor = {0.5f, 0.0f, 0.0f},
                 .length = 2.5f});
    _store.Set(bob, Joint{.type = JointKind::Rope, .other_anchor = {0.0f, 6.0f, 1.0f}});

    Step();

    ASSERT_EQ(_physics.created_joints.size(), 2u);
    EXPECT_EQ(_physics.created_joints[0].kind, JointKind::Rope);
    ExpectVector(_physics.created_joints[0].anchor, 4.0f, 2.5f, 0.0f);
    ExpectVector(_physics.created_joints[0].other_anchor, 4.0f, 4.0f, 0.0f);
    EXPECT_EQ(_physics.created_joints[0].length, 2.5f);
    EXPECT_TRUE(_physics.created_joints[0].collide_with_other);

    ExpectVector(_physics.created_joints[1].other_anchor, 0.0f, 6.0f, 1.0f);
    EXPECT_EQ(_physics.created_joints[1].length, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, KeepsTheLimitsOfASliderInUnits)
  {
    const Entity sled = Create("sled", At(0.0f, 1.0f, 0.0f), RigidBody{});
    _store.Set(sled, Joint{.type = JointKind::Slider, .axis = {1.0f, 0.0f, 0.0f}, .limits = {-3.0f, 2.0f}});

    Step();

    ASSERT_EQ(_physics.created_joints.size(), 1u);
    EXPECT_EQ(_physics.created_joints[0].limit_min, -3.0f);
    EXPECT_EQ(_physics.created_joints[0].limit_max, 2.0f);
    EXPECT_TRUE(_physics.created_joints[0].has_limits);
  }

  TEST_F(PhysicsSimulationTest, HandsOnWhetherTheBodiesOfAJointCollide)
  {
    Create("frame", At(3.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Static});
    const Entity door = Create("door", At(4.0f, 1.0f, 0.0f), RigidBody{});
    const Entity bob = Create("bob", At(0.0f, 1.0f, 0.0f), RigidBody{});
    _store.Set(door, Joint{.type = JointKind::Hinge, .other = "frame", .axis = {0.0f, 1.0f, 0.0f}});
    _store.Set(bob, Joint{.type = JointKind::Point, .other = "frame"});

    Step();

    ASSERT_EQ(_physics.created_joints.size(), 2u);
    EXPECT_FALSE(_physics.created_joints[0].collide_with_other);
    EXPECT_TRUE(_physics.created_joints[1].collide_with_other);
  }

  TEST_F(PhysicsSimulationTest, HandsOnTheMotorOfAHingeInRadiansPerSecondAndOfASliderInUnits)
  {
    const Entity door = Create("door", At(4.0f, 1.0f, 0.0f), RigidBody{});
    const Entity sled = Create("sled", At(0.0f, 1.0f, 0.0f), RigidBody{});
    _store.Set(door, Joint{
                 .type = JointKind::Hinge,
                 .axis = {0.0f, 1.0f, 0.0f},
                 .motor_velocity = 90.0f,
                 .motor_strength = 20.0f,
               });
    _store.Set(sled, Joint{
                 .type = JointKind::Slider,
                 .axis = {1.0f, 0.0f, 0.0f},
                 .motor_velocity = 3.0f,
                 .motor_strength = 50.0f,
                 .spring_stiffness = 0.0f,
                 .spring_damping = 0.0f,
               });

    Step();

    ASSERT_EQ(_physics.created_joints.size(), 2u);
    EXPECT_NEAR(_physics.created_joints[0].motor_velocity, glm::half_pi<float>(), tolerance);
    EXPECT_EQ(_physics.created_joints[0].motor_strength, 20.0f);
    EXPECT_EQ(_physics.created_joints[1].motor_velocity, 3.0f);
    EXPECT_EQ(_physics.created_joints[1].motor_strength, 50.0f);
  }

  TEST_F(PhysicsSimulationTest, HandsOnTheSpringOfAJoint)
  {
    const Entity door = Create("door", At(4.0f, 1.0f, 0.0f), RigidBody{});
    _store.Set(door, Joint{
                 .type = JointKind::Hinge,
                 .axis = {0.0f, 1.0f, 0.0f},
                 .spring_stiffness = 12.0f,
                 .spring_damping = 1.5f,
               });

    Step();

    ASSERT_EQ(_physics.created_joints.size(), 1u);
    EXPECT_EQ(_physics.created_joints[0].spring_stiffness, 12.0f);
    EXPECT_EQ(_physics.created_joints[0].spring_damping, 1.5f);
    EXPECT_EQ(_physics.created_joints[0].motor_strength, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, JoinsABodyToTheWorldWhenTheJointNamesNothing)
  {
    const Entity bob = Create("bob", At(0.0f, 3.0f, 0.0f), RigidBody{});
    _store.Set(bob, Joint{.type = JointKind::Point, .anchor = {0.0f, 2.0f, 0.0f}});

    Step();

    ASSERT_EQ(_physics.created_joints.size(), 1u);
    EXPECT_EQ(_physics.created_joints[0].other, No_Body);
    EXPECT_FALSE(_physics.created_joints[0].has_limits);
    ExpectVector(_physics.created_joints[0].anchor, 0.0f, 5.0f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, MakesTheJointOfAnEntityThatJoinsLater)
  {
    Create("frame", At(3.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Static});
    Step(3);

    const Entity door = Create("door", At(4.0f, 1.0f, 0.0f), RigidBody{});
    _store.Set(door, Joint{.type = JointKind::Hinge, .other = "frame"});
    Step();

    ASSERT_EQ(_physics.created_joints.size(), 1u);
    EXPECT_EQ(_physics.created_joints[0].body, BodyOf(door).body);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(PhysicsSimulationTest, SaysOnceWhatIsWrongWithAJoint)
  {
    const Entity door = Create("door", At(0.0f, 1.0f, 0.0f), RigidBody{});
    _store.Set(door, Joint{.other = "house/frame"});
    const Entity sign = _store.CreateEntity("sign");
    _store.Set(sign, Transform{});
    _store.Set(sign, Joint{});
    const Entity frame = _store.CreateEntity("frame");
    _store.Set(frame, Transform{});
    const Entity bell = Create("bell", At(0.0f, 1.0f, 0.0f), RigidBody{});
    _store.Set(bell, Joint{.other = "frame"});

    Step(3);

    EXPECT_TRUE(_physics.created_joints.empty());
    EXPECT_TRUE(_store.Get<Joint>(door)->failed);
    EXPECT_TRUE(_store.Get<Joint>(sign)->failed);
    EXPECT_TRUE(_store.Get<Joint>(bell)->failed);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 3u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The Joint of entity 'door' cannot be created: it names 'house/frame', which is no entity"));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The Joint of entity 'sign' cannot be created: the entity has no RigidBody. Only a RigidBody can be "
      "joined to something"));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The Joint of entity 'bell' cannot be created: it names 'frame', which has no RigidBody"));
  }

  TEST_F(PhysicsSimulationTest, SaysWhenThePhysicsRefusesAJoint)
  {
    const Entity door = Create("door", At(0.0f, 1.0f, 0.0f), RigidBody{});
    Step();
    _store.Set(door, Joint{});
    _physics.fail_with = "neither body is dynamic";

    Step(3);

    EXPECT_TRUE(_store.Get<Joint>(door)->failed);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The Joint of entity 'door' cannot be created: neither body is dynamic"));
  }

  TEST_F(PhysicsSimulationTest, TakesAJointApartWhenItsComponentLeaves)
  {
    const Entity bob = Create("bob", At(0.0f, 3.0f, 0.0f), RigidBody{});
    _store.Set(bob, Joint{.type = JointKind::Point});
    Step();
    const auto made = _store.Get<Joint>(bob)->joint;

    _store.Remove<Joint>(bob);
    Step();

    EXPECT_THAT(_physics.destroyed_joints, ::testing::ElementsAre(made));
    EXPECT_EQ(_physics.GetJointCount(), 0u);
  }

  TEST_F(PhysicsSimulationTest, MakesAJointAgainWhenABodyItHeldIsReplaced)
  {
    const Entity frame = Create("frame", At(3.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Static});
    const Entity door = Create("door", At(4.0f, 1.0f, 0.0f), RigidBody{});
    _store.Set(door, Joint{.type = JointKind::Hinge, .other = "frame"});
    Step();
    EXPECT_EQ(_physics.GetJointCount(), 1u);

    // the frame is made anew, and the joint went with its body
    _store.Set(frame, RigidBody{.kind = BodyKind::Static});
    Step();

    EXPECT_EQ(_physics.created_joints.size(), 2u);
    EXPECT_EQ(_physics.GetJointCount(), 1u);
    EXPECT_EQ(_physics.created_joints[1].other, BodyOf(frame).body);
    EXPECT_EQ(_store.Get<Joint>(door)->joint, 2u);
  }

  TEST_F(PhysicsSimulationTest, LetsAJointGoWithTheEntityItIsOn)
  {
    const Entity bob = Create("bob", At(0.0f, 3.0f, 0.0f), RigidBody{});
    _store.Set(bob, Joint{.type = JointKind::Point});
    Step();

    _store.DestroyEntity(bob);

    EXPECT_EQ(_physics.GetJointCount(), 0u);
    EXPECT_EQ(_physics.GetBodyCount(), 0u);
  }

  // in the world

  TEST_F(PhysicsSimulationTest, PlacesTheBodyOfAChildInTheWorld)
  {
    Transform of_parent = At(10.0f, 0.0f, 0.0f);
    of_parent.rotation = {.yaw = 90.0f};
    of_parent.scale = {2.0f, 2.0f, 2.0f};
    const Entity parent = _store.CreateEntity("parent");
    _store.Set(parent, of_parent);
    Create("crate", At(0.0f, 0.0f, -1.0f), RigidBody{}, Collider{}, parent);

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    // 1 in front of the parent, which looks to the left and is twice the size
    ExpectVector(_physics.created[0].position, 8.0f, 0.0f, 0.0f);
    ExpectSameOrientation(_physics.created[0].rotation, Rotation{.yaw = 90.0f}.GetQuaternion());
    ExpectVector(_physics.created[0].shapes[0].scale, 2.0f, 2.0f, 2.0f);
  }

  TEST_F(PhysicsSimulationTest, WritesWhereTheBodyOfAChildIsRelativeToItsParent)
  {
    Transform of_parent = At(10.0f, 0.0f, 0.0f);
    of_parent.rotation = {.yaw = 90.0f};
    const Entity parent = _store.CreateEntity("parent");
    _store.Set(parent, of_parent);
    const Entity crate = Create("crate", At(0.0f, 0.0f, -1.0f), RigidBody{.gravity_scale = 0.0f}, Collider{}, parent);
    Step();

    // the physics moves it to the right of the world, which is behind the
    // parent
    FakeOf(crate).state.linear_velocity = {60.0f, 0.0f, 0.0f};
    Step();

    ExpectVector(FakeOf(crate).state.position, 10.0f, 0.0f, 0.0f);
    ExpectVector(TransformOf(crate).position, 0.0f, 0.0f, 0.0f);
    EXPECT_NEAR(TransformOf(crate).rotation.yaw, 0.0f, 1e-3f);
  }

  TEST_F(PhysicsSimulationTest, LeavesADynamicBodyWhereItIsWhenItsParentMoves)
  {
    const Entity parent = _store.CreateEntity("parent");
    _store.Set(parent, At(10.0f, 0.0f, 0.0f));
    const Entity crate = Create("crate", At(1.0f, 0.0f, 0.0f), RigidBody{.gravity_scale = 0.0f}, Collider{}, parent);
    Step();

    TransformOf(parent).position = {20.0f, 0.0f, 0.0f};
    Step();

    ExpectVector(FakeOf(crate).state.position, 11.0f, 0.0f, 0.0f);
    ExpectVector(TransformOf(crate).position, -9.0f, 0.0f, 0.0f);
    EXPECT_TRUE(_physics.placed.empty());
  }

  TEST_F(PhysicsSimulationTest, CarriesAStaticBodyWithItsParent)
  {
    const Entity parent = _store.CreateEntity("parent");
    _store.Set(parent, At(10.0f, 0.0f, 0.0f));
    const Entity wall = Create("wall", At(1.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Static}, Collider{}, parent);
    Step();

    TransformOf(parent).position = {20.0f, 0.0f, 0.0f};
    Step();

    ASSERT_EQ(_physics.placed.size(), 1u);
    ExpectVector(_physics.placed[0].position, 21.0f, 0.0f, 0.0f);
    ExpectVector(TransformOf(wall).position, 1.0f, 0.0f, 0.0f);
  }

  // a dynamic body

  TEST_F(PhysicsSimulationTest, WritesWhereTheSimulationPutADynamicBody)
  {
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});

    Step(60);

    const auto &state = FakeOf(crate).state;
    EXPECT_LT(state.position.y, 0.5f);
    EXPECT_EQ(TransformOf(crate).position, state.position);
    EXPECT_EQ(BodyOf(crate).linear_velocity, state.linear_velocity);
  }

  TEST_F(PhysicsSimulationTest, WritesHowADynamicBodyIsTurned)
  {
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});
    Step();

    // straight up, where the angles of a rotation are not unique
    const auto tumbled = Rotation{.pitch = 90.0f, .yaw = 40.0f, .roll = 25.0f}.GetQuaternion();
    FakeOf(crate).state.rotation = tumbled;
    FakeOf(crate).state.angular_velocity = {glm::pi<float>(), 0.0f, 0.0f};
    Step();

    ExpectSameOrientation(TransformOf(crate).rotation.GetQuaternion(), tumbled);
    ExpectVector(BodyOf(crate).angular_velocity, 180.0f, 0.0f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, LeavesTheScaleOfABodyAlone)
  {
    Transform transform = At(0.0f, 5.0f, 0.0f);
    transform.scale = {2.0f, 3.0f, 4.0f};
    const Entity crate = Create("crate", transform, RigidBody{});

    Step(3);

    EXPECT_EQ(TransformOf(crate).scale, glm::vec3(2.0f, 3.0f, 4.0f));
  }

  TEST_F(PhysicsSimulationTest, PutsADynamicBodyWhereAGameWroteItsTransform)
  {
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{.gravity_scale = 0.0f});
    Step(3);
    EXPECT_TRUE(_physics.placed.empty());

    TransformOf(crate).position = {7.0f, 8.0f, 9.0f};
    TransformOf(crate).rotation = {.yaw = 45.0f};
    Step();

    ASSERT_EQ(_physics.placed.size(), 1u);
    EXPECT_EQ(_physics.placed[0].body, BodyOf(crate).body);
    ExpectVector(_physics.placed[0].position, 7.0f, 8.0f, 9.0f);
    ExpectSameOrientation(_physics.placed[0].rotation, Rotation{.yaw = 45.0f}.GetQuaternion());

    // and once only
    Step(3);
    EXPECT_EQ(_physics.placed.size(), 1u);
  }

  TEST_F(PhysicsSimulationTest, MovesADynamicBodyWithTheVelocityAGameWrote)
  {
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{.gravity_scale = 0.0f});
    Step();

    BodyOf(crate).linear_velocity = {6.0f, 0.0f, 0.0f};
    BodyOf(crate).angular_velocity = {0.0f, 180.0f, 0.0f};
    Step();

    ExpectVector(FakeOf(crate).state.linear_velocity, 6.0f, 0.0f, 0.0f);
    ExpectVector(FakeOf(crate).state.angular_velocity, 0.0f, glm::pi<float>(), 0.0f);
    ExpectVector(TransformOf(crate).position, 0.1f, 5.0f, 0.0f);
  }

  // a static body

  TEST_F(PhysicsSimulationTest, NeverWritesTheTransformOfAStaticBody)
  {
    const Entity floor = Create("floor", At(0.0f, -1.0f, 0.0f), RigidBody{.kind = BodyKind::Static});
    Step();

    FakeOf(floor).state.position = {5.0f, 5.0f, 5.0f};
    Step();

    ExpectVector(TransformOf(floor).position, 0.0f, -1.0f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, PutsAStaticBodyWhereItsTransformIsMovedTo)
  {
    const Entity floor = Create("floor", At(0.0f, -1.0f, 0.0f), RigidBody{.kind = BodyKind::Static});
    Step(3);
    EXPECT_TRUE(_physics.placed.empty());

    TransformOf(floor).position = {0.0f, -2.0f, 0.0f};
    Step(3);

    ASSERT_EQ(_physics.placed.size(), 1u);
    ExpectVector(_physics.placed[0].position, 0.0f, -2.0f, 0.0f);
  }

  // a kinematic body

  TEST_F(PhysicsSimulationTest, MovesAKinematicBodyToWhereItsTransformIsMovedTo)
  {
    const Entity lift = Create("lift", At(0.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Kinematic});
    Step();
    EXPECT_TRUE(_physics.moved.empty());

    TransformOf(lift).position = {0.0f, 0.5f, 0.0f};
    TransformOf(lift).rotation = {.yaw = 30.0f};
    Step();

    ASSERT_EQ(_physics.moved.size(), 1u);
    EXPECT_EQ(_physics.moved[0].body, BodyOf(lift).body);
    ExpectVector(_physics.moved[0].position, 0.0f, 0.5f, 0.0f);
    ExpectSameOrientation(_physics.moved[0].rotation, Rotation{.yaw = 30.0f}.GetQuaternion());
    EXPECT_EQ(_physics.moved[0].seconds, step);
    EXPECT_TRUE(_physics.placed.empty());
  }

  TEST_F(PhysicsSimulationTest, LeavesTheTransformOfAKinematicBodyAsItWasWritten)
  {
    const Entity lift = Create("lift", At(0.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Kinematic});
    Step();

    TransformOf(lift).position = {0.1f, 0.5f, 0.0f};
    TransformOf(lift).rotation = {.pitch = 10.0f, .yaw = 30.0f, .roll = 5.0f};
    Step(3);

    EXPECT_EQ(TransformOf(lift).position, glm::vec3(0.1f, 0.5f, 0.0f));
    EXPECT_EQ(TransformOf(lift).rotation.pitch, 10.0f);
    EXPECT_EQ(TransformOf(lift).rotation.yaw, 30.0f);
    EXPECT_EQ(TransformOf(lift).rotation.roll, 5.0f);
    EXPECT_EQ(_physics.moved.size(), 1u);
  }

  TEST_F(PhysicsSimulationTest, StopsAKinematicBodyOnceItArrived)
  {
    const Entity lift = Create("lift", At(0.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Kinematic});
    Step();
    TransformOf(lift).position = {0.0f, 0.5f, 0.0f};
    Step();

    // what a backend does: it keeps the velocity it was moved with
    FakeOf(lift).state.linear_velocity = {0.0f, 30.0f, 0.0f};
    Step();

    ExpectVector(FakeOf(lift).state.linear_velocity, 0.0f, 0.0f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, MovesAKinematicBodyInEveryStepItsTransformChanges)
  {
    const Entity lift = Create("lift", At(0.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Kinematic});
    Step();

    for (int i = 1; i <= 10; i++)
    {
      TransformOf(lift).position.y = 0.1f * static_cast<float>(i);
      Step();
    }

    EXPECT_EQ(_physics.moved.size(), 10u);
    ExpectVector(FakeOf(lift).state.position, 0.0f, 1.0f, 0.0f);
  }

  // a trigger

  TEST_F(PhysicsSimulationTest, CreatesATriggerAsABodyThatStopsNothing)
  {
    const Entity door = _store.CreateEntity("door");
    _store.Set(door, At(0.0f, 1.0f, -5.0f));
    _store.Set(door, Trigger{.layers = 0b100, .mask = 0b11});
    _store.Set(door, Collider{.shape = ShapeKind::Sphere, .radius = 2.0f});

    Step();

    ASSERT_EQ(_physics.created.size(), 1u);
    const auto &info = _physics.created[0];
    EXPECT_EQ(info.entity, door);
    EXPECT_TRUE(info.trigger);
    EXPECT_EQ(info.layers, 0b100u);
    EXPECT_EQ(info.mask, 0b11u);
    ExpectVector(info.position, 0.0f, 1.0f, -5.0f);
    ASSERT_EQ(info.shapes.size(), 1u);
    EXPECT_EQ(info.shapes[0].kind, ShapeKind::Sphere);
    EXPECT_EQ(info.shapes[0].radius, 2.0f);
    EXPECT_NE(_store.Get<Trigger>(door)->body, No_Body);
  }

  TEST_F(PhysicsSimulationTest, PutsATriggerWhereItsTransformIsMovedTo)
  {
    const Entity door = _store.CreateEntity("door");
    _store.Set(door, At(0.0f, 1.0f, -5.0f));
    _store.Set(door, Trigger{});
    _store.Set(door, Collider{});
    Step();

    TransformOf(door).position = {3.0f, 1.0f, -5.0f};
    Step(3);

    ASSERT_EQ(_physics.placed.size(), 1u);
    ExpectVector(_physics.placed[0].position, 3.0f, 1.0f, -5.0f);
    ExpectVector(TransformOf(door).position, 3.0f, 1.0f, -5.0f);
  }

  TEST_F(PhysicsSimulationTest, CountsWhatIsInsideATrigger)
  {
    const Entity door = _store.CreateEntity("door");
    _store.Set(door, Transform{});
    _store.Set(door, Trigger{});
    _store.Set(door, Collider{});
    const Entity first = Create("first", At(0.0f, 5.0f, 0.0f), RigidBody{});
    const Entity second = Create("second", At(0.0f, 8.0f, 0.0f), RigidBody{});
    Step();
    const auto trigger = _store.Get<Trigger>(door)->body;

    const auto event = [&](const PhysicsEventKind kind, const Entity entity)
    {
      return PhysicsEvent{
        .kind = kind,
        .trigger = true,
        .first = door,
        .second = entity,
        .first_body = trigger,
        .second_body = BodyOf(entity).body
      };
    };

    _physics.next_events = {event(PhysicsEventKind::Began, first)};
    Step();
    EXPECT_EQ(_store.Get<Trigger>(door)->inside, 1u);

    _physics.next_events = {event(PhysicsEventKind::Began, second)};
    Step();
    EXPECT_EQ(_store.Get<Trigger>(door)->inside, 2u);

    // nothing was reported
    Step();
    EXPECT_EQ(_store.Get<Trigger>(door)->inside, 2u);

    _physics.next_events = {event(PhysicsEventKind::Ended, first), event(PhysicsEventKind::Ended, second)};
    Step();
    EXPECT_EQ(_store.Get<Trigger>(door)->inside, 0u);
  }

  TEST_F(PhysicsSimulationTest, IgnoresWhatIsReportedOfATriggerThatIsGone)
  {
    const Entity door = _store.CreateEntity("door");
    _store.Set(door, Transform{});
    _store.Set(door, Trigger{});
    _store.Set(door, Collider{});
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});
    Step();
    const auto trigger = _store.Get<Trigger>(door)->body;

    _store.DestroyEntity(door);
    _physics.next_events = {
      PhysicsEvent{
        .kind = PhysicsEventKind::Ended,
        .trigger = true,
        .first = door,
        .second = crate,
        .first_body = trigger,
        .second_body = BodyOf(crate).body
      }
    };

    EXPECT_NO_THROW(Step());
  }

  TEST_F(PhysicsSimulationTest, DoesNotCountWhatCollidesWithoutATrigger)
  {
    const Entity door = _store.CreateEntity("door");
    _store.Set(door, Transform{});
    _store.Set(door, Trigger{});
    _store.Set(door, Collider{});
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});
    Step();

    _physics.next_events = {
      PhysicsEvent{.kind = PhysicsEventKind::Began, .trigger = false, .first = door, .second = crate}
    };
    Step();

    EXPECT_EQ(_store.Get<Trigger>(door)->inside, 0u);
  }

  // a character

  TEST_F(PhysicsSimulationTest, CreatesACharacterWithEveryValue)
  {
    Transform transform = At(1.0f, 2.0f, 3.0f);
    transform.rotation = {.yaw = 90.0f};
    CharacterBody character;
    character.max_slope = 60.0f;
    character.step_height = 0.4f;
    character.mass = 80.0f;
    character.push_strength = 250.0f;
    character.layers = 0b10;
    character.mask = 0b111;
    const Entity player = CreateCharacter("player", transform, character);

    Step();

    ASSERT_EQ(_physics.GetCharacterCount(), 1u);
    EXPECT_EQ(_physics.GetBodyCount(), 0u);
    const auto &info = _physics.CharacterOf(player)->info;
    EXPECT_EQ(info.entity, player);
    ExpectVector(info.position, 1.0f, 2.0f, 3.0f);
    EXPECT_EQ(info.max_slope, 60.0f);
    EXPECT_EQ(info.step_height, 0.4f);
    EXPECT_EQ(info.mass, 80.0f);
    EXPECT_EQ(info.push_strength, 250.0f);
    EXPECT_EQ(info.layers, 0b10u);
    EXPECT_EQ(info.mask, 0b111u);
    ASSERT_EQ(info.shapes.size(), 1u);
    EXPECT_EQ(info.shapes[0].kind, ShapeKind::Capsule);
    EXPECT_NE(_store.Get<CharacterBody>(player)->character, No_Character);
  }

  TEST_F(PhysicsSimulationTest, MovesACharacterByItsVelocityInEveryStep)
  {
    const Entity player = CreateCharacter(
      "player", At(0.0f, 1.0f, 0.0f), CharacterBody{.velocity = {3.0f, 0.0f, 0.0f}, .gravity_scale = 0.0f});

    Step(60);

    ASSERT_EQ(_physics.character_moves.size(), 60u);
    for (const auto &move : _physics.character_moves)
    {
      EXPECT_EQ(move.seconds, step);
      EXPECT_EQ(move.velocity, glm::vec3(3.0f, 0.0f, 0.0f));
    }
    ExpectVector(TransformOf(player).position, 3.0f, 1.0f, 0.0f);

    // the velocity is what the game asked for, and is kept
    EXPECT_EQ(_store.Get<CharacterBody>(player)->velocity, glm::vec3(3.0f, 0.0f, 0.0f));
    EXPECT_EQ(_store.Get<CharacterBody>(player)->real_velocity, glm::vec3(3.0f, 0.0f, 0.0f));
  }

  TEST_F(PhysicsSimulationTest, LetsACharacterFallWhileItIsInTheAir)
  {
    const Entity player = CreateCharacter("player", At(0.0f, 10.0f, 0.0f));

    Step(3);

    const auto *character = _store.Get<CharacterBody>(player);
    EXPECT_FALSE(character->on_floor);
    EXPECT_NEAR(character->fall_velocity.y, -9.81f * 3.0f / 60.0f, tolerance);
    EXPECT_LT(TransformOf(player).position.y, 10.0f);
    ASSERT_EQ(_physics.character_moves.size(), 3u);
    EXPECT_NEAR(_physics.character_moves[2].velocity.y, -9.81f * 3.0f / 60.0f, tolerance);
  }

  TEST_F(PhysicsSimulationTest, EndsTheFallOfACharacterOnTheGround)
  {
    const Entity player = CreateCharacter("player", At(0.0f, 10.0f, 0.0f));
    Step(30);

    _physics.on_floor = true;
    Step(30);

    const auto *character = _store.Get<CharacterBody>(player);
    EXPECT_TRUE(character->on_floor);
    // what is left is the pull of one step, which keeps it on the ground
    EXPECT_NEAR(character->fall_velocity.y, -9.81f / 60.0f, tolerance);
  }

  TEST_F(PhysicsSimulationTest, LetsACharacterJumpFromTheGround)
  {
    _physics.on_floor = true;
    const Entity player = CreateCharacter("player", At(0.0f, 0.0f, 0.0f));
    Step(5);

    _store.Get<CharacterBody>(player)->fall_velocity = {0.0f, 5.0f, 0.0f};
    Step();

    EXPECT_NEAR(_physics.character_moves.back().velocity.y, 5.0f - 9.81f / 60.0f, tolerance);
  }

  TEST_F(PhysicsSimulationTest, DoesNotLetACharacterFallThatDoesNotFall)
  {
    const Entity player = CreateCharacter("player", At(0.0f, 10.0f, 0.0f), CharacterBody{.gravity_scale = 0.0f});

    Step(30);

    ExpectVector(TransformOf(player).position, 0.0f, 10.0f, 0.0f);
    ExpectVector(_store.Get<CharacterBody>(player)->fall_velocity, 0.0f, 0.0f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, LeavesHowACharacterIsTurnedToItsGame)
  {
    Transform transform = At(0.0f, 1.0f, 0.0f);
    transform.rotation = {.pitch = 10.0f, .yaw = 90.0f, .roll = 5.0f};
    const Entity player = CreateCharacter(
      "player", transform, CharacterBody{.velocity = {1.0f, 0.0f, 0.0f}, .gravity_scale = 0.0f});

    Step(10);

    EXPECT_EQ(TransformOf(player).rotation.pitch, 10.0f);
    EXPECT_EQ(TransformOf(player).rotation.yaw, 90.0f);
    EXPECT_EQ(TransformOf(player).rotation.roll, 5.0f);

    // turning it does not count as moving it somewhere
    TransformOf(player).rotation.yaw = 120.0f;
    Step();
    EXPECT_EQ(TransformOf(player).rotation.yaw, 120.0f);
    EXPECT_NEAR(TransformOf(player).position.x, 11.0f / 60.0f, tolerance);
  }

  TEST_F(PhysicsSimulationTest, PutsACharacterWhereAGameWroteItsTransform)
  {
    const Entity player = CreateCharacter("player", At(0.0f, 1.0f, 0.0f), CharacterBody{.gravity_scale = 0.0f});
    Step(3);

    TransformOf(player).position = {50.0f, 1.0f, 0.0f};
    Step();

    ExpectVector(_physics.CharacterOf(player)->state.position, 50.0f, 1.0f, 0.0f);
    ExpectVector(TransformOf(player).position, 50.0f, 1.0f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, WritesWhatTheMoveOfACharacterLedTo)
  {
    _physics.on_floor = true;
    const Entity player = CreateCharacter("player", At(0.0f, 1.0f, 0.0f));

    Step();

    EXPECT_TRUE(_store.Get<CharacterBody>(player)->on_floor);
    EXPECT_FALSE(_store.Get<CharacterBody>(player)->on_wall);
    EXPECT_EQ(_store.Get<CharacterBody>(player)->floor_normal, glm::vec3(0.0f, 1.0f, 0.0f));
  }

  // steps

  TEST_F(PhysicsSimulationTest, TakesOneStepOfThePhysicsInEveryStepOfTheWorld)
  {
    Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});

    Step(7);

    ASSERT_EQ(_physics.steps.size(), 7u);
    for (const double seconds : _physics.steps) { EXPECT_EQ(seconds, step); }
  }

  TEST_F(PhysicsSimulationTest, DoesNothingWithTheTimeOfAFrame)
  {
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{.linear_velocity = {1.0f, 0.0f, 0.0f}});
    const Entity lift = Create("lift", At(3.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Kinematic});
    const Entity player = CreateCharacter("player", At(6.0f, 0.0f, 0.0f), CharacterBody{.velocity = {1.0f, 0.0f, 0.0f}});
    Step();
    const auto crate_at = TransformOf(crate).position;
    const auto player_at = TransformOf(player).position;
    const auto fall = _store.Get<CharacterBody>(player)->fall_velocity;
    TransformOf(lift).position.y = 1.0f;

    // frames of every length, and no step
    for (const double frame : {0.0001, 0.004, 0.0167, 0.1, 2.0})
    {
      _system.Update(_store, frame);
      _system.Interpolate(_store, 0.0);
    }

    EXPECT_EQ(_physics.steps.size(), 1u);
    EXPECT_EQ(_physics.character_moves.size(), 1u);
    EXPECT_TRUE(_physics.moved.empty());
    EXPECT_EQ(TransformOf(crate).position, crate_at);
    EXPECT_EQ(TransformOf(player).position, player_at);
    EXPECT_EQ(_store.Get<CharacterBody>(player)->fall_velocity, fall);
  }

  TEST_F(PhysicsSimulationTest, MovesByTheLengthOfAStepAndNotByTheTimeOfAFrame)
  {
    // A velocity or a force that were multiplied by the time of a frame
    // would lead somewhere else for frames of another length. The same
    // steps are taken here between frames of 1/30, 1/1000, and 1/4 of a
    // second.
    const auto run = [](const std::vector<double> &frames)
    {
      const auto logger = std::make_shared<RecordingLogger>();
      MemoryFileSystem files{SettingsConfig{}, logger};
      FakePhysicsContext physics;
      PhysicsSimulation system{&physics, &files, logger};
      FakeEntityStore store;
      store.Initialize();
      store.Register<Transform>("Transform");
      system.Register(store);
      system.Initialize(store);

      const Entity crate = store.CreateEntity("crate");
      store.Set(crate, At(0.0f, 5.0f, 0.0f));
      store.Set(crate, RigidBody{.linear_velocity = {2.0f, 0.0f, 0.0f}});
      store.Set(crate, Collider{});
      const Entity player = store.CreateEntity("player");
      store.Set(player, At(0.0f, 0.0f, 5.0f));
      store.Set(player, CharacterBody{.velocity = {0.0f, 0.0f, -3.0f}});
      store.Set(player, Collider{.shape = ShapeKind::Capsule});

      for (int i = 0; i < 120; i++)
      {
        system.FixedUpdate(store, step);
        system.Update(store, frames[static_cast<std::size_t>(i) % frames.size()]);
        system.Interpolate(store, 0.5);
      }

      for (const double seconds : physics.steps) { EXPECT_EQ(seconds, step); }
      for (const auto &move : physics.character_moves) { EXPECT_EQ(move.seconds, step); }

      const std::vector<glm::vec3> result = {
        store.Get<Transform>(crate)->position,
        store.Get<RigidBody>(crate)->linear_velocity,
        store.Get<Transform>(player)->position,
        store.Get<CharacterBody>(player)->fall_velocity
      };
      store.CleanUp();
      return result;
    };

    const auto at_60 = run({1.0 / 60.0});

    EXPECT_EQ(run({1.0 / 30.0}), at_60);
    EXPECT_EQ(run({1.0 / 1000.0}), at_60);
    EXPECT_EQ(run({0.25, 0.001, 0.02}), at_60);

    // and it is where two seconds of steps lead to
    EXPECT_NEAR(at_60[0].x, 4.0f, 1e-3f);
    EXPECT_NEAR(at_60[2].z, -1.0f, 1e-3f);
  }

  // what is drawn

  TEST_F(PhysicsSimulationTest, DrawsABodyBetweenTheLastTwoSteps)
  {
    const Entity crate = Create(
      "crate", At(0.0f, 5.0f, 0.0f), RigidBody{.gravity_scale = 0.0f, .linear_velocity = {60.0f, 0.0f, 0.0f}});
    Step(2);
    ExpectVector(TransformOf(crate).position, 2.0f, 5.0f, 0.0f);

    _system.Interpolate(_store, 0.25);
    ExpectVector(TransformOf(crate).world_coordinates[3], 1.25f, 5.0f, 0.0f);

    _system.Interpolate(_store, 0.0);
    ExpectVector(TransformOf(crate).world_coordinates[3], 1.0f, 5.0f, 0.0f);

    _system.Interpolate(_store, 1.0);
    ExpectVector(TransformOf(crate).world_coordinates[3], 2.0f, 5.0f, 0.0f);

    // where the body is has not changed
    ExpectVector(TransformOf(crate).position, 2.0f, 5.0f, 0.0f);
    ExpectVector(FakeOf(crate).state.position, 2.0f, 5.0f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, DrawsHowABodyIsTurnedBetweenTheLastTwoSteps)
  {
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{.gravity_scale = 0.0f});
    Step();
    FakeOf(crate).state.rotation = Rotation{.yaw = 90.0f}.GetQuaternion();
    Step();

    _system.Interpolate(_store, 0.5);

    const auto expected = mat4_cast(Rotation{.yaw = 45.0f}.GetQuaternion());
    const auto &drawn = TransformOf(crate).world_coordinates;
    for (int column = 0; column < 3; column++)
    {
      for (int row = 0; row < 3; row++) { EXPECT_NEAR(drawn[column][row], expected[column][row], tolerance); }
    }
  }

  TEST_F(PhysicsSimulationTest, DrawsABodyAtItsSize)
  {
    Transform transform = At(0.0f, 5.0f, 0.0f);
    transform.scale = {2.0f, 3.0f, 4.0f};
    const Entity crate = Create("crate", transform, RigidBody{.linear_velocity = {1.0f, 0.0f, 0.0f}});
    Step(2);

    _system.Interpolate(_store, 0.5);

    const auto &drawn = TransformOf(crate).world_coordinates;
    EXPECT_NEAR(length(glm::vec3(drawn[0])), 2.0f, tolerance);
    EXPECT_NEAR(length(glm::vec3(drawn[1])), 3.0f, tolerance);
    EXPECT_NEAR(length(glm::vec3(drawn[2])), 4.0f, tolerance);
  }

  TEST_F(PhysicsSimulationTest, DrawsWhatIsBelowABodyWithIt)
  {
    const Entity crate = Create(
      "crate", At(0.0f, 5.0f, 0.0f), RigidBody{.gravity_scale = 0.0f, .linear_velocity = {60.0f, 0.0f, 0.0f}});
    const Entity label = _store.CreateEntity("label", crate);
    _store.Set(label, At(0.0f, 1.0f, 0.0f));
    const Entity folder = _store.CreateEntity("folder", crate);
    const Entity light = _store.CreateEntity("light", folder);
    _store.Set(light, At(0.0f, 0.0f, 2.0f));
    Step(2);

    _system.Interpolate(_store, 0.5);

    ExpectVector(TransformOf(label).world_coordinates[3], 1.5f, 6.0f, 0.0f);
    ExpectVector(TransformOf(light).world_coordinates[3], 1.5f, 5.0f, 2.0f);
  }

  TEST_F(PhysicsSimulationTest, DrawsACharacterBetweenTheLastTwoStepsTurnedAsItsTransformSays)
  {
    Transform transform = At(0.0f, 1.0f, 0.0f);
    transform.rotation = {.yaw = 90.0f};
    const Entity player = CreateCharacter(
      "player", transform, CharacterBody{.velocity = {60.0f, 0.0f, 0.0f}, .gravity_scale = 0.0f});
    Step(2);

    // what placing every entity in the world does
    TransformOf(player).world_coordinates =
      translate(glm::mat4(1.0f), TransformOf(player).position) * mat4_cast(transform.rotation.GetQuaternion());
    _system.Interpolate(_store, 0.5);

    ExpectVector(TransformOf(player).world_coordinates[3], 1.5f, 1.0f, 0.0f);
    ExpectVector(TransformOf(player).Forward(), -1.0f, 0.0f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, DrawsAKinematicBodyBetweenTheLastTwoSteps)
  {
    const Entity lift = Create("lift", At(0.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Kinematic});
    Step();
    TransformOf(lift).position = {0.0f, 1.0f, 0.0f};
    Step();

    _system.Interpolate(_store, 0.5);

    ExpectVector(TransformOf(lift).world_coordinates[3], 0.0f, 0.5f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, LeavesWhatRestsWhereItsTransformPutIt)
  {
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{.gravity_scale = 0.0f});
    const Entity floor = Create("floor", At(0.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Static});
    Step(2);
    TransformOf(crate).world_coordinates = glm::mat4(7.0f);
    TransformOf(floor).world_coordinates = glm::mat4(7.0f);

    _system.Interpolate(_store, 0.5);

    EXPECT_EQ(TransformOf(crate).world_coordinates, glm::mat4(7.0f));
    EXPECT_EQ(TransformOf(floor).world_coordinates, glm::mat4(7.0f));
  }

  TEST_F(PhysicsSimulationTest, DrawsABodyThatWasPutSomewhereThereAtOnce)
  {
    const Entity crate = Create(
      "crate", At(0.0f, 5.0f, 0.0f), RigidBody{.gravity_scale = 0.0f, .linear_velocity = {60.0f, 0.0f, 0.0f}});
    Step(2);

    TransformOf(crate).position = {100.0f, 5.0f, 0.0f};
    Step();
    _system.Interpolate(_store, 0.5);

    // between where it was put and where the step after led to, and
    // nowhere on the way from where it was before
    ExpectVector(TransformOf(crate).world_coordinates[3], 100.5f, 5.0f, 0.0f);
  }

  TEST_F(PhysicsSimulationTest, DrawsADynamicBodyBelowAParentWhereThePhysicsPutIt)
  {
    const Entity parent = _store.CreateEntity("parent");
    _store.Set(parent, At(10.0f, 0.0f, 0.0f));
    const Entity crate = Create("crate", At(1.0f, 0.0f, 0.0f), RigidBody{.gravity_scale = 0.0f}, Collider{}, parent);
    Step(2);

    // a game moves the parent in a frame without a step
    TransformOf(parent).position = {20.0f, 0.0f, 0.0f};
    TransformOf(crate).world_coordinates = translate(glm::mat4(1.0f), glm::vec3(21.0f, 0.0f, 0.0f));
    _system.Interpolate(_store, 0.5);

    ExpectVector(TransformOf(crate).world_coordinates[3], 11.0f, 0.0f, 0.0f);
  }

  // events

  TEST_F(PhysicsSimulationTest, EndsTheFrameOfThePhysicsOncePerFrame)
  {
    Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});
    _physics.next_events = {PhysicsEvent{.kind = PhysicsEventKind::Began}};
    Step(2);
    EXPECT_EQ(_physics.end_frame_count, 0);
    EXPECT_EQ(_physics.GetFrameEvents().size(), 1u);

    _system.Update(_store, 0.016);
    EXPECT_EQ(_physics.GetFrameEvents().size(), 1u);

    _system.Interpolate(_store, 0.5);

    EXPECT_EQ(_physics.end_frame_count, 1);
    EXPECT_TRUE(_physics.GetFrameEvents().empty());
  }

  // releasing

  TEST_F(PhysicsSimulationTest, DestroysTheBodyOfAComponentThatIsRemoved)
  {
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});
    Step();
    const auto body = BodyOf(crate).body;

    _store.Remove<RigidBody>(crate);

    EXPECT_FALSE(_physics.HasBody(body));
    EXPECT_THAT(_physics.destroyed, ::testing::ElementsAre(body));

    EXPECT_NO_THROW(Step(3));
    EXPECT_EQ(_physics.GetBodyCount(), 0u);
  }

  TEST_F(PhysicsSimulationTest, DestroysTheBodyOfAnEntityThatIsDestroyed)
  {
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});
    const Entity other = Create("other", At(3.0f, 5.0f, 0.0f), RigidBody{});
    Step();
    const auto body = BodyOf(crate).body;

    _store.DestroyEntity(crate);

    EXPECT_FALSE(_physics.HasBody(body));
    EXPECT_TRUE(_physics.HasBody(BodyOf(other).body));
    EXPECT_NO_THROW(Step(3));
  }

  TEST_F(PhysicsSimulationTest, DestroysTheBodiesBelowAnEntityThatIsDestroyed)
  {
    const Entity car = Create("car", At(0.0f, 1.0f, 0.0f), RigidBody{});
    Create("trailer", At(0.0f, 0.0f, 3.0f), RigidBody{}, Collider{}, car);
    Step();
    ASSERT_EQ(_physics.GetBodyCount(), 2u);

    _store.DestroyEntity(car);

    EXPECT_EQ(_physics.GetBodyCount(), 0u);
  }

  TEST_F(PhysicsSimulationTest, DestroysATriggerAndACharacterThatAreRemoved)
  {
    const Entity door = _store.CreateEntity("door");
    _store.Set(door, Transform{});
    _store.Set(door, Trigger{});
    _store.Set(door, Collider{});
    const Entity player = CreateCharacter("player", At(5.0f, 0.0f, 0.0f));
    Step();
    ASSERT_EQ(_physics.GetBodyCount(), 1u);
    ASSERT_EQ(_physics.GetCharacterCount(), 1u);

    _store.Remove<Trigger>(door);
    _store.DestroyEntity(player);

    EXPECT_EQ(_physics.GetBodyCount(), 0u);
    EXPECT_EQ(_physics.GetCharacterCount(), 0u);
  }

  TEST_F(PhysicsSimulationTest, DestroysEverythingWhenTheStoreIsCleanedUp)
  {
    Create("floor", At(0.0f, 0.0f, 0.0f), RigidBody{.kind = BodyKind::Static});
    Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});
    CreateCharacter("player", At(5.0f, 0.0f, 0.0f));
    Step();
    ASSERT_EQ(_physics.GetBodyCount(), 2u);

    _store.CleanUp();

    EXPECT_EQ(_physics.GetBodyCount(), 0u);
    EXPECT_EQ(_physics.GetCharacterCount(), 0u);
  }

  TEST_F(PhysicsSimulationTest, DestroysNothingForWhatWasNeverCreated)
  {
    _physics.fail_with = "no";
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});
    const Entity never_stepped = Create("late", At(0.0f, 5.0f, 0.0f), RigidBody{});
    Step();

    _store.DestroyEntity(crate);
    _store.DestroyEntity(never_stepped);

    EXPECT_TRUE(_physics.destroyed.empty());
  }

  TEST_F(PhysicsSimulationTest, CreatesTheBodyAnewWhenItsComponentIsReplaced)
  {
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{.mass = 1.0f});
    Step();
    const auto before = BodyOf(crate).body;

    _store.Set(crate, RigidBody{.kind = BodyKind::Static, .mass = 5.0f});
    Step();

    EXPECT_FALSE(_physics.HasBody(before));
    EXPECT_NE(BodyOf(crate).body, before);
    EXPECT_EQ(_physics.GetBodyCount(), 1u);
    EXPECT_EQ(FakeOf(crate).info.kind, BodyKind::Static);
    EXPECT_EQ(FakeOf(crate).info.mass, 5.0f);
  }

  TEST_F(PhysicsSimulationTest, CreatesABodyOfItsOwnForAComponentThatWasCopied)
  {
    const Entity crate = Create("crate", At(0.0f, 5.0f, 0.0f), RigidBody{});
    Step();

    // the copy names the body of the entity it was copied from
    const Entity copy = Create("copy", At(3.0f, 5.0f, 0.0f), BodyOf(crate));
    Step();

    EXPECT_EQ(_physics.GetBodyCount(), 2u);
    EXPECT_NE(BodyOf(copy).body, BodyOf(crate).body);

    _store.DestroyEntity(copy);
    EXPECT_TRUE(_physics.HasBody(BodyOf(crate).body));
  }
}
