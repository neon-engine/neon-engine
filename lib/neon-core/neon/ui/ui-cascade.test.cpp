#include "ui-cascade.hpp"

#include <gtest/gtest.h>

#include "elements/ui-button.hpp"
#include "elements/ui-label.hpp"
#include "elements/ui-panel.hpp"
#include "ui-style-sheets.hpp"

// The cascade on elements that belong to no user interface: what the
// defaults of the engine, style sheets, the file, and a script come to, in
// the order https://www.w3.org/TR/css-cascade-4/ asks for.

namespace
{
  using neon::Color;
  using neon::UiButton;
  using neon::UiCascade;
  using neon::UiCascadeSettings;
  using neon::UiElement;
  using neon::UiLabel;
  using neon::UiPanel;
  using neon::UiStates;
  using neon::UiStyle;
  using neon::UiStyleSheets;

  class UiCascadeTest : public ::testing::Test
  {
  protected:
    UiStyleSheets _sheets;
    std::vector<std::string> _warnings;
    std::vector<std::string> _problems;

    // a panel with a label and a button inside, as a file would make them
    UiPanel _panel;
    UiLabel *_label = nullptr;
    UiButton *_button = nullptr;

    void SetUp() override
    {
      _panel.SetIdentity("panel", "window", 1);
      _panel.SetClasses({"window"});

      auto label = std::make_unique<UiLabel>();
      label->SetIdentity("label", "title", 2);
      _label = label.get();
      _panel.AddChild(std::move(label));

      auto button = std::make_unique<UiButton>();
      button->SetIdentity("button", "start", 3);
      button->SetClasses({"primary"});
      _button = button.get();
      _panel.AddChild(std::move(button));
    }

    void Sheet(const std::string &css)
    {
      _sheets.AddText(css, "theme.css", _warnings);
    }

    [[nodiscard]] UiCascadeSettings Settings()
    {
      UiCascadeSettings settings;
      settings.sheets = &_sheets;
      settings.problems = &_problems;
      return settings;
    }

    /// Works out the styles from the top, as the user interface does.
    void Compute()
    {
      UiCascade::Compute(_panel, Settings());
      UiCascade::Compute(*_label, Settings());
      UiCascade::Compute(*_button, Settings());
    }

    static bool IsColor(const Color &color, const float r, const float g, const float b)
    {
      return std::abs(color.r - r) < 0.01f && std::abs(color.g - g) < 0.01f && std::abs(color.b - b) < 0.01f;
    }
  };

  TEST_F(UiCascadeTest, StartsFromTheDefaultsOfTheKind)
  {
    Compute();

    EXPECT_TRUE(IsColor(_button->GetComputedStyle().background_color, 0.231f, 0.259f, 0.322f));
    EXPECT_FLOAT_EQ(_label->GetComputedStyle().font_size, 16.0f);
    EXPECT_EQ(_panel.GetComputedStyle().layout.width.unit, neon::LayoutLength::Unit::Auto);
  }

  TEST_F(UiCascadeTest, ASheetWinsOverTheDefaultsAndTheFileOverTheSheet)
  {
    Sheet("button { background-color: #ff0000; opacity: 0.5; }");

    neon::DataValue written = neon::DataValue::Map();
    written.Set("opacity", neon::DataValue::Number(0.25));
    _button->SetWritten(written, "test.ui.yml");

    Compute();

    EXPECT_TRUE(IsColor(_button->GetComputedStyle().background_color, 1.0f, 0.0f, 0.0f));
    EXPECT_FLOAT_EQ(_button->GetComputedStyle().opacity, 0.25f);
  }

  TEST_F(UiCascadeTest, AScriptWinsOverTheFileAndImportantOverEverything)
  {
    Sheet("button { opacity: 0.5 !important; }\n.primary { width: 100px !important; }");

    neon::DataValue written = neon::DataValue::Map();
    written.Set("opacity", neon::DataValue::Number(0.25));
    written.Set("width", neon::DataValue::Number(50));
    _button->SetWritten(written, "test.ui.yml");
    _button->SetProperty("width", "80px");

    Compute();

    EXPECT_FLOAT_EQ(_button->GetComputedStyle().opacity, 0.5f);
    EXPECT_FLOAT_EQ(_button->GetComputedStyle().layout.width.value, 100.0f);

    // without the important rule, the script wins
    _sheets = UiStyleSheets{};
    Compute();
    EXPECT_FLOAT_EQ(_button->GetComputedStyle().layout.width.value, 80.0f);
  }

  TEST_F(UiCascadeTest, TheMoreSpecificSelectorWinsWhateverTheOrder)
  {
    Sheet(
      "#start { color: #ff0000; }\n"
      ".primary { color: #00ff00; }\n"
      "button { color: #0000ff; }\n"
      "panel button { opacity: 0.5; }\n"
      "button { opacity: 0.25; }");

    Compute();

    EXPECT_TRUE(IsColor(_button->GetComputedStyle().color, 1.0f, 0.0f, 0.0f));
    EXPECT_FLOAT_EQ(_button->GetComputedStyle().opacity, 0.5f);
  }

