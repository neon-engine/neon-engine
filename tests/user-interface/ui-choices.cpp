#include "ui-fixture.hpp"

#include <neon/ui/elements/ui-checkbox.hpp>
#include <neon/ui/elements/ui-radio.hpp>
#include <neon/ui/elements/ui-select.hpp>
#include <neon/ui/elements/ui-slider.hpp>

// Choosing with a checkbox, a toggle, radios, a slider, and a select: with
// the pointer, the keys, and a controller, and what the game hears of it.

namespace
{
  using neon::Action;
  using neon::FieldValue;
  using neon::Key;
  using neon::UiHandle;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  class UiChoicesTest : public UiTest
  {
  protected:
    /// Elements under a column that starts at the left top corner.
    void ShowColumn(const std::string &children, const std::string &top = "")
    {
      ASSERT_GE(Show(
        top +
        "ui: test\n"
        "root:\n"
        "  type: panel\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  flex_direction: column\n"
        "  align_items: flex-start\n"
        "  children:\n" +
        Indented(children, "    ")), 0)
        << _logger->Messages(LogLevel::Error);

      Frame();
    }

    [[nodiscard]] bool Flag(const std::string &name, const std::string &field) const
    {
      FieldValue value;
      return _ui->GetField(_ui->FindByName(name), field, value) && std::get<bool>(value);
    }

    [[nodiscard]] std::string Text(const std::string &name, const std::string &field) const
    {
      FieldValue value;
      return _ui->GetField(_ui->FindByName(name), field, value) ? std::get<std::string>(value) : "(none)";
    }

    [[nodiscard]] float Number(const std::string &name, const std::string &field) const
    {
      FieldValue value;
      return _ui->GetField(_ui->FindByName(name), field, value) ? std::get<float>(value) : -1.0f;
    }

    /// A frame with the action held down. What happened in it can be
    /// looked at before Let() lets go.
    void Hold(const Action action)
    {
      Release();
      _input.state.SetAction(action);
      Frame();
    }

    void Let()
    {
      Release();
      Frame();
    }

    void PressKey(const Key key)
    {
      Release();
      _input.state.AddKeyEvent({key, true, false, {}});
      _input.state.AddKeyEvent({key, false, false, {}});
      Frame();
      Release();
    }

    [[nodiscard]] std::vector<std::string> Happened() const
    {
      std::vector<std::string> happened;
      for (const auto &event : _ui->GetElementEvents())
      {
        if (event.name != "changed") { continue; }
        happened.push_back(event.name + " " + event.target_name + " " + event.value);
      }
      return happened;
    }

    /// The colours of the boxes that are not textured, from the top left.
    [[nodiscard]] std::vector<neon::testing::RecordedQuad> Boxes() const
    {
      std::vector<neon::testing::RecordedQuad> boxes;
      for (const auto &quad : _renderer.Quads())
      {
        if (!quad.textured) { boxes.push_back(quad); }
      }
      return boxes;
    }

    [[nodiscard]] bool HasBox(const float left, const float top, const float width, const float height) const
    {
      for (const auto &box : Boxes())
      {
        if (std::abs(box.left - left) < 0.01f && std::abs(box.top - top) < 0.01f &&
            std::abs(box.Width() - width) < 0.01f && std::abs(box.Height() - height) < 0.01f)
        {
          return true;
        }
      }
      return false;
    }
  };

  // a checkbox

