#include "ui-fixture.hpp"

// Elements as a game and a script reach them: finding them, changing them,
// making and removing them, and hearing of what happens to them.

namespace
{
  using neon::Action;
  using neon::FieldValue;
  using neon::UiBox;
  using neon::UiElementEvent;
  using neon::UiHandle;
  using neon::UiRow;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  class UiScriptingTest : public UiTest
  {
  protected:
    /// An inventory with a title, a list of three slots, and two buttons.
    void ShowInventory(const std::string &top = "")
    {
      WriteAsset(
        "ui/theme.css",
        ".slot { width: 64px; height: 64px; box-sizing: border-box; }\n"
        ".slot.selected { opacity: 0.5; }\n"
        "button.danger { background-color: #ff0000; }\n");

      ASSERT_GE(Show(
        top +
        "ui: inventory\n"
        "styles: [theme.css]\n"
        "templates:\n"
        "  slot:\n"
        "    type: button\n"
        "    name: \"slot-${index}\"\n"
        "    class: slot\n"
        "    text: \"${title}\"\n"
        "  divider:\n"
        "    type: panel\n"
        "    class: divider\n"
        "    height: 2\n"
        "root:\n"
        "  type: panel\n"
        "  name: window\n"
        "  class: inventory\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  flex_direction: column\n"
        "  align_items: flex-start\n"
        "  children:\n"
        "    - type: label\n"
        "      name: title\n"
        "      class: heading\n"
        "      text: Inventory\n"
        "    - type: panel\n"
        "      name: slots\n"
        "      children:\n"
        "        - {type: button, name: sword, class: slot weapon, text: Sword}\n"
        "        - {type: button, name: shield, class: slot, text: Shield, enabled: false}\n"
        "        - {type: button, name: potion, class: slot, text: Potion}\n"
        "    - type: panel\n"
        "      name: actions\n"
        "      children:\n"
        "        - {type: button, name: use, text: Use}\n"
        "        - {type: button, name: drop, class: danger, text: Drop}\n"), 0)
        << _logger->Messages(LogLevel::Error);

      Frame();
    }

    [[nodiscard]] UiHandle Handle(const std::string &name) const
    {
      return _ui->FindByName(name);
    }

    [[nodiscard]] std::vector<std::string> NamesOf(const std::vector<UiHandle> &handles) const
    {
      std::vector<std::string> names;
      for (const UiHandle handle : handles)
      {
        const std::string name = _ui->GetElementName(handle);
        names.push_back(name.empty() ? "(" + _ui->GetElementType(handle) + ")" : name);
      }
      return names;
    }

    [[nodiscard]] std::vector<std::string> Queried(const std::string &selector, const UiHandle from = {}) const
    {
      return NamesOf(_ui->Query(selector, from));
    }

    /// The names of the events of the last frame, with the element each
    /// happened to.
    [[nodiscard]] std::vector<std::string> Happened() const
    {
      std::vector<std::string> happened;
      for (const auto &event : _ui->GetElementEvents())
      {
        happened.push_back(event.name + " " + event.target_name);
      }
      return happened;
    }
  };

  // finding

  TEST_F(UiScriptingTest, FindsAnElementByItsName)
  {
    ShowInventory();

    const UiHandle sword = Handle("sword");
    ASSERT_TRUE(sword.IsSet());
    EXPECT_TRUE(_ui->IsAlive(sword));
    EXPECT_EQ(_ui->GetElementName(sword), "sword");
    EXPECT_EQ(_ui->GetElementType(sword), "button");

    EXPECT_FALSE(Handle("missing").IsSet());
    EXPECT_FALSE(Handle("").IsSet());
  }

  TEST_F(UiScriptingTest, FindsAnElementInsideOfAnother)
  {
    ShowInventory();

    EXPECT_EQ(_ui->FindByName("potion", Handle("slots")), Handle("potion"));
    EXPECT_FALSE(_ui->FindByName("potion", Handle("actions")).IsSet());

    // inside, which the element itself is not
    EXPECT_FALSE(_ui->FindByName("slots", Handle("slots")).IsSet());
  }

  TEST_F(UiScriptingTest, FindsElementsByClassAndByType)
  {
    ShowInventory();

    EXPECT_THAT(NamesOf(_ui->FindByClass("slot")), ElementsAre("sword", "shield", "potion"));
    EXPECT_THAT(NamesOf(_ui->FindByClass("weapon")), ElementsAre("sword"));
    EXPECT_THAT(NamesOf(_ui->FindByClass("missing")), IsEmpty());

    EXPECT_THAT(NamesOf(_ui->FindByType("button")), ElementsAre("sword", "shield", "potion", "use", "drop"));
    EXPECT_THAT(NamesOf(_ui->FindByType("button", Handle("actions"))), ElementsAre("use", "drop"));
    EXPECT_THAT(NamesOf(_ui->FindByType("label")), ElementsAre("title"));
  }

  TEST_F(UiScriptingTest, FindsElementsByASelector)
  {
    ShowInventory();

    EXPECT_THAT(Queried("panel.inventory > panel > button:enabled"), ElementsAre("sword", "potion", "use", "drop"));
    EXPECT_THAT(Queried("#slots > .slot:disabled"), ElementsAre("shield"));
    EXPECT_THAT(Queried(".slot:not(.weapon)"), ElementsAre("shield", "potion"));
    EXPECT_THAT(Queried("#slots > :first-child, #actions > :last-child"), ElementsAre("sword", "drop"));
    EXPECT_THAT(Queried(".weapon + button"), ElementsAre("shield"));
    EXPECT_THAT(Queried(".weapon ~ button"), ElementsAre("shield", "potion"));
    EXPECT_THAT(Queried(":root"), ElementsAre("window"));
    EXPECT_THAT(Queried("slider"), IsEmpty());
  }

  TEST_F(UiScriptingTest, FindsElementsByASelectorInsideOfAnElement)
  {
    ShowInventory();

    EXPECT_THAT(Queried("button", Handle("actions")), ElementsAre("use", "drop"));
    EXPECT_THAT(Queried(":scope > button", Handle("slots")), ElementsAre("sword", "shield", "potion"));
    EXPECT_THAT(Queried(":scope > button", Handle("window")), IsEmpty());

    // the selector is about the whole file, and what is found is inside
    EXPECT_THAT(Queried(".inventory button.danger", Handle("actions")), ElementsAre("drop"));

    EXPECT_THAT(Queried("*", Handle("sword")), IsEmpty());
  }

  TEST_F(UiScriptingTest, FindsTheFirstOfWhatASelectorMatches)
  {
    ShowInventory();

    EXPECT_EQ(_ui->QueryFirst(".slot"), Handle("sword"));
    EXPECT_EQ(_ui->QueryFirst("button", Handle("actions")), Handle("use"));
    EXPECT_FALSE(_ui->QueryFirst("slider").IsSet());
  }

  TEST_F(UiScriptingTest, SaysWhetherASelectorMatchesAnElement)
  {
    ShowInventory();

    EXPECT_TRUE(_ui->Matches(Handle("sword"), ".slot.weapon"));
    EXPECT_TRUE(_ui->Matches(Handle("sword"), "#slots > button"));
    EXPECT_FALSE(_ui->Matches(Handle("sword"), ".danger"));
    EXPECT_FALSE(_ui->Matches(UiHandle{9999}, "*"));
  }

