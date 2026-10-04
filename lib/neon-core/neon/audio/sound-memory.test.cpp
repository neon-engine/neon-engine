#include "sound-memory.hpp"

#include <gtest/gtest.h>

namespace
{
  using neon::SoundMemory;

  TEST(SoundMemoryTest, TellsASoundInMemoryFromAFile)
  {
    EXPECT_TRUE(SoundMemory::IsNamedBy("sound://quake/door"));
    EXPECT_FALSE(SoundMemory::IsNamedBy("assets://sounds/door.wav"));
    // a path that names nothing is taken as a file, which then is not found
    EXPECT_FALSE(SoundMemory::IsNamedBy("sound://"));
  }

  TEST(SoundMemoryTest, GivesTheNameAPathCarriesAndThePathOfAName)
  {
    EXPECT_EQ(SoundMemory::NameOf("sound://quake/doors/open.wav"), "quake/doors/open.wav");
    EXPECT_EQ(SoundMemory::NameOf("assets://sounds/door.wav"), "");
    EXPECT_EQ(SoundMemory::For("quake/door"), "sound://quake/door");
  }
}
