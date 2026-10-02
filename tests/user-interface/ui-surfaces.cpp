#include "ui-fixture.hpp"

#include <neon/testing/fake-entity-store.hpp>
#include <neon/ui/elements/ui-label.hpp>
#include <neon/world-system/ecs/components/ui-surface-view.hpp>
#include <neon/world-system/ecs/scene-file/scene-file.hpp>
#include <neon/world-system/ecs/systems/ui-surface-loading.hpp>

// User interfaces on several surfaces: the window, and images that are
// drawn to and shown in the world. Each is laid out for its surface, has
// values and events of its own, and is pointed at on its own.

namespace
{
  using neon::Action;
  using neon::No_Render_Target;
  using neon::No_Texture;
  using neon::No_Ui_Surface;
  using neon::SceneFile;
  using neon::Ui_Window_Surface;
  using neon::UiEvent;
  using neon::UiSurfaceLoading;
  using neon::UiSurfaceView;
  using neon::testing::FakeEntityStore;
  using neon::testing::LogLevel;
  using neon::testing::RecordedQuad;
  using neon::testing::RecordingRenderer2D;
  using neon::testing::UiTest;
  using ::testing::ElementsAre;

  class UiSurfaceTest : public UiTest
  {
  protected:
    void SetUp() override
    {
      UiTest::SetUp();

      // what is shown during play, and what a screen in the world shows.
      // Both have a button that is called `go`
      WriteAsset(
        "ui/hud.ui.yml",
        "ui: hud\n"
        "values:\n"
        "  health: 75\n"
        "root:\n"
        "  type: panel\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  children:\n"
        "    - type: label\n"
        "      name: health\n"
        "      text: \"{health}\"\n"
        "      font_size: 20\n"
        "    - type: button\n"
        "      name: go\n"
        "      position: absolute\n"
        "      left: 0\n"
        "      top: 100\n"
        "      width: 100\n"
        "      height: 100\n"
        "      padding: 0\n");

      WriteAsset(
        "ui/terminal.ui.yml",
        "ui: terminal\n"
        "reference_size: [400, 300]\n"
        "values:\n"
        "  health: 10\n"
        "  door: locked\n"
        "root:\n"
        "  type: panel\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  background_color: \"#102030\"\n"
        "  children:\n"
        "    - type: label\n"
        "      name: health\n"
        "      text: \"{health}\"\n"
        "      font_size: 20\n"
        "    - type: label\n"
        "      name: door\n"
        "      text: \"{door}\"\n"
        "      font_size: 20\n"
        "    - type: button\n"
        "      name: go\n"
        "      position: absolute\n"
        "      left: 300\n"
        "      top: 200\n"
        "      width: 100\n"
        "      height: 100\n"
        "      padding: 0\n"
        "    - type: button\n"
        "      name: stop\n"
        "      position: absolute\n"
        "      left: 0\n"
        "      top: 200\n"
        "      width: 100\n"
        "      height: 100\n"
        "      padding: 0\n");
    }

    /// The surface `terminal` of 400 by 300 with its user interface on it.
    int ShowTerminal(const int width = 400, const int height = 300, const float scale = 1.0f)
    {
      const int surface = _ui->CreateSurface("terminal", width, height, scale);
      EXPECT_NE(surface, No_Ui_Surface) << _logger->Messages(LogLevel::Error);
      EXPECT_GE(_ui->LoadOnto(surface, "assets://ui/terminal.ui.yml"), 0) << _logger->Messages(LogLevel::Error);
      return surface;
    }

    int ShowHud()
    {
      const int document = _ui->Load("assets://ui/hud.ui.yml");
      EXPECT_GE(document, 0) << _logger->Messages(LogLevel::Error);
      return document;
    }

    /// A frame with the button of the pointer of a surface held down, and
    /// one with it released.
    void ClickOn(const int surface, const float x, const float y)
    {
      _ui->SetPointer(surface, x, y, true);
      Frame();
      _ui->SetPointer(surface, x, y, false);
      Frame();
    }

    [[nodiscard]] std::vector<RecordedQuad> QuadsOf(const std::string &surface) const
    {
      const RecordingRenderer2D::Target *target = _renderer.TargetOf(surface);
      return target != nullptr ? RecordingRenderer2D::QuadsOf(target->batches) : std::vector<RecordedQuad>{};
    }

    [[nodiscard]] const neon::UiElement &ElementIn(const std::string &interface, const std::string &name) const
    {
      const neon::UiElement *element = _ui->FindIn(interface, name);
      if (element == nullptr) { throw std::runtime_error("There is no element '" + name + "' in " + interface); }
      return *element;
    }

    [[nodiscard]] std::string TextOf(const std::string &interface, const std::string &name) const
    {
      return dynamic_cast<const neon::UiLabel &>(ElementIn(interface, name)).GetText();
    }
  };

  // surfaces

