#include "graphics-preset.hpp"

#include <gtest/gtest.h>

#include <neon/render/graphics-presets.hpp>
#include <neon/runtime/settings-config.hpp>

namespace
{
  using neon::GraphicsPreset;
  using neon::GraphicsPresets;

  TEST(GraphicsPresetTest, StartsWithTheDefaultsOfTheEngine)
  {
    const GraphicsPreset preset;
    EXPECT_TRUE(preset.name.empty());
    EXPECT_TRUE(preset.HasTheValuesOf(GraphicsPresets::ValuesOf(SettingsConfig{})))
      << "a value a project leaves out of a preset is the engine's default";
    EXPECT_TRUE(preset.HasTheValuesOf(*GraphicsPresets().Named("high"))) << "which are high";
  }

  TEST(GraphicsPresetTest, SetsTheSettingsItDecidesAndLeavesTheRest)
  {
    SettingsConfig settings;
    settings.vertical_sync = false;
    settings.exposure = 2.0;
    settings.max_fps = 60;

    GraphicsPresets().Named("low")->ApplyTo(settings);

    EXPECT_EQ(settings.anisotropy, 2);
    EXPECT_DOUBLE_EQ(settings.texture_scale, 0.5);
    EXPECT_DOUBLE_EQ(settings.target_scale, 0.5);
    EXPECT_EQ(settings.target_mipmaps, 1);
    EXPECT_EQ(settings.shadow_map_size, 1024);
    EXPECT_EQ(settings.shadow_filter, neon::ShadowFilter::None);
    EXPECT_EQ(settings.shadow_cascades, 1u);
    EXPECT_DOUBLE_EQ(settings.shadow_distance, 40.0);
    EXPECT_TRUE(settings.shadows) << "whether shadows are drawn at all is the player's own choice";

    EXPECT_FALSE(settings.vertical_sync) << "the taste of the player is not touched";
    EXPECT_DOUBLE_EQ(settings.exposure, 2.0);
    EXPECT_EQ(settings.max_fps, 60);
  }

  TEST(GraphicsPresetTest, ComparesItsValuesAndNotItsName)
  {
    GraphicsPreset one = *GraphicsPresets().Named("medium");
    GraphicsPreset other = one;
    other.name = "something else";
    EXPECT_TRUE(one.HasTheValuesOf(other));

    other.shadow_distance += 1.0;
    EXPECT_FALSE(one.HasTheValuesOf(other)) << "one value changed";
  }
} // namespace
