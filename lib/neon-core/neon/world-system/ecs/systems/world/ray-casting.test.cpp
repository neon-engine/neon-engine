#include "ray-casting.hpp"

#include <string>

#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/fake-physics-context.hpp>
#include <neon/world-system/ecs/components/ray-cast.hpp>

namespace
{
  using neon::Entity;
  using neon::QueryFilter;
  using neon::Ray;
  using neon::RayCast;
  using neon::RayCasting;
  using neon::RayHit;
  using neon::Transform;
  using neon::testing::FakeEntityStore;
  using neon::testing::FakePhysicsContext;

  constexpr float tolerance = 1e-4f;

  /// A transform that was placed in the world: at a position, turned by a
  /// yaw in degrees.
  Transform Placed(const glm::vec3 &position, const float yaw = 0.0f)
  {
    Transform transform;
    transform.position = position;
    transform.rotation.yaw = yaw;
    transform.world_coordinates = translate(glm::mat4{1.0f}, position) * mat4_cast(transform.rotation.GetQuaternion());
    return transform;
  }

  /// A physics whose ray cast answers what the test says, and keeps what
  /// it was asked.
  class ScriptedPhysics final : public FakePhysicsContext
  {
  public:
    bool hits = false;
    RayHit answer;
    int asked = 0;
    Ray last_ray;
    QueryFilter last_filter;

    bool CastRay(const Ray &ray, const QueryFilter &filter, RayHit &hit) override
    {
      asked++;
      last_ray = ray;
      last_filter = filter;
      if (!hits) { return false; }
      hit = answer;
      return true;
    }
  };

  class RayCastingTest : public ::testing::Test
  {
  protected:
    FakeEntityStore _store;
    ScriptedPhysics _physics;
    RayCasting _system{&_physics};
    int _eyes = 0;

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Transform>("Transform");
      _store.Register<RayCast>("RayCast");
      _system.Initialize(_store);
    }

    void TearDown() override
    {
      _store.CleanUp();
    }

    /// An entity with a ray at a place, looking along -Z unless turned.
    Entity CreateRay(const glm::vec3 &position, const float yaw = 0.0f, const RayCast &ray = RayCast{})
    {
      const Entity entity = _store.CreateEntity("eye " + std::to_string(++_eyes));
      _store.Set(entity, Placed(position, yaw));
      _store.Set(entity, ray);
      return entity;
    }

    /// The physics answers with a hit on `entity` at `point`.
    void Hits(const Entity entity, const glm::vec3 &point, const float distance)
    {
      _physics.hits = true;
      _physics.answer = RayHit{.entity = entity, .point = point, .normal = {0.0f, 0.0f, 1.0f}, .distance = distance};
    }

    const RayCast &RayOf(const Entity entity)
    {
      return *_store.Get<RayCast>(entity);
    }