  TEST_F(UiSurfaceTest, TheWindowIsASurfaceFromTheStart)
  {
    EXPECT_EQ(_ui->FindSurface("window"), Ui_Window_Surface);
    EXPECT_EQ(_ui->GetInputSurface(), Ui_Window_Surface);
    EXPECT_EQ(_ui->GetSurfaceTexture(Ui_Window_Surface), No_Texture);

    EXPECT_EQ(_renderer.targets_created, 0u) << "and needs no render target";
  }

  TEST_F(UiSurfaceTest, ASurfaceIsARenderTargetOfItsSize)
  {
    const int surface = _ui->CreateSurface("terminal", 1024, 768);

    ASSERT_NE(surface, No_Ui_Surface);
    EXPECT_NE(surface, Ui_Window_Surface);
    EXPECT_EQ(_ui->FindSurface("terminal"), surface);

    const auto *target = _renderer.TargetOf("terminal");
    ASSERT_NE(target, nullptr);
    EXPECT_EQ(target->width, 1024);
    EXPECT_EQ(target->height, 768);

    // what shows the surface in two dimensions draws with its texture
    EXPECT_EQ(_ui->GetSurfaceTexture(surface), target->texture);
    EXPECT_NE(target->texture, No_Texture);
  }

  TEST_F(UiSurfaceTest, DrawsAUserInterfaceIntoItsSurfaceAndNotIntoTheFrame)
  {
    ShowTerminal();
    Frame();

    EXPECT_TRUE(_renderer.batches.empty()) << "nothing is shown on the window";

    const auto *target = _renderer.TargetOf("terminal");
    ASSERT_NE(target, nullptr);
    EXPECT_EQ(target->begun, 1u);
    EXPECT_FALSE(target->batches.empty());

    // cleared to nothing, so that what shows the surface says what is
    // behind it
    EXPECT_FLOAT_EQ(target->clear.a, 0);

    // the background of the terminal over the whole surface
    const auto quads = QuadsOf("terminal");
    ASSERT_FALSE(quads.empty());
    EXPECT_FLOAT_EQ(quads[0].left, 0);
    EXPECT_FLOAT_EQ(quads[0].top, 0);
    EXPECT_FLOAT_EQ(quads[0].Width(), 400);
    EXPECT_FLOAT_EQ(quads[0].Height(), 300);

    EXPECT_EQ(_renderer.current_target, No_Render_Target) << "and what follows is drawn into the frame again";
  }

  TEST_F(UiSurfaceTest, DrawsEverySurfaceInEveryFrame)
  {
    ShowTerminal();
    ShowHud();

    Frame();
    Frame();
    Frame();

    EXPECT_EQ(_renderer.TargetOf("terminal")->begun, 3u);

    // the two do not share a call: each is drawn to what it is shown on
    EXPECT_EQ(_renderer.batches.size(), 1u);
    EXPECT_EQ(_renderer.TargetOf("terminal")->batches.size(), 1u);
    EXPECT_EQ(_ui->GetDrawCalls(), 2u);
  }

  TEST_F(UiSurfaceTest, ASurfaceWithoutAUserInterfaceIsNotDrawnTo)
  {
    (void) _ui->CreateSurface("terminal", 400, 300);
    ShowHud();
    Frame();

    EXPECT_EQ(_renderer.TargetOf("terminal")->begun, 0u);
  }

  TEST_F(UiSurfaceTest, LaysAUserInterfaceOutForTheSizeOfItsSurface)
  {
    // the frame is 1920 by 1080, and the surface 800 by 600, for which the
    // file was made at half that size
    ShowTerminal(800, 600);
    ShowHud();
    Frame();

    const auto &root = *ElementIn("terminal", "go").GetParent();
    EXPECT_FLOAT_EQ(root.GetBox().Width(), 400) << "in units of the file";

    const auto quads = QuadsOf("terminal");
    ASSERT_FALSE(quads.empty());
    EXPECT_FLOAT_EQ(quads[0].Width(), 800) << "in pixels of the surface";
    EXPECT_FLOAT_EQ(quads[0].Height(), 600);

    // the window is laid out for the frame, as before
    EXPECT_FLOAT_EQ(ElementIn("hud", "go").GetParent()->GetBox().Width(), 1920);
  }

  TEST_F(UiSurfaceTest, TheScaleOfASurfaceMakesEverythingOnItLarger)
  {
    ShowTerminal(400, 300, 2.0f);
    Frame();

    // twice as large, in a surface of the same size: half as much fits
    EXPECT_FLOAT_EQ(ElementIn("terminal", "go").GetParent()->GetBox().Width(), 200);

    const auto quads = QuadsOf("terminal");
    EXPECT_FLOAT_EQ(quads.at(0).Width(), 400);

    // a text of the size 20 is drawn at 40
    EXPECT_FLOAT_EQ(quads.at(1).Height(), 28);
  }

