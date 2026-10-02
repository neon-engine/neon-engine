#include "ui-view-loading.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/mock-ui-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/ui-view.hpp>

namespace
{
  using neon::Entity;
  using neon::UiView;
  using neon::UiViewLoading;
  using neon::testing::FakeEntityStore;
  using neon::testing::MockUiContext;
  using ::testing::_;
  using ::testing::InSequence;
  using ::testing::Return;
  using ::testing::StrictMock;

  class UiViewLoadingTest : public ::testing::Test
  {
  protected:
    // the user interface outlives the store, as it does in an application:
    // a store that is cleaned up tells it to stop showing what is left
    StrictMock<MockUiContext> _ui;
    FakeEntityStore _store;
    std::shared_ptr<neon::testing::RecordingLogger> _logger = std::make_shared<neon::testing::RecordingLogger>();
    UiViewLoading _system{&_ui, _logger};

    void SetUp() override
    {
      _store.Initialize();
      _system.Register(_store);
      _system.Initialize(_store);
    }

    void TearDown() override
    {
      _store.CleanUp();
    }

    Entity CreateView(const std::string &file, const std::string &name = "")
    {
      const Entity entity = _store.CreateEntity(name);
      _store.Set(entity, UiView{.file = file});
      return entity;
    }
  };

  TEST_F(UiViewLoadingTest, RegistersTheComponentUnderTheNameSceneFilesUse)
  {
    EXPECT_NE(_store.FindComponent("Ui"), neon::No_Component);
    EXPECT_STREQ(UiViewLoading::kComponent_Name, "Ui");
  }

  TEST_F(UiViewLoadingTest, DoesNothingInAWorldWithoutAUserInterface)
  {
    (void) _store.CreateEntity("player");

    _system.Update(_store, 0.016);
  }

  TEST_F(UiViewLoadingTest, ShowsTheFileOfAnEntity)
  {
    const Entity hud = CreateView("assets://ui/hud.ui.yml", "hud");

    EXPECT_CALL(_ui, Load("assets://ui/hud.ui.yml")).WillOnce(Return(3));
    _system.Update(_store, 0.016);

    EXPECT_EQ(_store.Get<UiView>(hud)->document, 3);
    EXPECT_TRUE(_store.Get<UiView>(hud)->is_tried);

    EXPECT_CALL(_ui, Unload(3));
  }

  TEST_F(UiViewLoadingTest, ShowsAFileOnce)
  {
    (void) CreateView("assets://ui/hud.ui.yml");

    EXPECT_CALL(_ui, Load("assets://ui/hud.ui.yml")).WillOnce(Return(0));
    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);

    EXPECT_CALL(_ui, Unload(0));
  }

  TEST_F(UiViewLoadingTest, ShowsTheFileOfEveryEntity)
  {
    (void) CreateView("assets://ui/hud.ui.yml");
    (void) CreateView("assets://ui/minimap.ui.yml");

    {
      InSequence in_order;
      EXPECT_CALL(_ui, Load("assets://ui/hud.ui.yml")).WillOnce(Return(0));
      EXPECT_CALL(_ui, Load("assets://ui/minimap.ui.yml")).WillOnce(Return(1));
    }
    _system.Update(_store, 0.016);

    EXPECT_CALL(_ui, Unload(_)).Times(2);
  }

  TEST_F(UiViewLoadingTest, ShowsTheFileOfAnEntityThatIsCreatedLater)
  {
    _system.Update(_store, 0.016);

    (void) CreateView("assets://ui/pause.ui.yml");

    EXPECT_CALL(_ui, Load("assets://ui/pause.ui.yml")).WillOnce(Return(0));
    _system.Update(_store, 0.016);

    EXPECT_CALL(_ui, Unload(0));
  }

  TEST_F(UiViewLoadingTest, StopsShowingAFileWhenItsEntityIsDestroyed)
  {
    const Entity hud = CreateView("assets://ui/hud.ui.yml");

    EXPECT_CALL(_ui, Load(_)).WillOnce(Return(5));
    _system.Update(_store, 0.016);

    EXPECT_CALL(_ui, Unload(5));
    _store.DestroyEntity(hud);
    ::testing::Mock::VerifyAndClearExpectations(&_ui);

    _system.Update(_store, 0.016);
  }

  TEST_F(UiViewLoadingTest, StopsShowingAFileWhenTheComponentIsRemoved)
  {
    const Entity hud = CreateView("assets://ui/hud.ui.yml");

    EXPECT_CALL(_ui, Load(_)).WillOnce(Return(5));
    _system.Update(_store, 0.016);

    EXPECT_CALL(_ui, Unload(5));
    _store.Remove<UiView>(hud);
    ::testing::Mock::VerifyAndClearExpectations(&_ui);
  }

  TEST_F(UiViewLoadingTest, StopsShowingEveryFileWhenTheStoreIsCleanedUp)
  {
    (void) CreateView("assets://ui/hud.ui.yml");

    EXPECT_CALL(_ui, Load(_)).WillOnce(Return(5));
    _system.Update(_store, 0.016);

    EXPECT_CALL(_ui, Unload(5));
    _store.CleanUp();
    ::testing::Mock::VerifyAndClearExpectations(&_ui);
  }

  TEST_F(UiViewLoadingTest, UnloadsNothingForAFileThatWasNeverShown)
  {
    const Entity hud = CreateView("assets://ui/hud.ui.yml");

    // the mock is strict, a call to Unload fails the test
    _store.DestroyEntity(hud);
  }

  TEST_F(UiViewLoadingTest, SaysWhenAFileCannotBeUsedAndGoesOn)
  {
    (void) CreateView("assets://ui/broken.ui.yml");

    EXPECT_CALL(_ui, Load("assets://ui/broken.ui.yml")).WillOnce(Return(-1));

    _system.Update(_store, 0.016);

    EXPECT_TRUE(_logger->Contains(
      neon::testing::LogLevel::Error,
      "The user interface assets://ui/broken.ui.yml cannot be used, the entity shows nothing"))
      << _logger->Messages(neon::testing::LogLevel::Error);
  }

  TEST_F(UiViewLoadingTest, DoesNotReadAFileAgainThatCouldNotBeUsed)
  {
    const Entity hud = CreateView("assets://ui/broken.ui.yml");

    EXPECT_CALL(_ui, Load(_)).WillOnce(Return(-1));
    _system.Update(_store, 0.016);

    // for a game that goes on all the same, and is told once
    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
    EXPECT_EQ(_logger->Count(neon::testing::LogLevel::Error), 1u) << _logger->Messages(neon::testing::LogLevel::Error);

    EXPECT_EQ(_store.Get<UiView>(hud)->document, -1);
  }
}
