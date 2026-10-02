#include "ui-fixture.hpp"

// Scrolling: what is larger than its box is cut off and scrolled to, by the
// wheel, the scrollbars, keys, a controller, and by dragging.

namespace
{
  using neon::Action;
  using neon::Key;
  using neon::UiElement;
  using neon::UiHandle;
  using neon::UiScrollbars;
  using neon::testing::LogLevel;
  using neon::testing::RecordedQuad;
  using neon::testing::UiTest;

  class UiScrollingTest : public UiTest
  {
  protected:
    /// A list of 300 by 200 at 100, 100 with rows of 40 that are named
    /// `row-0` and so on.
    void ShowList(const int rows, const std::string &of_list = "", const std::string &of_row = "")
    {
      std::string yaml = std::string() +
        "ui: test\n"
        "root:\n"
        "  type: panel\n"
        "  name: root\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  children:\n"
        "    - type: panel\n"
        "      name: list\n"
        "      position: absolute\n"
        "      left: 100\n"
        "      top: 100\n"
        "      width: 300\n"
        "      height: 200\n"
        "      flex_direction: column\n" +
        (of_list.find("overflow") == std::string::npos ? "      overflow_y: auto\n" : "") +
        Indented(of_list, "      ") +
        "      children:\n";

      for (int i = 0; i < rows; i++)
      {
        yaml +=
          "        - type: button\n"
          "          name: row-" + std::to_string(i) + "\n"
          "          text: Row\n"
          "          height: 40\n"
          "          box_sizing: border-box\n" +
          Indented(of_row, "          ");
      }

      ASSERT_GE(Show(yaml), 0) << _logger->Messages(LogLevel::Error);
      Frame();
    }

    [[nodiscard]] float ScrollY(const std::string &name = "list") const
    {
      return Element(name).GetScrollY();
    }

    [[nodiscard]] float ScrollX(const std::string &name = "list") const
    {
      return Element(name).GetScrollX();
    }

    [[nodiscard]] UiHandle Handle(const std::string &name) const
    {
      return _ui->FindByName(name);
    }

    /// A frame in which the wheel is turned.
    void Wheel(const double x, const double y, const bool precise = false)
    {
      Release();
      _input.state.AddWheel(x, y, precise);
      Frame();
      Release();
    }

    void PressKey(const Key key)
    {
      Release();
      _input.state.AddKeyEvent({key, true, false, {}});
      _input.state.AddKeyEvent({key, false, false, {}});
      Frame();
      Release();
    }

    /// A frame of a sixtieth of a second.
    void Tick()
    {
      _ui->Advance(1.0 / 60.0);
      Frame();
    }
  };

  // what does not fit

  TEST_F(UiScrollingTest, KeepsTheRowsAsHighAsTheyAskToBe)
  {
    ShowList(10);

    // 10 rows of 40 in a box of 200: they are not made to fit
    for (int i = 0; i < 10; i++)
    {
      ExpectBox("row-" + std::to_string(i), 100.0f, 100.0f + static_cast<float>(i) * 40.0f, 300.0f, 40.0f);
    }

    EXPECT_FLOAT_EQ(Element("list").GetContentHeight(), 400.0f);
    EXPECT_FLOAT_EQ(Element("list").GetMaxScrollY(), 200.0f);
    EXPECT_TRUE(Element("list").CanScrollY());
    EXPECT_FALSE(Element("list").CanScrollX());
  }