  TEST_F(UiSurfaceTest, SaysOnceThatASurfaceCannotBeCreated)
  {
    _renderer.refused_targets.push_back("broken");

    for (int i = 0; i < 3; i++)
    {
      EXPECT_EQ(_ui->CreateSurface("broken", 400, 300), No_Ui_Surface);
      EXPECT_EQ(_ui->CreateSurface("", 400, 300), No_Ui_Surface);
      EXPECT_EQ(_ui->CreateSurface("flat", 400, 0), No_Ui_Surface);
      EXPECT_EQ(_ui->CreateSurface("small", 400, 300, 0.0f), No_Ui_Surface);
      EXPECT_EQ(_ui->CreateSurface("window", 400, 300), No_Ui_Surface);
    }

    EXPECT_THAT(
      Errors(),
      ElementsAre(
        "The surface 'broken' cannot be created: the renderer made no render target for it",
        "The surface '' cannot be created: it has no name",
        "The surface 'flat' cannot be created: its size is 400 by 0, where both are above 0",
        "The surface 'small' cannot be created: its scale is 0, where a number above 0 was expected",
        "The surface 'window' cannot be created: there is a surface of that name"));

    EXPECT_EQ(_renderer.targets_created, 3u) << "the renderer is asked where nothing else is wrong";
    EXPECT_TRUE(_renderer.targets.empty());
  }

  TEST_F(UiSurfaceTest, TheNameOfASurfaceIsGivenOnce)
  {
    ASSERT_NE(_ui->CreateSurface("terminal", 400, 300), No_Ui_Surface);

    EXPECT_EQ(_ui->CreateSurface("terminal", 800, 600), No_Ui_Surface);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The surface 'terminal' cannot be created: there is a surface of that name"));
    EXPECT_EQ(_renderer.targets.size(), 1u);
  }

  TEST_F(UiSurfaceTest, RefusesToShowAFileOnASurfaceThatDoesNotExist)
  {
    EXPECT_EQ(_ui->LoadOnto(7, "assets://ui/terminal.ui.yml"), -1);
    EXPECT_EQ(_ui->LoadOnto(No_Ui_Surface, "assets://ui/terminal.ui.yml"), -1);

    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The user interface assets://ui/terminal.ui.yml cannot be shown on surface 7, which does not exist"));
  }

  TEST_F(UiSurfaceTest, WhatIsShownOnASurfaceGoesWithIt)
  {
    const int surface = ShowTerminal();
    ShowHud();
    Frame();

    _ui->DestroySurface(surface);

    EXPECT_EQ(_ui->FindSurface("terminal"), No_Ui_Surface);
    EXPECT_EQ(_ui->FindIn("terminal", "go"), nullptr);
    EXPECT_NE(_ui->FindIn("hud", "go"), nullptr) << "what is shown on the window stays";

    EXPECT_EQ(_renderer.targets_destroyed, 1u);
    EXPECT_TRUE(_renderer.targets.empty());

    Frame();
    EXPECT_EQ(_renderer.batches.size(), 1u);

    // and the name is free again
    EXPECT_NE(_ui->CreateSurface("terminal", 400, 300), No_Ui_Surface);
  }

  TEST_F(UiSurfaceTest, TheWindowCannotBeDestroyed)
  {
    ShowHud();

    _ui->DestroySurface(Ui_Window_Surface);
    _ui->DestroySurface(99);
    Frame();

    EXPECT_EQ(_ui->FindSurface("window"), Ui_Window_Surface);
    EXPECT_FALSE(_renderer.batches.empty());
  }

  TEST_F(UiSurfaceTest, ReleasesItsSurfacesWhenItIsCleanedUp)
  {
    ShowTerminal();
    (void) _ui->CreateSurface("other", 64, 64);
    Frame();

    _ui->CleanUp();

    EXPECT_EQ(_renderer.targets_destroyed, 2u);
    EXPECT_TRUE(_renderer.targets.empty());
    EXPECT_EQ(_ui->FindSurface("terminal"), No_Ui_Surface);
    EXPECT_EQ(_ui->FindSurface("window"), Ui_Window_Surface);
  }

  TEST_F(UiSurfaceTest, AUserInterfaceShowsASurfaceAsAnImage)
  {
    const int surface = ShowTerminal();

    ASSERT_GE(ShowUnderRoot(
      "- type: image\n"
      "  name: screen\n"
      "  src: surface://terminal\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 100\n"
      "  width: 200\n"
      "  height: 150\n"), 0) << _logger->Messages(LogLevel::Error);
    Frame();

    const auto quads = _renderer.Quads();
    ASSERT_EQ(quads.size(), 1u);
    EXPECT_EQ(quads[0].texture, _ui->GetSurfaceTexture(surface));
    EXPECT_FLOAT_EQ(quads[0].Width(), 200);

    // the surface was drawn to before the frame that shows it
    EXPECT_EQ(_renderer.TargetOf("terminal")->begun, 1u);
  }

