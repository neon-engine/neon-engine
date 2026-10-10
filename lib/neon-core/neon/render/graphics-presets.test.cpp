#include "graphics-presets.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/render/anisotropy.hpp>
#include <neon/render/shadow-map-size.hpp>
#include <neon/render/target-quality.hpp>
#include <neon/render/texture-scale.hpp>
#include <neon/runtime/settings-config.hpp>

namespace
{
  using neon::Anisotropy;
  using neon::GraphicsPreset;
  using neon::GraphicsPresets;
  using neon::TargetQuality;
  using neon::TextureScale;
  using ::testing::ElementsAre;

  TEST(GraphicsPresetsTest, BringsFourPresetsFromTheCheapestToTheFinest)
  {
    const GraphicsPresets presets;
    const auto &all = presets.All();
    ASSERT_EQ(all.size(), 4u);
    EXPECT_EQ(all[0].name, "low");
    EXPECT_EQ(all[1].name, "medium");
    EXPECT_EQ(all[2].name, "high");
    EXPECT_EQ(all[3].name, "ultra");
    EXPECT_EQ(GraphicsPresets::BuiltIn().size(), 4u);

    for (std::size_t i = 1; i < all.size(); i++)
    {
      EXPECT_GE(all[i].anisotropy, all[i - 1].anisotropy) << all[i].name;
      EXPECT_GE(all[i].texture_scale, all[i - 1].texture_scale) << all[i].name;
      EXPECT_GE(all[i].target_scale, all[i - 1].target_scale) << all[i].name;
      EXPECT_GE(all[i].shadow_map_size, all[i - 1].shadow_map_size) << all[i].name;
      EXPECT_GE(all[i].shadow_filter, all[i - 1].shadow_filter) << all[i].name;
      EXPECT_GE(all[i].shadow_cascades, all[i - 1].shadow_cascades) << all[i].name;
      EXPECT_GE(all[i].shadow_distance, all[i - 1].shadow_distance) << all[i].name;
      EXPECT_FALSE(all[i].HasTheValuesOf(all[i - 1])) << all[i].name << " is the same as the one before";
    }
  }

  TEST(GraphicsPresetsTest, EveryValueOfEveryBuiltInPresetIsOneASettingTakes)
  {
    for (const auto &preset : GraphicsPresets::BuiltIn())
    {
      EXPECT_TRUE(Anisotropy::IsLevel(preset.anisotropy)) << preset.name;
      EXPECT_TRUE(TextureScale::IsScale(preset.texture_scale)) << preset.name;
      EXPECT_TRUE(TargetQuality::IsScale(preset.target_scale)) << preset.name;
      EXPECT_TRUE(TargetQuality::IsMipmaps(preset.target_mipmaps)) << preset.name;
      EXPECT_TRUE(neon::ShadowMapSize::IsSize(preset.shadow_map_size)) << preset.name;
      EXPECT_GE(preset.shadow_cascades, 1) << preset.name;
      EXPECT_LE(preset.shadow_cascades, 4) << preset.name;
      EXPECT_GT(preset.shadow_distance, 0.0) << preset.name;
    }
  }

  TEST(GraphicsPresetsTest, TheDefaultsOfTheEngineAreHigh)
  {
    const GraphicsPresets presets;
    EXPECT_EQ(GraphicsPresets::kDefault, "high");
    EXPECT_EQ(presets.NameOf(GraphicsPresets::ValuesOf(SettingsConfig{})), "high");
    EXPECT_TRUE(GraphicsPreset{}.HasTheValuesOf(*presets.Named("high"))) << "and what a new preset starts with";
  }

  TEST(GraphicsPresetsTest, FindsAPresetByItsNameAndNoneForCustom)
  {
    const GraphicsPresets presets;
    ASSERT_NE(presets.Named("low"), nullptr);
    EXPECT_EQ(presets.Named("low")->anisotropy, 2);
    EXPECT_EQ(presets.Named("custom"), nullptr);
    EXPECT_EQ(presets.Named("best"), nullptr);
    EXPECT_EQ(presets.Named("High"), nullptr) << "names are lower case";

    for (const auto name : {"low", "medium", "high", "ultra", "custom"}) { EXPECT_TRUE(presets.IsName(name)) << name; }
    EXPECT_FALSE(presets.IsName("best"));
    EXPECT_FALSE(presets.IsName(""));
  }

