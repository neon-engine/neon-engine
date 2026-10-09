#include "shadow-map-size.hpp"

#include <gtest/gtest.h>

namespace
{
  using neon::ShadowMapSize;

  TEST(ShadowMapSizeTest, KnowsItsSizes)
  {
    for (const int size : {512, 1024, 2048, 4096}) { EXPECT_TRUE(ShadowMapSize::IsSize(size)) << size; }
    for (const int other : {0, -1, 256, 1000, 3072, 8192}) { EXPECT_FALSE(ShadowMapSize::IsSize(other)) << other; }
  }

  TEST(ShadowMapSizeTest, IsTwoThousandAndFortyEightUnlessToldOtherwise)
  {
    EXPECT_EQ(ShadowMapSize::kDefault, 2048);
    EXPECT_TRUE(ShadowMapSize::IsSize(ShadowMapSize::kDefault));
  }
} // namespace
