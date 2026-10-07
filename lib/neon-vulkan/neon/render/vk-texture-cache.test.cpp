#include "vk-texture-cache.hpp"

#include <gtest/gtest.h>

// The keys textures are held under, and the size the cache reads them at.
// Making a texture needs a device and is not tested.

namespace
{
  using neon::VK_TextureCache;
  using neon::VK_TextureOptions;

  TEST(VkTextureCacheTest, ReadsTexturesAtTheirSizeUnlessToldOtherwise)
  {
    const VK_TextureCache cache;

    EXPECT_DOUBLE_EQ(cache.GetScale(), 1.0);
  }

  TEST(VkTextureCacheTest, ReadsTexturesAtTheSizeItIsTold)
  {
    VK_TextureCache cache;
    cache.SetScale(0.25);

    EXPECT_DOUBLE_EQ(cache.GetScale(), 0.25);
  }

  TEST(VkTextureCacheTest, KeepsTheSameImageAtAnotherSizeApart)
  {
    const std::string whole = VK_TextureCache::KeyOf("assets://textures/floor.png", "", VK_TextureOptions{});
    const std::string half = VK_TextureCache::KeyOf("assets://textures/floor.png", "", VK_TextureOptions{.scale = 0.5});
    const std::string quarter = VK_TextureCache::KeyOf("assets://textures/floor.png", "", VK_TextureOptions{.scale = 0.25});

    EXPECT_NE(whole, half);
    EXPECT_NE(half, quarter);
  }

  TEST(VkTextureCacheTest, KeepsTheKeyOfAnImageAtItsSizeAsItWas)
  {
    EXPECT_EQ(VK_TextureCache::KeyOf("assets://textures/floor.png", "", VK_TextureOptions{}), "assets://textures/floor.png|color");
  }
} // namespace