  TEST_F(UiChoicesTest, ACheckboxIsTickedByAClickAndTellsTheGame)
  {
    ShowColumn("- {type: checkbox, name: fullscreen, text: Full screen}\n");
    EXPECT_FALSE(Flag("fullscreen", "checked"));

    // 18 box and a padding of 4: 26 high, and 18 + 8 + 88 + 8 wide
    ExpectBox("fullscreen", 0.0f, 0.0f, 122.0f, 26.0f);

    ClickAt(10.0, 13.0);
    EXPECT_TRUE(Flag("fullscreen", "checked"));
    EXPECT_THAT(Happened(), ElementsAre("changed fullscreen true"));
    EXPECT_TRUE(_ui->WasClicked("fullscreen"));

    // the tick fills the middle of the box: 18 with an inset of 4 (25%)
    EXPECT_TRUE(HasBox(8.0f, 8.0f, 10.0f, 10.0f));

    ClickAt(100.0, 13.0);
    EXPECT_FALSE(Flag("fullscreen", "checked"));
    EXPECT_THAT(Happened(), ElementsAre("changed fullscreen false"));
    EXPECT_FALSE(HasBox(8.0f, 8.0f, 10.0f, 10.0f));
  }

  TEST_F(UiChoicesTest, ACheckboxIsTickedByAcceptWhileItHasTheFocus)
  {
    ShowColumn("- {type: checkbox, name: fullscreen, text: Full screen, autofocus: true}\n");
    EXPECT_EQ(_ui->GetFocused(), "fullscreen");

    Press(Action::Ui_Accept);
    EXPECT_TRUE(Flag("fullscreen", "checked"));

    Press(Action::Ui_Accept);
    EXPECT_FALSE(Flag("fullscreen", "checked"));
  }

  TEST_F(UiChoicesTest, ACheckboxFollowsAFlagOfTheGameAndSetsIt)
  {
    ShowColumn("- {type: checkbox, name: fullscreen, text: Full screen, checked: \"{fullscreen}\"}\n");
    EXPECT_FALSE(Flag("fullscreen", "checked"));

    _ui->SetFlag("fullscreen", true);
    Frame();
    EXPECT_TRUE(Flag("fullscreen", "checked"));

    ClickAt(10.0, 13.0);
    EXPECT_FALSE(Flag("fullscreen", "checked"));
    EXPECT_EQ(_ui->GetValue("fullscreen", nullptr), "false");
  }

  TEST_F(UiChoicesTest, ACheckboxThatIsTickedMatchesChecked)
  {
    WriteAsset("ui/theme.css", "checkbox:checked { color: #00ff00; }\ncheckbox::mark { background-color: #ff0000; }\n");
    ShowColumn("- {type: checkbox, name: fullscreen, text: Full screen, checked: true}\n", "styles: [theme.css]\n");

    EXPECT_EQ(_ui->GetComputed(_ui->FindByName("fullscreen"), "color"), "rgb(0, 255, 0)");

    bool red_mark = false;
    for (const auto &box : Boxes())
    {
      if (std::abs(box.left - 8.0f) < 0.01f && std::abs(box.Width() - 10.0f) < 0.01f && box.color.r > 0.99f)
      {
        red_mark = true;
      }
    }
    EXPECT_TRUE(red_mark);

    ClickAt(10.0, 13.0);
    EXPECT_EQ(_ui->GetComputed(_ui->FindByName("fullscreen"), "color"), "rgb(255, 255, 255)");
  }

  TEST_F(UiChoicesTest, ACheckboxThatIsNotEnabledCannotBeTicked)
  {
    ShowColumn("- {type: checkbox, name: fullscreen, text: Full screen, enabled: false}\n");

    ClickAt(10.0, 13.0);
    EXPECT_FALSE(Flag("fullscreen", "checked"));
    EXPECT_THAT(Happened(), IsEmpty());
  }

  TEST_F(UiChoicesTest, AScriptTicksACheckbox)
  {
    ShowColumn("- {type: checkbox, name: fullscreen, text: Full screen}\n");

    EXPECT_TRUE(_ui->SetField(_ui->FindByName("fullscreen"), "checked", true));
    Frame();
    EXPECT_TRUE(Flag("fullscreen", "checked"));
    EXPECT_THAT(Happened(), ElementsAre("changed fullscreen true"));
  }

  // a toggle

