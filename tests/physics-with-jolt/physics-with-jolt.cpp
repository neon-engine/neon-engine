// The physics as an application puts it together: a scene that is read from
// YAML, the entity store of Flecs, EntityWorld with the system of the
// physics, and the physics of Jolt. Only the window, the input, and the
// renderer are stand-ins.
//
// The unit tests test each of these by itself. This is where they meet, and
// where it shows that the physics does the same whatever the frame rate is.

#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/data/ryml-document-format.hpp>
#include <neon/physics/jolt-physics-system.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/mock-render-pipeline.hpp>
#include <neon/testing/mock-window-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/character-body.hpp>
#include <neon/world-system/ecs/components/collider.hpp>
#include <neon/world-system/ecs/components/joint.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>
#include <neon/world-system/ecs/components/rigid-body.hpp>
#include <neon/world-system/ecs/components/trigger.hpp>
#include <neon/world-system/ecs/entity-world.hpp>
#include <neon/world-system/ecs/scene-file/scene-file.hpp>
#include <neon/world-system/ecs/systems/physics-simulation.hpp>
#include <neon/world-system/flecs-entity-store.hpp>

namespace
{
  using neon::CharacterBody;
  using neon::Entity;
  using neon::EntityStore;
  using neon::EntitySystem;
  using neon::EntityWorld;
  using neon::Flecs_EntityStore;
  using neon::Jolt_PhysicsSystem;
  using neon::No_Entity;
  using neon::PhysicsContext;
  using neon::PhysicsEvent;
  using neon::PhysicsEventKind;
  using neon::PhysicsSimulation;
  using neon::Renderable;
  using neon::RigidBody;
  using neon::RYML_DocumentFormat;
  using neon::SceneFile;
  using neon::Transform;
  using neon::Trigger;
  using neon::testing::FakeInputContext;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::MockRenderPipeline;
  using neon::testing::MockWindowContext;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::HasSubstr;
  using ::testing::Not;
  using ::testing::NiceMock;
  using ::testing::Return;

  // A floor, a pile of crates that falls over, a ball on a slope that is a
  // mesh, a lift that is moved by its velocity, a character that walks into
  // a wall, and a trigger.
  const std::string scene_text = R"(scene: physics
version: 1

entities:
  - name: floor
    components:
      Transform:
        position: [0, -0.5, 0]
        scale: [100, 1, 100]
      RigidBody:
        kind: static
      Collider:
        shape: box

  - name: slope
    components:
      Transform:
        position: [-8, 1, 0]
        rotation: [0, 0, -25]
        scale: [6, 0.5, 6]
      RigidBody:
        kind: static
      Collider:
        shape: mesh
        model: assets://models/slab.obj
        fit: unit

  - name: ball
    components:
      Transform:
        position: [-9, 4, 0]
      RigidBody:
        mass: 2
        bounce: 0.3
      Collider:
        shape: sphere
        radius: 0.4

  - name: pile
    children:
      - name: lower
        components:
          Transform:
            position: [0, 0.5, 0]
          RigidBody: {}
          Collider:
            shape: box
      - name: middle
        components:
          Transform:
            position: [0.3, 1.5, 0]
            rotation: [0, 20, 0]
          RigidBody: {}
          Collider:
            shape: convex_hull
            model: assets://models/slab.obj
            fit: unit
      - name: upper
        components:
          Transform:
            position: [0.6, 2.5, 0]
          RigidBody:
            mass: 5
          Collider:
            shape: box

  - name: lift
    components:
      Transform:
        position: [6, 0.25, 6]
      RigidBody:
        kind: kinematic
        linear_velocity: [0, 0.5, 0]
      Collider:
        shape: box
        size: [2, 0.5, 2]

  - name: passenger
    components:
      Transform:
        position: [6, 1.5, 6]
      RigidBody:
        can_sleep: false
      Collider:
        shape: box

  - name: wall
    components:
      Transform:
        position: [3.5, 2, -10]
        scale: [1, 4, 20]
      RigidBody:
        kind: static
      Collider:
        shape: box

  - name: player
    components:
      Transform:
        position: [0, 1.1, -10]
      CharacterBody:
        velocity: [2, 0, -1]
      Collider:
        shape: capsule
        radius: 0.5
        height: 2

