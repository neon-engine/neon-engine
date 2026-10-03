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

  TEST(VkMaterialCache, FreesAMaterialWhenTheLastObjectReleasesItAndNotBefore)
  {
    VK_MaterialCache cache(8);
    const std::string key = VK_MaterialCache::KeyOf(Crate(), MaterialInfo{});
    const int id = cache.Keep(key, VK_Material{});
    EXPECT_EQ(cache.Find(key), id);

    VK_Material freed;
    EXPECT_FALSE(cache.Release(id, freed));
    EXPECT_TRUE(cache.Contains(id));
    EXPECT_TRUE(cache.Release(id, freed));
    EXPECT_FALSE(cache.Contains(id));
    EXPECT_EQ(cache.Size(), 0);

    // made anew afterwards, under an id of its own
    EXPECT_EQ(cache.Find(key), -1);
    EXPECT_GE(cache.Keep(key, VK_Material{}), 0);
    EXPECT_EQ(cache.Makes(), 2u);
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
    VK_Material freed;
    EXPECT_FALSE(cache.Release(5, freed));
  }
}