  TEST_F(UiScrollingTest, HasNothingToScrollToWhenEverythingFits)
  {
    ShowList(3);

    EXPECT_FLOAT_EQ(Element("list").GetContentHeight(), 200.0f);
    EXPECT_FLOAT_EQ(Element("list").GetMaxScrollY(), 0.0f);
    EXPECT_FALSE(Element("list").CanScrollY());

    PointAt(200, 150);
    Wheel(0, 3);
    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);
  }

  TEST_F(UiScrollingTest, CountsThePaddingBehindTheLastRow)
  {
    ShowList(10, "padding: 10\n");

    // 10 above, 400 of rows, and 10 below, in a padding box of 220
    EXPECT_FLOAT_EQ(Element("list").GetContentHeight(), 420.0f);
    EXPECT_FLOAT_EQ(Element("list").GetMaxScrollY(), 200.0f);
  }

  TEST_F(UiScrollingTest, ShrinksTheRowsWhereNothingIsScrolled)
  {
    // as flexbox does, where the layout takes the least size of an item
    // for 0
    ShowList(10, "overflow_y: hidden\n");

    EXPECT_NEAR(Element("row-0").GetBox().Height(), 20.0f, 0.01f);
    EXPECT_FLOAT_EQ(Element("list").GetMaxScrollY(), 0.0f);
  }

  TEST_F(UiScrollingTest, MovesWhatIsInsideByHowFarItIsScrolled)
  {
    ShowList(10);

    ASSERT_TRUE(_ui->SetScroll(Handle("list"), 0.0f, 60.0f));
    Frame();

    EXPECT_FLOAT_EQ(ScrollY(), 60.0f);
    ExpectBox("row-0", 100.0f, 40.0f, 300.0f, 40.0f);
    ExpectBox("row-5", 100.0f, 240.0f, 300.0f, 40.0f);

    // the list itself stays where it is
    ExpectBox("list", 100.0f, 100.0f, 300.0f, 200.0f);
  }

  TEST_F(UiScrollingTest, HoldsTheScrollPositionToWhatThereIsToScroll)
  {
    ShowList(10);

    _ui->SetScroll(Handle("list"), 0.0f, 5000.0f);
    EXPECT_FLOAT_EQ(ScrollY(), 200.0f);

    _ui->SetScroll(Handle("list"), 0.0f, -50.0f);
    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);

    // along a side that does not scroll, not at all
    _ui->SetScroll(Handle("list"), 80.0f, 100.0f);
    EXPECT_FLOAT_EQ(ScrollX(), 0.0f);
    EXPECT_FLOAT_EQ(ScrollY(), 100.0f);
  }

  TEST_F(UiScrollingTest, ReadsAndSetsTheScrollPosition)
  {
    ShowList(10);

    float x = -1.0f;
    float y = -1.0f;
    ASSERT_TRUE(_ui->GetScroll(Handle("list"), x, y));
    EXPECT_FLOAT_EQ(x, 0.0f);
    EXPECT_FLOAT_EQ(y, 0.0f);

    ASSERT_TRUE(_ui->SetScroll(Handle("list"), 0.0f, 120.0f));
    ASSERT_TRUE(_ui->GetScroll(Handle("list"), x, y));
    EXPECT_FLOAT_EQ(y, 120.0f);

    EXPECT_FALSE(_ui->GetScroll(UiHandle{12345}, x, y));
    EXPECT_FALSE(_ui->SetScroll(UiHandle{12345}, 0.0f, 0.0f));
  }

  TEST_F(UiScrollingTest, ComesBackToWhatThereIsToScrollWhenWhatIsInsideGetsSmaller)
  {
    ShowList(10);
    _ui->SetScroll(Handle("list"), 0.0f, 200.0f);
    Frame();

    for (int i = 5; i < 10; i++) { ASSERT_TRUE(_ui->Remove(Handle("row-" + std::to_string(i)))); }
    Frame();

    EXPECT_FLOAT_EQ(Element("list").GetContentHeight(), 200.0f);
    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);
  }

  // what is drawn

  TEST_F(UiScrollingTest, CutsOffWhatIsOutsideOfTheBox)
  {
    ShowList(10, "scrollbar_width: none\n");

    for (const RecordedQuad &quad : _renderer.Quads())
    {
      if (!quad.clipped) { continue; }

      EXPECT_EQ(quad.clip.x, 100);
      EXPECT_EQ(quad.clip.y, 100);
      EXPECT_EQ(quad.clip.width, 300);
      EXPECT_EQ(quad.clip.height, 200);
    }
  }

  TEST_F(UiScrollingTest, DrawsOnlyTheRowsThatCanBeSeen)
  {
    ShowList(1000, "scrollbar_width: none\n");

    // the background of a row, which is that of a button
    const auto rows = [this]
    {
      std::size_t count = 0;
      for (const RecordedQuad &quad : _renderer.Quads())
      {
        if (!quad.textured && quad.Width() == 300.0f && quad.Height() == 40.0f) { count++; }
      }
      return count;
    };

    // 200 of height hold 5 rows of 40
    EXPECT_EQ(rows(), 5u);

    _ui->SetScroll(Handle("list"), 0.0f, 20000.0f + 20.0f);
    Frame();

    // and 6 when the first and the last are cut in half
    EXPECT_EQ(rows(), 6u);

    // far fewer than there are rows, text included
    EXPECT_LT(_renderer.Quads().size(), 60u);
  }

  TEST_F(UiScrollingTest, SaysWhatCanBeSeenOfAnElement)
  {
    ShowList(10);

    EXPECT_FALSE(Element("row-0").IsClippedAway());
    EXPECT_FALSE(Element("row-4").IsClippedAway());
    EXPECT_TRUE(Element("row-5").IsClippedAway());
    EXPECT_TRUE(Element("row-9").IsClippedAway());

    _ui->SetScroll(Handle("list"), 0.0f, 100.0f);
    Frame();

    EXPECT_TRUE(Element("row-1").IsClippedAway());

    // half of it
    EXPECT_FALSE(Element("row-2").IsClippedAway());
    EXPECT_FLOAT_EQ(Element("row-2").GetVisibleBox().top, 100.0f);
    EXPECT_FLOAT_EQ(Element("row-2").GetVisibleBox().bottom, 120.0f);
    EXPECT_FALSE(Element("row-7").IsClippedAway());
    EXPECT_TRUE(Element("row-8").IsClippedAway());
  }

  TEST_F(UiScrollingTest, DoesNotPointAtWhatIsCutOff)
  {
    ShowList(10);

    // row-5 would be at 300 to 340, below the list
    ClickAt(200, 320);
    EXPECT_TRUE(_ui->GetEvents().empty());

    _ui->SetScroll(Handle("list"), 0.0f, 100.0f);
    Frame();

    // at 100 of scroll, row-3 is at 120 to 160
    ClickAt(200, 140);
    EXPECT_TRUE(_ui->WasClicked("row-3"));
  }

  TEST_F(UiScrollingTest, PlacesNothingAgainWhenItIsScrolled)
  {
    ShowList(100);
    Frame();

    const auto before = _ui->GetStatistics();

    _ui->SetScroll(Handle("list"), 0.0f, 500.0f);
    Frame();

    const auto &after = _ui->GetStatistics();
    EXPECT_EQ(after.layouts, before.layouts);
    EXPECT_EQ(after.styles, before.styles);
    EXPECT_EQ(after.places, before.places + 1);
    EXPECT_EQ(after.paints, before.paints + 1);
  }

  // the wheel

  TEST_F(UiScrollingTest, ScrollsWithTheWheelUnderThePointer)
  {
    ShowList(20);

    PointAt(200, 150);
    Wheel(0, 1);

    // a notch is three lines
    EXPECT_FLOAT_EQ(ScrollY(), 60.0f);

    Wheel(0, 2);
    EXPECT_FLOAT_EQ(ScrollY(), 180.0f);

    Wheel(0, -1);
    EXPECT_FLOAT_EQ(ScrollY(), 120.0f);
  }

  TEST_F(UiScrollingTest, ScrollsByPartsOfANotchWithATrackpad)
  {
    ShowList(20);

    PointAt(200, 150);
    Wheel(0, 0.25, true);
    EXPECT_FLOAT_EQ(ScrollY(), 15.0f);

    Wheel(0, 0.1, true);
    EXPECT_FLOAT_EQ(ScrollY(), 21.0f);
  }

  TEST_F(UiScrollingTest, DoesNotScrollWhatThePointerIsNotOver)
  {
    ShowList(20);

    PointAt(600, 600);
    Wheel(0, 1);
    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);

    _input.state.ClearPointer();
    Wheel(0, 1);
    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);
  }

  TEST_F(UiScrollingTest, ScrollsOverWhatTakesNoPointer)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: list\n"
      "  width: 300\n"
      "  height: 200\n"
      "  flex_direction: column\n"
      "  overflow_y: scroll\n"
      "  children:\n"
      "    - type: label\n"
      "      text: a\n"
      "      height: 1000\n"), 0);
    Frame();

    PointAt(50, 50);
    Wheel(0, 1);
    EXPECT_FLOAT_EQ(ScrollY(), 60.0f);
  }

  TEST_F(UiScrollingTest, KeepsTheWheelFromTheGameOverWhatScrolls)
  {
    ShowList(20);

    Release();
    PointAt(200, 150);
    _input.state.AddWheel(0, 1, false);
    Frame();
    EXPECT_EQ(_ui->GetGameInput()->GetInputState().GetWheel().y, 0.0);

    Release();
    PointAt(900, 900);
    _input.state.AddWheel(0, 1, false);
    Frame();
    EXPECT_EQ(_ui->GetGameInput()->GetInputState().GetWheel().y, 1.0);
  }

  TEST_F(UiScrollingTest, ScrollsSidewaysWithAWheelThatOnlyTurnsOneWay)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: list\n"
      "  width: 300\n"
      "  height: 100\n"
      "  overflow_x: auto\n"
      "  children:\n"
      "    - type: panel\n"
      "      width: 1000\n"
      "      height: 50\n"), 0);
    Frame();

    EXPECT_FLOAT_EQ(Element("list").GetMaxScrollX(), 700.0f);

    PointAt(50, 50);
    Wheel(0, 1);
    EXPECT_FLOAT_EQ(ScrollX(), 60.0f);

    Wheel(2, 0);
    EXPECT_FLOAT_EQ(ScrollX(), 180.0f);
  }

  // what is inside of what

  class UiNestedScrollingTest : public UiScrollingTest
  {
  protected:
    /// A list of 300 by 200 that holds a list of 300 by 100 and rows
    /// behind it. The inner list holds 300 of height, the outer 500.
    void ShowNested()
    {
      ASSERT_GE(ShowUnderRoot(
        "- type: panel\n"
        "  name: outer\n"
        "  width: 300\n"
        "  height: 200\n"
        "  flex_direction: column\n"
        "  overflow_y: auto\n"
        "  children:\n"
        "    - type: panel\n"
        "      name: inner\n"
        "      height: 100\n"
        "      flex_direction: column\n"
        "      overflow_y: auto\n"
        "      children:\n"
        "        - type: panel\n"
        "          height: 300\n"
        "    - type: panel\n"
        "      height: 400\n"), 0);
      Frame();
    }
  };

  TEST_F(UiNestedScrollingTest, ScrollsTheInnermostThatHasSomethingLeftToScrollTo)
  {
    ShowNested();

    EXPECT_FLOAT_EQ(Element("inner").GetMaxScrollY(), 200.0f);
    EXPECT_FLOAT_EQ(Element("outer").GetMaxScrollY(), 300.0f);

    PointAt(50, 50);
    Wheel(0, 2);
    EXPECT_FLOAT_EQ(ScrollY("inner"), 120.0f);
    EXPECT_FLOAT_EQ(ScrollY("outer"), 0.0f);

    Wheel(0, 2);
    EXPECT_FLOAT_EQ(ScrollY("inner"), 200.0f);
    EXPECT_FLOAT_EQ(ScrollY("outer"), 0.0f);

    // the inner one is at its end, and the outer one takes the wheel
    Wheel(0, 1);
    EXPECT_FLOAT_EQ(ScrollY("inner"), 200.0f);
    EXPECT_FLOAT_EQ(ScrollY("outer"), 60.0f);
  }

  TEST_F(UiNestedScrollingTest, ScrollsTheOuterOneBackWhenTheInnerOneIsAtItsStart)
  {
    ShowNested();

    _ui->SetScroll(Handle("outer"), 0.0f, 50.0f);
    Frame();

    // the inner list is at -50 to 50 by now
    PointAt(50, 25);
    Wheel(0, -1);

    EXPECT_FLOAT_EQ(ScrollY("inner"), 0.0f);
    EXPECT_FLOAT_EQ(ScrollY("outer"), 0.0f);
  }

  TEST_F(UiNestedScrollingTest, ScrollsTheOuterOneNextToTheInnerOne)
  {
    ShowNested();

    PointAt(50, 150);
    Wheel(0, 1);

    EXPECT_FLOAT_EQ(ScrollY("inner"), 0.0f);
    EXPECT_FLOAT_EQ(ScrollY("outer"), 60.0f);
  }

  // scrollbars

  TEST_F(UiScrollingTest, PutsTheScrollbarAtTheRightOfThePaddingBox)
  {
    ShowList(10, "border_width: 5\n");

    // the border box is 310 by 210 by now, and the padding box 300 by 200
    const UiElement &list = Element("list");
    const auto track = UiScrollbars::TrackOf(list, UiScrollbars::Axis::Vertical);

    EXPECT_FLOAT_EQ(track.left, 105.0f + 300.0f - 12.0f);
    EXPECT_FLOAT_EQ(track.right, 405.0f);
    EXPECT_FLOAT_EQ(track.top, 105.0f);
    EXPECT_FLOAT_EQ(track.bottom, 305.0f);

    EXPECT_TRUE(UiScrollbars::TrackOf(list, UiScrollbars::Axis::Horizontal).IsEmpty());
  }

  TEST_F(UiScrollingTest, MakesTheThumbAsLongAsThePartThatIsSeen)
  {
    ShowList(10);

    // 200 of 400 are seen: half of a track of 200
    const UiElement &list = Element("list");
    auto thumb = UiScrollbars::ThumbOf(list, UiScrollbars::Axis::Vertical);

    EXPECT_FLOAT_EQ(thumb.top, 100.0f);
    EXPECT_FLOAT_EQ(thumb.Height(), 100.0f);
    EXPECT_FLOAT_EQ(thumb.Width(), 12.0f);

    _ui->SetScroll(Handle("list"), 0.0f, 100.0f);
    thumb = UiScrollbars::ThumbOf(list, UiScrollbars::Axis::Vertical);
    EXPECT_FLOAT_EQ(thumb.top, 150.0f);

    _ui->SetScroll(Handle("list"), 0.0f, 200.0f);
    thumb = UiScrollbars::ThumbOf(list, UiScrollbars::Axis::Vertical);
    EXPECT_FLOAT_EQ(thumb.top, 200.0f);
    EXPECT_FLOAT_EQ(thumb.bottom, 300.0f);
  }

  TEST_F(UiScrollingTest, KeepsTheThumbLongEnoughToTakeHoldOf)
  {
    ShowList(1000);

    const auto thumb = UiScrollbars::ThumbOf(Element("list"), UiScrollbars::Axis::Vertical);
    EXPECT_FLOAT_EQ(thumb.Height(), UiScrollbars::least_thumb);

    _ui->SetScroll(Handle("list"), 0.0f, 1.0e6f);
    EXPECT_FLOAT_EQ(UiScrollbars::ThumbOf(Element("list"), UiScrollbars::Axis::Vertical).bottom, 300.0f);
  }

  TEST_F(UiScrollingTest, ShowsNoScrollbarForAutoWhenEverythingFits)
  {
    ShowList(3);
    EXPECT_FALSE(UiScrollbars::IsShown(Element("list"), UiScrollbars::Axis::Vertical));

    // and keeps no room for one
    ExpectBox("row-0", 100.0f, 100.0f, 300.0f, 40.0f);
  }

  TEST_F(UiScrollingTest, KeepsTheRoomOfTheScrollbarForScroll)
  {
    ShowList(3, "overflow_y: scroll\n");

    EXPECT_TRUE(UiScrollbars::IsShown(Element("list"), UiScrollbars::Axis::Vertical));

    // the rows end where the scrollbar starts
    ExpectBox("row-0", 100.0f, 100.0f, 288.0f, 40.0f);
  }

  TEST_F(UiScrollingTest, TakesTheWidthOfTheScrollbarFromTheStyle)
  {
    ShowList(10, "overflow_y: scroll\nscrollbar_width: thin\n");
    ExpectBox("row-0", 100.0f, 100.0f, 292.0f, 40.0f);
    EXPECT_FLOAT_EQ(Element("list").GetScrollbarWidth(), 8.0f);
  }

  TEST_F(UiScrollingTest, HidesTheScrollbarAndScrollsAllTheSame)
  {
    ShowList(10, "overflow_y: scroll\nscrollbar_width: none\n");

    EXPECT_FALSE(UiScrollbars::IsShown(Element("list"), UiScrollbars::Axis::Vertical));
    ExpectBox("row-0", 100.0f, 100.0f, 300.0f, 40.0f);

    PointAt(200, 150);
    Wheel(0, 1);
    EXPECT_FLOAT_EQ(ScrollY(), 60.0f);
  }

  TEST_F(UiScrollingTest, DrawsTheScrollbarInTheColoursOfTheStyle)
  {
    ShowList(10, "scrollbar_color: \"#ff0000 #0000ff\"\n");

    const RecordedQuad *track = nullptr;
    const RecordedQuad *thumb = nullptr;

    const auto quads = _renderer.Quads();
    for (const RecordedQuad &quad : quads)
    {
      if (quad.Width() != 12.0f) { continue; }

      if (quad.color.b == 1.0f && quad.color.r == 0.0f) { track = &quad; }
      if (quad.color.r == 1.0f && quad.color.b == 0.0f) { thumb = &quad; }
    }

    ASSERT_NE(track, nullptr);
    ASSERT_NE(thumb, nullptr);

    EXPECT_FLOAT_EQ(track->left, 388.0f);
    EXPECT_FLOAT_EQ(track->Height(), 200.0f);
    EXPECT_FLOAT_EQ(thumb->top, 100.0f);
    EXPECT_FLOAT_EQ(thumb->Height(), 100.0f);
  }

  TEST_F(UiScrollingTest, StylesThePartsOfTheScrollbarFromAStyleSheet)
  {
    WriteAsset(
      "ui/theme.css",
      "#list { width: 300px; height: 200px; overflow-y: scroll; flex-direction: column; }\n"
      "#list::scrollbar-thumb { background-color: #00ff00; }\n"
      "#list::scrollbar-track { background-color: rgba(255, 0, 0, 0.5); }\n");

    ASSERT_GE(Show(
      "styles: [theme.css]\n"
      "root:\n"
      "  type: panel\n"
      "  name: list\n"
      "  children:\n"
      "    - type: panel\n"
      "      height: 800\n"), 0);
    Frame();

    EXPECT_FLOAT_EQ(Element("list").GetPartStyle("scrollbar-thumb").background_color.g, 1.0f);
    EXPECT_FLOAT_EQ(Element("list").GetPartStyle("scrollbar-thumb").background_color.r, 0.0f);
    EXPECT_FLOAT_EQ(Element("list").GetPartStyle("scrollbar-track").background_color.r, 1.0f);
    EXPECT_NEAR(Element("list").GetPartStyle("scrollbar-track").background_color.a, 0.5f, 0.01f);
  }

  TEST_F(UiScrollingTest, ScrollsByDraggingTheThumb)
  {
    ShowList(10);

    // the thumb is at 388 to 400 by 100 to 200. Taken at 120, and dragged
    // down by 50 of the 100 it can move
    Release();
    PointAt(394, 120);
    _input.state.SetAction(Action::Pointer_Primary);
    Frame();
    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);

    PointAt(394, 170);
    Frame();
    EXPECT_FLOAT_EQ(ScrollY(), 100.0f);

    // next to the scrollbar, the thumb is still held
    PointAt(700, 195);
    Frame();
    EXPECT_FLOAT_EQ(ScrollY(), 150.0f);

    PointAt(394, 900);
    Frame();
    EXPECT_FLOAT_EQ(ScrollY(), 200.0f);

    Release();
    Frame();

    // let go, it stays
    PointAt(394, 120);
    Frame();
    EXPECT_FLOAT_EQ(ScrollY(), 200.0f);
  }

  TEST_F(UiScrollingTest, ScrollsByAPageWhenTheTrackIsPressed)
  {
    ShowList(20);

    // below the thumb
    ClickAt(394, 280);
    EXPECT_FLOAT_EQ(ScrollY(), 180.0f);

    ClickAt(394, 290);
    EXPECT_FLOAT_EQ(ScrollY(), 360.0f);

    // above it
    ClickAt(394, 105);
    EXPECT_FLOAT_EQ(ScrollY(), 180.0f);
  }

  TEST_F(UiScrollingTest, DoesNotClickWhatIsUnderTheScrollbar)
  {
    ShowList(10);

    // a row reaches under the scrollbar, which lies on top of it
    ClickAt(394, 120);
    EXPECT_TRUE(_ui->GetEvents().empty());

    ClickAt(200, 120);
    EXPECT_TRUE(_ui->WasClicked("row-0"));
  }

  // keys

  TEST_F(UiScrollingTest, ScrollsTheFocusIntoViewWhenKeysMoveIt)
  {
    ShowList(10);

    ASSERT_TRUE(_ui->Focus("row-0"));
    Frame();

    for (int i = 0; i < 4; i++) { Press(Action::Ui_Down); }
    EXPECT_EQ(_ui->GetFocused(), "row-4");
    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);

    // row-5 is at 200 to 240 of what is inside: as little as it takes
    Press(Action::Ui_Down);
    EXPECT_EQ(_ui->GetFocused(), "row-5");
    EXPECT_FLOAT_EQ(ScrollY(), 40.0f);

    Press(Action::Ui_Down);
    EXPECT_FLOAT_EQ(ScrollY(), 80.0f);

    // back up, nothing is scrolled until the focus reaches the top
    Press(Action::Ui_Up);
    Press(Action::Ui_Up);
    Press(Action::Ui_Up);
    EXPECT_EQ(_ui->GetFocused(), "row-3");
    EXPECT_FLOAT_EQ(ScrollY(), 80.0f);

    Press(Action::Ui_Up);
    Press(Action::Ui_Up);
    EXPECT_EQ(_ui->GetFocused(), "row-1");
    EXPECT_FLOAT_EQ(ScrollY(), 40.0f);
  }

  TEST_F(UiScrollingTest, ScrollsTheFocusIntoViewWhenTheGameMovesIt)
  {
    ShowList(20);

    ASSERT_TRUE(_ui->Focus("row-15"));
    Frame();

    // row-15 is at 600 to 640 of what is inside
    EXPECT_FLOAT_EQ(ScrollY(), 440.0f);
    EXPECT_FALSE(Element("row-15").IsClippedAway());
  }

  TEST_F(UiScrollingTest, LeavesWhatIsSeenWhereItIsWhenItGetsTheFocus)
  {
    ShowList(20);
    _ui->SetScroll(Handle("list"), 0.0f, 50.0f);
    Frame();

    ASSERT_TRUE(_ui->Focus("row-3"));
    Frame();
    EXPECT_FLOAT_EQ(ScrollY(), 50.0f);
  }

  TEST_F(UiScrollingTest, DoesNotScrollWhenThePointerGivesTheFocus)
  {
    ShowList(20);
    _ui->SetScroll(Handle("list"), 0.0f, 20.0f);
    Frame();

    // row-0 is half seen, and pressed
    ClickAt(200, 105);

    EXPECT_EQ(_ui->GetFocused(), "row-0");
    EXPECT_FLOAT_EQ(ScrollY(), 20.0f);
  }

  TEST_F(UiScrollingTest, ScrollsIntoViewInEveryListAroundAnElement)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: outer\n"
      "  width: 300\n"
      "  height: 200\n"
      "  flex_direction: column\n"
      "  overflow_y: auto\n"
      "  children:\n"
      "    - type: panel\n"
      "      height: 300\n"
      "    - type: panel\n"
      "      name: inner\n"
      "      height: 100\n"
      "      flex_direction: column\n"
      "      overflow_y: auto\n"
      "      children:\n"
      "        - type: panel\n"
      "          height: 400\n"
      "        - type: button\n"
      "          name: deep\n"
      "          text: a\n"
      "          height: 40\n"
      "          box_sizing: border-box\n"), 0);
    Frame();

    ASSERT_TRUE(_ui->ScrollIntoView(Handle("deep")));
    Frame();

    // at 400 to 440 of the inner list, which shows 100
    EXPECT_FLOAT_EQ(ScrollY("inner"), 340.0f);

    // and the inner list at 300 to 400 of the outer one, which shows 200.
    // The button is at its end
    EXPECT_FLOAT_EQ(ScrollY("outer"), 200.0f);
    EXPECT_FALSE(Element("deep").IsClippedAway());
    EXPECT_NEAR(Element("deep").GetBox().bottom, 200.0f, 0.01f);
  }

  TEST_F(UiScrollingTest, ScrollsWithTheKeysForPagesAndEnds)
  {
    ShowList(30);

    ASSERT_TRUE(_ui->Focus("row-0"));
    Frame();

    PressKey(Key::PageDown);
    EXPECT_FLOAT_EQ(ScrollY(), 180.0f);

    PressKey(Key::PageDown);
    EXPECT_FLOAT_EQ(ScrollY(), 360.0f);

    PressKey(Key::PageUp);
    EXPECT_FLOAT_EQ(ScrollY(), 180.0f);

    PressKey(Key::End);
    EXPECT_FLOAT_EQ(ScrollY(), 1000.0f);

    PressKey(Key::Home);
    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);
  }

  TEST_F(UiScrollingTest, DoesNotScrollWithKeysWhileTheFocusIsElsewhere)
  {
    ShowList(30);

    PressKey(Key::PageDown);
    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);

    ASSERT_GE(Show("root:\n  type: button\n  name: other\n  text: a\n", "ui/other.ui.yml"), 0);
    ASSERT_TRUE(_ui->Focus("other"));

    PressKey(Key::PageDown);
    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);
  }

  TEST_F(UiScrollingTest, ScrollsWithAnArrowThatFindsNothingToMoveTheFocusTo)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: list\n"
      "  width: 300\n"
      "  height: 200\n"
      "  flex_direction: column\n"
      "  overflow_y: auto\n"
      "  children:\n"
      "    - type: button\n"
      "      name: only\n"
      "      text: a\n"
      "    - type: panel\n"
      "      height: 1000\n"), 0);

    ASSERT_TRUE(_ui->Focus("only"));
    Frame();

    Press(Action::Ui_Down);
    EXPECT_EQ(_ui->GetFocused(), "only");
    EXPECT_FLOAT_EQ(ScrollY(), 40.0f);

    Press(Action::Ui_Down);
    EXPECT_FLOAT_EQ(ScrollY(), 80.0f);

    Press(Action::Ui_Up);
    EXPECT_FLOAT_EQ(ScrollY(), 40.0f);
  }

  // a controller

  TEST_F(UiScrollingTest, ScrollsWithTheRightStickWhileTheFocusIsInside)
  {
    ShowList(100);
    ASSERT_TRUE(_ui->Focus("row-0"));
    Frame();

    // pushed all the way for a tenth of a second
    Release();
    _input.state.SetRightStick(0.0, 1.0);
    _ui->Advance(0.1);
    Frame();
    EXPECT_FLOAT_EQ(ScrollY(), 120.0f);

    // and half the way
    Release();
    _input.state.SetRightStick(0.0, 0.5);
    _ui->Advance(0.1);
    Frame();
    EXPECT_FLOAT_EQ(ScrollY(), 180.0f);

    Release();
    _input.state.SetRightStick(0.0, -1.0);
    _ui->Advance(0.05);
    Frame();
    EXPECT_FLOAT_EQ(ScrollY(), 120.0f);
  }

  TEST_F(UiScrollingTest, KeepsTheStickFromTheGameWhileItScrolls)
  {
    ShowList(100);
    ASSERT_TRUE(_ui->Focus("row-0"));
    Frame();

    Release();
    _input.state.SetRightStick(0.0, 1.0);
    _ui->Advance(0.1);
    Frame();
    EXPECT_EQ(_ui->GetGameInput()->GetInputState().GetRightStick().y, 0.0);

    _ui->Blur();
    Release();
    _input.state.SetRightStick(0.0, 1.0);
    _input.state.ClearPointer();
    Frame();
    EXPECT_EQ(_ui->GetGameInput()->GetInputState().GetRightStick().y, 1.0);
  }

  // over a short time

  TEST_F(UiScrollingTest, TakesAShortTimeToScrollWhereTheStyleAsksForIt)
  {
    ShowList(30, "scroll_behavior: smooth\n");

    _ui->SetScroll(Handle("list"), 0.0f, 400.0f);
    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);

    float before = 0.0f;
    for (int i = 0; i < 14; i++)
    {
      Tick();

      // on its way, and never back
      EXPECT_GT(ScrollY(), before) << i;
      EXPECT_LT(ScrollY(), 400.0f) << i;
      before = ScrollY();
    }

    // a quarter of a second in all
    Tick();
    Tick();
    EXPECT_FLOAT_EQ(ScrollY(), 400.0f);

    Tick();
    EXPECT_FLOAT_EQ(ScrollY(), 400.0f);
  }

  TEST_F(UiScrollingTest, ScrollsAtOnceWhereTheStyleSaysNothing)
  {
    ShowList(30);

    _ui->SetScroll(Handle("list"), 0.0f, 400.0f);
    EXPECT_FLOAT_EQ(ScrollY(), 400.0f);
  }

  TEST_F(UiScrollingTest, ScrollsAtOnceWithTheWheelWhateverTheStyleSays)
  {
    ShowList(30, "scroll_behavior: smooth\n");

    PointAt(200, 150);
    Wheel(0, 1);
    EXPECT_FLOAT_EQ(ScrollY(), 60.0f);
  }

  TEST_F(UiScrollingTest, ScrollsAtOnceWhenLessMotionIsWanted)
  {
    ShowList(30, "scroll_behavior: smooth\n");
    _ui->SetReducedMotion(true);
    Frame();

    _ui->SetScroll(Handle("list"), 0.0f, 400.0f);
    EXPECT_FLOAT_EQ(ScrollY(), 400.0f);
  }

  TEST_F(UiScrollingTest, GoesToWhereItWasSentLastWhenItIsSentAgainOnItsWay)
  {
    ShowList(30, "scroll_behavior: smooth\n");

    _ui->SetScroll(Handle("list"), 0.0f, 400.0f);
    for (int i = 0; i < 5; i++) { Tick(); }

    const float on_its_way = ScrollY();
    _ui->SetScroll(Handle("list"), 0.0f, 100.0f);

    for (int i = 0; i < 20; i++) { Tick(); }
    EXPECT_FLOAT_EQ(ScrollY(), 100.0f);
    EXPECT_GT(on_its_way, 100.0f);
  }

  // dragging what is inside

  TEST_F(UiScrollingTest, ScrollsByDraggingWhatIsInsideWhereTheStyleAsksForIt)
  {
    ShowList(30, "scroll_drag: inertia\nscrollbar_width: none\n");

    Release();
    PointAt(200, 250);
    _input.state.SetAction(Action::Pointer_Primary);
    _ui->Advance(1.0 / 60.0);
    Frame();

    // not far enough to count as dragging
    PointAt(200, 247);
    _ui->Advance(1.0 / 60.0);
    Frame();
    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);

    // up by 50: what is inside follows the pointer
    PointAt(200, 200);
    _ui->Advance(1.0 / 60.0);
    Frame();
    EXPECT_FLOAT_EQ(ScrollY(), 50.0f);

    PointAt(200, 150);
    _ui->Advance(1.0 / 60.0);
    Frame();
    EXPECT_FLOAT_EQ(ScrollY(), 100.0f);
  }

  TEST_F(UiScrollingTest, GoesOnAndSlowsDownWhenWhatWasDraggedIsLetGo)
  {
    ShowList(200, "scroll_drag: inertia\nscrollbar_width: none\n");

    Release();
    PointAt(200, 280);
    _input.state.SetAction(Action::Pointer_Primary);
    _ui->Advance(1.0 / 60.0);
    Frame();

    for (int i = 1; i <= 6; i++)
    {
      PointAt(200, 280 - i * 20);
      _ui->Advance(1.0 / 60.0);
      Frame();
    }

    const float let_go_at = ScrollY();
    EXPECT_FLOAT_EQ(let_go_at, 120.0f);

    Release();
    Tick();

    // it goes on the way it went, slower from frame to frame
    float before = ScrollY();
    float step_before = before - let_go_at;
    EXPECT_GT(step_before, 0.0f);

    for (int i = 0; i < 30; i++)
    {
      Tick();

      const float step = ScrollY() - before;
      EXPECT_GE(step, 0.0f) << i;
      EXPECT_LT(step, step_before + 0.001f) << i;

      before = ScrollY();
      if (step > 0.0f) { step_before = step; }
    }

    // and comes to rest
    for (int i = 0; i < 600; i++) { Tick(); }
    const float at_rest = ScrollY();

    Tick();
    EXPECT_FLOAT_EQ(ScrollY(), at_rest);
    EXPECT_GT(at_rest, let_go_at);
  }

  TEST_F(UiScrollingTest, DoesNotClickWhatItWasDraggedBy)
  {
    ShowList(30, "scroll_drag: inertia\nscrollbar_width: none\n");

    Release();
    PointAt(200, 250);
    _input.state.SetAction(Action::Pointer_Primary);
    Frame();

    PointAt(200, 200);
    Frame();

    Release();
    PointAt(200, 200);
    Frame();

    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  TEST_F(UiScrollingTest, ClicksWhatIsPressedWithoutBeingDragged)
  {
    ShowList(30, "scroll_drag: inertia\nscrollbar_width: none\n");

    ClickAt(200, 120);
    EXPECT_TRUE(_ui->WasClicked("row-0"));
    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);
  }

  TEST_F(UiScrollingTest, IsNotDraggedWhereTheStyleSaysNothing)
  {
    ShowList(30);

    Release();
    PointAt(200, 250);
    _input.state.SetAction(Action::Pointer_Primary);
    Frame();

    PointAt(200, 150);
    Frame();

    EXPECT_FLOAT_EQ(ScrollY(), 0.0f);
  }

  // what the game is told

  TEST_F(UiScrollingTest, TellsThatAnElementWasScrolled)
  {
    ShowList(30);

    std::vector<float> scrolled;
    _ui->On(Handle("list"), "scroll", [&](const neon::UiElementEvent &event)
    {
      float x = 0.0f;
      float y = 0.0f;
      EXPECT_TRUE(_ui->GetScroll(event.target, x, y));
      scrolled.push_back(y);
    });

    PointAt(200, 150);
    Wheel(0, 1);
    Wheel(0, 1);
    Frame();

    EXPECT_THAT(scrolled, ::testing::ElementsAre(60.0f, 120.0f));
  }
} // namespace