  TEST_F(UiSurfaceTest, AnImageOfASurfaceThatIsNotThereIsDrawnOnceThereIsOne)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: image\n"
      "  name: screen\n"
      "  src: surface://minimap\n"
      "  width: 200\n"
      "  height: 150\n"), 0) << _logger->Messages(LogLevel::Error);

    Frame();
    Frame();
    Frame();

    EXPECT_TRUE(_renderer.batches.empty());
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u) << _logger->Messages(LogLevel::Warn);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn, "There is no surface 'minimap', image 'screen' is drawn without it until there is"));

    // what a camera sees, for one, is there from the first frame it is
    // drawn in
    const int target = _renderer.CreateRenderTarget("minimap", 256, 256);
    Frame();

    ASSERT_EQ(_renderer.Quads().size(), 1u);
    EXPECT_EQ(_renderer.Quads()[0].texture, _renderer.GetRenderTargetTexture(target));
  }

  // values

  TEST_F(UiSurfaceTest, AUserInterfaceInTheWorldKeepsWhatItStartsWithToItself)
  {
    ShowHud();
    ShowTerminal();
    Frame();

    // both files start `health` with a value of their own
    EXPECT_EQ(TextOf("hud", "health"), "75");
    EXPECT_EQ(TextOf("terminal", "health"), "10");
    EXPECT_EQ(TextOf("terminal", "door"), "locked");
  }

  TEST_F(UiSurfaceTest, SetsAValueForOneUserInterface)
  {
    ShowHud();
    ShowTerminal();

    _ui->SetNumberOf("terminal", "health", 3);
    _ui->SetTextOf("terminal", "door", "open");
    Frame();

    EXPECT_EQ(TextOf("terminal", "health"), "3");
    EXPECT_EQ(TextOf("terminal", "door"), "open");
    EXPECT_EQ(TextOf("hud", "health"), "75") << "which knows nothing of it";

    _ui->SetNumberOf("hud", "health", 50);
    Frame();

    EXPECT_EQ(TextOf("hud", "health"), "50");
    EXPECT_EQ(TextOf("terminal", "health"), "3");
  }

  TEST_F(UiSurfaceTest, AValueForOneUserInterfaceWinsOverTheOneAllShare)
  {
    ShowHud();
    ShowTerminal();

    _ui->SetNumber("health", 99);
    Frame();

    // The window shows what the game has set for all. The terminal has
    // started the value for itself, which is nearer to it.
    EXPECT_EQ(TextOf("hud", "health"), "99");
    EXPECT_EQ(TextOf("terminal", "health"), "10");

    _ui->SetNumberOf("hud", "health", 1);
    Frame();

    EXPECT_EQ(TextOf("hud", "health"), "1");
  }

  TEST_F(UiSurfaceTest, AUserInterfaceFallsBackOnTheValuesAllShare)
  {
    // a file that starts none of its values itself
    WriteAsset(
      "ui/sign.ui.yml",
      "ui: sign\n"
      "root:\n"
      "  type: label\n"
      "  name: door\n"
      "  text: \"{door}\"\n");

    const int surface = _ui->CreateSurface("sign", 200, 100);
    ASSERT_GE(_ui->LoadOnto(surface, "assets://ui/sign.ui.yml"), 0);

    _ui->SetText("door", "sealed");
    Frame();

    EXPECT_EQ(TextOf("sign", "door"), "sealed");

    _ui->SetText("door", "open");
    Frame();

    EXPECT_EQ(TextOf("sign", "door"), "open") << "and follows them";

    // until it has a value of its own
    _ui->SetTextOf("sign", "door", "welded shut");
    _ui->SetText("door", "ajar");
    Frame();

    EXPECT_EQ(TextOf("sign", "door"), "welded shut");
  }

  TEST_F(UiSurfaceTest, WhatAFileStartsAValueWithDoesNotReplaceWhatTheGameHasSetForIt)
  {
    _ui->SetTextOf("terminal", "door", "open");

    ShowTerminal();
    Frame();

    EXPECT_EQ(TextOf("terminal", "door"), "open");
    EXPECT_EQ(TextOf("terminal", "health"), "10");
  }

  TEST_F(UiSurfaceTest, AValueCanBeSetBeforeItsUserInterfaceIsShown)
  {
    _ui->SetFlagOf("terminal", "door", true);

    ShowTerminal();
    Frame();

    EXPECT_EQ(TextOf("terminal", "door"), "true");
  }

  TEST_F(UiSurfaceTest, AValueThatIsSetForOneUserInterfaceIsNotMissedByAnother)
  {
    ShowHud();
    ShowTerminal();
    Frame();

    EXPECT_EQ(_logger->Count(LogLevel::Warn), 0u) << _logger->Messages(LogLevel::Warn);
  }

  // the pointer

  TEST_F(UiSurfaceTest, APointerOnASurfaceIsOverWhatIsShownThere)
  {
    const int surface = ShowTerminal();
    ShowHud();

    _ui->SetPointer(surface, 350, 250, false);
    Frame();

    EXPECT_TRUE(ElementIn("terminal", "go").GetStates().hover);
    EXPECT_FALSE(ElementIn("terminal", "stop").GetStates().hover);
    EXPECT_FALSE(ElementIn("hud", "go").GetStates().hover) << "the window has a pointer of its own";

    _ui->SetPointer(surface, 50, 250, false);
    Frame();

    EXPECT_FALSE(ElementIn("terminal", "go").GetStates().hover);
    EXPECT_TRUE(ElementIn("terminal", "stop").GetStates().hover);
  }

  TEST_F(UiSurfaceTest, APointerStaysWhereItWasPutUntilItIsTakenAway)
  {
    const int surface = ShowTerminal();

    _ui->SetPointer(surface, 350, 250, false);
    Frame();
    Frame();
    Frame();

    EXPECT_TRUE(ElementIn("terminal", "go").GetStates().hover);

    _ui->ClearPointer(surface);
    Frame();

    EXPECT_FALSE(ElementIn("terminal", "go").GetStates().hover);
  }

  TEST_F(UiSurfaceTest, APointerIsSaidInPartsOfTheSurfaceAsWell)
  {
    const int surface = ShowTerminal(800, 600);

    // the middle of the button, which is from three quarters to the right
    // edge and from two thirds to the bottom
    _ui->SetPointerUv(surface, 0.875f, 0.8333f, false);
    Frame();

    EXPECT_TRUE(ElementIn("terminal", "go").GetStates().hover);

    _ui->SetPointerUv(surface, 0.5f, 0.5f, false);
    Frame();

    EXPECT_FALSE(ElementIn("terminal", "go").GetStates().hover);
  }

  TEST_F(UiSurfaceTest, APointerInPixelsIsInPixelsOfTheSurface)
  {
    const int surface = ShowTerminal(800, 600);

    // the button is at 300, 200 in units of the file, which is 600, 400
    _ui->SetPointer(surface, 350, 250, false);
    Frame();
    EXPECT_FALSE(ElementIn("terminal", "go").GetStates().hover);

    _ui->SetPointer(surface, 700, 500, false);
    Frame();
    EXPECT_TRUE(ElementIn("terminal", "go").GetStates().hover);
  }

  TEST_F(UiSurfaceTest, ThePointerOfTheWindowIsNotOverWhatIsShownOnASurface)
  {
    ShowTerminal();
    ShowHud();

    PointAt(350, 250);
    Frame();

    EXPECT_FALSE(ElementIn("terminal", "go").GetStates().hover);

    PointAt(50, 150);
    Frame();

    EXPECT_TRUE(ElementIn("hud", "go").GetStates().hover);
    EXPECT_FALSE(ElementIn("terminal", "stop").GetStates().hover);
  }

  TEST_F(UiSurfaceTest, TheWindowIsToldWhereThePlayerPointsByTheInputAlone)
  {
    ShowHud();

    _ui->SetPointer(Ui_Window_Surface, 50, 150, false);
    _ui->SetPointer(99, 50, 150, false);
    _ui->ClearPointer(99);
    Frame();

    EXPECT_FALSE(ElementIn("hud", "go").GetStates().hover);
  }

  // events

  TEST_F(UiSurfaceTest, AClickOnASurfaceSaysWhereItWas)
  {
    const int surface = ShowTerminal();
    ShowHud();
    Frame();

    ClickOn(surface, 350, 250);

    ASSERT_EQ(_ui->GetEvents().size(), 1u);

    const UiEvent &event = _ui->GetEvents()[0];
    EXPECT_EQ(event.kind, UiEvent::Kind::Click);
    EXPECT_EQ(event.element, "go");
    EXPECT_EQ(event.document, "terminal");
    EXPECT_EQ(event.surface, "terminal");
  }

  TEST_F(UiSurfaceTest, AClickOnTheWindowSaysSoAsWell)
  {
    ShowTerminal();
    ShowHud();
    Frame();

    ClickAt(50, 150);

    ASSERT_EQ(_ui->GetEvents().size(), 1u);
    EXPECT_EQ(_ui->GetEvents()[0].document, "hud");
    EXPECT_EQ(_ui->GetEvents()[0].surface, "window");
  }

  TEST_F(UiSurfaceTest, TellsTwoElementsOfOneNameApartByTheirUserInterface)
  {
    const int surface = ShowTerminal();
    ShowHud();
    Frame();

    ClickOn(surface, 350, 250);

    EXPECT_TRUE(_ui->WasClickedIn("terminal", "go"));
    EXPECT_FALSE(_ui->WasClickedIn("hud", "go"));
    EXPECT_FALSE(_ui->WasClickedIn("terminal", "stop"));
    EXPECT_TRUE(_ui->WasClicked("go")) << "asked without a user interface, either counts";

    ClickAt(50, 150);

    EXPECT_TRUE(_ui->WasClickedIn("hud", "go"));
    EXPECT_FALSE(_ui->WasClickedIn("terminal", "go"));
  }

  TEST_F(UiSurfaceTest, CallsWhatListensToOneUserInterface)
  {
    const int surface = ShowTerminal();
    ShowHud();
    Frame();

    int in_terminal = 0;
    int in_hud = 0;
    int anywhere = 0;

    _ui->OnClickIn("terminal", "go", [&in_terminal] { in_terminal++; });
    _ui->OnClickIn("hud", "go", [&in_hud] { in_hud++; });
    _ui->OnClick("stop", [&anywhere] { anywhere++; });

    ClickOn(surface, 350, 250);
    EXPECT_EQ(in_terminal, 1);
    EXPECT_EQ(in_hud, 0);

    ClickAt(50, 150);
    EXPECT_EQ(in_terminal, 1);
    EXPECT_EQ(in_hud, 1);

    // what listens to a name alone is called for every user interface
    ClickOn(surface, 50, 250);
    EXPECT_EQ(anywhere, 1);

    // and no longer once it is taken away
    _ui->OnClickIn("terminal", "go", nullptr);
    ClickOn(surface, 350, 250);
    EXPECT_EQ(in_terminal, 1);
  }

  TEST_F(UiSurfaceTest, APressAndAReleaseOnTwoSurfacesAreNoClick)
  {
    const int surface = ShowTerminal();
    ShowHud();
    Frame();

    // pressed on the terminal, and released on the window
    _ui->SetPointer(surface, 350, 250, true);
    Frame();

    _ui->SetPointer(surface, 350, 250, false);
    _ui->ClearPointer(surface);

    Release();
    PointAt(50, 150);
    Frame();

    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  TEST_F(UiSurfaceTest, SeveralSurfacesArePointedAtInOneFrame)
  {
    const int terminal = ShowTerminal();

    const int second = _ui->CreateSurface("second", 400, 300);
    WriteAsset(
      "ui/second.ui.yml",
      "ui: second\n"
      "root:\n"
      "  type: button\n"
      "  name: go\n"
      "  width: 100%\n"
      "  height: 100%\n");
    ASSERT_GE(_ui->LoadOnto(second, "assets://ui/second.ui.yml"), 0);
    Frame();

    _ui->SetPointer(terminal, 350, 250, true);
    _ui->SetPointer(second, 10, 10, true);
    Frame();

    EXPECT_TRUE(ElementIn("terminal", "go").GetStates().active);
    EXPECT_TRUE(ElementIn("second", "go").GetStates().active);

    _ui->SetPointer(terminal, 350, 250, false);
    _ui->SetPointer(second, 10, 10, false);
    Frame();

    EXPECT_TRUE(_ui->WasClickedIn("terminal", "go"));
    EXPECT_TRUE(_ui->WasClickedIn("second", "go"));
    EXPECT_EQ(_ui->GetEvents().size(), 2u);
  }

  // what is left for the game

  TEST_F(UiSurfaceTest, APointerOnASurfaceTakesNothingAwayFromTheGame)
  {
    const int surface = ShowTerminal();

    _input.state.SetPointer(350, 250);
    _input.state.SetAction(Action::Pointer_Primary);
    _ui->SetPointer(surface, 350, 250, true);
    Frame();

    // the game sees its pointer and its button as they are
    const auto &game = _ui->GetGameInput()->GetInputState();
    EXPECT_TRUE(game.HasPointer());
    EXPECT_TRUE(game[Action::Pointer_Primary]);
  }

  TEST_F(UiSurfaceTest, AMenuOnTheWindowDoesNotHoldBackASurface)
  {
    const int surface = ShowTerminal();

    ASSERT_GE(Show(
      "ui: menu\n"
      "modal: true\n"
      "root:\n"
      "  type: button\n"
      "  name: resume\n"
      "  text: Resume\n"), 0);
    Frame();

    ClickOn(surface, 350, 250);

    EXPECT_TRUE(_ui->WasClickedIn("terminal", "go"));
  }

  TEST_F(UiSurfaceTest, AMenuOnASurfaceDoesNotHoldBackTheGame)
  {
    const int surface = _ui->CreateSurface("terminal", 400, 300);

    WriteAsset(
      "ui/menu.ui.yml",
      "ui: menu\n"
      "modal: true\n"
      "root:\n"
      "  type: button\n"
      "  name: resume\n"
      "  text: Resume\n");
    ASSERT_GE(_ui->LoadOnto(surface, "assets://ui/menu.ui.yml"), 0);

    _input.state.SetAction(Action::L_Up);
    Frame();

    EXPECT_TRUE(_ui->GetGameInput()->GetInputState()[Action::L_Up]);
  }

  // keys and controller

  TEST_F(UiSurfaceTest, TheKeysBelongToTheWindowUntilTheGameSaysOtherwise)
  {
    ShowTerminal();
    ShowHud();
    Frame();

    Press(Action::Ui_Down);

    EXPECT_EQ(_ui->GetFocused(), "go");
    EXPECT_TRUE(ElementIn("hud", "go").GetStates().focus);
    EXPECT_FALSE(ElementIn("terminal", "go").GetStates().focus);

    // an event is there in the frame the key goes down in
    Release();
    _input.state.SetAction(Action::Ui_Accept);
    Frame();

    EXPECT_TRUE(_ui->WasClickedIn("hud", "go"));
    EXPECT_FALSE(_ui->WasClickedIn("terminal", "go"));
  }

  TEST_F(UiSurfaceTest, HandsTheKeysToASurface)
  {
    const int surface = ShowTerminal();
    ShowHud();
    Frame();

    ASSERT_TRUE(_ui->SetInputSurface(surface));
    EXPECT_EQ(_ui->GetInputSurface(), surface);

    // the focus goes to what is shown there, and moves among it
    Press(Action::Ui_Down);
    EXPECT_TRUE(ElementIn("terminal", "go").GetStates().focus || ElementIn("terminal", "stop").GetStates().focus);
    EXPECT_FALSE(ElementIn("hud", "go").GetStates().focus);

    ASSERT_TRUE(_ui->Focus("stop"));
    Frame();
    EXPECT_TRUE(ElementIn("terminal", "stop").GetStates().focus);

    Press(Action::Ui_Right);
    EXPECT_TRUE(ElementIn("terminal", "go").GetStates().focus);

    Release();
    _input.state.SetAction(Action::Ui_Accept);
    Frame();

    EXPECT_TRUE(_ui->WasClickedIn("terminal", "go"));
    EXPECT_FALSE(_ui->WasClickedIn("hud", "go"));
  }

  TEST_F(UiSurfaceTest, WhatHadTheFocusLosesItWhenTheKeysGoElsewhere)
  {
    const int surface = ShowTerminal();
    ShowHud();
    Frame();

    Press(Action::Ui_Down);
    ASSERT_TRUE(ElementIn("hud", "go").GetStates().focus);

    ASSERT_TRUE(_ui->SetInputSurface(surface));
    Frame();

    EXPECT_EQ(_ui->GetFocused(), "");
    EXPECT_FALSE(ElementIn("hud", "go").GetStates().focus);

    // and the keys no longer reach the window
    Release();
    _input.state.SetAction(Action::Ui_Accept);
    Frame();
    EXPECT_TRUE(_ui->GetEvents().empty());
    Release();
    Frame();

    ASSERT_TRUE(_ui->SetInputSurface(Ui_Window_Surface));
    Press(Action::Ui_Down);
    EXPECT_TRUE(ElementIn("hud", "go").GetStates().focus);
  }

  TEST_F(UiSurfaceTest, AClickOnASurfaceWithoutTheKeysMovesNoFocus)
  {
    const int surface = ShowTerminal();
    ShowHud();
    Frame();

    Press(Action::Ui_Down);
    ASSERT_EQ(_ui->GetFocused(), "go");
    ASSERT_TRUE(ElementIn("hud", "go").GetStates().focus);

    ClickOn(surface, 50, 250);

    EXPECT_TRUE(_ui->WasClickedIn("terminal", "stop"));
    EXPECT_TRUE(ElementIn("hud", "go").GetStates().focus) << "the focus stays where the keys are";
  }

  TEST_F(UiSurfaceTest, TheKeysComeBackToTheWindowWhenTheirSurfaceGoes)
  {
    const int surface = ShowTerminal();
    ShowHud();

    ASSERT_TRUE(_ui->SetInputSurface(surface));
    _ui->DestroySurface(surface);

    EXPECT_EQ(_ui->GetInputSurface(), Ui_Window_Surface);
    EXPECT_FALSE(_ui->SetInputSurface(surface));
    EXPECT_FALSE(_ui->SetInputSurface(99));
  }

  // scenes

  class UiSurfaceSceneTest : public UiSurfaceTest
  {
  protected:
    FakeEntityStore _store;
    std::unique_ptr<UiSurfaceLoading> _loading;

    void SetUp() override
    {
      UiSurfaceTest::SetUp();

      _store.Initialize();
      _loading = std::make_unique<UiSurfaceLoading>(_ui.get());
      _loading->Register(_store);
      _loading->Initialize(_store);
    }

    void TearDown() override
    {
      // the store first, as an application does it
      _store.CleanUp();
      UiSurfaceTest::TearDown();
    }

    bool Populate(const std::string &yaml)
    {
      WriteAsset("scenes/test.scene.yml", yaml);

      SceneFile scene(&_file_system, &_yaml, "assets://scenes/test.scene.yml", _logger);
      scene.GetComponentFormats().Add(neon::UiSurfaceFormat());

      try
      {
        scene.Populate(_store);
      } catch (const std::exception &)
      {
        return false;
      }
      return true;
    }
  };

  TEST_F(UiSurfaceSceneTest, ASceneSaysWhichEntityCarriesAUserInterface)
  {
    ASSERT_TRUE(Populate(
      "scene: test\n"
      "entities:\n"
      "  - name: monitor\n"
      "    components:\n"
      "      UiSurface:\n"
      "        ui: assets://ui/terminal.ui.yml\n"
      "        name: terminal\n"
      "        size: [800, 600]\n")) << _logger->Messages(LogLevel::Error);

    _loading->Update(_store, 0.016);
    Frame();

    const int surface = _ui->FindSurface("terminal");
    ASSERT_NE(surface, No_Ui_Surface);

    const auto *target = _renderer.TargetOf("terminal");
    ASSERT_NE(target, nullptr);
    EXPECT_EQ(target->width, 800);
    EXPECT_EQ(target->height, 600);
    EXPECT_EQ(target->begun, 1u);

    EXPECT_NE(_ui->FindIn("terminal", "go"), nullptr);
    EXPECT_TRUE(_renderer.batches.empty()) << "nothing of it is shown on the window";
  }

  TEST_F(UiSurfaceSceneTest, MakesASurfaceOnce)
  {
    ASSERT_TRUE(Populate(
      "scene: test\n"
      "entities:\n"
      "  - name: monitor\n"
      "    components:\n"
      "      UiSurface:\n"
      "        ui: assets://ui/terminal.ui.yml\n"
      "        name: terminal\n"));

    _loading->Update(_store, 0.016);
    _loading->Update(_store, 0.016);
    _loading->Update(_store, 0.016);

    EXPECT_EQ(_renderer.targets_created, 1u);
    EXPECT_EQ(_renderer.TargetOf("terminal")->width, 1024) << "the size it has when none is written";
  }

  TEST_F(UiSurfaceSceneTest, TheSurfaceGoesWithItsEntity)
  {
    ASSERT_TRUE(Populate(
      "scene: test\n"
      "entities:\n"
      "  - name: monitor\n"
      "    components:\n"
      "      UiSurface:\n"
      "        ui: assets://ui/terminal.ui.yml\n"
      "        name: terminal\n"));

    _loading->Update(_store, 0.016);
    ASSERT_NE(_ui->FindSurface("terminal"), No_Ui_Surface);

    _store.DestroyEntity(_store.FindEntity("monitor"));

    EXPECT_EQ(_ui->FindSurface("terminal"), No_Ui_Surface);
    EXPECT_EQ(_ui->FindIn("terminal", "go"), nullptr);
    EXPECT_EQ(_renderer.targets_destroyed, 1u);
  }

  TEST_F(UiSurfaceSceneTest, ASurfaceThatCannotBeMadeEndsTheRun)
  {
    _renderer.refused_targets.push_back("terminal");

    ASSERT_TRUE(Populate(
      "scene: test\n"
      "entities:\n"
      "  - name: monitor\n"
      "    components:\n"
      "      UiSurface:\n"
      "        ui: assets://ui/terminal.ui.yml\n"
      "        name: terminal\n"));

    EXPECT_THROW(_loading->Update(_store, 0.016), std::runtime_error);

    // and is not tried again
    _loading->Update(_store, 0.016);
    EXPECT_EQ(_renderer.targets_created, 1u);
    EXPECT_EQ(Errors().size(), 1u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(UiSurfaceSceneTest, SaysWhatIsWrongWithTheComponent)
  {
    EXPECT_FALSE(Populate(
      "scene: test\n"
      "entities:\n"
      "  - name: monitor\n"
      "    components:\n"
      "      UiSurface:\n"
      "        size: [800]\n"
      "        scale: 0\n"
      "        colour: red\n"));

    EXPECT_THAT(
      Errors(),
      ::testing::IsSupersetOf({
        std::string("assets://scenes/test.scene.yml:5: UiSurface of entity 'monitor' has no 'ui', where the "
          "virtual path of a user interface was expected"),
        std::string("assets://scenes/test.scene.yml:5: UiSurface of entity 'monitor' has no 'name', where what "
          "the surface is called was expected. A model shows the surface as the texture surface:// and the "
          "name"),
        std::string("assets://scenes/test.scene.yml:6: 'size' of UiSurface of entity 'monitor' is another list, "
          "where a list of 2 numbers above 0 was expected, such as [1024, 768]"),
        std::string("assets://scenes/test.scene.yml:7: 'scale' of UiSurface of entity 'monitor' is 0, where a "
          "number above 0 was expected"),
        std::string("assets://scenes/test.scene.yml:8: 'colour' is not known to UiSurface of entity 'monitor'. "
          "Known are: ui, name, size, reach, scale")
      })) << _logger->Messages(LogLevel::Error);
  }
}
