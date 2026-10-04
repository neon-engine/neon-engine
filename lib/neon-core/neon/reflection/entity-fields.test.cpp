#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>

#include "entity-fields.hpp"

namespace
{
  using neon::Camera;
  using neon::Color;
  using neon::Entity;
  using neon::FieldValue;
  using neon::Renderable;
  using neon::RenderTarget;
  using neon::Transform;
  using neon::TypeInfo;
  using neon::testing::FakeEntityStore;

  class EntityFieldsTest : public ::testing::Test
  {
  protected:
    FakeEntityStore _store;
    TypeInfo _camera = TypeInfo::Of<Camera>();
    TypeInfo _renderable = TypeInfo::Of<Renderable>();
    TypeInfo _transform = TypeInfo::Of<Transform>();
    Entity _entity = neon::No_Entity;
    FieldValue _value;
    std::string _error;

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Camera>("Camera");
      _store.Register<Renderable>("Renderable");

      _entity = _store.CreateEntity("thing");
      _store.Set(_entity, Camera{});
      _store.Set(_entity, Renderable{});
    }

    void TearDown() override
    {
      _store.CleanUp();
    }
  };

  TEST_F(EntityFieldsTest, ReadsAFieldOfAComponentByItsName)
  {
    _store.Get<Camera>(_entity)->fov = 70.0f;

    ASSERT_TRUE(neon::GetField(_store, _entity, _camera, "fov", _value, _error)) << _error;

    EXPECT_EQ(std::get<float>(_value), 70.0f);
  }

  TEST_F(EntityFieldsTest, ReadsAFieldThatIsCalledDifferentlyFromItsMember)
  {
    _store.Get<Camera>(_entity)->near_plane = 0.5f;

    ASSERT_TRUE(neon::GetField(_store, _entity, _camera, "near", _value, _error)) << _error;

    EXPECT_EQ(std::get<float>(_value), 0.5f);
  }

  TEST_F(EntityFieldsTest, ReadsAChoiceAsItsWord)
  {
    _store.Get<Camera>(_entity)->target = RenderTarget::Texture;

    ASSERT_TRUE(neon::GetField(_store, _entity, _camera, "target", _value, _error)) << _error;

    EXPECT_EQ(std::get<std::string>(_value), "texture");
  }

  TEST_F(EntityFieldsTest, ChangesAFieldOfAComponentByItsName)
  {
    ASSERT_TRUE(neon::SetField(_store, _entity, _camera, "fov", 90.0f, _error)) << _error;

    EXPECT_EQ(_store.Get<Camera>(_entity)->fov, 90.0f);
  }

  TEST_F(EntityFieldsTest, ChangesAFieldOfAGroupByItsPath)
  {
    const Color red{1.0f, 0.0f, 0.0f, 1.0f};

    ASSERT_TRUE(neon::SetField(_store, _entity, _renderable, "material.color", red, _error)) << _error;

    EXPECT_EQ(_store.Get<Renderable>(_entity)->render_info.material_info.color.g, 0.0f);
  }

  TEST_F(EntityFieldsTest, RefusesAValueOfAnotherKindAndLeavesTheFieldAlone)
  {
    EXPECT_FALSE(neon::SetField(_store, _entity, _camera, "fov", std::string("wide"), _error));

    EXPECT_EQ(_error, "'fov' of Camera takes a number");
    EXPECT_EQ(_store.Get<Camera>(_entity)->fov, Camera{}.fov);
  }

  TEST_F(EntityFieldsTest, RefusesAWordThatIsNotAmongTheChoices)
  {
    EXPECT_FALSE(neon::SetField(_store, _entity, _camera, "target", std::string("screen"), _error));

    EXPECT_EQ(_error, "'target' of Camera is 'screen', where one of these was expected: window, texture");
  }

  TEST_F(EntityFieldsTest, SaysWhichFieldsThereAreWhenOneIsNotKnown)
  {
    EXPECT_FALSE(neon::GetField(_store, _entity, _camera, "zoom", _value, _error));

    EXPECT_EQ(_error, "'zoom' is not known to Camera. Known are: target, fov, near, far, up, rolls_with_entity, effects, screen_effects, texture, size");
  }

  TEST_F(EntityFieldsTest, DoesNotReachWhatIsNotDescribed)
  {
    EXPECT_FALSE(neon::GetField(_store, _entity, _renderable, "render_object_id", _value, _error));
  }

  TEST_F(EntityFieldsTest, SaysWhenTheEntityDoesNotCarryTheComponent)
  {
    const Entity other = _store.CreateEntity("other");

    EXPECT_FALSE(neon::GetField(_store, other, _camera, "fov", _value, _error));

    EXPECT_EQ(_error, "The entity does not carry a Camera");
  }

  TEST_F(EntityFieldsTest, SaysWhenTheComponentIsNotRegistered)
  {
    EXPECT_FALSE(neon::GetField(_store, _entity, _transform, "position", _value, _error));

    EXPECT_EQ(_error, "Component Transform is not registered");
  }

  TEST_F(EntityFieldsTest, DoesNotReadAGroupAsAValue)
  {
    EXPECT_FALSE(neon::GetField(_store, _entity, _renderable, "material", _value, _error));

    EXPECT_EQ(_error, "'material' of Renderable is a group and holds no value of its own");
  }
}
