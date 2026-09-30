#include "ui-element-types.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "elements/ui-bar.hpp"
#include "elements/ui-button.hpp"
#include "elements/ui-image.hpp"
#include "elements/ui-label.hpp"
#include "elements/ui-panel.hpp"

namespace
{
  using neon::UiElement;
  using neon::UiElementTypes;

  class Minimap final : public UiElement {};

  class Compass final : public UiElement {};

  TEST(UiElementTypes, KnowNothingAtFirst)
  {
    const UiElementTypes types;

    EXPECT_TRUE(types.GetNames().empty());
    EXPECT_EQ(types.CreateElement("panel"), nullptr);
  }

  TEST(UiElementTypes, KnowTheKindsOfTheEngine)
  {
    UiElementTypes types;
    types.AddEngineElements();

    EXPECT_EQ(types.GetNames(), (std::vector<std::string>{"panel", "label", "image", "button", "bar"}));

    EXPECT_NE(dynamic_cast<neon::UiPanel *>(types.CreateElement("panel").get()), nullptr);
    EXPECT_NE(dynamic_cast<neon::UiLabel *>(types.CreateElement("label").get()), nullptr);
    EXPECT_NE(dynamic_cast<neon::UiImageElement *>(types.CreateElement("image").get()), nullptr);
    EXPECT_NE(dynamic_cast<neon::UiButton *>(types.CreateElement("button").get()), nullptr);
    EXPECT_NE(dynamic_cast<neon::UiBar *>(types.CreateElement("bar").get()), nullptr);
  }

  TEST(UiElementTypes, CreateANewElementEveryTime)
  {
    UiElementTypes types;
    types.AddEngineElements();

    const auto first = types.CreateElement("panel");
    const auto second = types.CreateElement("panel");

    EXPECT_NE(first.get(), second.get());
  }

  TEST(UiElementTypes, DoNotKnowANameThatWasNotAdded)
  {
    UiElementTypes types;
    types.AddEngineElements();

    EXPECT_EQ(types.CreateElement("minimap"), nullptr);
    EXPECT_EQ(types.CreateElement("Panel"), nullptr);
    EXPECT_EQ(types.CreateElement(""), nullptr);
  }

  TEST(UiElementTypes, TakeAKindOfAGame)
  {
    UiElementTypes types;
    types.AddEngineElements();

    types.Add<Minimap>("minimap");

    EXPECT_NE(dynamic_cast<Minimap *>(types.CreateElement("minimap").get()), nullptr);
    EXPECT_EQ(types.GetNames().back(), "minimap");
    EXPECT_EQ(types.GetNames().size(), 6u);
  }

  TEST(UiElementTypes, ReplaceAKindOfTheSameName)
  {
    UiElementTypes types;
    types.Add<Minimap>("map");
    types.Add<Compass>("map");

    EXPECT_NE(dynamic_cast<Compass *>(types.CreateElement("map").get()), nullptr);
    EXPECT_EQ(types.GetNames(), (std::vector<std::string>{"map"}));
  }

  TEST(UiElementTypes, CanBeAddedToTwice)
  {
    UiElementTypes types;
    types.AddEngineElements();
    types.AddEngineElements();

    EXPECT_EQ(types.GetNames().size(), 5u);
  }

  TEST(UiElementTypes, TakeAFunctionThatCreates)
  {
    UiElementTypes types;
    int created = 0;

    types.Add("counted", [&created]
    {
      created++;
      return std::make_unique<Minimap>();
    });

    (void) types.CreateElement("counted");
    (void) types.CreateElement("counted");

    EXPECT_EQ(created, 2);
  }
}