  TEST_F(UiScriptingTest, FindsNothingWithASelectorThatCannotBeReadAndSaysSo)
  {
    ShowInventory();
    _logger->Clear();

    EXPECT_THAT(Queried("button:visited"), IsEmpty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "The selector 'button:visited' cannot be read"));
    EXPECT_FALSE(_ui->Matches(Handle("sword"), "> button"));
  }

  TEST_F(UiScriptingTest, LooksInTheFileOnTopFirst)
  {
    ShowInventory();
    ASSERT_GE(Show("root:\n  type: button\n  name: sword\n  class: slot\n  text: a\n", "ui/second.ui.yml"), 0);

    const auto slots = _ui->FindByClass("slot");
    ASSERT_EQ(slots.size(), 4u);
    EXPECT_EQ(_ui->GetParent(slots[0]), UiHandle{});
    EXPECT_EQ(_ui->GetElementName(_ui->GetParent(slots[1])), "slots");

    EXPECT_EQ(Handle("sword"), slots[0]);
  }

  TEST_F(UiScriptingTest, GoesFromAnElementToThoseAroundIt)
  {
    ShowInventory();

    EXPECT_EQ(_ui->GetParent(Handle("sword")), Handle("slots"));
    EXPECT_EQ(_ui->GetParent(Handle("slots")), Handle("window"));
    EXPECT_FALSE(_ui->GetParent(Handle("window")).IsSet());

    EXPECT_THAT(NamesOf(_ui->GetChildren(Handle("slots"))), ElementsAre("sword", "shield", "potion"));
    EXPECT_THAT(NamesOf(_ui->GetChildren(Handle("sword"))), IsEmpty());

    EXPECT_EQ(_ui->GetNextSibling(Handle("sword")), Handle("shield"));
    EXPECT_EQ(_ui->GetNextSibling(Handle("shield")), Handle("potion"));
    EXPECT_FALSE(_ui->GetNextSibling(Handle("potion")).IsSet());

    EXPECT_EQ(_ui->GetPreviousSibling(Handle("potion")), Handle("shield"));
    EXPECT_FALSE(_ui->GetPreviousSibling(Handle("sword")).IsSet());
    EXPECT_FALSE(_ui->GetPreviousSibling(Handle("window")).IsSet());
  }

  TEST_F(UiScriptingTest, HandsOutTheElementAtTheTopOfAFile)
  {
    ShowInventory();
    const int second = Show("root:\n  type: panel\n  name: second\n", "ui/second.ui.yml");
    ASSERT_GE(second, 0);

    EXPECT_EQ(_ui->GetRoot(), Handle("second"));
    EXPECT_EQ(_ui->GetRoot(second), Handle("second"));
    EXPECT_EQ(_ui->GetRoot(second - 1), Handle("window"));
    EXPECT_FALSE(_ui->GetRoot(99).IsSet());
  }

  // handles

  TEST_F(UiScriptingTest, KeepsAHandleForAsLongAsTheElementIsThere)
  {
    ShowInventory();

    const UiHandle sword = Handle("sword");
    for (int i = 0; i < 5; i++) { Frame(); }

    EXPECT_TRUE(_ui->IsAlive(sword));
    EXPECT_EQ(Handle("sword"), sword);
  }

  TEST_F(UiScriptingTest, SaysThatAnElementIsGoneOnceItWasRemoved)
  {
    ShowInventory();

    const UiHandle slots = Handle("slots");
    const UiHandle sword = Handle("sword");

    ASSERT_TRUE(_ui->Remove(slots));

    // with everything inside it
    EXPECT_FALSE(_ui->IsAlive(slots));
    EXPECT_FALSE(_ui->IsAlive(sword));
    EXPECT_FALSE(Handle("sword").IsSet());
    EXPECT_TRUE(_ui->IsAlive(Handle("use")));

    Frame();
    EXPECT_THAT(NamesOf(_ui->GetChildren(Handle("window"))), ElementsAre("title", "actions"));
  }

  TEST_F(UiScriptingTest, SaysThatAnElementIsGoneOnceItsFileIsNoLongerShown)
  {
    ShowInventory();
    const UiHandle sword = Handle("sword");

    _ui->Unload(_ui->Load(kPath) - 1);
    EXPECT_FALSE(_ui->IsAlive(sword));
  }

  TEST_F(UiScriptingTest, DoesNothingWithTheHandleOfAnElementThatIsGone)
  {
    ShowInventory();

    const UiHandle sword = Handle("sword");
    ASSERT_TRUE(_ui->Remove(sword));
    Frame();

    float x = 0.0f;
    float y = 0.0f;
    UiBox box;
    FieldValue value;

    EXPECT_EQ(_ui->GetElementName(sword), "");
    EXPECT_EQ(_ui->GetElementType(sword), "");
    EXPECT_FALSE(_ui->GetParent(sword).IsSet());
    EXPECT_THAT(_ui->GetChildren(sword), IsEmpty());
    EXPECT_FALSE(_ui->Set(sword, "opacity", "0.5"));
    EXPECT_EQ(_ui->GetComputed(sword, "opacity"), "");
    EXPECT_FALSE(_ui->AddClass(sword, "a"));
    EXPECT_FALSE(_ui->RemoveClass(sword, "slot"));
    EXPECT_FALSE(_ui->ToggleClass(sword, "a"));
    EXPECT_FALSE(_ui->HasClass(sword, "slot"));
    EXPECT_THAT(_ui->GetClasses(sword), IsEmpty());
    EXPECT_FALSE(_ui->SetElementText(sword, "a"));
    EXPECT_EQ(_ui->GetElementText(sword), "");
    EXPECT_FALSE(_ui->SetField(sword, "text", FieldValue{std::string("a")}));
    EXPECT_FALSE(_ui->GetField(sword, "text", value));
    EXPECT_FALSE(_ui->SetVisible(sword, false));
    EXPECT_FALSE(_ui->IsVisible(sword));
    EXPECT_FALSE(_ui->FocusElement(sword));
    EXPECT_FALSE(_ui->GetScroll(sword, x, y));
    EXPECT_FALSE(_ui->SetScroll(sword, 1.0f, 1.0f));
    EXPECT_FALSE(_ui->ScrollIntoView(sword));
    EXPECT_FALSE(_ui->GetBox(sword, box));
    EXPECT_FALSE(_ui->Remove(sword));
    EXPECT_FALSE(_ui->Move(sword, Handle("actions")));
    EXPECT_FALSE(_ui->Create("{type: label}", sword).IsSet());
    EXPECT_EQ(_ui->On(sword, "click", [](const UiElementEvent &) {}), 0);
    EXPECT_FALSE(_ui->StartAnimation(sword, "fade"));
    EXPECT_FALSE(_ui->StopAnimation(sword));
  }

  TEST_F(UiScriptingTest, NeverGivesTheNumberOfAnElementThatIsGoneToAnother)
  {
    ShowInventory();

    const UiHandle sword = Handle("sword");
    ASSERT_TRUE(_ui->Remove(sword));

    for (int i = 0; i < 20; i++)
    {
      const UiHandle made = _ui->Create("{type: button, text: New}", Handle("slots"));
      ASSERT_TRUE(made.IsSet());
      EXPECT_NE(made, sword);
    }

    EXPECT_FALSE(_ui->IsAlive(sword));
  }

  TEST_F(UiScriptingTest, LosesTheFocusOfAnElementThatIsRemoved)
  {
    ShowInventory();

    ASSERT_TRUE(_ui->FocusElement(Handle("sword")));
    Frame();
    EXPECT_EQ(_ui->GetFocused(), "sword");

    ASSERT_TRUE(_ui->Remove(Handle("slots")));
    Frame();

    EXPECT_EQ(_ui->GetFocused(), "");
    EXPECT_FALSE(_ui->GetFocusedElement().IsSet());
  }

  TEST_F(UiScriptingTest, SurvivesAnElementThatIsRemovedWhileItIsPressed)
  {
    ShowInventory();

    const auto &box = Element("sword").GetBox();
    Release();
    PointAt(box.left + 5.0, box.top + 5.0);
    _input.state.SetAction(Action::Pointer_Primary);
    Frame();

    ASSERT_TRUE(_ui->Remove(Handle("sword")));

    Frame();
    Release();
    Frame();

    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  // properties

  TEST_F(UiScriptingTest, SetsAPropertyOfAnElement)
  {
    ShowInventory();
    const UiHandle sword = Handle("sword");

    ASSERT_TRUE(_ui->Set(sword, "background-color", "#334"));
    ASSERT_TRUE(_ui->Set(sword, "width", "120px"));
    Frame();

    EXPECT_EQ(_ui->GetComputed(sword, "background-color"), "rgb(51, 51, 68)");
    EXPECT_NEAR(Element("sword").GetBox().Width(), 120.0f, 0.01f);
  }

  TEST_F(UiScriptingTest, WritesThePropertyEitherWay)
  {
    ShowInventory();
    const UiHandle sword = Handle("sword");

    ASSERT_TRUE(_ui->Set(sword, "background_color", "rgb(1, 2, 3)"));
    EXPECT_EQ(_ui->GetComputed(sword, "background-color"), "rgb(1, 2, 3)");
    EXPECT_EQ(_ui->GetComputed(sword, "background_color"), "rgb(1, 2, 3)");
  }

  TEST_F(UiScriptingTest, LetsWhatIsSetWinOverTheSheetAndTheFile)
  {
    ShowInventory();
    const UiHandle drop = Handle("drop");

    EXPECT_EQ(_ui->GetComputed(drop, "background-color"), "rgb(255, 0, 0)");

    ASSERT_TRUE(_ui->Set(drop, "background-color", "#00ff00"));
    EXPECT_EQ(_ui->GetComputed(drop, "background-color"), "rgb(0, 255, 0)");

    // what was set last holds
    ASSERT_TRUE(_ui->Set(drop, "background-color", "blue"));
    EXPECT_EQ(_ui->GetComputed(drop, "background-color"), "rgb(0, 0, 255)");
  }

  TEST_F(UiScriptingTest, TakesBackWhatWasSetWithAnEmptyValue)
  {
    ShowInventory();
    const UiHandle drop = Handle("drop");

    ASSERT_TRUE(_ui->Set(drop, "background-color", "#00ff00"));
    ASSERT_TRUE(_ui->Set(drop, "background-color", ""));

    EXPECT_EQ(_ui->GetComputed(drop, "background-color"), "rgb(255, 0, 0)");
  }

  TEST_F(UiScriptingTest, RefusesAPropertyThatIsNotKnownAndAValueThatCannotBeRead)
  {
    ShowInventory();
    const UiHandle sword = Handle("sword");
    _logger->Clear();

    EXPECT_FALSE(_ui->Set(sword, "colour", "#ff0000"));
    EXPECT_FALSE(_ui->Set(sword, "opacity", "most"));
    EXPECT_FALSE(_ui->Set(sword, "width", "calc(1px +)"));

    EXPECT_EQ(_ui->GetComputed(sword, "opacity"), "1");
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn,
      "'colour' of button 'sword' is not a property that is known. Near to it are: color. Nothing is set"));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn,
      "'opacity' of button 'sword' is 'most', where a number from 0 to 1 was expected. Nothing is set"));
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 3u);
  }

  TEST_F(UiScriptingTest, ReadsWhatAPropertyCameToAfterTheCascade)
  {
    ShowInventory();

    EXPECT_EQ(_ui->GetComputed(Handle("sword"), "width"), "64px");
    EXPECT_EQ(_ui->GetComputed(Handle("sword"), "box-sizing"), "border-box");
    EXPECT_EQ(_ui->GetComputed(Handle("sword"), "opacity"), "1");
    EXPECT_EQ(_ui->GetComputed(Handle("sword"), "color"), "rgb(255, 255, 255)");
    EXPECT_EQ(_ui->GetComputed(Handle("sword"), "padding"), "8px 16px 8px 16px");
    EXPECT_EQ(_ui->GetComputed(Handle("sword"), "display"), "flex");
    EXPECT_EQ(_ui->GetComputed(Handle("title"), "font-size"), "16px");
    EXPECT_EQ(_ui->GetComputed(Handle("window"), "flex-direction"), "column");
    EXPECT_EQ(_ui->GetComputed(Handle("window"), "width"), "100%");
    EXPECT_EQ(_ui->GetComputed(Handle("sword"), "colour"), "");
  }

  TEST_F(UiScriptingTest, SetsACustomPropertyForAnElementAndWhatIsInside)
  {
    WriteAsset("ui/theme.css", "label { color: var(--accent, #ffffff); }");

    ASSERT_GE(Show(
      "styles: [theme.css]\n"
      "root:\n"
      "  type: panel\n"
      "  name: window\n"
      "  children:\n"
      "    - {type: label, name: a, text: a}\n"), 0);
    Frame();

    EXPECT_EQ(_ui->GetComputed(Handle("a"), "color"), "rgb(255, 255, 255)");

    ASSERT_TRUE(_ui->Set(Handle("window"), "--accent", "#ff8000"));
    Frame();

    EXPECT_EQ(_ui->GetComputed(Handle("a"), "color"), "rgb(255, 128, 0)");
    EXPECT_EQ(_ui->GetComputed(Handle("a"), "--accent"), "#ff8000");
    EXPECT_EQ(_ui->GetComputed(Handle("window"), "--accent"), "#ff8000");
    EXPECT_EQ(_ui->GetComputed(Handle("a"), "--missing"), "");
  }

  // classes

  TEST_F(UiScriptingTest, AddsRemovesAndTogglesClasses)
  {
    ShowInventory();
    const UiHandle potion = Handle("potion");

    EXPECT_THAT(_ui->GetClasses(potion), ElementsAre("slot"));
    EXPECT_TRUE(_ui->HasClass(potion, "slot"));
    EXPECT_FALSE(_ui->HasClass(potion, "selected"));

    EXPECT_TRUE(_ui->AddClass(potion, "selected"));
    EXPECT_FALSE(_ui->AddClass(potion, "selected"));
    EXPECT_THAT(_ui->GetClasses(potion), ElementsAre("slot", "selected"));

    EXPECT_TRUE(_ui->RemoveClass(potion, "slot"));
    EXPECT_FALSE(_ui->RemoveClass(potion, "slot"));
    EXPECT_THAT(_ui->GetClasses(potion), ElementsAre("selected"));

    EXPECT_FALSE(_ui->ToggleClass(potion, "selected"));
    EXPECT_THAT(_ui->GetClasses(potion), IsEmpty());

    EXPECT_TRUE(_ui->ToggleClass(potion, "selected"));
    EXPECT_TRUE(_ui->HasClass(potion, "selected"));
  }

  TEST_F(UiScriptingTest, GivesAnElementTheStyleOfTheClassesItHas)
  {
    ShowInventory();
    const UiHandle potion = Handle("potion");

    EXPECT_EQ(_ui->GetComputed(potion, "opacity"), "1");

    _ui->AddClass(potion, "selected");
    Frame();
    EXPECT_EQ(_ui->GetComputed(potion, "opacity"), "0.5");

    _ui->RemoveClass(potion, "selected");
    Frame();
    EXPECT_EQ(_ui->GetComputed(potion, "opacity"), "1");

    _ui->RemoveClass(potion, "slot");
    Frame();
    EXPECT_EQ(_ui->GetComputed(potion, "width"), "auto");
  }

  TEST_F(UiScriptingTest, StylesWhatIsInsideAnElementWhoseClassChanged)
  {
    WriteAsset("ui/theme.css", ".open label { opacity: 0.5; }\n.open + panel { opacity: 0.25; }");

    ASSERT_GE(Show(
      "styles: [theme.css]\n"
      "root:\n"
      "  type: panel\n"
      "  children:\n"
      "    - type: panel\n"
      "      name: menu\n"
      "      children:\n"
      "        - {type: label, name: inside, text: a}\n"
      "    - {type: panel, name: behind}\n"), 0);
    Frame();

    _ui->AddClass(Handle("menu"), "open");
    Frame();

    EXPECT_EQ(_ui->GetComputed(Handle("inside"), "opacity"), "0.5");
    EXPECT_EQ(_ui->GetComputed(Handle("behind"), "opacity"), "0.25");

    _ui->RemoveClass(Handle("menu"), "open");
    Frame();

    EXPECT_EQ(_ui->GetComputed(Handle("inside"), "opacity"), "1");
    EXPECT_EQ(_ui->GetComputed(Handle("behind"), "opacity"), "1");
  }

  // text, fields, and whether an element is shown

  TEST_F(UiScriptingTest, ReadsAndChangesTheTextOfAnElement)
  {
    ShowInventory();

    EXPECT_EQ(_ui->GetElementText(Handle("title")), "Inventory");
    EXPECT_EQ(_ui->GetElementText(Handle("sword")), "Sword");

    ASSERT_TRUE(_ui->SetElementText(Handle("title"), "Chest"));
    Frame();

    EXPECT_EQ(_ui->GetElementText(Handle("title")), "Chest");
    EXPECT_NEAR(Element("title").GetBox().Width(), 5 * 8.0f, 0.01f);

    // what has no text has none to set
    EXPECT_FALSE(_ui->SetElementText(Handle("slots"), "a"));
    EXPECT_EQ(_ui->GetElementText(Handle("slots")), "");
  }

  TEST_F(UiScriptingTest, ShowsTheValuesOfTheGameInATextThatWasSet)
  {
    ShowInventory();
    _ui->SetNumber("gold", 12);

    ASSERT_TRUE(_ui->SetElementText(Handle("title"), "Gold: {gold}"));
    Frame();
    EXPECT_EQ(_ui->GetElementText(Handle("title")), "Gold: 12");

    _ui->SetNumber("gold", 13);
    Frame();
    EXPECT_EQ(_ui->GetElementText(Handle("title")), "Gold: 13");
  }

  TEST_F(UiScriptingTest, ReadsAndChangesTheFieldsOfAnElement)
  {
    ShowInventory();
    const UiHandle shield = Handle("shield");

    FieldValue value;
    ASSERT_TRUE(_ui->GetField(shield, "enabled", value));
    EXPECT_EQ(std::get<bool>(value), false);

    ASSERT_TRUE(_ui->GetField(shield, "text", value));
    EXPECT_EQ(std::get<std::string>(value), "Shield");

    ASSERT_TRUE(_ui->SetField(shield, "enabled", FieldValue{true}));
    Frame();

    ASSERT_TRUE(_ui->GetField(shield, "enabled", value));
    EXPECT_EQ(std::get<bool>(value), true);
    EXPECT_THAT(Queried("#slots > :disabled"), IsEmpty());

    // what every element has
    ASSERT_TRUE(_ui->SetField(shield, "title", FieldValue{std::string("Blocks")}));
    ASSERT_TRUE(_ui->GetField(shield, "title", value));
    EXPECT_EQ(std::get<std::string>(value), "Blocks");

    ASSERT_TRUE(_ui->SetField(shield, "tab_index", FieldValue{3}));
    ASSERT_TRUE(_ui->GetField(shield, "tab_index", value));
    EXPECT_EQ(std::get<int>(value), 3);
  }

  TEST_F(UiScriptingTest, RefusesAFieldAnElementDoesNotHaveAndAValueItDoesNotTake)
  {
    ShowInventory();
    _logger->Clear();

    FieldValue value;
    EXPECT_FALSE(_ui->GetField(Handle("title"), "enabled", value));
    EXPECT_FALSE(_ui->SetField(Handle("title"), "enabled", FieldValue{true}));
    EXPECT_FALSE(_ui->SetField(Handle("sword"), "enabled", FieldValue{std::string("yes")}));
    EXPECT_FALSE(_ui->SetField(Handle("sword"), "text", FieldValue{3}));

    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "'enabled' is not a field of label 'title'. Nothing is set"));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn, "'enabled' of button 'sword' takes true or false. Nothing is set"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "'text' of button 'sword' takes text. Nothing is set"));
  }

  TEST_F(UiScriptingTest, HidesAnElementAndShowsItAgain)
  {
    ShowInventory();
    const UiHandle slots = Handle("slots");

    EXPECT_TRUE(_ui->IsVisible(slots));
    EXPECT_TRUE(_ui->IsVisible(Handle("sword")));
    const float top = Element("actions").GetBox().top;

    ASSERT_TRUE(_ui->SetVisible(slots, false));
    Frame();

    EXPECT_FALSE(_ui->IsVisible(slots));
    EXPECT_FALSE(_ui->IsVisible(Handle("sword")));

    // what is hidden takes no room
    EXPECT_LT(Element("actions").GetBox().top, top);

    ASSERT_TRUE(_ui->SetVisible(slots, true));
    Frame();

    EXPECT_TRUE(_ui->IsVisible(Handle("sword")));
    EXPECT_NEAR(Element("actions").GetBox().top, top, 0.01f);
  }

  TEST_F(UiScriptingTest, SaysThatWhatIsHiddenByItsStyleIsNotShown)
  {
    ShowInventory();

    _ui->Set(Handle("slots"), "visibility", "hidden");
    Frame();

    // it is inherited
    EXPECT_FALSE(_ui->IsVisible(Handle("sword")));

    _ui->Set(Handle("sword"), "visibility", "visible");
    Frame();
    EXPECT_TRUE(_ui->IsVisible(Handle("sword")));
    EXPECT_FALSE(_ui->IsVisible(Handle("potion")));
  }

  TEST_F(UiScriptingTest, MovesTheFocusAndTakesItAway)
  {
    ShowInventory();

    EXPECT_FALSE(_ui->GetFocusedElement().IsSet());

    ASSERT_TRUE(_ui->FocusElement(Handle("potion")));
    EXPECT_EQ(_ui->GetFocusedElement(), Handle("potion"));
    EXPECT_EQ(_ui->GetFocused(), "potion");

    // what cannot have the focus does not get it
    EXPECT_FALSE(_ui->FocusElement(Handle("title")));
    EXPECT_EQ(_ui->GetFocusedElement(), Handle("potion"));

    _ui->Blur();
    EXPECT_FALSE(_ui->GetFocusedElement().IsSet());
  }

  TEST_F(UiScriptingTest, SaysWhereAnElementIs)
  {
    ShowInventory();
    _renderer.SetResolution(960, 540);
    Frame();

    UiBox box;
    ASSERT_TRUE(_ui->GetBox(Handle("shield"), box));

    // in units of the file, whatever the size of what is shown
    EXPECT_FLOAT_EQ(box.width, 64.0f);
    EXPECT_FLOAT_EQ(box.height, 64.0f);
    EXPECT_FLOAT_EQ(box.left, 64.0f);
    EXPECT_FLOAT_EQ(box.top, 16.0f);
    EXPECT_FLOAT_EQ(box.scale, 0.5f);
  }

  // making and removing

  TEST_F(UiScriptingTest, MakesAnElementFromText)
  {
    ShowInventory();

    const UiHandle made = _ui->Create(
      "{type: button, name: bow, class: slot weapon, text: Bow}", Handle("slots"));

    ASSERT_TRUE(made.IsSet());
    EXPECT_TRUE(_ui->IsAlive(made));
    EXPECT_EQ(Handle("bow"), made);
    EXPECT_THAT(NamesOf(_ui->GetChildren(Handle("slots"))), ElementsAre("sword", "shield", "potion", "bow"));

    Frame();

    // it has the style of its classes, and a place
    EXPECT_EQ(_ui->GetComputed(made, "width"), "64px");
    ExpectBox("bow", 192.0f, 16.0f, 64.0f, 64.0f);
    EXPECT_THAT(Queried(".weapon"), ElementsAre("sword", "bow"));
  }

  TEST_F(UiScriptingTest, MakesAnElementWithWhatIsInsideIt)
  {
    ShowInventory();

    const UiHandle made = _ui->Create(
      "type: panel\n"
      "name: details\n"
      "flex_direction: column\n"
      "children:\n"
      "  - {type: label, name: heading, text: Sword}\n"
      "  - type: panel\n"
      "    children:\n"
      "      - {type: bar, name: damage, value: 3, max: 10}\n",
      Handle("window"));

    ASSERT_TRUE(made.IsSet());
    Frame();

    EXPECT_EQ(_ui->GetParent(Handle("heading")), made);
    EXPECT_TRUE(Handle("damage").IsSet());
    EXPECT_GT(Element("damage").GetBox().Width(), 0.0f);
    EXPECT_THAT(Queried("#details bar"), ElementsAre("damage"));
  }

  TEST_F(UiScriptingTest, PutsAnElementInFrontOfAnother)
  {
    ShowInventory();

    ASSERT_TRUE(_ui->Create("{type: button, name: first, text: a}", Handle("slots"), 0).IsSet());
    ASSERT_TRUE(_ui->Create("{type: button, name: third, text: a}", Handle("slots"), 2).IsSet());
    ASSERT_TRUE(_ui->Create("{type: button, name: last, text: a}", Handle("slots"), 99).IsSet());

    EXPECT_THAT(
      NamesOf(_ui->GetChildren(Handle("slots"))),
      ElementsAre("first", "sword", "third", "shield", "potion", "last"));

    Frame();
    EXPECT_NEAR(Element("first").GetBox().left, 0.0f, 0.01f);
    EXPECT_LT(Element("first").GetBox().left, Element("sword").GetBox().left);
    EXPECT_LT(Element("third").GetBox().left, Element("shield").GetBox().left);
  }

  TEST_F(UiScriptingTest, MakesAnElementFromValues)
  {
    ShowInventory();

    neon::DataValue description = neon::DataValue::Map();
    description.Set("type", neon::DataValue::Text("label"));
    description.Set("name", neon::DataValue::Text("hint"));
    description.Set("text", neon::DataValue::Text("Press to use"));
    description.Set("font_size", neon::DataValue::Number(20));

    const UiHandle made = _ui->CreateFrom(description, Handle("actions"));
    ASSERT_TRUE(made.IsSet());
    Frame();

    EXPECT_EQ(_ui->GetElementText(made), "Press to use");
    EXPECT_EQ(_ui->GetComputed(made, "font-size"), "20px");
  }

  TEST_F(UiScriptingTest, SaysWhatIsWrongWithWhatDescribesAnElementAndMakesNone)
  {
    ShowInventory();
    _logger->Clear();

    EXPECT_FALSE(_ui->Create("{type: lable, text: a}", Handle("slots")).IsSet());
    EXPECT_FALSE(_ui->Create("{type: label, colour: red}", Handle("slots")).IsSet());
    EXPECT_FALSE(_ui->Create("type: [", Handle("slots")).IsSet());

    EXPECT_THAT(NamesOf(_ui->GetChildren(Handle("slots"))), ElementsAre("sword", "shield", "potion"));

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "type 'lable' of the element is not known"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "'colour' is not known to the element"));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "assets://ui/test.ui.yml: what describes the element is wrong, and no element is made"));
  }

  TEST_F(UiScriptingTest, MakesNothingInsideOfWhatTakesNothingInside)
  {
    ShowInventory();
    _logger->Clear();

    EXPECT_FALSE(_ui->Create("{type: label, text: a}", Handle("title")).IsSet());
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn,
      "assets://ui/test.ui.yml: label 'title' takes nothing inside, and no element is made"));
  }

  TEST_F(UiScriptingTest, MakesCopiesOfATemplate)
  {
    ShowInventory();

    const UiHandle bow = _ui->CreateFromTemplate(
      "slot", Handle("slots"), UiRow{{"index", "7"}, {"title", "Bow"}});
    const UiHandle axe = _ui->CreateFromTemplate(
      "slot", Handle("slots"), UiRow{{"index", "8"}, {"title", "Axe"}}, 0);

    ASSERT_TRUE(bow.IsSet());
    ASSERT_TRUE(axe.IsSet());
    Frame();

    EXPECT_EQ(_ui->GetElementName(bow), "slot-7");
    EXPECT_EQ(_ui->GetElementText(bow), "Bow");
    EXPECT_EQ(_ui->GetElementText(axe), "Axe");
    EXPECT_TRUE(_ui->HasClass(bow, "slot"));
    EXPECT_EQ(_ui->GetComputed(bow, "width"), "64px");

    EXPECT_THAT(
      NamesOf(_ui->GetChildren(Handle("slots"))),
      ElementsAre("slot-8", "sword", "shield", "potion", "slot-7"));
  }

  TEST_F(UiScriptingTest, MakesCopiesOfATemplateThatShareAName)
  {
    ShowInventory();

    const UiHandle first = _ui->CreateFromTemplate("divider", Handle("window"));
    const UiHandle second = _ui->CreateFromTemplate("divider", Handle("window"));

    ASSERT_TRUE(first.IsSet());
    ASSERT_TRUE(second.IsSet());
    EXPECT_NE(first, second);
    EXPECT_THAT(Queried(".divider").size(), 2u);
  }

  TEST_F(UiScriptingTest, ShowsWhatATemplateAsksForAndIsNotGiven)
  {
    ShowInventory();

    const UiHandle made = _ui->CreateFromTemplate("slot", Handle("slots"), UiRow{{"index", "1"}});
    ASSERT_TRUE(made.IsSet());
    Frame();

    // as a value of the game that is not set is
    EXPECT_EQ(_ui->GetElementText(made), "${title}");
  }

  TEST_F(UiScriptingTest, SaysThatThereIsNoSuchTemplate)
  {
    ShowInventory();
    _logger->Clear();

    EXPECT_FALSE(_ui->CreateFromTemplate("missing", Handle("slots")).IsSet());
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn, "assets://ui/test.ui.yml: there is no template 'missing', and no element is made"));
  }

  TEST_F(UiScriptingTest, DoesNotShowTheTemplatesThemselves)
  {
    ShowInventory();

    EXPECT_THAT(Queried(".divider"), IsEmpty());
    EXPECT_FALSE(Handle("slot-${index}").IsSet());
  }

  TEST_F(UiScriptingTest, SaysWhatIsWrongWithATemplateWhenTheFileIsRead)
  {
    ExpectProblems(
      "templates:\n"
      "  row:\n"
      "    type: label\n"
      "    colour: red\n"
      "  other:\n"
      "    text: \"${title}\"\n"
      "root:\n"
      "  type: panel\n",
      {
        "assets://ui/test.ui.yml:4: 'colour' is not known to template 'row'. Known are: type, name, hidden, "
        "text, focus, hover, active, disabled, display, position, box_sizing, width, height, min_width, "
        "min_height, max_width, max_height, margin, margin_top, margin_right, margin_bottom, margin_left, "
        "padding, padding_top, padding_right, padding_bottom, padding_left, border, border_width, "
        "border_color, top, right, bottom, left, flex_direction, flex_wrap, justify_content, align_items, "
        "align_self, align_content, flex, flex_grow, flex_shrink, flex_basis, gap, row_gap, column_gap, "
        "background_color, background_image, border_image_source, border_image_slice, border_image_width, "
        "outline_width, outline_offset, outline_color, opacity, overflow, pointer_events, z_index, color, "
        "font_family, font_size, font_weight, text_align, line_height, accent_color, object_fit, "
        "letter_spacing, word_spacing, text_transform, text_decoration, text_decoration_line, "
        "text_decoration_color, text_decoration_thickness, text_shadow, white_space, text_overflow, "
        "font_style, text_stroke_width, text_stroke_color, direction, image_rendering, object_position, "
        "background, background_size, background_position, background_repeat, border_image_repeat, "
        "border_radius, border_top_left_radius, border_top_right_radius, border_bottom_right_radius, "
        "border_bottom_left_radius, border_top, border_right, border_bottom, border_left, border_top_width, "
        "border_right_width, border_bottom_width, border_left_width, border_top_color, border_right_color, "
        "border_bottom_color, border_left_color, box_shadow, transform, transform_origin, shader, "
        "shader_values, "
        "visibility, cursor, overflow_x, overflow_y, scrollbar_width, scrollbar_color, scroll_behavior, "
        "scroll_drag, caret_color, transition, transition_property, transition_duration, transition_delay, "
        "transition_timing_function, animation, animation_name, animation_duration, animation_delay, "
        "animation_timing_function, animation_iteration_count, animation_direction, animation_fill_mode, "
        "animation_play_state, class, title, draggable, for_each, template, tab_index",
        "assets://ui/test.ui.yml:5: template 'other' has no 'type', where one of these was expected: "
        "input, textarea, checkbox, radio, toggle, slider, select, panel, label, image, button, bar"
      });
  }

  TEST_F(UiScriptingTest, MovesAnElementIntoAnother)
  {
    ShowInventory();
    const UiHandle potion = Handle("potion");

    ASSERT_TRUE(_ui->Move(potion, Handle("actions"), 1));
    Frame();

    // it is the element it was
    EXPECT_TRUE(_ui->IsAlive(potion));
    EXPECT_EQ(Handle("potion"), potion);
    EXPECT_EQ(_ui->GetParent(potion), Handle("actions"));

    EXPECT_THAT(NamesOf(_ui->GetChildren(Handle("slots"))), ElementsAre("sword", "shield"));
    EXPECT_THAT(NamesOf(_ui->GetChildren(Handle("actions"))), ElementsAre("use", "potion", "drop"));

    EXPECT_GT(Element("potion").GetBox().top, Element("sword").GetBox().bottom - 0.01f);
  }

  TEST_F(UiScriptingTest, MovesAnElementAmongThoseNextToIt)
  {
    ShowInventory();

    ASSERT_TRUE(_ui->Move(Handle("potion"), Handle("slots"), 0));
    EXPECT_THAT(NamesOf(_ui->GetChildren(Handle("slots"))), ElementsAre("potion", "sword", "shield"));

    ASSERT_TRUE(_ui->Move(Handle("potion"), Handle("slots")));
    EXPECT_THAT(NamesOf(_ui->GetChildren(Handle("slots"))), ElementsAre("sword", "shield", "potion"));
  }

  TEST_F(UiScriptingTest, KeepsTheFocusOfAnElementThatIsMoved)
  {
    ShowInventory();

    ASSERT_TRUE(_ui->FocusElement(Handle("potion")));
    Frame();

    ASSERT_TRUE(_ui->Move(Handle("potion"), Handle("actions")));
    Frame();

    EXPECT_EQ(_ui->GetFocused(), "potion");
  }

  TEST_F(UiScriptingTest, DoesNotMoveAnElementIntoItselfNorTheTopOfAFile)
  {
    ShowInventory();

    EXPECT_FALSE(_ui->Move(Handle("slots"), Handle("slots")));
    EXPECT_FALSE(_ui->Move(Handle("slots"), Handle("sword")));
    EXPECT_FALSE(_ui->Move(Handle("window"), Handle("slots")));
    EXPECT_FALSE(_ui->Move(Handle("sword"), Handle("title")));

    EXPECT_THAT(NamesOf(_ui->GetChildren(Handle("slots"))), ElementsAre("sword", "shield", "potion"));
  }

  TEST_F(UiScriptingTest, DoesNotRemoveTheElementAtTheTopOfAFile)
  {
    ShowInventory();

    EXPECT_FALSE(_ui->Remove(Handle("window")));
    EXPECT_TRUE(_ui->IsAlive(Handle("window")));
  }

  TEST_F(UiScriptingTest, PlacesWhatIsLeftWhenAnElementIsRemoved)
  {
    ShowInventory();

    const float left = Element("shield").GetBox().left;
    ASSERT_TRUE(_ui->Remove(Handle("sword")));
    Frame();

    EXPECT_LT(Element("shield").GetBox().left, left);
    EXPECT_NEAR(Element("shield").GetBox().left, 0.0f, 0.01f);
  }

  TEST_F(UiScriptingTest, StylesElementsByWhereTheyAreOnceOneIsAddedOrRemoved)
  {
    WriteAsset("ui/theme.css", "#list > label:last-child { opacity: 0.5; }");

    ASSERT_GE(Show(
      "styles: [theme.css]\n"
      "root:\n"
      "  type: panel\n"
      "  name: list\n"
      "  children:\n"
      "    - {type: label, name: a, text: a}\n"
      "    - {type: label, name: b, text: b}\n"), 0);
    Frame();

    EXPECT_EQ(_ui->GetComputed(Handle("b"), "opacity"), "0.5");

    const UiHandle made = _ui->Create("{type: label, name: c, text: c}", Handle("list"));
    Frame();

    EXPECT_EQ(_ui->GetComputed(Handle("b"), "opacity"), "1");
    EXPECT_EQ(_ui->GetComputed(made, "opacity"), "0.5");

    _ui->Remove(made);
    Frame();

    EXPECT_EQ(_ui->GetComputed(Handle("b"), "opacity"), "0.5");
  }

  // lists of the game

  class UiListTest : public UiScriptingTest
  {
  protected:
    void ShowList()
    {
      ASSERT_GE(Show(
        "templates:\n"
        "  row:\n"
        "    type: button\n"
        "    name: \"item-${index}\"\n"
        "    class: row\n"
        "    text: \"${number}. ${name} x${count}\"\n"
        "root:\n"
        "  type: panel\n"
        "  name: window\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  children:\n"
        "    - type: panel\n"
        "      name: list\n"
        "      flex_direction: column\n"
        "      align_items: flex-start\n"
        "      for_each: \"{items}\"\n"
        "      template: row\n"), 0) << _logger->Messages(LogLevel::Error);
    }
  };

  TEST_F(UiListTest, MakesAnElementForEveryRowOfAList)
  {
    ShowList();

    _ui->SetList("items", {
                   UiRow{{"name", "Sword"}, {"count", "1"}},
                   UiRow{{"name", "Arrow"}, {"count", "40"}},
                   UiRow{{"name", "Potion"}, {"count", "3"}}
                 });
    Frame();

    EXPECT_THAT(NamesOf(_ui->GetChildren(Handle("list"))), ElementsAre("item-0", "item-1", "item-2"));
    EXPECT_EQ(_ui->GetElementText(Handle("item-0")), "1. Sword x1");
    EXPECT_EQ(_ui->GetElementText(Handle("item-1")), "2. Arrow x40");
    EXPECT_EQ(_ui->GetElementText(Handle("item-2")), "3. Potion x3");

    ExpectBox("item-1", 0.0f, 32.0f, 12 * 8.0f + 32.0f, 32.0f);
  }

  TEST_F(UiListTest, ShowsNothingForAListThatWasNotHandedOver)
  {
    ShowList();
    Frame();

    EXPECT_THAT(_ui->GetChildren(Handle("list")), IsEmpty());
  }

  TEST_F(UiListTest, FollowsTheListWhenItChanges)
  {
    ShowList();

    _ui->SetList("items", {UiRow{{"name", "Sword"}, {"count", "1"}}, UiRow{{"name", "Arrow"}, {"count", "40"}}});
    Frame();
    EXPECT_EQ(_ui->GetChildren(Handle("list")).size(), 2u);

    _ui->SetList("items", {UiRow{{"name", "Arrow"}, {"count", "39"}}});
    Frame();

    EXPECT_THAT(NamesOf(_ui->GetChildren(Handle("list"))), ElementsAre("item-0"));
    EXPECT_EQ(_ui->GetElementText(Handle("item-0")), "1. Arrow x39");

    _ui->SetList("items", {});
    Frame();
    EXPECT_THAT(_ui->GetChildren(Handle("list")), IsEmpty());
  }

  TEST_F(UiListTest, MakesNothingAnewForAListThatIsWhatItWas)
  {
    ShowList();

    const std::vector<UiRow> rows = {UiRow{{"name", "Sword"}, {"count", "1"}}};
    _ui->SetList("items", rows);
    Frame();

    const UiHandle row = Handle("item-0");
    const auto before = _ui->GetStatistics();

    _ui->SetList("items", rows);
    Frame();

    EXPECT_TRUE(_ui->IsAlive(row));
    EXPECT_EQ(_ui->GetStatistics().layouts, before.layouts);
    EXPECT_EQ(_ui->GetStatistics().paints, before.paints);
  }

  TEST_F(UiListTest, ReportsAClickOnARow)
  {
    ShowList();
    _ui->SetList("items", {UiRow{{"name", "Sword"}, {"count", "1"}}, UiRow{{"name", "Arrow"}, {"count", "40"}}});
    Frame();

    ClickAt(10, 40);
    EXPECT_TRUE(_ui->WasClicked("item-1"));
  }

  TEST_F(UiListTest, ShowsAThousandRows)
  {
    ShowList();

    std::vector<UiRow> rows;
    for (int i = 0; i < 1000; i++) { rows.push_back(UiRow{{"name", "Item"}, {"count", std::to_string(i)}}); }

    _ui->SetList("items", rows);
    Frame();

    EXPECT_EQ(_ui->GetChildren(Handle("list")).size(), 1000u);
    EXPECT_EQ(_ui->GetElementText(Handle("item-999")), "1000. Item x999");
  }

  TEST_F(UiListTest, SaysWhatIsWrongWithForEach)
  {
    ExpectProblemsUnderRoot(
      "- {type: panel, name: a, for_each: items, template: row}\n"
      "- {type: panel, name: b, for_each: \"{items}\"}\n"
      "- {type: panel, name: c, template: row}\n"
      "- {type: label, name: d, for_each: \"{items}\", template: row}\n",
      {
        "assets://ui/test.ui.yml:4: 'for_each' of panel 'a' is 'items', where the name of a list was expected, "
        "such as \"{items}\"",
        "assets://ui/test.ui.yml:5: panel 'b' has 'for_each' and no 'template', where the name of the template "
        "its rows are made from was expected",
        "assets://ui/test.ui.yml:6: panel 'c' has 'template' and no 'for_each', where the list its rows are "
        "made for was expected",
        "assets://ui/test.ui.yml:7: label 'd' takes nothing inside, and cannot have 'for_each'"
      });
  }

  // events

  TEST_F(UiScriptingTest, TellsWhatListensAtAnElementOfAClick)
  {
    ShowInventory();

    std::vector<std::string> heard;
    const int subscription = _ui->On(Handle("sword"), "click", [&](const UiElementEvent &event)
    {
      heard.push_back(event.name + " " + event.target_name);
      EXPECT_EQ(event.target, Handle("sword"));
      EXPECT_EQ(event.current, Handle("sword"));
      EXPECT_EQ(event.document, "inventory");
    });
    EXPECT_GT(subscription, 0);

    const auto &box = Element("sword").GetBox();
    ClickAt(box.left + 5.0, box.top + 5.0);

    EXPECT_THAT(heard, ElementsAre("click sword"));

    // and of nothing that happens to another
    const auto &other = Element("potion").GetBox();
    ClickAt(other.left + 5.0, other.top + 5.0);
    EXPECT_EQ(heard.size(), 1u);
  }

  TEST_F(UiScriptingTest, HandsAnEventUpToTheElementsAbove)
  {
    ShowInventory();

    std::vector<std::string> heard;
    const auto listen = [&](const std::string &name)
    {
      _ui->On(Handle(name), "click", [&heard, name, this](const UiElementEvent &event)
      {
        heard.push_back(name + " hears of " + event.target_name);
        EXPECT_EQ(event.current, Handle(name));
      });
    };

    listen("window");
    listen("slots");
    listen("potion");
    listen("actions");

    const auto &box = Element("potion").GetBox();
    ClickAt(box.left + 5.0, box.top + 5.0);

    // from the element to the top, and not to what is next to it
    EXPECT_THAT(
      heard,
      ElementsAre("potion hears of potion", "slots hears of potion", "window hears of potion"));
  }

  TEST_F(UiScriptingTest, KeepsAnEventThatWasStoppedFromTheElementsAbove)
  {
    ShowInventory();

    std::vector<std::string> heard;
    _ui->On(Handle("window"), "click", [&](const UiElementEvent &) { heard.emplace_back("window"); });
    _ui->On(Handle("slots"), "click", [&](const UiElementEvent &event)
    {
      heard.emplace_back("slots");
      event.Stop();
    });
    _ui->On(Handle("slots"), "click", [&](const UiElementEvent &) { heard.emplace_back("slots again"); });
    _ui->On(Handle("potion"), "click", [&](const UiElementEvent &) { heard.emplace_back("potion"); });

    const auto &box = Element("potion").GetBox();
    ClickAt(box.left + 5.0, box.top + 5.0);

    // what listens at the same element still hears of it
    EXPECT_THAT(heard, ElementsAre("potion", "slots", "slots again"));
  }

  TEST_F(UiScriptingTest, StopsTellingWhatNoLongerListens)
  {
    ShowInventory();

    int heard = 0;
    const int subscription = _ui->On(Handle("sword"), "click", [&](const UiElementEvent &) { heard++; });

    const auto &box = Element("sword").GetBox();
    ClickAt(box.left + 5.0, box.top + 5.0);
    EXPECT_EQ(heard, 1);

    _ui->Off(subscription);
    ClickAt(box.left + 5.0, box.top + 5.0);
    EXPECT_EQ(heard, 1);

    // taking away what is not there does nothing
    _ui->Off(subscription);
    _ui->Off(0);
    _ui->Off(12345);
  }

  TEST_F(UiScriptingTest, TellsWhatListensToEveryElement)
  {
    ShowInventory();

    std::vector<std::string> heard;
    _ui->OnAny("click", [&](const UiElementEvent &event) { heard.push_back(event.target_name); });

    const auto &sword = Element("sword").GetBox();
    const auto &drop = Element("drop").GetBox();
    ClickAt(sword.left + 5.0, sword.top + 5.0);
    ClickAt(drop.left + 5.0, drop.top + 5.0);

    EXPECT_THAT(heard, ElementsAre("sword", "drop"));
  }

  TEST_F(UiScriptingTest, LetsAListenerChangeEverything)
  {
    ShowInventory();

    const UiHandle slots = Handle("slots");
    int heard_above = 0;

    _ui->On(Handle("window"), "click", [&](const UiElementEvent &) { heard_above++; });
    _ui->On(Handle("sword"), "click", [&](const UiElementEvent &)
    {
      // what it happened to, and everything around it
      _ui->Remove(slots);
      _ui->Create("{type: label, name: gone, text: Gone}", Handle("window"));
    });

    const auto &box = Element("sword").GetBox();
    ClickAt(box.left + 5.0, box.top + 5.0);

    EXPECT_FALSE(_ui->IsAlive(slots));
    EXPECT_TRUE(Handle("gone").IsSet());

    // the way up ended with the element that was removed
    EXPECT_EQ(heard_above, 0);

    Frame();
  }

  TEST_F(UiScriptingTest, LetsAListenerStopListening)
  {
    ShowInventory();

    int first = 0;
    int second = 0;
    int second_subscription = 0;

    _ui->On(Handle("sword"), "click", [&](const UiElementEvent &)
    {
      first++;
      _ui->Off(second_subscription);
    });
    second_subscription = _ui->On(Handle("sword"), "click", [&](const UiElementEvent &) { second++; });

    const auto &box = Element("sword").GetBox();
    ClickAt(box.left + 5.0, box.top + 5.0);

    EXPECT_EQ(first, 1);
    EXPECT_EQ(second, 0);
  }

  TEST_F(UiScriptingTest, LetsAListenerUnloadTheFile)
  {
    ShowInventory();
    const int document = 0;

    _ui->On(Handle("sword"), "click", [&](const UiElementEvent &) { _ui->Unload(document); });

    const auto &box = Element("sword").GetBox();
    ClickAt(box.left + 5.0, box.top + 5.0);

    EXPECT_FALSE(Handle("sword").IsSet());
    Frame();
  }

  TEST_F(UiScriptingTest, TellsOfThePointerComingAndGoing)
  {
    ShowInventory();

    const auto &sword = Element("sword").GetBox();
    const auto &potion = Element("potion").GetBox();

    // As in the DOM, the pointer enters what an element is inside of
    // with it, from the outside in, and leaves from the inside out.
    PointAt(sword.left + 5.0, sword.top + 5.0);
    Frame();
    EXPECT_THAT(Happened(), ElementsAre("pointer_enter window", "pointer_enter slots", "pointer_enter sword"));

    Frame();
    EXPECT_THAT(Happened(), IsEmpty());

    // what both are inside of, it neither leaves nor enters
    PointAt(potion.left + 5.0, potion.top + 5.0);
    Frame();
    EXPECT_THAT(Happened(), ElementsAre("pointer_leave sword", "pointer_enter potion"));

    PointAt(1900, 1000);
    Frame();
    EXPECT_THAT(Happened(), ElementsAre("pointer_leave potion", "pointer_leave slots", "pointer_leave window"));
  }

  TEST_F(UiScriptingTest, KeepsThePointerComingAndGoingAtTheElement)
  {
    ShowInventory();

    std::vector<std::string> heard;
    _ui->On(Handle("slots"), "pointer_enter", [&](const UiElementEvent &event)
    {
      heard.push_back(event.target_name);
    });

    const auto &sword = Element("sword").GetBox();
    const auto &potion = Element("potion").GetBox();

    PointAt(sword.left + 5.0, sword.top + 5.0);
    Frame();

    // it hears that the pointer entered it, and not what is inside it
    EXPECT_THAT(heard, ElementsAre("slots"));

    PointAt(potion.left + 5.0, potion.top + 5.0);
    Frame();
    EXPECT_THAT(heard, ElementsAre("slots"));
  }

  TEST_F(UiScriptingTest, TellsOfTheButtonOfThePointer)
  {
    ShowInventory();

    const auto &sword = Element("sword").GetBox();

    Release();
    PointAt(sword.left + 5.0, sword.top + 6.0);
    Frame();

    _input.state.SetAction(Action::Pointer_Primary);
    Frame();

    // what is pressed gets the focus
    EXPECT_THAT(Happened(), ElementsAre("pointer_down sword", "focused sword"));

    const auto &down = _ui->GetElementEvents()[0];
    EXPECT_FLOAT_EQ(down.x, sword.left + 5.0f);
    EXPECT_FLOAT_EQ(down.y, sword.top + 6.0f);
    EXPECT_EQ(down.clicks, 1);

    Release();
    Frame();
    EXPECT_THAT(Happened(), ElementsAre("pointer_up sword", "click sword"));
  }

  TEST_F(UiScriptingTest, TellsOfADoubleClick)
  {
    ShowInventory();
    const auto &sword = Element("sword").GetBox();

    PointAt(sword.left + 5.0, sword.top + 5.0);
    Frame();

    _ui->Advance(0.1);
    ClickAt(sword.left + 5.0, sword.top + 5.0);
    EXPECT_THAT(Happened(), ElementsAre("pointer_up sword", "click sword"));

    _ui->Advance(0.1);
    ClickAt(sword.left + 6.0, sword.top + 5.0);
    EXPECT_THAT(Happened(), ElementsAre("pointer_up sword", "click sword", "double_click sword"));
    EXPECT_EQ(_ui->GetElementEvents()[1].clicks, 2);
  }

  TEST_F(UiScriptingTest, TakesTwoClicksThatAreFarApartForTwo)
  {
    ShowInventory();
    const auto &sword = Element("sword").GetBox();

    ClickAt(sword.left + 5.0, sword.top + 5.0);

    // in time
    _ui->Advance(1.0);
    ClickAt(sword.left + 5.0, sword.top + 5.0);
    EXPECT_THAT(Happened(), ElementsAre("pointer_up sword", "click sword"));

    // and in place
    _ui->Advance(0.1);
    ClickAt(sword.left + 40.0, sword.top + 5.0);
    EXPECT_THAT(Happened(), ElementsAre("pointer_up sword", "click sword"));
  }

  TEST_F(UiScriptingTest, TellsOfTheFocusComingAndGoing)
  {
    ShowInventory();

    _ui->FocusElement(Handle("sword"));
    Frame();
    EXPECT_THAT(Happened(), ElementsAre("focused sword"));

    _ui->FocusElement(Handle("potion"));
    Frame();
    EXPECT_THAT(Happened(), ElementsAre("blurred sword", "focused potion"));
    EXPECT_EQ(_ui->GetElementEvents()[0].related, Handle("potion"));
    EXPECT_EQ(_ui->GetElementEvents()[1].related, Handle("sword"));

    _ui->Blur();
    Frame();
    EXPECT_THAT(Happened(), ElementsAre("blurred potion"));
  }

  TEST_F(UiScriptingTest, TellsWhatHasTheFocusOfKeys)
  {
    ShowInventory();
    _ui->FocusElement(Handle("sword"));
    Frame();

    Release();
    _input.state.AddKeyEvent({neon::Key::Home, true, false, {.shift = true}});
    _input.state.AddKeyEvent({neon::Key::Home, false, false, {}});
    Frame();

    EXPECT_THAT(Happened(), ElementsAre("key_down sword", "key_up sword"));
    EXPECT_EQ(_ui->GetElementEvents()[0].key, neon::Key::Home);
    EXPECT_EQ(_ui->GetElementEvents()[0].value, "home");
    EXPECT_TRUE(_ui->GetElementEvents()[0].modifiers.shift);

    // and nothing when nothing has it
    _ui->Blur();
    Frame();
    Release();
    _input.state.AddKeyEvent({neon::Key::Home, true, false, {}});
    Frame();
    EXPECT_THAT(Happened(), IsEmpty());
  }

  TEST_F(UiScriptingTest, TellsOfAClickWithTheKeys)
  {
    ShowInventory();
    _ui->FocusElement(Handle("sword"));
    Frame();

    Release();
    _input.state.SetAction(Action::Ui_Accept);
    Frame();

    EXPECT_THAT(Happened(), ElementsAre("click sword"));
  }

  TEST_F(UiScriptingTest, KeepsTheEventsOfAFrameUntilTheNext)
  {
    ShowInventory();
    const auto &sword = Element("sword").GetBox();

    ClickAt(sword.left + 5.0, sword.top + 5.0);
    EXPECT_FALSE(_ui->GetElementEvents().empty());

    _renderer.batches.clear();
    _ui->Draw();
    EXPECT_FALSE(_ui->GetElementEvents().empty());

    Frame();
    EXPECT_TRUE(_ui->GetElementEvents().empty());
  }

  // dragging an element onto another

  class UiDragTest : public UiScriptingTest
  {
  protected:
    void ShowSlots()
    {
      ASSERT_GE(ShowUnderRoot(
        "- {type: button, name: item, text: Item, draggable: true, width: 100, height: 100, box_sizing: border-box}\n"
        "- {type: button, name: plain, text: Plain, width: 100, height: 100, box_sizing: border-box}\n"
        "- {type: panel, name: bag, width: 100, height: 100, pointer_events: auto}\n"
        "- {type: panel, name: floor, width: 100, height: 100}\n"), 0);
      Frame();
    }

    void Hold(const double x, const double y)
    {
      Release();
      PointAt(x, y);
      _input.state.SetAction(Action::Pointer_Primary);
      Frame();
    }

    void LetGo()
    {
      Release();
      Frame();
    }
  };

  TEST_F(UiDragTest, DragsAnElementOntoAnother)
  {
    ShowSlots();

    PointAt(50, 50);
    Frame();

    Hold(50, 50);
    EXPECT_THAT(Happened(), ElementsAre("pointer_down item", "focused item"));

    // far enough to count
    Hold(60, 50);
    EXPECT_THAT(Happened(), ElementsAre("drag_start item"));

    Hold(250, 50);
    EXPECT_THAT(Happened(), ElementsAre("pointer_leave item", "pointer_enter bag", "drag_over bag"));
    EXPECT_EQ(_ui->GetElementEvents()[2].related, Handle("item"));

    LetGo();
    EXPECT_THAT(Happened(), ElementsAre("pointer_up bag", "drop bag", "drag_end item"));
    EXPECT_EQ(_ui->GetElementEvents()[1].related, Handle("item"));
    EXPECT_EQ(_ui->GetElementEvents()[2].related, Handle("bag"));

    // what was dragged was not clicked
    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  TEST_F(UiDragTest, DropsOntoWhatTakesNoPointer)
  {
    ShowSlots();

    Hold(50, 50);
    Hold(60, 50);
    Hold(350, 50);

    LetGo();

    const auto happened = Happened();
    ASSERT_EQ(happened.size(), 2u);
    EXPECT_EQ(happened[0], "drop floor");
    EXPECT_EQ(happened[1], "drag_end item");
  }

  TEST_F(UiDragTest, ClicksWhatIsNotDraggedFarEnough)
  {
    ShowSlots();

    Hold(50, 50);
    Hold(52, 51);
    LetGo();

    EXPECT_TRUE(_ui->WasClicked("item"));
    EXPECT_THAT(Happened(), ElementsAre("pointer_up item", "click item"));
  }

  TEST_F(UiDragTest, DragsOnlyWhatSaysThatItCanBeDragged)
  {
    ShowSlots();

    Hold(150, 50);
    Hold(250, 50);
    LetGo();

    for (const auto &happened : Happened())
    {
      EXPECT_FALSE(happened.starts_with("drag")) << happened;
      EXPECT_FALSE(happened.starts_with("drop")) << happened;
    }
  }

  TEST_F(UiDragTest, KeepsThePointerFromTheGameWhileSomethingIsDragged)
  {
    ShowSlots();

    Hold(50, 50);
    Hold(60, 50);
    Hold(900, 900);

    EXPECT_FALSE(_ui->GetGameInput()->GetInputState()[Action::Pointer_Primary]);
    EXPECT_FALSE(_ui->GetGameInput()->GetInputState().HasPointer());
  }
} // namespace