  TEST_F(UiChoicesTest, AToggleIsASwitchThatLeftAndRightMove)
  {
    ShowColumn("- {type: toggle, name: vsync, text: V-Sync, autofocus: true}\n");
    ExpectBox("vsync", 0.0f, 0.0f, 36.0f + 8.0f + 6.0f * 8.0f + 8.0f, 28.0f);

    // the knob at the left: 16 in a track of 36 by 20, with a rim of 2
    EXPECT_TRUE(HasBox(6.0f, 6.0f, 16.0f, 16.0f));

    Press(Action::Ui_Right);
    EXPECT_TRUE(Flag("vsync", "checked"));
    EXPECT_TRUE(HasBox(22.0f, 6.0f, 16.0f, 16.0f));

    // right again changes nothing, and does not move the focus
    Press(Action::Ui_Right);
    EXPECT_TRUE(Flag("vsync", "checked"));
    EXPECT_EQ(_ui->GetFocused(), "vsync");

    Press(Action::Ui_Left);
    EXPECT_FALSE(Flag("vsync", "checked"));

    Press(Action::Ui_Accept);
    EXPECT_TRUE(Flag("vsync", "checked"));

    ClickAt(20.0, 14.0);
    EXPECT_FALSE(Flag("vsync", "checked"));
  }

  // radios

  TEST_F(UiChoicesTest, RadiosOfAGroupLetGoOfEachOther)
  {
    ShowColumn(
      "- {type: radio, name: low, group: quality, value: low, text: Low, checked: true}\n"
      "- {type: radio, name: high, group: quality, value: high, text: High}\n"
      "- {type: radio, name: on, group: shadows, value: on, text: On}\n");

    EXPECT_TRUE(Flag("low", "checked"));
    EXPECT_FALSE(Flag("high", "checked"));

    ClickAt(10.0, 26.0 + 13.0);
    EXPECT_FALSE(Flag("low", "checked"));
    EXPECT_TRUE(Flag("high", "checked"));
    EXPECT_FALSE(Flag("on", "checked")) << "another group is not touched";
    EXPECT_THAT(Happened(), ElementsAre("changed low false", "changed high true")) << "in the order of the file";

    // choosing what is chosen changes nothing
    ClickAt(10.0, 26.0 + 13.0);
    EXPECT_TRUE(Flag("high", "checked"));
    EXPECT_THAT(Happened(), IsEmpty());
  }

  TEST_F(UiChoicesTest, RadiosFollowAValueOfTheGameAndSetIt)
  {
    ShowColumn(
      "- {type: radio, name: low, group: quality, value: low, text: Low, checked: \"{quality}\"}\n"
      "- {type: radio, name: high, group: quality, value: high, text: High, checked: \"{quality}\"}\n");

    EXPECT_FALSE(Flag("low", "checked"));
    EXPECT_FALSE(Flag("high", "checked"));

    _ui->SetText("quality", "high");
    Frame();
    EXPECT_FALSE(Flag("low", "checked"));
    EXPECT_TRUE(Flag("high", "checked"));

    ClickAt(10.0, 13.0);
    EXPECT_TRUE(Flag("low", "checked"));
    EXPECT_FALSE(Flag("high", "checked"));
    EXPECT_EQ(_ui->GetValue("quality", nullptr), "low");

    // something none of them stands for lets go of all
    _ui->SetText("quality", "ultra");
    Frame();
    EXPECT_FALSE(Flag("low", "checked"));
    EXPECT_FALSE(Flag("high", "checked"));
  }

