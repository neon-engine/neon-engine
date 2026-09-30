#include "ui-fixture.hpp"

// What reaches the game: a click on a button, as an event it asks for and
// as a function it is called with.

namespace
{
  using neon::Action;
  using neon::UiEvent;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;

  class UiEventTest : public UiTest
  {
  protected:
    /// Two buttons of 100 by 50 next to each other, from the left top
    /// corner of the frame.
    void ShowMenu()
    {
      ASSERT_GE(Show(
        "ui: menu\n"
        "root:\n"
        "  type: panel\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  align_items: flex-start\n"
        "  children:\n"
        "    - type: button\n"
        "      name: start\n"
        "      text: Start\n"
        "      box_sizing: border-box\n"
        "      width: 100\n"
        "      height: 50\n"
        "    - type: button\n"
        "      name: quit\n"
        "      text: Quit\n"
        "      box_sizing: border-box\n"
        "      width: 100\n"
        "      height: 50\n"), 0) << _logger->Messages(LogLevel::Error);
    }
  };

  TEST_F(UiEventTest, NothingHappensWhileNothingIsDone)
  {
    ShowMenu();
    Frame();

    EXPECT_TRUE(_ui->GetEvents().empty());
    EXPECT_FALSE(_ui->WasClicked("start"));
  }

  TEST_F(UiEventTest, AClickOnAButtonIsAnEventWithItsName)
  {
    ShowMenu();
    ClickAt(50, 25);

    ASSERT_EQ(_ui->GetEvents().size(), 1u);

    const UiEvent &event = _ui->GetEvents().front();
    EXPECT_EQ(event.kind, UiEvent::Kind::Click);
    EXPECT_EQ(event.element, "start");
    EXPECT_EQ(event.document, "menu");

    EXPECT_TRUE(_ui->WasClicked("start"));
    EXPECT_FALSE(_ui->WasClicked("quit"));
  }

  TEST_F(UiEventTest, AnEventIsThereForOneFrame)
  {
    ShowMenu();
    ClickAt(50, 25);
    ASSERT_TRUE(_ui->WasClicked("start"));

    Frame();

    EXPECT_TRUE(_ui->GetEvents().empty());
    EXPECT_FALSE(_ui->WasClicked("start"));
  }

  TEST_F(UiEventTest, EventsStayUntilTheNextUpdateAndNotUntilTheNextDraw)
  {
    ShowMenu();
    ClickAt(50, 25);

    // what the game reads between the two
    _ui->Draw();
    _ui->Draw();

    EXPECT_TRUE(_ui->WasClicked("start"));
  }

  TEST_F(UiEventTest, AClickCallsTheFunctionOfItsName)
  {
    ShowMenu();

    int started = 0;
    int quit = 0;
    _ui->OnClick("start", [&started] { started++; });
    _ui->OnClick("quit", [&quit] { quit++; });

    ClickAt(50, 25);

    EXPECT_EQ(started, 1);
    EXPECT_EQ(quit, 0);

    ClickAt(150, 25);
    ClickAt(150, 25);

    EXPECT_EQ(started, 1);
    EXPECT_EQ(quit, 2);
  }

  TEST_F(UiEventTest, AFunctionCanBeRegisteredBeforeItsFileIsShown)
  {
    int started = 0;
    _ui->OnClick("start", [&started] { started++; });

    ShowMenu();
    ClickAt(50, 25);

    EXPECT_EQ(started, 1);
  }

  TEST_F(UiEventTest, ASecondFunctionReplacesTheFirst)
  {
    ShowMenu();

    int first = 0;
    int second = 0;
    _ui->OnClick("start", [&first] { first++; });
    _ui->OnClick("start", [&second] { second++; });

    ClickAt(50, 25);

    EXPECT_EQ(first, 0);
    EXPECT_EQ(second, 1);
  }

  TEST_F(UiEventTest, NoFunctionTakesTheFunctionAway)
  {
    ShowMenu();

    int started = 0;
    _ui->OnClick("start", [&started] { started++; });
    _ui->OnClick("start", {});

    ClickAt(50, 25);

    EXPECT_EQ(started, 0);

    // the event is there all the same
    EXPECT_TRUE(_ui->WasClicked("start"));
  }

