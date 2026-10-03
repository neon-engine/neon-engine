#include "rope-drawing.hpp"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/joint.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>

namespace
{
  using neon::Entity;
  using neon::Joint;
  using neon::JointKind;
  using neon::MeshData;
  using neon::Renderable;
  using neon::Rope;
  using neon::RopeDrawing;
  using neon::Transform;
  using neon::testing::FakeEntityStore;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;
  using ::testing::IsEmpty;
  using ::testing::SizeIs;

  /// A transform that was placed in the world already.
  Transform Placed(const glm::vec3 &position, const glm::vec3 &scale = glm::vec3(1.0f))
  {
    Transform transform;
    transform.position = position;
    transform.scale = scale;
    transform.world_coordinates = glm::scale(translate(glm::mat4(1.0f), position), scale);
    return transform;
  }

  class RopeDrawingTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    FakeEntityStore _store;
    RopeDrawing _system{_logger};

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Transform>("Transform");
      _store.Register<Renderable>("Renderable");
      _store.Register<Joint>("Joint");
      _system.Register(_store);
      _system.Initialize(_store);
    }

    void TearDown() override
    {
      _store.CleanUp();
    }

    Entity Thing(const std::string &name, const Transform &transform)
    {
      const Entity entity = _store.CreateEntity(name);
      _store.Set(entity, transform);
      return entity;
    }

    Entity Draw(const Rope &rope, const Transform &transform = Placed({0.0f, 0.0f, 0.0f}))
    {
      const Entity entity = Thing("rope", transform);
      _store.Set(entity, rope);
      _store.Set(entity, Renderable{});
      return entity;
    }

    const MeshData &MeshOf(const Entity entity)
    {
      return *_store.Get<Renderable>(entity)->render_info.mesh;
    }

    /// The lowest and the highest a mesh reaches.
    static std::pair<float, float> Heights(const MeshData &mesh)
    {
      float lowest = 1e9f;
      float highest = -1e9f;
      for (const auto &vertex : mesh.vertices)
      {
        lowest = std::min(lowest, vertex.position.y);
        highest = std::max(highest, vertex.position.y);
      }
      return {lowest, highest};
    }
  };

  TEST_F(RopeDrawingTest, DrawsAStraightRopeBetweenTwoPlacesInTheWorld)
  {
    const Entity rope = Draw(Rope{
      .from_anchor = {0.0f, 2.0f, 0.0f},
      .to_anchor = {3.0f, 2.0f, 0.0f},
      .thickness = 0.1f,
      .sides = 8,
      .segments = 4});

    _system.Interpolate(_store, 1.0);

    const auto &mesh = MeshOf(rope);
    // five rings of eight and a seam, two caps
    EXPECT_THAT(mesh.vertices, SizeIs(5u * 9u + 2u * 9u));
    const auto [lowest, highest] = Heights(mesh);
    EXPECT_NEAR(lowest, 1.95f, 1e-4f);
    EXPECT_NEAR(highest, 2.05f, 1e-4f);
    EXPECT_THAT(_logger->Messages(LogLevel::Error), IsEmpty());
  }

  TEST_F(RopeDrawingTest, HangsARopeThatIsLongerThanItsEndsAreApart)
  {
    const Entity rope = Draw(Rope{
      .from_anchor = {0.0f, 2.0f, 0.0f},
      .to_anchor = {3.0f, 2.0f, 0.0f},
      .length = 4.0f});

    _system.Interpolate(_store, 1.0);

    EXPECT_LT(Heights(MeshOf(rope)).first, 1.0f);
  }

  TEST_F(RopeDrawingTest, EndsOnItsEntitiesWhereTheyAreDrawn)
  {
    Thing("beam", Placed({0.0f, 3.0f, 0.0f}, {2.0f, 1.0f, 1.0f}));
    Thing("bob", Placed({5.0f, 1.0f, 0.0f}));
    const Entity rope = Draw(Rope{.from = "beam", .from_anchor = {0.5f, 0.0f, 0.0f}, .to = "bob"});

    _system.Interpolate(_store, 1.0);

    // from the end of the beam, which its scale puts at x = 1, to the bob
    const auto *drawn = _store.Get<Rope>(rope);
    EXPECT_EQ(drawn->drawn_from, glm::vec3(1.0f, 3.0f, 0.0f));
    EXPECT_EQ(drawn->drawn_to, glm::vec3(5.0f, 1.0f, 0.0f));
  }

  TEST_F(RopeDrawingTest, FollowsAnEndThatMovesAndLeavesARopeThatRestsAlone)
  {
    const Entity bob = Thing("bob", Placed({0.0f, 1.0f, 0.0f}));
    const Entity rope = Draw(Rope{.from_anchor = {0.0f, 3.0f, 0.0f}, .to = "bob"});

    _system.Interpolate(_store, 1.0);
    const auto first = _store.Get<Renderable>(rope)->render_info.mesh_version;
    const std::size_t vertices = MeshOf(rope).vertices.size();

    // nothing moved: the mesh is not written again
    _system.Interpolate(_store, 1.0);
    EXPECT_EQ(_store.Get<Renderable>(rope)->render_info.mesh_version, first);

    // the bob swings out: written again, into as many vertices
    *_store.Get<Transform>(bob) = Placed({1.5f, 1.5f, 0.0f});
    _system.Interpolate(_store, 1.0);

    EXPECT_EQ(_store.Get<Renderable>(rope)->render_info.mesh_version, first + 1);
    EXPECT_EQ(MeshOf(rope).vertices.size(), vertices);
    EXPECT_EQ(_store.Get<Rope>(rope)->drawn_to, glm::vec3(1.5f, 1.5f, 0.0f));
  }

  TEST_F(RopeDrawingTest, DrawsTheRopeOfAJoint)
  {
    // a lamp on a rope of two from a beam, hung half way up it
    Thing("beam", Placed({0.0f, 4.0f, 0.0f}));
    const Entity lamp = Thing("lamp", Placed({0.0f, 3.0f, 0.0f}));
    _store.Set(lamp, Joint{
                 .type = JointKind::Rope,
                 .other = "beam",
                 .anchor = {0.0f, 0.2f, 0.0f},
                 .other_anchor = {0.0f, -0.1f, 0.0f},
                 .length = 2.0f});
    const Entity rope = Draw(Rope{.joint = "lamp"});

    _system.Interpolate(_store, 1.0);

    const auto *drawn = _store.Get<Rope>(rope);
    EXPECT_EQ(drawn->drawn_from, glm::vec3(0.0f, 3.2f, 0.0f));
    EXPECT_EQ(drawn->drawn_to, glm::vec3(0.0f, 3.9f, 0.0f));
    EXPECT_EQ(drawn->drawn_length, 2.0f);

    // slack by more than a metre, so it hangs below the lamp's end of it
    EXPECT_LT(Heights(MeshOf(rope)).first, 3.0f);
  }

  TEST_F(RopeDrawingTest, TakesTheLengthOfAJointWithoutOneFromWhereItsEndsAre)
  {
    const Entity bob = Thing("bob", Placed({0.0f, 1.0f, 0.0f}));
    _store.Set(bob, Joint{.type = JointKind::Rope, .other_anchor = {0.0f, 3.0f, 0.0f}});
    const Entity rope = Draw(Rope{.joint = "bob"});

    _system.Interpolate(_store, 1.0);
    EXPECT_EQ(_store.Get<Rope>(rope)->drawn_length, 2.0f);

    // lifted: the rope is still two long, and hangs
    *_store.Get<Transform>(bob) = Placed({0.0f, 2.5f, 0.0f});
    _system.Interpolate(_store, 1.0);
    EXPECT_EQ(_store.Get<Rope>(rope)->drawn_length, 2.0f);
    EXPECT_LT(Heights(MeshOf(rope)).first, 2.2f);
  }

  TEST_F(RopeDrawingTest, IsDrawnWhereItsEndsAreWhereverItsOwnEntityIs)
  {
    const Entity rope = Draw(
      Rope{.from_anchor = {0.0f, 2.0f, 0.0f}, .to_anchor = {3.0f, 2.0f, 0.0f}, .thickness = 0.1f},
      Placed({10.0f, 1.0f, 0.0f}));

    _system.Interpolate(_store, 1.0);

    // on its entity, which is at a height of 1, the rope is at a height of 1
    const auto [lowest, highest] = Heights(MeshOf(rope));
    EXPECT_NEAR(lowest, 0.95f, 1e-4f);
    EXPECT_NEAR(highest, 1.05f, 1e-4f);
  }

  TEST_F(RopeDrawingTest, SaysOnceThatAnEndIsNoEntity)
  {
    Draw(Rope{.to = "nobody"});

    _system.Interpolate(_store, 1.0);
    _system.Interpolate(_store, 1.0);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u);
  }

  TEST_F(RopeDrawingTest, SaysThatTheJointItNamesIsNoRope)
  {
    const Entity door = Thing("door", Placed({0.0f, 1.0f, 0.0f}));
    _store.Set(door, Joint{.type = JointKind::Hinge});
    Draw(Rope{.joint = "door"});

    _system.Interpolate(_store, 1.0);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u);
  }
} // namespace