  TEST_F(UiChoicesTest, ARadioNeedsAGroup)
  {
    ExpectProblemsUnderRoot(
      "- {type: radio, name: low, value: low}\n"
      "- {type: radio, name: high, group: quality, value: [1]}\n"
      "- {type: checkbox, name: box, checked: maybe}\n"
      "- {type: slider, name: volume, step: 0}\n"
      "- {type: slider, name: pan, min: low}\n"
      "- {type: select, name: quality, options: low}\n"
      "- {type: select, name: level, options: [low, [2]], value: ultra}\n",
      {
        "assets://ui/test.ui.yml:4: radio 'low' has no 'group', which says which radios belong together",
        "assets://ui/test.ui.yml:5: 'value' of radio 'high' is a list, where the text it stands for was expected",
        "assets://ui/test.ui.yml:6: 'checked' of checkbox 'box' is 'maybe', where true, false, or a value such as "
        "\"{fullscreen}\" was expected",
        "assets://ui/test.ui.yml:7: 'step' of slider 'volume' is 0, where a number above 0 was expected",
        "assets://ui/test.ui.yml:8: 'min' of slider 'pan' is text, where a number was expected",
        "assets://ui/test.ui.yml:9: 'options' of select 'quality' is text, where a list was expected",
        "assets://ui/test.ui.yml:10: an option of select 'level' is a list, where text or a map with 'value' and "
        "'text' was expected",
        "assets://ui/test.ui.yml:10: 'value' of select 'level' is 'ultra', which is none of its options"
      });
  }

  // a slider

  TEST_F(UiChoicesTest, ASliderIsMovedByTheKeysInSteps)
  {
    ShowColumn("- {type: slider, name: volume, min: 0, max: 100, step: 5, value: 50, autofocus: true}\n");
    ExpectBox("volume", 0.0f, 0.0f, 160.0f, 24.0f);
    EXPECT_FLOAT_EQ(Number("volume", "value"), 50.0f);

    Hold(Action::Ui_Right);
    EXPECT_FLOAT_EQ(Number("volume", "value"), 55.0f);
    EXPECT_THAT(Happened(), ElementsAre("changed volume 55"));
    Let();

    Press(Action::Ui_Left);
    Press(Action::Ui_Left);
    EXPECT_FLOAT_EQ(Number("volume", "value"), 45.0f);
    EXPECT_EQ(_ui->GetFocused(), "volume") << "left does not move the focus";

    PressKey(Key::PageUp);
    EXPECT_FLOAT_EQ(Number("volume", "value"), 95.0f);

    PressKey(Key::End);
    EXPECT_FLOAT_EQ(Number("volume", "value"), 100.0f);

    Press(Action::Ui_Right);
    EXPECT_FLOAT_EQ(Number("volume", "value"), 100.0f) << "no further than the end";

    PressKey(Key::Home);
    EXPECT_FLOAT_EQ(Number("volume", "value"), 0.0f);

    PressKey(Key::PageDown);
    EXPECT_FLOAT_EQ(Number("volume", "value"), 0.0f);
  }

  TEST_F(UiChoicesTest, ASliderIsMovedByThePointer)
  {
    ShowColumn("- {type: slider, name: volume, min: 0, max: 100, step: 1, value: 0}\n");

    // the knob is 16 wide, and moves over 144 of the 160
    ClickAt(8.0 + 72.0, 12.0);
    EXPECT_FLOAT_EQ(Number("volume", "value"), 50.0f);
    EXPECT_TRUE(HasBox(72.0f, 4.0f, 16.0f, 16.0f)) << "the knob at the middle";

    // dragged beyond the end
    Release();
    PointAt(80.0, 12.0);
    _input.state.SetAction(Action::Pointer_Primary);
    Frame();
    PointAt(300.0, 12.0);
    Frame();
    EXPECT_FLOAT_EQ(Number("volume", "value"), 100.0f);

    PointAt(8.0 + 36.0, 12.0);
    Frame();
    EXPECT_FLOAT_EQ(Number("volume", "value"), 25.0f);

    Release();
    Frame();
    PointAt(150.0, 12.0);
    Frame();
    EXPECT_FLOAT_EQ(Number("volume", "value"), 25.0f) << "letting go stops the drag";
  }