  - name: gate
    components:
      Transform:
        position: [2, 2.1, -11]
      Trigger: {}
      Collider:
        shape: box
        size: [1, 4, 1]
)";

  // a cube with sides of 2, which is read as one with sides of 1
  const std::string slab =
    "v -1 -1 -1\nv 1 -1 -1\nv 1 1 -1\nv -1 1 -1\nv -1 -1 1\nv 1 -1 1\nv 1 1 1\nv -1 1 1\n"
    "f 1 3 2\nf 1 4 3\nf 5 6 7\nf 5 7 8\nf 1 5 8\nf 1 8 4\nf 2 3 7\nf 2 7 6\nf 4 8 7\nf 4 7 3\nf 1 2 6\nf 1 6 5\n";

  /// What a body is like after a step, down to the last bit.
  struct Snapshot
  {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 velocity{0.0f};

    bool operator==(const Snapshot &other) const
    {
      // by their bits, so that the same is the same and not merely equal
      return std::memcmp(this, &other, sizeof(Snapshot)) == 0;
    }
  };

  /// Writes down where everything is after every step, and what the
  /// physics reported.
  class Recorder final : public EntitySystem
  {
    PhysicsContext *_physics;
    std::vector<std::string> _names;

  public:
    struct Record
    {
      /// One for every step, with one snapshot for every name.
      std::vector<std::vector<Snapshot>> steps;
      std::vector<PhysicsEvent> step_events;
      std::vector<PhysicsEvent> frame_events;
      std::vector<double> step_times;
      std::size_t frames = 0;
    };

    Record *record;

    Recorder(PhysicsContext *physics, const std::vector<std::string> &names, Record *record)
    {
      _physics = physics;
      _names = names;
      this->record = record;
    }

    void Initialize(EntityStore &) override {}

    void FixedUpdate(EntityStore &store, const double fixed_delta_time) override
    {
      record->step_times.push_back(fixed_delta_time);

      std::vector<Snapshot> snapshots;
      for (const auto &name : _names)
      {
        const Entity entity = store.FindEntity(name);
        Snapshot snapshot;

        if (const auto *transform = store.Get<Transform>(entity); transform != nullptr)
        {
          snapshot.position = transform->position;
          snapshot.rotation = {transform->rotation.pitch, transform->rotation.yaw, transform->rotation.roll};
        }
        if (const auto *body = store.Get<RigidBody>(entity); body != nullptr)
        {
          snapshot.velocity = body->linear_velocity;
        }
        if (const auto *character = store.Get<CharacterBody>(entity); character != nullptr)
        {
          snapshot.velocity = character->real_velocity;
        }
        snapshots.push_back(snapshot);
      }
      record->steps.push_back(snapshots);

      const auto &events = _physics->GetStepEvents();
      record->step_events.insert(record->step_events.end(), events.begin(), events.end());
    }

    void Update(EntityStore &, double) override
    {
      record->frames++;

      const auto &events = _physics->GetFrameEvents();
      record->frame_events.insert(record->frame_events.end(), events.begin(), events.end());
    }
  };

  /// Pushes a body in every step, the way a game does it: with a force, in
  /// FixedUpdate.
  class Thruster final : public EntitySystem
  {
    PhysicsContext *_physics;
    std::string _name;
    glm::vec3 _force;

  public:
    Thruster(PhysicsContext *physics, const std::string &name, const glm::vec3 &force)
    {
      _physics = physics;
      _name = name;
      _force = force;
    }

    void Initialize(EntityStore &) override {}

    void Update(EntityStore &, double) override {}

    void FixedUpdate(EntityStore &store, double) override
    {
      if (const auto *body = store.Get<RigidBody>(store.FindEntity(_name)); body != nullptr)
      {
        _physics->AddForce(body->body, _force);
      }
    }
  };

  const std::vector<std::string> watched = {
    "ball", "pile/lower", "pile/middle", "pile/upper", "lift", "passenger", "player"
  };

  /// A world with everything an application has.
  class World
  {
  public:
    std::shared_ptr<RecordingLogger> logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem files{SettingsConfig{}, logger};
    RYML_DocumentFormat yaml;
    Jolt_PhysicsSystem physics{SettingsConfig{}, logger};
    Flecs_EntityStore store{logger};
    NiceMock<MockRenderPipeline> pipeline{logger};
    NiceMock<FakeInputContext> input{logger};
    NiceMock<MockWindowContext> window;
    std::unique_ptr<SceneFile> scene;
    std::unique_ptr<EntityWorld> world;
    Recorder::Record record;

    explicit World(const std::string &text = scene_text, const bool with_thruster = false)
    {
      files.Initialize();
      files.AddNativeFile("/assets/scenes/physics.scene.yml", text);
      files.AddNativeFile("/assets/models/slab.obj", slab);

      physics.Initialize();

      scene = std::make_unique<SceneFile>(&files, &yaml, "assets://scenes/physics.scene.yml", logger);
      world = std::make_unique<EntityWorld>(&store, scene.get(), &pipeline, &input, &window, logger);

      // the systems of a game come before the physics, so that what they
      // ask for in a step is part of that step
      if (with_thruster)
      {
        world->AddSystem(std::make_unique<Thruster>(&physics, "ball", glm::vec3(0.0f, 0.0f, 6.0f)));
      }
      world->AddSystem(std::make_unique<PhysicsSimulation>(&physics, &files, logger));
      world->AddSystem(std::make_unique<Recorder>(&physics, watched, &record));
    }

    ~World()
    {
      // in the order an application shuts down in: the world lets go of
      // its bodies while the physics is there to take them
      world->CleanUp();
      physics.CleanUp();
    }

    /// Runs frames of one length for as long as they add up to `seconds`.
    void Run(const double frames_per_second, const double seconds)
    {
      const auto frames = static_cast<std::size_t>(frames_per_second * seconds + 0.5);

      ON_CALL(window, GetDeltaTime()).WillByDefault(Return(1.0 / frames_per_second));
      for (std::size_t frame = 0; frame < frames; frame++) { world->Update(); }
    }

    void RunFrame(const double seconds)
    {
      ON_CALL(window, GetDeltaTime()).WillByDefault(Return(seconds));
      world->Update();
    }

    [[nodiscard]] Transform &TransformOf(const std::string &name)
    {
      return *store.Get<Transform>(store.FindEntity(name));
    }

    [[nodiscard]] std::size_t Count(
      const PhysicsEventKind kind,
      const std::string &first,
      const std::string &second)
    {
      const Entity one = store.FindEntity(first);
      const Entity other = store.FindEntity(second);

      return static_cast<std::size_t>(std::ranges::count_if(record.step_events, [&](const PhysicsEvent &event)
      {
        return event.kind == kind
               && ((event.first == one && event.second == other) || (event.first == other && event.second == one));
      }));
    }
  };

  // the three ways a body behaves

  TEST(PhysicsWithJolt, LoadsTheSceneWithoutAProblem)
  {
    World world;

    EXPECT_NO_THROW(world.world->Initialize());
    world.Run(60.0, 0.1);

    EXPECT_EQ(world.logger->Count(LogLevel::Error), 0u) << world.logger->Messages(LogLevel::Error);
    EXPECT_EQ(world.logger->Count(LogLevel::Warn), 0u) << world.logger->Messages(LogLevel::Warn);
    EXPECT_EQ(world.physics.GetBodyCount(), 10u);
    EXPECT_EQ(world.physics.GetCharacterCount(), 1u);
  }

  TEST(PhysicsWithJolt, SaysInTheLogWhichBodyEveryEntityGotWithItsShapes)
  {
    World world;
    world.world->Initialize();
    world.Run(60.0, 0.1);

    const auto said = world.logger->Messages(LogLevel::Debug);
    EXPECT_TRUE(world.logger->Contains(LogLevel::Debug, "Created static body for 'floor' with 1 box shape")) << said;
    EXPECT_TRUE(world.logger->Contains(LogLevel::Debug, "Created static body for 'slope' with 1 mesh shape")) << said;
    EXPECT_TRUE(world.logger->Contains(
      LogLevel::Debug, "Created dynamic body for 'pile/middle' with 1 convex hull shape")) << said;
    EXPECT_TRUE(world.logger->Contains(LogLevel::Debug, "Created kinematic body for 'lift' with 1 box shape")) << said;
    EXPECT_TRUE(world.logger->Contains(LogLevel::Debug, "Created trigger for 'gate' with 1 box shape")) << said;
    EXPECT_TRUE(world.logger->Contains(LogLevel::Debug, "Created character for 'player' with 1 capsule shape")) << said;
  }

  TEST(PhysicsWithJolt, SaysInTheLogEveryKindOfShapeOfABodyOfSeveral)
  {
    World world(R"(scene: table
entities:
  - name: table
    components:
      Transform:
        position: [0, 1, 0]
      RigidBody: Default
    children:
      - name: top
        components:
          Transform:
            position: [0, 0.9, 0]
          Collider:
            shape: box
            size: [2, 0.2, 2]
      - name: leg
        components:
          Transform:
            position: [-0.9, 0.4, -0.9]
          Collider:
            shape: box
            size: [0.2, 0.8, 0.2]
      - name: knob
        components:
          Transform:
            position: [0.9, 1.1, 0.9]
          Collider:
            shape: sphere
            radius: 0.1
)");
    world.world->Initialize();
    world.Run(60.0, 0.1);

    EXPECT_TRUE(world.logger->Contains(
      LogLevel::Debug, "Created dynamic body for 'table' with 2 box shapes and 1 sphere shape"))
      << world.logger->Messages(LogLevel::Debug);
  }

  TEST(PhysicsWithJolt, LetsDynamicBodiesCollideAndReact)
  {
    World world;
    world.world->Initialize();

    world.Run(60.0, 6.0);

    // the pile leaned, and fell over: the crate that was on top lies on the
    // floor
    EXPECT_NEAR(world.TransformOf("pile/lower").position.y, 0.5f, 0.05f);
    EXPECT_LT(world.TransformOf("pile/upper").position.y, 1.0f);
    EXPECT_GT(world.TransformOf("pile/upper").position.x, 0.9f);

    // the ball rolled down the slope and away from it
    EXPECT_GT(world.TransformOf("ball").position.x, -5.0f);
    EXPECT_NEAR(world.TransformOf("ball").position.y, 0.4f, 0.05f);

    EXPECT_GE(world.Count(PhysicsEventKind::Began, "slope", "ball"), 1u);
    EXPECT_GE(world.Count(PhysicsEventKind::Began, "floor", "pile/upper"), 1u);
  }

  TEST(PhysicsWithJolt, LetsACharacterCollideAndStopWithoutBeingMoved)
  {
    World world;
    world.world->Initialize();

    world.Run(60.0, 4.0);

    const auto &player = world.TransformOf("player");
    const auto *character = world.store.Get<CharacterBody>(world.store.FindEntity("player"));

    // the wall begins at 3, and the capsule is half a unit wide
    EXPECT_NEAR(player.position.x, 2.5f, 0.05f);
    // it slid along the wall by what its velocity says
    EXPECT_NEAR(player.position.z, -14.0f, 0.2f);
    EXPECT_NEAR(player.position.y, 1.0f, 0.05f);
    EXPECT_TRUE(character->on_floor);
    EXPECT_TRUE(character->on_wall);
    EXPECT_EQ(character->floor, world.store.FindEntity("floor"));
    EXPECT_NEAR(character->real_velocity.x, 0.0f, 0.05f);
    EXPECT_NEAR(character->real_velocity.z, -1.0f, 0.05f);

    // what it asked for is kept
    EXPECT_EQ(character->velocity, glm::vec3(2.0f, 0.0f, -1.0f));

    // the wall is where it was
    EXPECT_EQ(world.TransformOf("wall").position, glm::vec3(3.5f, 2.0f, -10.0f));
  }

  TEST(PhysicsWithJolt, MovesAKinematicBodyThatCarriesWhatIsOnIt)
  {
    World world;
    world.world->Initialize();

    world.Run(60.0, 4.0);

    // half a unit per second for four seconds, whatever lies on it
    EXPECT_NEAR(world.TransformOf("lift").position.y, 2.25f, 0.01f);
    EXPECT_NEAR(world.TransformOf("lift").position.x, 6.0f, 1e-4f);
    EXPECT_NEAR(world.TransformOf("passenger").position.y, 3.0f, 0.05f);
  }

  TEST(PhysicsWithJolt, ReportsWhatOverlapsATriggerAndStopsNothing)
  {
    World world;
    world.world->Initialize();

    // the player walks through the gate on its way to the wall
    std::uint32_t most_inside = 0;
    for (int frame = 0; frame < 240; frame++)
    {
      world.RunFrame(1.0 / 60.0);
      most_inside = std::max(most_inside, world.store.Get<Trigger>(world.store.FindEntity("gate"))->inside);
    }

    EXPECT_EQ(world.Count(PhysicsEventKind::Began, "gate", "player"), 1u);
    EXPECT_EQ(world.Count(PhysicsEventKind::Ended, "gate", "player"), 1u);
    EXPECT_EQ(most_inside, 1u);
    EXPECT_EQ(world.store.Get<Trigger>(world.store.FindEntity("gate"))->inside, 0u);

    for (const auto &event : world.record.step_events)
    {
      if (event.first != world.store.FindEntity("gate")) { continue; }
      EXPECT_TRUE(event.trigger);
    }

    // the gate is where it was
    EXPECT_EQ(world.TransformOf("gate").position, glm::vec3(2.0f, 2.1f, -11.0f));

    // and the player went on as if the gate were not there
    World without;
    without.files.AddNativeFile(
      "/assets/scenes/physics.scene.yml",
      scene_text.substr(0, scene_text.find("  - name: gate")));
    without.world->Initialize();
    without.Run(60.0, 4.0);

    const auto player = static_cast<std::size_t>(
      std::ranges::find(watched, std::string("player")) - watched.begin());
    ASSERT_EQ(world.record.steps.size(), without.record.steps.size());
    for (std::size_t step = 0; step < world.record.steps.size(); step++)
    {
      EXPECT_EQ(world.record.steps[step][player], without.record.steps[step][player]) << "step " << step;
    }
  }

  // queries

  TEST(PhysicsWithJolt, FindsWithACastShapeWhatAGameWouldPutSomewhere)
  {
    World world;
    world.world->Initialize();
    world.Run(60.0, 2.0);

    // a crate that is let down over the lift lands on the passenger, and
    // one let down beside everything lands on the floor
    neon::ShapeInfo crate;
    crate.kind = neon::ShapeKind::Box;
    const glm::quat upright{1.0f, 0.0f, 0.0f, 0.0f};

    neon::ShapeCastHit hit;
    const auto let_down = [&](const float x, const float z, const float to)
    {
      return world.physics.CastShape(crate, {x, 10.0f, z}, upright, {x, to, z}, neon::QueryFilter{}, hit);
    };

    ASSERT_TRUE(let_down(6.0f, 6.0f, 0.0f));
    EXPECT_EQ(hit.entity, world.store.FindEntity("passenger"));
    EXPECT_GT(hit.point.y, 1.0f);
    EXPECT_NEAR(hit.normal.y, 1.0f, 1e-3f);

    ASSERT_TRUE(let_down(20.0f, 20.0f, 0.0f));
    EXPECT_EQ(hit.entity, world.store.FindEntity("floor"));
    EXPECT_NEAR(hit.point.y, 0.0f, 1e-2f);
    EXPECT_NEAR(hit.fraction, 0.95f, 1e-2f);

    // not far enough down to reach anything
    EXPECT_FALSE(let_down(60.0f, 60.0f, 5.0f));
  }

  // shapes that change while a body lives

  TEST(PhysicsWithJolt, LiftsACrateWhoseColliderGrowsWhileItRests)
  {
    World world;
    world.world->Initialize();
    world.Run(60.0, 3.0);

    const Entity passenger = world.store.FindEntity("passenger");
    const auto body = world.store.Get<RigidBody>(passenger)->body;
    const float before = world.TransformOf("passenger").position.y;

    // three times as high, with the same middle, so that it stands in the
    // lift and is lifted out
    world.store.Get<neon::Collider>(passenger)->size = {1.0f, 3.0f, 1.0f};
    world.Run(60.0, 2.0);

    EXPECT_GT(world.TransformOf("passenger").position.y, before + 0.8f);
    EXPECT_EQ(world.store.Get<RigidBody>(passenger)->body, body);
    EXPECT_EQ(world.physics.GetBodyCount(), 10u);
    EXPECT_EQ(world.logger->Count(LogLevel::Error), 0u) << world.logger->Messages(LogLevel::Error);
  }

  // joints

  // A door on a hinge in a frame, which a ball rolls into, and a pendulum
  // on a point joint to the world. The door hangs 2 cm above the floor, as
  // that of joints.scene.yml does (#206), so that the floor does not brake
  // its swing.
  const std::string joints_scene = R"(scene: joints
