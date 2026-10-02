#include "ui-fixture.hpp"

#include <neon/testing/fake-entity-store.hpp>
#include <neon/world-system/ecs/components/ui-view.hpp>
#include <neon/world-system/ecs/scene-file/scene-file.hpp>
#include <neon/world-system/ecs/scene-file/ui-view-format.hpp>
#include <neon/world-system/ecs/systems/ui-view-loading.hpp>

// A scene names the user interface it comes with, and the user interface
// that is shown from the start.

namespace
{
  using neon::SceneFile;
  using neon::UiView;
  using neon::UiViewLoading;
  using neon::testing::FakeEntityStore;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;

  class UiSceneTest : public UiTest
  {
  protected:
    FakeEntityStore _store;
    std::unique_ptr<UiViewLoading> _loading;

    void SetUp() override
    {
      UiTest::SetUp();

      WriteAsset(
        "ui/hud.ui.yml",
        "ui: hud\n"
        "root:\n"
        "  type: label\n"
        "  name: health\n"
        "  text: \"Health: {health}\"\n");

      _store.Initialize();
      _loading = std::make_unique<UiViewLoading>(_ui.get(), _logger);
      _loading->Register(_store);
      _loading->Initialize(_store);
    }

    void TearDown() override
    {
      // the store first, as an application does it
      _store.CleanUp();
      UiTest::TearDown();
    }

    /// Reads a scene the way the runtime does.
    /// Returns what the scene returns: whether everything could be read.
    bool Populate(const std::string &yaml)
    {
      WriteAsset("scenes/test.scene.yml", yaml);

      SceneFile scene(&_file_system, &_yaml, "assets://scenes/test.scene.yml", _logger);
      scene.GetComponentFormats().Add(neon::UiViewFormat());
      return scene.Populate(_store);
    }
  };