  TEST_F(UiChoicesTest, ASliderFollowsANumberOfTheGameAndSetsIt)
  {
    ShowColumn("- {type: slider, name: volume, min: 0, max: 1, step: 0.1, value: \"{volume}\"}\n");
    EXPECT_FLOAT_EQ(Number("volume", "value"), 0.0f);

    _ui->SetNumber("volume", 0.75);
    Frame();
    EXPECT_FLOAT_EQ(Number("volume", "value"), 0.8f) << "snapped to a step";

    PointAt(8.0 + 72.0, 12.0);
    Hold(Action::Pointer_Primary);
    EXPECT_FLOAT_EQ(Number("volume", "value"), 0.5f);
    EXPECT_EQ(_ui->GetValue("volume", nullptr), "0.50");
    EXPECT_THAT(Happened(), ElementsAre("changed volume 0.50"));
    Let();
  }

  TEST_F(UiChoicesTest, ASliderIsDrawnFromItsParts)
  {
    WriteAsset(
      "ui/theme.css",
      "slider::track { background-color: #111111; }\n"
      "slider::fill { background-color: #22ff22; }\n"
      "slider::thumb { background-color: #ffffff; }\n");
    ShowColumn("- {type: slider, name: volume, value: 50}\n", "styles: [theme.css]\n");

    bool track = false;
    bool fill = false;
    for (const auto &box : Boxes())
    {
      // the track runs between the middles of the knob at either end
      if (std::abs(box.left - 8.0f) < 0.01f && std::abs(box.Width() - 144.0f) < 0.01f && box.color.r < 0.1f)
      {
        track = true;
      }
      if (std::abs(box.left - 8.0f) < 0.01f && std::abs(box.Width() - 72.0f) < 0.01f && box.color.g > 0.99f)
      {
        fill = true;
      }
    }
    EXPECT_TRUE(track);
    EXPECT_TRUE(fill);
  }

  // a select

  TEST_F(UiChoicesTest, ASelectShowsWhatIsChosenAndOpensAList)
  {
    ShowColumn("- {type: select, name: quality, options: [low, medium, high], value: medium}\n");
    EXPECT_EQ(Text("quality", "value"), "medium");
    EXPECT_EQ(Text("quality", "text"), "medium");
    EXPECT_FALSE(Flag("quality", "open"));

    // a line of 16 and a padding of 6 and a border of 1: 30 high
    ExpectBox("quality", 0.0f, 0.0f, 182.0f, 30.0f);

    Release();
    PointAt(50.0, 15.0);
    _input.state.SetAction(Action::Pointer_Primary);
    Frame();
    EXPECT_TRUE(Flag("quality", "open"));
    EXPECT_EQ(_ui->GetFocused(), "quality");

    Release();
    Frame();
    EXPECT_TRUE(Flag("quality", "open")) << "letting go keeps it open";

    // three rows of 28 under the box
    EXPECT_TRUE(HasBox(0.0f, 30.0f, 182.0f, 84.0f));

    // the third row is chosen as soon as it is pressed
    PointAt(50.0, 30.0 + 28.0 * 2.0 + 14.0);
    Hold(Action::Pointer_Primary);
    EXPECT_EQ(Text("quality", "value"), "high");
    EXPECT_FALSE(Flag("quality", "open"));
    EXPECT_THAT(Happened(), ElementsAre("changed quality high"));
    Let();
  }