version: 1

entities:
  - name: floor
    components:
      Transform:
        position: [0, -0.5, 0]
        scale: [100, 1, 100]
      RigidBody:
        kind: static
      Collider:
        shape: box

  - name: house
    children:
      - name: frame
        components:
          Transform:
            position: [0, 1, 0]
            scale: [0.2, 2, 0.2]
          RigidBody:
            kind: static
          Collider:
            shape: box

  - name: door
    components:
      Transform:
        position: [1.05, 1.02, 0]
      RigidBody:
        mass: 5
      Collider:
        shape: box
        size: [1.6, 2, 0.1]
      Joint:
        type: hinge
        other: house/frame
        anchor: [-0.85, 0, 0]
        axis: [0, 1, 0]
        limits: [-90, 90]

  - name: ball
    components:
      Transform:
        position: [1.2, 0.4, 4]
      RigidBody:
        mass: 10
        linear_velocity: [0, 0, -6]
      Collider:
        shape: sphere
        radius: 0.4

  - name: hook
    components:
      Transform:
        position: [6, 5, 0]
        scale: 0.2
      RigidBody:
        kind: static
      Collider:
        shape: box

  - name: bob
    components:
      Transform:
        position: [7.5, 3.5, 0]
      RigidBody:
        mass: 2
      Collider:
        shape: sphere
        radius: 0.3
      Joint:
        type: point
        anchor: [-1.5, 1.5, 0]
)";

  TEST(PhysicsWithJolt, LetsABallSwingADoorOpenOnItsHinge)
  {
    World world(joints_scene);
    world.world->Initialize();

    // half a swing of the pendulum takes a second and a half
    world.Run(60.0, 1.5);
    EXPECT_EQ(world.logger->Count(LogLevel::Error), 0u) << world.logger->Messages(LogLevel::Error);
    EXPECT_EQ(world.physics.GetJointCount(), 2u);

    // the pendulum swung to the other side, and hangs from the hook
    const auto &bob = world.TransformOf("bob");
    EXPECT_LT(bob.position.x, 6.0f);
    EXPECT_NEAR(length(bob.position - glm::vec3(6.0f, 5.0f, 0.0f)), std::sqrt(4.5f), 0.1f);

    // The ball reached the door a little before a second, and the door,
    // which hangs above the floor as that of joints.scene.yml does, swung
    // wide open at once, up to its limit and no further
    const auto &door = world.TransformOf("door");
    EXPECT_GT(std::abs(door.rotation.yaw), 75.0f);
    EXPECT_LT(std::abs(door.rotation.yaw), 91.0f);
    EXPECT_NEAR(door.rotation.pitch, 0.0f, 2.0f);
    EXPECT_NEAR(door.rotation.roll, 0.0f, 2.0f);

    // the hinge of the door is still at the frame, and the door hangs there
    // without sinking to the floor
    const auto hinge = door.position + door.rotation.GetQuaternion() * glm::vec3(-0.85f, 0.0f, 0.0f);
    EXPECT_NEAR(hinge.x, 0.2f, 0.1f);
    EXPECT_NEAR(hinge.z, 0.0f, 0.1f);
    EXPECT_NEAR(door.position.y, 1.02f, 0.01f);

    const auto *joint = world.store.Get<neon::Joint>(world.store.FindEntity("door"));
    EXPECT_NE(joint->joint, neon::No_Joint);
    EXPECT_FALSE(joint->failed);
  }

  TEST(PhysicsWithJolt, SaysWhatAJointNamesThatIsNotThereAndGoesOn)
  {
    World world(R"(scene: wrong
entities:
  - name: crate
    components:
      Transform:
        position: [0, 5, 0]
      RigidBody: {}
      Collider: {}
      Joint:
        type: hinge
        other: house/frame
  - name: sign
    components:
      Transform: {}
      Joint: {}
)");
    world.world->Initialize();
    world.Run(60.0, 1.0);

    EXPECT_EQ(world.logger->Count(LogLevel::Error), 2u) << world.logger->Messages(LogLevel::Error);
    EXPECT_TRUE(world.logger->Contains(
      LogLevel::Error,
      "The Joint of entity 'crate' cannot be created: it names 'house/frame', which is no entity"));
    EXPECT_TRUE(world.logger->Contains(
      LogLevel::Error,
      "The Joint of entity 'sign' cannot be created: the entity has no RigidBody. Only a RigidBody can be "
      "joined to something"));

    // the crate falls all the same
    EXPECT_EQ(world.physics.GetJointCount(), 0u);
    EXPECT_LT(world.TransformOf("crate").position.y, 1.0f);
    EXPECT_TRUE(world.store.Get<neon::Joint>(world.store.FindEntity("crate"))->failed);
  }

  // events

  TEST(PhysicsWithJolt, HandsEveryEventToAFrameOnce)
  {
    // frames that take several steps, one, and none
    for (const double frames_per_second : {20.0, 60.0, 144.0, 1000.0})
    {
      World world;
      world.world->Initialize();

      world.Run(frames_per_second, 3.0);

      ASSERT_FALSE(world.record.step_events.empty());
      ASSERT_EQ(world.record.frame_events.size(), world.record.step_events.size())
        << frames_per_second << " frames per second";

      for (std::size_t i = 0; i < world.record.step_events.size(); i++)
      {
        EXPECT_EQ(world.record.frame_events[i].kind, world.record.step_events[i].kind);
        EXPECT_EQ(world.record.frame_events[i].first, world.record.step_events[i].first);
        EXPECT_EQ(world.record.frame_events[i].second, world.record.step_events[i].second);
      }
    }
  }

  // the frame rate

  TEST(PhysicsWithJolt, TakesTheStepsOfTheTimeThatPassedAtEveryFrameRate)
  {
    for (const double frames_per_second : {30.0, 60.0, 144.0, 1000.0})
    {
      World world;
      world.world->Initialize();

      world.Run(frames_per_second, 2.0);

      EXPECT_EQ(world.record.steps.size(), 120u) << frames_per_second << " frames per second";
      EXPECT_EQ(world.record.frames, static_cast<std::size_t>(2.0 * frames_per_second));
      EXPECT_EQ(world.world->GetFixedClock().GetStepCount(), 120u);
      EXPECT_EQ(world.world->GetFixedClock().GetHeldBackCount(), 0u);

      for (const double seconds : world.record.step_times) { EXPECT_EQ(seconds, 1.0 / 60.0); }
    }
  }

  TEST(PhysicsWithJolt, IsInTheSameStateAfterTheSameStepsAtEveryFrameRate)
  {
    World at_60(scene_text, true);
    at_60.world->Initialize();
    at_60.Run(60.0, 4.0);
    ASSERT_EQ(at_60.record.steps.size(), 240u);

    // something happened, and the force that pushes the ball was felt
    EXPECT_NE(at_60.record.steps[239][0].position, at_60.record.steps[0][0].position);
    EXPECT_GT(at_60.record.steps[239][0].position.z, 1.0f);

    for (const double frames_per_second : {30.0, 144.0, 1000.0})
    {
      World world(scene_text, true);
      world.world->Initialize();
      world.Run(frames_per_second, 4.0);

      ASSERT_EQ(world.record.steps.size(), 240u) << frames_per_second << " frames per second";

      for (std::size_t step = 0; step < 240; step++)
      {
        for (std::size_t body = 0; body < watched.size(); body++)
        {
          // the same down to the last bit, and not merely close
          ASSERT_EQ(world.record.steps[step][body], at_60.record.steps[step][body])
            << watched[body] << " after step " << step + 1 << " at " << frames_per_second
            << " frames per second";
        }
      }

      ASSERT_EQ(world.record.step_events.size(), at_60.record.step_events.size());
      for (std::size_t i = 0; i < world.record.step_events.size(); i++)
      {
        EXPECT_EQ(world.record.step_events[i].kind, at_60.record.step_events[i].kind);
        EXPECT_EQ(world.record.step_events[i].first, at_60.record.step_events[i].first);
        EXPECT_EQ(world.record.step_events[i].second, at_60.record.step_events[i].second);
        EXPECT_EQ(world.record.step_events[i].point, at_60.record.step_events[i].point);
      }
    }
  }

  TEST(PhysicsWithJolt, IsInTheSameStateAfterTheSameStepsWhenFramesDiffer)
  {
    World at_60(scene_text, true);
    at_60.world->Initialize();
    at_60.Run(60.0, 4.0);

    World world(scene_text, true);
    world.world->Initialize();

    // frames as a machine under load has them: short ones, long ones, and
    // one that hangs
    const std::vector<double> frames = {0.004, 0.031, 0.0167, 0.002, 0.05, 0.0083, 0.011, 0.0274, 0.12, 0.0001};
    double passed = 0.0;
    for (std::size_t frame = 0; passed < 3.5; frame++)
    {
      const double seconds = frames[frame % frames.size()];
      passed += seconds;
      world.RunFrame(seconds);
    }

    // no frame was long enough to give up steps
    ASSERT_EQ(world.world->GetFixedClock().GetHeldBackCount(), 0u);
    EXPECT_EQ(world.record.steps.size(), static_cast<std::size_t>(passed * 60.0));
    ASSERT_GT(world.record.steps.size(), 200u);
    ASSERT_LE(world.record.steps.size(), at_60.record.steps.size());

    for (std::size_t step = 0; step < world.record.steps.size(); step++)
    {
      for (std::size_t body = 0; body < watched.size(); body++)
      {
        ASSERT_EQ(world.record.steps[step][body], at_60.record.steps[step][body])
          << watched[body] << " after step " << step + 1;
      }
    }
  }

  TEST(PhysicsWithJolt, TakesNoMoreThanTheMostStepsForAFrameThatTookLong)
  {
    World at_60;
    at_60.world->Initialize();
    at_60.Run(60.0, 1.0);

    World world;
    world.world->Initialize();

    // a frame of ten seconds, as after the machine was asleep
    world.RunFrame(10.0);

    EXPECT_EQ(world.record.steps.size(), 8u);
    EXPECT_EQ(world.world->GetFixedClock().GetHeldBackCount(), 592u);
    for (const double seconds : world.record.step_times) { EXPECT_EQ(seconds, 1.0 / 60.0); }

    // The world is behind the clock on the wall, and where it is, it is
    // right: the same as after 8 steps of any other run.
    for (std::size_t body = 0; body < watched.size(); body++)
    {
      EXPECT_EQ(world.record.steps[7][body], at_60.record.steps[7][body]) << watched[body];
    }

    // the frames after it do not make up for it
    world.Run(60.0, 1.0);
    EXPECT_EQ(world.record.steps.size(), 68u);
    for (std::size_t body = 0; body < watched.size(); body++)
    {
      EXPECT_EQ(world.record.steps[59][body], at_60.record.steps[59][body]) << watched[body];
    }
  }

  TEST(PhysicsWithJolt, StepsAsOftenAsTheClockIsSetTo)
  {
    World world;
    world.world->GetFixedClock().SetStepsPerSecond(120.0);
    world.world->Initialize();

    world.Run(60.0, 1.0);

    EXPECT_EQ(world.record.steps.size(), 120u);
    for (const double seconds : world.record.step_times) { EXPECT_EQ(seconds, 1.0 / 120.0); }

    // a second of falling is a second of falling
    EXPECT_NEAR(world.store.Get<RigidBody>(world.store.FindEntity("pile/lower"))->linear_velocity.y, 0.0f, 0.05f);
    EXPECT_NEAR(world.TransformOf("lift").position.y, 0.75f, 0.01f);
  }

  // what is drawn

  /// Where a crate that falls is drawn in every frame.
  std::vector<float> DrawnHeights(World &world, const double frames_per_second, const double seconds)
  {
    std::vector<float> heights;

    ON_CALL(world.pipeline, CreateRenderObject(_)).WillByDefault(Return(1));
    ON_CALL(world.pipeline, EnqueueForRendering(_, _)).WillByDefault([&](int, const Transform &transform)
    {
      heights.push_back(transform.world_coordinates[3].y);
    });

    world.Run(frames_per_second, seconds);
    return heights;
  }

  const std::string falling_crate = R"(scene: falling
