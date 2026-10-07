#include "anisotropy.hpp"

#include <gtest/gtest.h>

namespace
{
  using neon::Anisotropy;

  TEST(AnisotropyTest, KnowsItsLevels)
  {
    for (const int level : {1, 2, 4, 8, 16}) { EXPECT_TRUE(Anisotropy::IsLevel(level)) << level; }
    for (const int other : {0, -1, 3, 6, 12, 32}) { EXPECT_FALSE(Anisotropy::IsLevel(other)) << other; }
  }

  TEST(AnisotropyTest, ReadsWithEightSamplesUnlessToldOtherwise)
  {
    EXPECT_EQ(Anisotropy::kDefault, 8);
    EXPECT_TRUE(Anisotropy::IsLevel(Anisotropy::kDefault));
  }
} // namespace