  TEST_F(UiChoicesTest, ASelectIsChosenFromWithTheKeys)
  {
    ShowColumn(
      "- {type: select, name: quality, options: [low, medium, high], value: low, autofocus: true}\n"
      "- {type: button, name: ok, text: OK}\n");

    // closed, down goes to the next choice right away
    Press(Action::Ui_Down);
    EXPECT_EQ(Text("quality", "value"), "medium");
    EXPECT_FALSE(Flag("quality", "open"));
    EXPECT_EQ(_ui->GetFocused(), "quality");

    Press(Action::Ui_Up);
    Press(Action::Ui_Up);
    EXPECT_EQ(Text("quality", "value"), "low") << "no further than the first";

    // accept opens, down moves through the list, and accept chooses
    Press(Action::Ui_Accept);
    EXPECT_TRUE(Flag("quality", "open"));

    Press(Action::Ui_Down);
    Press(Action::Ui_Down);
    EXPECT_EQ(Text("quality", "value"), "low") << "nothing is chosen until accept";
    EXPECT_EQ(dynamic_cast<const neon::UiSelect &>(Element("quality")).GetHighlighted(), 2u);

    Press(Action::Ui_Accept);
    EXPECT_EQ(Text("quality", "value"), "high");
    EXPECT_FALSE(Flag("quality", "open"));

    // cancel closes without choosing
    Press(Action::Ui_Accept);
    Press(Action::Ui_Up);
    Press(Action::Ui_Cancel);
    EXPECT_EQ(Text("quality", "value"), "high");
    EXPECT_FALSE(Flag("quality", "open"));
    EXPECT_EQ(_ui->GetFocused(), "quality") << "the first cancel only closed the list";
  }

  TEST_F(UiChoicesTest, ASelectClosesWhenSomethingElseIsPressed)
  {
    ShowColumn(
      "- {type: select, name: quality, options: [low, medium, high], value: low}\n"
      "- {type: button, name: ok, text: OK}\n");

    ClickAt(50.0, 15.0);
    EXPECT_TRUE(Flag("quality", "open"));

    ClickAt(500.0, 500.0);
    EXPECT_FALSE(Flag("quality", "open"));
    EXPECT_EQ(Text("quality", "value"), "low");
  }

  TEST_F(UiChoicesTest, TheListOfASelectIsOnTopOfWhatIsUnderIt)
  {
    ShowColumn(
      "- {type: select, name: quality, options: [low, medium, high], value: low}\n"
      "- {type: button, name: ok, text: OK, width: 150, height: 84}\n");

    ClickAt(50.0, 15.0);
    EXPECT_TRUE(Flag("quality", "open"));

    // the list lies over the button, which is not clicked
    ClickAt(50.0, 30.0 + 28.0 + 14.0);
    EXPECT_EQ(Text("quality", "value"), "medium");
    EXPECT_FALSE(_ui->WasClicked("ok"));

    // and the list is drawn after the button
    ClickAt(50.0, 15.0);
    const auto boxes = Boxes();
    std::size_t button = boxes.size();
    std::size_t list = boxes.size();
    for (std::size_t i = 0; i < boxes.size(); i++)
    {
      // the button is 182 by 100 with its padding, and the list 182 by 84
      if (std::abs(boxes[i].top - 30.0f) < 0.01f && std::abs(boxes[i].Height() - 100.0f) < 0.01f) { button = i; }
      if (std::abs(boxes[i].top - 30.0f) < 0.01f && std::abs(boxes[i].Height() - 84.0f) < 0.01f) { list = i; }
    }
    ASSERT_LT(button, boxes.size());
    ASSERT_LT(list, boxes.size());
    EXPECT_GT(list, button);
  }

  TEST_F(UiChoicesTest, ASelectFollowsAValueOfTheGameAndSetsIt)
  {
    ShowColumn(
      "- type: select\n"
      "  name: quality\n"
      "  placeholder: Choose\n"
      "  value: \"{quality}\"\n"
      "  options:\n"
      "    - {value: 1, text: Low}\n"
      "    - {value: 2, text: High}\n");

    EXPECT_EQ(Text("quality", "value"), "");
    EXPECT_EQ(Text("quality", "text"), "Choose");

    _ui->SetNumber("quality", 2);
    Frame();
    EXPECT_EQ(Text("quality", "value"), "2");
    EXPECT_EQ(Text("quality", "text"), "High");

    ClickAt(50.0, 15.0);
    ClickAt(50.0, 30.0 + 14.0);
    EXPECT_EQ(Text("quality", "value"), "1");
    EXPECT_EQ(_ui->GetValue("quality", nullptr), "1");
  }