entities:
  - name: crate
    components:
      Transform:
        position: [0, 100, 0]
      Renderable:
        model: assets://models/slab.obj
        fit: unit
        shader: engine://shaders/basic-lit
      RigidBody:
        linear_damping: 0
      Collider:
        shape: box
    children:
      - name: label
        components:
          Transform:
            position: [0, 1, 0]
)";

  TEST(PhysicsWithJolt, DrawsWhatMovesInEveryFrameWhenFramesAreShorterThanSteps)
  {
    World world(falling_crate);
    world.world->Initialize();

    const auto heights = DrawnHeights(world, 1000.0, 0.5);

    ASSERT_EQ(heights.size(), 500u);
    ASSERT_EQ(world.record.steps.size(), 30u);

    // from the second step on, it is drawn lower in every frame, and not
    // only in the 30 frames that took a step
    std::size_t lower = 0;
    for (std::size_t frame = 40; frame < heights.size(); frame++)
    {
      EXPECT_LE(heights[frame], heights[frame - 1]) << "frame " << frame;
      if (heights[frame] < heights[frame - 1]) { lower++; }
    }
    EXPECT_GT(lower, 440u);

    // what is drawn is never ahead of where the body is
    EXPECT_GE(heights.back(), world.TransformOf("crate").position.y);
  }

  TEST(PhysicsWithJolt, DrawsBetweenTheLastTwoStepsAndLeavesTheBodyWhereItIs)
  {
    World world(falling_crate);
    world.world->Initialize();

    ON_CALL(world.pipeline, CreateRenderObject(_)).WillByDefault(Return(1));

    // ten steps, and a quarter of one
    world.Run(60.0, 10.0 / 60.0);
    const float after_nine = [&]
    {
      World other(falling_crate);
      other.world->Initialize();
      other.Run(60.0, 9.0 / 60.0);
      return other.TransformOf("crate").position.y;
    }();
    const float after_ten = world.TransformOf("crate").position.y;
    ASSERT_LT(after_ten, after_nine);

    float drawn = 0.0f;
    ON_CALL(world.pipeline, EnqueueForRendering(_, _)).WillByDefault([&](int, const Transform &transform)
    {
      drawn = transform.world_coordinates[3].y;
    });
    world.RunFrame(0.25 / 60.0);

    ASSERT_EQ(world.record.steps.size(), 10u);
    EXPECT_NEAR(drawn, after_nine + 0.25f * (after_ten - after_nine), 1e-4f);

    // the body is where the last step put it, for the game and for the
    // step that follows
    EXPECT_EQ(world.TransformOf("crate").position.y, after_ten);

    // what is below the crate is drawn with it
    EXPECT_NEAR(world.TransformOf("crate/label").world_coordinates[3].y, drawn + 1.0f, 1e-4f);
    EXPECT_EQ(world.TransformOf("crate/label").position, glm::vec3(0.0f, 1.0f, 0.0f));
  }

  TEST(PhysicsWithJolt, DrawsTheSameWayAtFewFramesAsAtMany)
  {
    // at 30 frames per second two steps are taken in a frame, and what is
    // drawn is where the last of them led to
    World world(falling_crate);
    world.world->Initialize();
    const auto heights = DrawnHeights(world, 30.0, 1.0);

    ASSERT_EQ(heights.size(), 30u);
    for (std::size_t frame = 2; frame < heights.size(); frame++)
    {
      EXPECT_LT(heights[frame], heights[frame - 1]) << "frame " << frame;
    }

    // a second of falling, less the step that what is drawn lies behind
    EXPECT_NEAR(heights.back(), 100.0f - 0.5f * 9.81f, 0.4f);
  }

  // releasing

  TEST(PhysicsWithJolt, DestroysTheBodyOfAnEntityThatIsDestroyed)
  {
    World world;
    world.world->Initialize();
    world.Run(60.0, 0.5);
    ASSERT_EQ(world.physics.GetBodyCount(), 10u);

    // three crates, the player, and the ball
    world.store.DestroyEntity(world.store.FindEntity("pile"));
    world.store.DestroyEntity(world.store.FindEntity("player"));
    world.store.Remove<RigidBody>(world.store.FindEntity("ball"));

    EXPECT_EQ(world.physics.GetBodyCount(), 6u);
    EXPECT_EQ(world.physics.GetCharacterCount(), 0u);

    // and the world goes on
    EXPECT_NO_THROW(world.Run(60.0, 0.5));
    EXPECT_EQ(world.logger->Count(LogLevel::Error), 0u) << world.logger->Messages(LogLevel::Error);
  }

  TEST(PhysicsWithJolt, DestroysEveryBodyWhenTheWorldIsCleanedUp)
  {
    World world;
    world.world->Initialize();
    world.Run(60.0, 0.5);

    world.world->CleanUp();

    EXPECT_EQ(world.physics.GetBodyCount(), 0u);
    EXPECT_EQ(world.physics.GetCharacterCount(), 0u);
  }

  TEST(PhysicsWithJolt, SurvivesThePhysicsBeingCleanedUpFirst)
  {
    World world;
    world.world->Initialize();
    world.Run(60.0, 0.5);

    world.physics.CleanUp();

    EXPECT_NO_THROW(world.world->CleanUp());
  }

  // what cannot be

  TEST(PhysicsWithJolt, SaysThatADynamicBodyCannotHaveAMeshAndGoesOn)
  {
    World world(R"(scene: wrong
entities:
  - name: level
    children:
      - name: rock
        components:
          Transform:
            position: [0, 5, 0]
          RigidBody:
            kind: dynamic
          Collider:
            shape: mesh
            model: assets://models/slab.obj
            fit: unit
  - name: crate
    components:
      Transform:
        position: [3, 5, 0]
      RigidBody: {}
      Collider: {}
)");
    world.world->Initialize();

    world.Run(60.0, 1.0);

    EXPECT_EQ(world.logger->Count(LogLevel::Error), 1u) << world.logger->Messages(LogLevel::Error);
    EXPECT_TRUE(world.logger->Contains(
      LogLevel::Error,
      "The RigidBody of entity 'level/rock' cannot be created: a dynamic body cannot have a mesh. A mesh is "
      "a surface without an inside, so it has no mass. Make the body static or kinematic, or give it a "
      "convex hull")) << world.logger->Messages(LogLevel::Error);

    const auto *rock = world.store.Get<RigidBody>(world.store.FindEntity("level/rock"));
    EXPECT_TRUE(rock->failed);
    EXPECT_EQ(rock->body, neon::No_Body);
    EXPECT_EQ(world.TransformOf("level/rock").position, glm::vec3(0.0f, 5.0f, 0.0f));

    // everything else works
    EXPECT_EQ(world.physics.GetBodyCount(), 1u);
    EXPECT_LT(world.TransformOf("crate").position.y, 5.0f);
  }

  TEST(PhysicsWithJolt, SaysWhatIsWrongInASceneWithTheLine)
  {
    World world(R"(scene: wrong
entities:
  - name: crate
    components:
      Transform: {}
      RigidBody:
        kind: rigid
        mass: 0
        bouncy: 1
      Collider:
        shape: sphere
        size: [1, 1, 1]
        radius: -1
  - name: gate
    components:
      Trigger:
        mask: 40
      Collider:
        shape: convex_hull
)");

    // a scene with a mistake is said, and the world goes on with the rest
    world.world->Initialize();

    const auto said = world.logger->Messages(LogLevel::Error);
    const std::string file = "assets://scenes/physics.scene.yml";

    EXPECT_THAT(said, HasSubstr(
                  file + ":7: 'kind' of RigidBody of entity 'crate' is 'rigid', where one of these was "
                  "expected: static, kinematic, dynamic"));
    EXPECT_THAT(said, HasSubstr(
                  file + ":8: 'mass' of RigidBody of entity 'crate' is 0, where a number above 0 was expected"));
    EXPECT_THAT(said, HasSubstr(file + ":9: 'bouncy' is not known to RigidBody of entity 'crate'. Known are: "));
    EXPECT_THAT(said, HasSubstr(
                  file + ":13: 'radius' of Collider of entity 'crate' is -1, where a number above 0 was "
                  "expected"));
    EXPECT_THAT(said, HasSubstr(
                  file + ":12: 'size' is not known to Collider of entity 'crate'. Known are: shape, radius, "
                  "offset, rotation"));
    EXPECT_THAT(said, HasSubstr(
                  file + ":17: 'mask' of Trigger of entity 'gate' holds 40, where a layer from 1 to 32 was "
                  "expected"));
    // a hull without a model is no mistake of the recipe since a Geometry
    // may shape it; the physics says so when the body is made, not here
    EXPECT_THAT(said, Not(HasSubstr("needs a 'model'")));
    // the six problems, the line that counts them, and the world saying it goes on
    EXPECT_EQ(world.logger->Count(LogLevel::Error), 8u) << said;
  }

  // a scene that is written

  TEST(PhysicsWithJolt, WritesTheSceneAsItWasRead)
  {
    World world;
    world.world->Initialize();

    // before the first step, everything is where the file put it
    ASSERT_TRUE(world.scene->Save(world.store, "physics", "user://saved.scene.yml"));
    std::string saved;
    ASSERT_TRUE(world.files.ReadText("user://saved.scene.yml", saved));

    EXPECT_THAT(saved, HasSubstr("RigidBody:\n        kind: static"));
    EXPECT_THAT(saved, HasSubstr("shape: mesh\n        model: assets://models/slab.obj"));
    EXPECT_THAT(saved, HasSubstr("linear_velocity: [0, 0.5, 0]"));
    EXPECT_THAT(saved, HasSubstr("CharacterBody:\n        velocity: [2, 0, -1]"));
    EXPECT_THAT(saved, HasSubstr("Trigger: Default"));
    EXPECT_THAT(saved, HasSubstr("size: [2, 0.5, 2]"));

    // nothing of what the engine fills in
    EXPECT_THAT(saved, ::testing::Not(HasSubstr("body:")));
    EXPECT_THAT(saved, ::testing::Not(HasSubstr("failed")));
    EXPECT_THAT(saved, ::testing::Not(HasSubstr("inside")));
    EXPECT_THAT(saved, ::testing::Not(HasSubstr("on_floor")));

    // read and written again, it is the same text
    World again(saved);
    again.world->Initialize();
    ASSERT_TRUE(again.scene->Save(again.store, "physics", "user://saved.scene.yml"));
    std::string saved_again;
    ASSERT_TRUE(again.files.ReadText("user://saved.scene.yml", saved_again));

    EXPECT_EQ(saved_again, saved);

    // and it behaves the same
    world.Run(60.0, 2.0);
    again.Run(60.0, 2.0);
    ASSERT_EQ(world.record.steps.size(), again.record.steps.size());
    EXPECT_EQ(world.record.steps.back(), again.record.steps.back());
  }

  TEST(PhysicsWithJolt, WritesWhereTheSimulationPutWhatMoves)
  {
    World world;
    world.world->Initialize();
    world.Run(60.0, 6.0);

    ASSERT_TRUE(world.scene->Save(world.store, "physics", "user://saved.scene.yml"));
    std::string saved;
    ASSERT_TRUE(world.files.ReadText("user://saved.scene.yml", saved));

    World again(saved);
    EXPECT_NO_THROW(again.world->Initialize());

    const auto was = world.TransformOf("pile/upper");
    const auto is = again.TransformOf("pile/upper");
    EXPECT_NEAR(is.position.x, was.position.x, 1e-4f);
    EXPECT_NEAR(is.position.y, was.position.y, 1e-4f);
    EXPECT_NEAR(std::abs(dot(is.rotation.GetQuaternion(), was.rotation.GetQuaternion())), 1.0f, 1e-4f);
  }
}