  TEST_F(UiEventTest, AFunctionSeesTheEventItIsCalledFor)
  {
    ShowMenu();

    bool was_clicked = false;
    _ui->OnClick("start", [this, &was_clicked] { was_clicked = _ui->WasClicked("start"); });

    ClickAt(50, 25);

    EXPECT_TRUE(was_clicked);
  }

  TEST_F(UiEventTest, AFunctionCanTakeTheFileAwayThatItIsCalledFrom)
  {
    ASSERT_GE(Show(
      "root:\n  type: panel\n  name: hud\n  width: 10\n  height: 10\n", "ui/hud.ui.yml"), 0);

    const int menu = Show(
      "ui: menu\n"
      "modal: true\n"
      "root:\n"
      "  type: button\n"
      "  name: resume\n"
      "  text: Resume\n"
      "  width: 100\n"
      "  height: 50\n", "ui/menu.ui.yml");
    ASSERT_GE(menu, 0) << _logger->Messages(LogLevel::Error);

    _ui->OnClick("resume", [this, menu] { _ui->Unload(menu); });

    ClickAt(50, 25);

    EXPECT_EQ(_ui->Find("resume"), nullptr);
    EXPECT_NE(_ui->Find("hud"), nullptr);
    EXPECT_EQ(_ui->GetFocused(), "");

    // and the frames after it run as if it had never been there
    Frame();
    ClickAt(50, 25);
    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  TEST_F(UiEventTest, AFunctionCanShowAnotherFile)
  {
    ShowMenu();
    WriteAsset(
      "ui/options.ui.yml",
      "ui: options\nmodal: true\nroot:\n  type: button\n  name: back\n  text: Back\n");

    _ui->OnClick("start", [this] { _ui->Load("assets://ui/options.ui.yml"); });

    ClickAt(50, 25);
    Frame();

    EXPECT_NE(_ui->Find("back"), nullptr);
    EXPECT_EQ(_ui->GetFocused(), "back");
  }

  TEST_F(UiEventTest, AButtonWithoutANameReportsNothing)
  {
    ASSERT_GE(Show("root:\n  type: button\n  text: Start\n  width: 100\n  height: 50\n"), 0);

    ClickAt(50, 25);

    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  TEST_F(UiEventTest, AClickWithTheKeysIsTheSameEvent)
  {
    ShowMenu();
    ASSERT_TRUE(_ui->Focus("quit"));

    int quit = 0;
    _ui->OnClick("quit", [&quit] { quit++; });

    Release();
    _input.state.SetAction(Action::Ui_Accept);
    Frame();

    ASSERT_EQ(_ui->GetEvents().size(), 1u);
    EXPECT_EQ(_ui->GetEvents().front().element, "quit");
    EXPECT_EQ(_ui->GetEvents().front().document, "menu");
    EXPECT_EQ(quit, 1);
  }

  TEST_F(UiEventTest, EventsOfOneFrameAreInTheOrderTheyHappenedIn)
  {
    ShowMenu();
    ASSERT_TRUE(_ui->Focus("quit"));

    // the pointer lets go of one button in the frame the key is pressed
    // for the other
    Release();
    PointAt(50, 25);
    _input.state.SetAction(Action::Pointer_Primary);
    Frame();

    Release();
    ASSERT_TRUE(_ui->Focus("quit"));
    _input.state.SetAction(Action::Ui_Accept);
    Frame();

    ASSERT_EQ(_ui->GetEvents().size(), 2u);
    EXPECT_EQ(_ui->GetEvents()[0].element, "start");
    EXPECT_EQ(_ui->GetEvents()[1].element, "quit");
  }

  TEST_F(UiEventTest, AFunctionThatThrowsEndsTheUpdate)
  {
    ShowMenu();
    _ui->OnClick("start", [] { throw std::runtime_error("the game could not start"); });

    Release();
    PointAt(50, 25);
    _input.state.SetAction(Action::Pointer_Primary);
    Frame();

    Release();
    EXPECT_THROW(_ui->Update(), std::runtime_error);

    // the user interface goes on afterwards
    Release();
    Frame();
    EXPECT_TRUE(_ui->GetEvents().empty());
  }
}