  TEST_F(UiChoicesTest, ASelectShowsEightOfManyAndScrollsToTheHighlighted)
  {
    ShowColumn(
      "- {type: select, name: level, autofocus: true, options: [a, b, c, d, e, f, g, h, i, j, k, l]}\n");

    Press(Action::Ui_Accept);
    EXPECT_TRUE(HasBox(0.0f, 30.0f, 182.0f, 8.0f * 28.0f));

    PressKey(Key::End);
    EXPECT_EQ(dynamic_cast<const neon::UiSelect &>(Element("level")).GetHighlighted(), 11u);

    Press(Action::Ui_Accept);
    EXPECT_EQ(Text("level", "value"), "l");
  }

  TEST_F(UiChoicesTest, AScriptSetsTheOptionsOfASelect)
  {
    ShowColumn("- {type: select, name: quality, options: [low, high], value: high}\n");

    EXPECT_TRUE(_ui->SetField(_ui->FindByName("quality"), "options", std::vector<std::string>{"high", "ultra"}));
    EXPECT_EQ(Text("quality", "value"), "high") << "what was chosen stays while it is still there";

    EXPECT_TRUE(_ui->SetField(_ui->FindByName("quality"), "options", std::vector<std::string>{"a", "b"}));
    EXPECT_EQ(Text("quality", "value"), "");

    EXPECT_FALSE(_ui->SetField(_ui->FindByName("quality"), "value", std::string("c")));
    EXPECT_TRUE(_ui->SetField(_ui->FindByName("quality"), "value", std::string("b")));
    EXPECT_EQ(Text("quality", "value"), "b");
  }

  TEST_F(UiChoicesTest, TheChoicesAreStyledByTheirParts)
  {
    WriteAsset(
      "ui/theme.css",
      "select::list { background-color: #101010; }\n"
      "select::highlight { background-color: #ff00ff; }\n");
    ShowColumn(
      "- {type: select, name: quality, options: [low, high], value: low, autofocus: true}\n",
      "styles: [theme.css]\n");

    Press(Action::Ui_Accept);

    bool list = false;
    bool highlight = false;
    for (const auto &box : Boxes())
    {
      if (std::abs(box.top - 30.0f) < 0.01f && std::abs(box.Height() - 56.0f) < 0.01f && box.color.r < 0.1f)
      {
        list = true;
      }
      if (std::abs(box.top - 30.0f) < 0.01f && std::abs(box.Height() - 28.0f) < 0.01f && box.color.b > 0.99f &&
          box.color.g < 0.01f)
      {
        highlight = true;
      }
    }
    EXPECT_TRUE(list);
    EXPECT_TRUE(highlight);
  }

  TEST_F(UiChoicesTest, TheKindsHaveTheirFields)
  {
    ShowColumn(
      "- {type: checkbox, name: a}\n"
      "- {type: toggle, name: b}\n"
      "- {type: radio, name: c, group: g, value: v}\n"
      "- {type: slider, name: d}\n"
      "- {type: select, name: e, options: [x]}\n");

    const auto names = [this](const std::string &name)
    {
      std::vector<std::string> names;
      for (const auto &field : Element(name).GetFields()) { names.push_back(field.name); }
      return names;
    };

    EXPECT_THAT(names("a"), ElementsAre("checked", "text", "enabled", "autofocus"));
    EXPECT_THAT(names("b"), ElementsAre("checked", "text", "enabled", "autofocus"));
    EXPECT_THAT(names("c"), ElementsAre("checked", "value", "group", "text", "enabled", "autofocus"));
    EXPECT_THAT(names("d"), ElementsAre("value", "min", "max", "step", "enabled", "autofocus"));
    EXPECT_THAT(names("e"), ElementsAre("value", "text", "options", "placeholder", "open", "enabled", "autofocus"));
  }
}
