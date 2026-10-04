#include "texture-source.hpp"

#include <gtest/gtest.h>

namespace
{
  using neon::TextureSource;
  using neon::TextureSourceKind;

  TEST(TextureSourceTest, TakesAPathOfTheFileSystemForAFile)
  {
    const TextureSource source = TextureSource::Of("assets://textures/wood.png");

    EXPECT_EQ(source.kind, TextureSourceKind::File);
    EXPECT_EQ(source.name, "assets://textures/wood.png");
  }

  TEST(TextureSourceTest, FindsTheNameOfASurface)
  {
    EXPECT_EQ(TextureSource::Of("surface://terminal").kind, TextureSourceKind::Surface);
    EXPECT_EQ(TextureSource::Of("surface://terminal").name, "terminal");
    EXPECT_EQ(TextureSource::Of("surface://screens/left").name, "screens/left");
  }

  TEST(TextureSourceTest, FindsTheNameOfAnImage)
  {
    EXPECT_EQ(TextureSource::Of("image://quake/wall").kind, TextureSourceKind::Image);
    EXPECT_EQ(TextureSource::Of("image://quake/wall").name, "quake/wall");
  }

  TEST(TextureSourceTest, TakesWhatOnlyLooksLikeASurfaceOrAnImageForAFile)
  {
    EXPECT_EQ(TextureSource::Of("surface:/terminal").kind, TextureSourceKind::File);
    EXPECT_EQ(TextureSource::Of("terminal").kind, TextureSourceKind::File);
    EXPECT_EQ(TextureSource::Of("").kind, TextureSourceKind::File);

    // a name is needed
    EXPECT_EQ(TextureSource::Of("surface://").kind, TextureSourceKind::File);
    EXPECT_EQ(TextureSource::Of("image://").kind, TextureSourceKind::File);
  }

  TEST(TextureSourceTest, MakesThePathThatNamesASurfaceOrAnImage)
  {
    EXPECT_EQ(TextureSource::For(TextureSourceKind::Surface, "terminal"), "surface://terminal");
    EXPECT_EQ(TextureSource::For(TextureSourceKind::Image, "quake/wall"), "image://quake/wall");
    EXPECT_EQ(TextureSource::For(TextureSourceKind::File, "assets://textures/wood.png"), "assets://textures/wood.png");

    // and back
    const TextureSource back = TextureSource::Of(TextureSource::For(TextureSourceKind::Image, "quake/wall"));
    EXPECT_EQ(back.kind, TextureSourceKind::Image);
    EXPECT_EQ(back.name, "quake/wall");
  }
}
