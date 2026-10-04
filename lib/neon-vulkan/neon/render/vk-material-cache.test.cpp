#include "vk-material-cache.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
  using neon::MaterialInfo;
  using neon::RenderInfo;
  using neon::VK_Material;
  using neon::VK_MaterialCache;

  RenderInfo Crate()
  {
    RenderInfo info;
    info.shader_path = "assets://shaders/pbr";
    info.model_path = "assets://models/crate.glb";
    info.texture_paths = {"assets://textures/wood.png"};
    return info;
  }

  TEST(VkMaterialCache, TwoObjectsWithTheSameMaterialShareOneAndTheCountSaysSo)
  {
    VK_MaterialCache cache(8);
    const std::string key = VK_MaterialCache::KeyOf(Crate(), MaterialInfo{});

    EXPECT_EQ(cache.Find(key), -1);
    const int id = cache.Keep(key, VK_Material{});
    ASSERT_GE(id, 0);
    EXPECT_EQ(cache.CountOf(id), 1);

    EXPECT_EQ(cache.Find(key), id);
    EXPECT_EQ(cache.Find(key), id);
    EXPECT_EQ(cache.CountOf(id), 3);
    EXPECT_EQ(cache.Size(), 1);
    EXPECT_EQ(cache.Makes(), 1u);
    EXPECT_EQ(cache.Shares(), 2u);
  }

  TEST(VkMaterialCache, KeepsAMaterialThatNothingHoldsUntilItIsAskedToFreeIt)
  {
    VK_MaterialCache cache(8);
    const std::string key = VK_MaterialCache::KeyOf(Crate(), MaterialInfo{});
    const int id = cache.Keep(key, VK_Material{});
    EXPECT_EQ(cache.Find(key), id);

    cache.Release(id, 1);
    EXPECT_EQ(cache.CountOf(id), 1);
    EXPECT_EQ(cache.UnusedCount(), 0);

    // the last one gives it back: it stays, and counts as unused
    cache.Release(id, 2);
    EXPECT_TRUE(cache.Contains(id));
    EXPECT_EQ(cache.CountOf(id), 0);
    EXPECT_EQ(cache.UnusedCount(), 1);

    int freed = 0;
    EXPECT_EQ(cache.FreeUnused(3, [&](VK_Material) { freed++; }), 1);
    EXPECT_EQ(freed, 1);
    EXPECT_FALSE(cache.Contains(id));
    EXPECT_EQ(cache.Size(), 0);
    EXPECT_EQ(cache.UnusedCount(), 0);

    // made anew afterwards, under an id of its own
    EXPECT_EQ(cache.Find(key), -1);
    EXPECT_GE(cache.Keep(key, VK_Material{}), 0);
    EXPECT_EQ(cache.Makes(), 2u);
  }

  TEST(VkMaterialCache, AMaterialThatNothingHeldIsTakenAgainWithoutBeingMade)
  {
    VK_MaterialCache cache(8);
    const int id = cache.Keep("face-open", VK_Material{});
    cache.Release(id, 5);
    EXPECT_EQ(cache.UnusedCount(), 1);

    // what showed it shows it again: the same material, and it is held
    EXPECT_EQ(cache.Find("face-open"), id);
    EXPECT_EQ(cache.CountOf(id), 1);
    EXPECT_EQ(cache.UnusedCount(), 0);
    EXPECT_EQ(cache.Makes(), 1u);

    EXPECT_EQ(cache.FreeUnused(100, [](VK_Material) {}), 0);
    EXPECT_TRUE(cache.Contains(id));
  }

  TEST(VkMaterialCache, FreesOnlyWhatWasGivenBackBeforeTheFrameItIsAskedFor)
  {
    VK_MaterialCache cache(8);
    const int early = cache.Keep("early", VK_Material{});
    const int late = cache.Keep("late", VK_Material{});
    const int held = cache.Keep("held", VK_Material{});
    cache.Release(early, 4);
    cache.Release(late, 7);

    // a draw of frame 7 may still be waiting with what was given back in it
    EXPECT_EQ(cache.FreeUnused(7, [](VK_Material) {}), 1);
    EXPECT_FALSE(cache.Contains(early));
    EXPECT_TRUE(cache.Contains(late));
    EXPECT_TRUE(cache.Contains(held));

    EXPECT_EQ(cache.FreeUnused(8, [](VK_Material) {}), 1);
    EXPECT_FALSE(cache.Contains(late));
    EXPECT_TRUE(cache.Contains(held));
  }

  TEST(VkMaterialCache, GivingBackWhatNothingHoldsChangesNothing)
  {
    VK_MaterialCache cache(8);
    const int id = cache.Keep("a", VK_Material{});
    cache.Release(id, 1);
    cache.Release(id, 9);

    EXPECT_EQ(cache.UnusedCount(), 1);
    EXPECT_EQ(cache.FreeUnused(5, [](VK_Material) {}), 1);
  }

  TEST(VkMaterialCache, TellsMaterialsApartByEverythingTheyAreMadeFrom)
  {
    const std::string same = VK_MaterialCache::KeyOf(Crate(), MaterialInfo{});
    EXPECT_EQ(VK_MaterialCache::KeyOf(Crate(), MaterialInfo{}), same);

    RenderInfo other_shader = Crate();
    other_shader.shader_path = "assets://shaders/unlit";
    EXPECT_NE(VK_MaterialCache::KeyOf(other_shader, MaterialInfo{}), same);

    RenderInfo other_texture = Crate();
    other_texture.texture_paths = {"assets://textures/steel.png"};
    EXPECT_NE(VK_MaterialCache::KeyOf(other_texture, MaterialInfo{}), same);

    RenderInfo other_model = Crate();
    other_model.model_path = "assets://models/barrel.glb";
    EXPECT_NE(VK_MaterialCache::KeyOf(other_model, MaterialInfo{}), same);

    MaterialInfo red;
    red.color = {1.0f, 0.0f, 0.0f, 1.0f};
    EXPECT_NE(VK_MaterialCache::KeyOf(Crate(), red), same);

    MaterialInfo rough;
    rough.roughness = 1.0f;
    EXPECT_NE(VK_MaterialCache::KeyOf(Crate(), rough), same);

    // what the surface gives off tells two apart as well
    MaterialInfo glowing;
    glowing.emissive = {1.0f, 0.5f, 0.0f, 1.0f};
    EXPECT_NE(VK_MaterialCache::KeyOf(Crate(), glowing), same);

    MaterialInfo brighter;
    brighter.emissive_strength = 4.0f;
    EXPECT_NE(VK_MaterialCache::KeyOf(Crate(), brighter), same);

    MaterialInfo lit_by_texture;
    lit_by_texture.emissive_texture = "assets://textures/glow.png";
    EXPECT_NE(VK_MaterialCache::KeyOf(Crate(), lit_by_texture), same);

    // two textures and one are not the same, even with the same text
    RenderInfo two = Crate();
    two.texture_paths = {"assets://textures/wood.png", "assets://textures/wood.png"};
    EXPECT_NE(VK_MaterialCache::KeyOf(two, MaterialInfo{}), same);
  }

  TEST(VkMaterialCache, DifferentMaterialsAreHeldApart)
  {
    VK_MaterialCache cache(8);
    MaterialInfo red;
    red.color = {1.0f, 0.0f, 0.0f, 1.0f};
    const int plain = cache.Keep(VK_MaterialCache::KeyOf(Crate(), MaterialInfo{}), VK_Material{});
    const int tinted = cache.Keep(VK_MaterialCache::KeyOf(Crate(), red), VK_Material{});

    EXPECT_NE(plain, tinted);
    EXPECT_EQ(cache.Size(), 2);
    EXPECT_EQ(cache.Find(VK_MaterialCache::KeyOf(Crate(), red)), tinted);
  }

  TEST(VkMaterialCache, RemoveAllHandsEveryMaterialOverOnce)
  {
    VK_MaterialCache cache(8);
    cache.Keep("a", VK_Material{});
    cache.Keep("b", VK_Material{});
    EXPECT_GE(cache.Find("a"), 0);

    int handed = 0;
    cache.RemoveAll([&](VK_Material) { handed++; });

    EXPECT_EQ(handed, 2);
    EXPECT_EQ(cache.Size(), 0);
    EXPECT_EQ(cache.Find("a"), -1);
  }

  TEST(VkMaterialCache, RefusesToKeepMoreThanItsCapacity)
  {
    VK_MaterialCache cache(1);
    EXPECT_GE(cache.Keep("a", VK_Material{}), 0);
    EXPECT_EQ(cache.Keep("b", VK_Material{}), -1);

    // what is not held is given back without harm
    cache.Release(5, 1);
    EXPECT_EQ(cache.UnusedCount(), 0);

    // and room is made by what nothing holds
    cache.Release(0, 1);
    EXPECT_EQ(cache.FreeUnused(2, [](VK_Material) {}), 1);
    EXPECT_GE(cache.Keep("b", VK_Material{}), 0);
  }
}