    void Frame()
    {
      _system.Update(_store, 0.016);
    }
  };

  TEST_F(RayCastingTest, AsksThePhysicsFromWhereTheEntityIsAlongWhereItLooksAsFarAsItsLength)
  {
    CreateRay({1.0f, 2.0f, 3.0f}, 90.0f, RayCast{.length = 4.0f});

    Frame();

    ASSERT_EQ(_physics.asked, 1);
    EXPECT_NEAR(_physics.last_ray.origin.x, 1.0f, tolerance);
    EXPECT_NEAR(_physics.last_ray.origin.y, 2.0f, tolerance);
    EXPECT_NEAR(_physics.last_ray.origin.z, 3.0f, tolerance);
    // turned 90 degrees to the left, -Z becomes -X
    EXPECT_NEAR(_physics.last_ray.direction.x, -1.0f, tolerance);
    EXPECT_NEAR(_physics.last_ray.direction.z, 0.0f, tolerance);
    EXPECT_NEAR(_physics.last_ray.distance, 4.0f, tolerance);
  }

  TEST_F(RayCastingTest, ALengthOfZeroIsAsFarAsThereIsAnything)
  {
    CreateRay({0.0f, 0.0f, 0.0f}, 0.0f, RayCast{.length = 0.0f});

    Frame();

    EXPECT_EQ(_physics.last_ray.distance, Ray{}.distance);
  }

  TEST_F(RayCastingTest, WritesWhatIsHitWhereAndHowFar)
  {
    const Entity eye = CreateRay({0.0f, 0.0f, 2.0f});
    const Entity screen = _store.CreateEntity("screen");
    Hits(screen, {0.0f, 0.5f, 0.0f}, 2.0f);

    Frame();

    EXPECT_TRUE(RayOf(eye).is_hitting);
    EXPECT_EQ(RayOf(eye).hit, screen);
    EXPECT_NEAR(RayOf(eye).distance, 2.0f, tolerance);
    EXPECT_NEAR(RayOf(eye).point.y, 0.5f, tolerance);
    EXPECT_NEAR(RayOf(eye).normal.z, 1.0f, tolerance);
  }

  TEST_F(RayCastingTest, ForgetsTheHitWhenThePhysicsFindsNothing)
  {
    const Entity eye = CreateRay({0.0f, 0.0f, 2.0f});
    Hits(_store.CreateEntity("screen"), {0.0f, 0.0f, 0.0f}, 2.0f);

    Frame();
    ASSERT_TRUE(RayOf(eye).is_hitting);

    _physics.hits = false;
    Frame();

    EXPECT_FALSE(RayOf(eye).is_hitting);
    EXPECT_EQ(RayOf(eye).hit, neon::No_Entity);
  }

  TEST_F(RayCastingTest, LooksInItsLayersAndAtTriggersWhenAsked)
  {
    CreateRay({0.0f, 0.0f, 0.0f}, 0.0f, RayCast{.mask = 0b110u, .triggers = true});

    Frame();

    EXPECT_EQ(_physics.last_filter.mask, 0b110u);
    EXPECT_TRUE(_physics.last_filter.triggers);
  }

  TEST_F(RayCastingTest, ARayThatIsOffAsksNothingAndHitsNothing)
  {
    const Entity eye = CreateRay({0.0f, 0.0f, 2.0f}, 0.0f, RayCast{.enabled = false});
    Hits(_store.CreateEntity("screen"), {0.0f, 0.0f, 0.0f}, 2.0f);

    Frame();

    EXPECT_EQ(_physics.asked, 0);
    EXPECT_FALSE(RayOf(eye).is_hitting);
  }

  TEST_F(RayCastingTest, WithoutPhysicsNothingIsHit)
  {
    RayCasting system{nullptr};
    system.Initialize(_store);
    const Entity eye = CreateRay({0.0f, 0.0f, 2.0f});

    system.Update(_store, 0.016);

    EXPECT_FALSE(RayOf(eye).is_hitting);
  }

  TEST_F(RayCastingTest, FindsItsOwnerByItsPathOnceAndAgainWhenItIsGoneAndLooksPastIt)
  {
    const Entity group = _store.CreateEntity("hall");
    const Entity player = _store.CreateEntity("player", group);
    const Entity eye = CreateRay({0.0f, 0.0f, 2.0f}, 0.0f, RayCast{.owner = "hall/player"});

    Frame();
    EXPECT_EQ(RayOf(eye).owner_entity, player);
    EXPECT_EQ(_physics.last_filter.ignore, player);

    _store.DestroyEntity(player);
    Frame();
    EXPECT_EQ(RayOf(eye).owner_entity, neon::No_Entity);
    EXPECT_EQ(_physics.last_filter.ignore, eye);

    const Entity again = _store.CreateEntity("player", group);
    Frame();
    EXPECT_EQ(RayOf(eye).owner_entity, again);
  }

  TEST_F(RayCastingTest, LooksPastItsOwnEntityWhenNoOwnerIsNamed)
  {
    const Entity eye = CreateRay({0.0f, 0.0f, 2.0f});

    Frame();

    EXPECT_EQ(RayOf(eye).owner_entity, neon::No_Entity);
    EXPECT_EQ(_physics.last_filter.ignore, eye);
  }

  TEST_F(RayCastingTest, EveryRayIsCastOnItsOwn)
  {
    CreateRay({0.0f, 0.0f, 2.0f});
    CreateRay({2.0f, 0.0f, 2.0f});

    Frame();

    EXPECT_EQ(_physics.asked, 2);
  }
}