  TEST_F(UiCascadeTest, InheritsWhatCssInheritsFromTheElementAbove)
  {
    Sheet(".window { color: #ff0000; font-size: 20px; opacity: 0.5; padding: 4px; }");

    Compute();

    const UiStyle &label = _label->GetComputedStyle();
    EXPECT_TRUE(IsColor(label.color, 1.0f, 0.0f, 0.0f));
    EXPECT_FLOAT_EQ(label.font_size, 20.0f);

    // not inherited: each has its own
    EXPECT_FLOAT_EQ(label.opacity, 1.0f);
    EXPECT_FLOAT_EQ(label.layout.padding.left.value, 0.0f);
  }

  TEST_F(UiCascadeTest, InheritInitialAndUnsetAreReadForEveryProperty)
  {
    Sheet(
      ".window { opacity: 0.5; color: #ff0000; }\n"
      "label { opacity: inherit; color: initial; }\n"
      "button { color: unset; opacity: unset; }");

    Compute();

    EXPECT_FLOAT_EQ(_label->GetComputedStyle().opacity, 0.5f);
    EXPECT_TRUE(IsColor(_label->GetComputedStyle().color, 1.0f, 1.0f, 1.0f)) << "the initial colour of the text";

    // unset is inherit for what is inherited, and initial otherwise
    EXPECT_TRUE(IsColor(_button->GetComputedStyle().color, 1.0f, 0.0f, 0.0f));
    EXPECT_FLOAT_EQ(_button->GetComputedStyle().opacity, 1.0f);
  }

  TEST_F(UiCascadeTest, ResolvesCustomPropertiesAndUnitsAgainstTheElement)
  {
    Sheet(
      ":root { --accent: #00ff00; --unit: 2; }\n"
      ".window { font-size: 20px; }\n"
      "label { color: var(--accent); font-size: 2em; width: calc(10vw + 1rem); height: calc(var(--unit) * 8px); }");

    UiCascadeSettings settings = Settings();
    settings.viewport_width = 500.0f;

    UiCascade::Compute(_panel, settings);
    UiCascade::Compute(*_label, settings);

    const UiStyle &label = _label->GetComputedStyle();
    EXPECT_TRUE(IsColor(label.color, 0.0f, 1.0f, 0.0f));
    EXPECT_FLOAT_EQ(label.font_size, 40.0f) << "em of the parent, for font-size itself";
    EXPECT_FLOAT_EQ(label.layout.width.value, 50.0f + 20.0f) << "10vw of 500, and the rem of the root, which is the panel";
    EXPECT_FLOAT_EQ(label.layout.height.value, 16.0f);
  }

  TEST_F(UiCascadeTest, SaysWhatCannotBeReadAndSkipsTheDeclaration)
  {
    Sheet("label { color: var(--missing); opacity: 0.5; }");

    Compute();

    EXPECT_FLOAT_EQ(_label->GetComputedStyle().opacity, 0.5f);
    EXPECT_TRUE(IsColor(_label->GetComputedStyle().color, 1.0f, 1.0f, 1.0f));
    ASSERT_EQ(_problems.size(), 1u);
    EXPECT_EQ(
      _problems[0],
      "theme.css:1: 'color' of the rule 'label' is 'var(--missing)': the custom property '--missing' is not set, "
      "and var() names nothing in its place. The declaration is left out");
  }

  TEST_F(UiCascadeTest, TheStatesOfTheEngineAndOfTheSheetApply)
  {
    Sheet("button:hover { color: #ff0000; }");

    UiStates states;
    states.hover = true;
    _button->SetStates(states);
    Compute();

    EXPECT_TRUE(IsColor(_button->GetComputedStyle().color, 1.0f, 0.0f, 0.0f));
    EXPECT_TRUE(IsColor(_button->GetComputedStyle().background_color, 0.298f, 0.337f, 0.416f))
      << "the default of the engine for a button under the pointer";

    // what is disabled is drawn as that alone
    states.disabled = true;
    EXPECT_FALSE(UiCascade::DrawnStates(states).hover);
    EXPECT_TRUE(UiCascade::DrawnStates(states).disabled);
  }

  TEST_F(UiCascadeTest, APartTakesFromItsElementAndItsOwnRules)
  {
    Sheet("button { color: #ff0000; }\nbutton::tooltip { background-color: #0000ff; }");

    Compute();

    UiStyle part = _button->GetComputedStyle();
    UiCascade::ComputePart(*_button, "tooltip", Settings(), part);

    EXPECT_TRUE(IsColor(part.color, 1.0f, 0.0f, 0.0f));
    EXPECT_TRUE(IsColor(part.background_color, 0.0f, 0.0f, 1.0f));
  }
}
