#include "api-version.hpp"

#include <gtest/gtest.h>

namespace
{
  using neon::ApiVersion;

  TEST(ApiVersionTest, IsWrittenAsMajorDotMinor)
  {
    EXPECT_EQ((ApiVersion{1, 3}).ToString(), "1.3");
  }

  TEST(ApiVersionTest, IsWrittenWithThePatchWhenThereIsOne)
  {
    EXPECT_EQ((ApiVersion{1, 3, 290}).ToString(), "1.3.290");
  }

  TEST(ApiVersionTest, ComparesTheMinorAsANumberAndNotAsText)
  {
    EXPECT_LT((ApiVersion{1, 9}), (ApiVersion{1, 10}));
  }

  TEST(ApiVersionTest, ComparesTheMajorBeforeTheMinor)
  {
    EXPECT_LT((ApiVersion{1, 4}), (ApiVersion{2, 0}));
  }

  TEST(ApiVersionTest, ComparesThePatchLast)
  {
    EXPECT_LT((ApiVersion{1, 3, 0}), (ApiVersion{1, 3, 1}));
    EXPECT_EQ((ApiVersion{1, 3, 7}), (ApiVersion{1, 3, 7}));
  }
}