  TEST(GraphicsPresetsTest, NamesThePresetTheValuesMatchAndCustomOtherwise)
  {
    const GraphicsPresets presets;
    SettingsConfig settings;
    for (const auto &preset : presets.All())
    {
      preset.ApplyTo(settings);
      EXPECT_EQ(presets.NameOf(GraphicsPresets::ValuesOf(settings)), preset.name);
    }

    settings.anisotropy = 2;
    EXPECT_EQ(presets.NameOf(GraphicsPresets::ValuesOf(settings)), "custom") << "one value changed";

    GraphicsPreset values = *presets.Named("medium");
    values.name = "something else";
    EXPECT_EQ(presets.NameOf(values), "medium") << "the name is not looked at, the values are";
  }

  TEST(GraphicsPresetsTest, ListsItsNamesWithCustomLast)
  {
    const GraphicsPresets presets;
    EXPECT_THAT(presets.Names(), ElementsAre("low", "medium", "high", "ultra", "custom"));
    EXPECT_EQ(presets.NamesForAMessage(), "low, medium, high, ultra, or custom");
  }

  // what a project does to the table

  TEST(GraphicsPresetsTest, APresetSetAgainKeepsItsPlaceAndTakesTheNewValues)
  {
    GraphicsPresets presets;
    GraphicsPreset low = *presets.Named("low");
    low.texture_scale = 1.0;

    EXPECT_TRUE(presets.Set(low));

    EXPECT_THAT(presets.Names(), ElementsAre("low", "medium", "high", "ultra", "custom"));
    EXPECT_DOUBLE_EQ(presets.Named("low")->texture_scale, 1.0);
    EXPECT_EQ(presets.Named("low")->anisotropy, 2) << "the rest stays";
  }

  TEST(GraphicsPresetsTest, ANewPresetComesAfterTheBuiltInOnes)
  {
    GraphicsPresets presets;
    GraphicsPreset potato;
    potato.name = "potato";
    potato.anisotropy = 1;

    EXPECT_TRUE(presets.Set(potato));
    EXPECT_TRUE(presets.Set(GraphicsPreset{.name = "cinematic", .anisotropy = 16, .shadow_distance = 300.0}));

    EXPECT_THAT(presets.Names(), ElementsAre("low", "medium", "high", "ultra", "potato", "cinematic", "custom"));
    ASSERT_NE(presets.Named("potato"), nullptr);
    EXPECT_EQ(presets.Named("potato")->anisotropy, 1);
    EXPECT_EQ(presets.Named("potato")->shadow_map_size, 2048) << "what it left out is the engine's default";
    EXPECT_EQ(presets.NamesForAMessage(), "low, medium, high, ultra, potato, cinematic, or custom");
    EXPECT_TRUE(presets.IsName("potato"));
    EXPECT_EQ(presets.NameOf(potato), "potato");
  }

  TEST(GraphicsPresetsTest, ADroppedPresetIsNoNameAnyMore)
  {
    GraphicsPresets presets;

    EXPECT_TRUE(presets.Remove("ultra"));
    EXPECT_FALSE(presets.Remove("ultra")) << "it is gone";
    EXPECT_FALSE(presets.Remove("best"));

    EXPECT_THAT(presets.Names(), ElementsAre("low", "medium", "high", "custom"));
    EXPECT_FALSE(presets.IsName("ultra"));
    EXPECT_EQ(presets.Named("ultra"), nullptr);
    EXPECT_EQ(presets.NameOf(GraphicsPresets::BuiltIn()[3]), "custom") << "its values match none now";
    EXPECT_EQ(GraphicsPresets::BuiltIn().size(), 4u) << "the engine's table is not touched";
  }

  TEST(GraphicsPresetsTest, CustomAndNothingCannotBeSet)
  {
    GraphicsPresets presets;
    EXPECT_FALSE(presets.Set(GraphicsPreset{.name = "custom"}));
    EXPECT_FALSE(presets.Set(GraphicsPreset{}));
    EXPECT_EQ(presets.All().size(), 4u);
  }

  TEST(GraphicsPresetsTest, ATableIsCopied)
  {
    GraphicsPresets presets;
    EXPECT_TRUE(presets.Set(GraphicsPreset{.name = "potato", .anisotropy = 1}));

    const GraphicsPresets copy = presets;
    EXPECT_TRUE(presets.Remove("potato"));
    EXPECT_NE(copy.Named("potato"), nullptr) << "a menu keeps the table it was made with";
  }
} // namespace