  TEST_F(UiSceneTest, ASceneNamesTheUserInterfaceItShows)
  {
    Populate(
      "scene: test\n"
      "entities:\n"
      "  - name: hud\n"
      "    components:\n"
      "      Ui:\n"
      "        file: assets://ui/hud.ui.yml\n");

    EXPECT_EQ(_ui->Find("health"), nullptr) << "until the world is updated";

    _loading->Update(_store, 0.016);
    Frame();

    ASSERT_NE(_ui->Find("health"), nullptr);
    EXPECT_FALSE(_renderer.batches.empty());
    EXPECT_TRUE(Errors().empty()) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(UiSceneTest, ASceneShowsSeveralUserInterfaces)
  {
    WriteAsset("ui/minimap.ui.yml", "root:\n  type: panel\n  name: map\n");

    Populate(
      "entities:\n"
      "  - name: hud\n"
      "    components:\n"
      "      Ui:\n"
      "        file: assets://ui/hud.ui.yml\n"
      "  - name: minimap\n"
      "    components:\n"
      "      Ui:\n"
      "        file: assets://ui/minimap.ui.yml\n");

    _loading->Update(_store, 0.016);

    EXPECT_NE(_ui->Find("health"), nullptr);
    EXPECT_NE(_ui->Find("map"), nullptr);
  }

  TEST_F(UiSceneTest, TheUserInterfaceGoesWithItsEntity)
  {
    Populate(
      "entities:\n"
      "  - name: hud\n"
      "    components:\n"
      "      Ui:\n"
      "        file: assets://ui/hud.ui.yml\n");

    _loading->Update(_store, 0.016);
    ASSERT_NE(_ui->Find("health"), nullptr);

    _store.DestroyEntity(_store.FindEntity("hud"));

    EXPECT_EQ(_ui->Find("health"), nullptr);

    Frame();
    EXPECT_TRUE(_renderer.batches.empty());
  }

  TEST_F(UiSceneTest, TheUserInterfaceGoesWithTheScene)
  {
    Populate(
      "entities:\n"
      "  - name: hud\n"
      "    components:\n"
      "      Ui:\n"
      "        file: assets://ui/hud.ui.yml\n");

    _loading->Update(_store, 0.016);
    _store.CleanUp();

    EXPECT_EQ(_ui->Find("health"), nullptr);
  }

  TEST_F(UiSceneTest, ASceneThatNamesAFileThatIsWrongIsSaidAndTheRunGoesOn)
  {
    WriteAsset("ui/broken.ui.yml", "root:\n  type: lable\n");

    EXPECT_TRUE(Populate(
      "entities:\n"
      "  - name: hud\n"
      "    components:\n"
      "      Ui:\n"
      "        file: assets://ui/broken.ui.yml\n"));

    _loading->Update(_store, 0.016);

    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The user interface assets://ui/broken.ui.yml cannot be used, the entity shows nothing"))
      << _logger->Messages(LogLevel::Error);

    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "assets://ui/broken.ui.yml:2: type 'lable' of the root is not known. Known are: "
      "input, textarea, checkbox, radio, toggle, slider, select, panel, label, image, button, bar"));
  }

  TEST_F(UiSceneTest, SaysThatTheComponentNamesNoFile)
  {
    EXPECT_FALSE(Populate(
      "entities:\n"
      "  - name: hud\n"
      "    components:\n"
      "      Ui: {}\n"));

    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "assets://scenes/test.scene.yml:4: Ui of entity 'hud' has no 'file', where the virtual path of a user "
      "interface was expected")) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(UiSceneTest, SaysThatANameOfTheComponentIsNotKnown)
  {
    EXPECT_FALSE(
      Populate(
        "entities:\n"
        "  - name: hud\n"
        "    components:\n"
        "      Ui:\n"
        "        file: assets://ui/hud.ui.yml\n"
        "        modal: true\n"));

    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "assets://scenes/test.scene.yml:6: 'modal' is not known to Ui of entity 'hud'. Known are: file"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(UiSceneTest, ASceneIsSavedWithTheUserInterfaceItNames)
  {
    Populate(
      "scene: test\n"
      "entities:\n"
      "  - name: hud\n"
      "    components:\n"
      "      Ui:\n"
      "        file: assets://ui/hud.ui.yml\n");

    _loading->Update(_store, 0.016);

    SceneFile scene(&_file_system, &_yaml, "assets://scenes/test.scene.yml", _logger);
    scene.GetComponentFormats().Add(neon::UiViewFormat());
    ASSERT_TRUE(scene.Save(_store, "test", "user://saved.scene.yml"));

    std::string saved;
    ASSERT_TRUE(_file_system.ReadText("user://saved.scene.yml", saved));

    // what the file is known as while it is shown is not part of the scene
    EXPECT_EQ(
      saved,
      "scene: test\n"
      "version: 1\n"
      "\n"
      "entities:\n"
      "  - name: hud\n"
      "    components:\n"
      "      Ui:\n"
      "        file: assets://ui/hud.ui.yml\n");
  }

  TEST_F(UiSceneTest, ASceneWithoutTheFormatDoesNotKnowTheComponent)
  {
    WriteAsset(
      "scenes/test.scene.yml",
      "entities:\n"
      "  - name: hud\n"
      "    components:\n"
      "      Ui:\n"
      "        file: assets://ui/hud.ui.yml\n");

    SceneFile scene(&_file_system, &_yaml, "assets://scenes/test.scene.yml", _logger);

    EXPECT_FALSE(scene.Populate(_store));
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "component 'Ui' of entity 'hud' is not known"));
  }

  // the user interface that is shown from the start

  TEST_F(UiSceneTest, ShowsNothingFromTheStartUnlessItIsTold)
  {
    _ui->Initialize();
    Frame();

    EXPECT_TRUE(_renderer.batches.empty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Initializing the user interface"));
  }

  TEST_F(UiSceneTest, ShowsTheFileItIsToldToFromTheStart)
  {
    Create("assets://ui/hud.ui.yml");

    _ui->Initialize();
    Frame();

    EXPECT_NE(_ui->Find("health"), nullptr);
    EXPECT_FALSE(_renderer.batches.empty());
  }

  TEST_F(UiSceneTest, AFileFromTheStartThatIsWrongIsSaidAndTheRunGoesOn)
  {
    WriteAsset("ui/broken.ui.yml", "root:\n  type: lable\n");
    Create("assets://ui/broken.ui.yml");

    _ui->Initialize();

    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The user interface assets://ui/broken.ui.yml cannot be used, nothing is shown from the start"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(UiSceneTest, AFileFromTheStartThatIsMissingIsSaidAndTheRunGoesOn)
  {
    Create("assets://ui/missing.ui.yml");

    _ui->Initialize();

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "assets://ui/missing.ui.yml: the file cannot be read"));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The user interface assets://ui/missing.ui.yml cannot be used, nothing is shown from the start"));
  }

  TEST_F(UiSceneTest, CanBeCleanedUpTwiceAndStartedAgain)
  {
    Create("assets://ui/hud.ui.yml");

    _ui->Initialize();
    Frame();
    _ui->CleanUp();
    _ui->CleanUp();

    EXPECT_EQ(_ui->Find("health"), nullptr);
    EXPECT_EQ(_renderer.TextureCount(), 0u);

    _ui->Initialize();
    Frame();

    EXPECT_NE(_ui->Find("health"), nullptr);
    EXPECT_FALSE(_renderer.batches.empty());
  }

  TEST_F(UiSceneTest, UnloadingWhatIsNotShownIsLeftAlone)
  {
    _ui->Unload(7);
    _ui->Unload(-1);

    const int hud = _ui->Load("assets://ui/hud.ui.yml");
    _ui->Unload(hud);
    _ui->Unload(hud);

    EXPECT_EQ(_ui->Find("health"), nullptr);
    EXPECT_TRUE(Errors().empty());
  }

  TEST_F(UiSceneTest, EveryFileIsKnownAsSomethingElse)
  {
    const int first = _ui->Load("assets://ui/hud.ui.yml");
    const int second = _ui->Load("assets://ui/hud.ui.yml");
    _ui->Unload(first);
    const int third = _ui->Load("assets://ui/hud.ui.yml");

    EXPECT_NE(first, second);
    EXPECT_NE(second, third);
    EXPECT_NE(first, third);
  }
}
