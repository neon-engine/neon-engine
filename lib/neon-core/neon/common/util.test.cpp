#include "util.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  TEST(Util, SplitsAtEveryDelimiter)
  {
    EXPECT_THAT(neon::split("a,b,c", ','), ElementsAre("a", "b", "c"));
  }

  TEST(Util, SplitKeepsATextWithoutDelimiterWhole)
  {
    EXPECT_THAT(neon::split("models", '/'), ElementsAre("models"));
  }

  TEST(Util, SplitKeepsEmptyPartsBetweenDelimiters)
  {
    EXPECT_THAT(neon::split("a,,b", ','), ElementsAre("a", "", "b"));
    EXPECT_THAT(neon::split(",a", ','), ElementsAre("", "a"));
  }

  TEST(Util, SplitDropsTheEmptyPartAfterALastDelimiter)
  {
    EXPECT_THAT(neon::split("a,b,", ','), ElementsAre("a", "b"));
  }

  TEST(Util, SplitOfNothingIsEmpty)
  {
    EXPECT_THAT(neon::split("", ','), IsEmpty());
  }

  TEST(Util, FileExtensionIsWhatFollowsTheLastDot)
  {
    EXPECT_EQ(neon::get_file_extension("cube.obj"), "obj");
    EXPECT_EQ(neon::get_file_extension("basic-lit.vert.spv"), "spv");
  }

  TEST(Util, FileExtensionIsEmptyWithoutADot)
  {
    EXPECT_EQ(neon::get_file_extension("cube"), "");
    EXPECT_EQ(neon::get_file_extension(""), "");
  }

  TEST(Util, FileExtensionIsEmptyAfterALastDot)
  {
    EXPECT_EQ(neon::get_file_extension("cube."), "");
  }

  TEST(Util, ScaleMapsTheEndsOfOneRangeToTheEndsOfTheOther)
  {
    EXPECT_FLOAT_EQ(neon::scale(0.0f, {0.0f, 10.0f}, {100.0f, 200.0f}), 100.0f);
    EXPECT_FLOAT_EQ(neon::scale(10.0f, {0.0f, 10.0f}, {100.0f, 200.0f}), 200.0f);
  }

  TEST(Util, ScaleMapsWhatLiesBetweenInProportion)
  {
    EXPECT_FLOAT_EQ(neon::scale(2.5f, {0.0f, 10.0f}, {100.0f, 200.0f}), 125.0f);
    EXPECT_FLOAT_EQ(neon::scale(0.0f, {-1.0f, 1.0f}, {0.0f, 1.0f}), 0.5f);
  }

  TEST(Util, ScaleMapsToARangeThatFalls)
  {
    EXPECT_FLOAT_EQ(neon::scale(2.5f, {0.0f, 10.0f}, {1.0f, 0.0f}), 0.75f);
  }

  TEST(Util, MatrixIsWrittenRowByRow)
  {
    auto matrix = glm::mat4(1.0f);
    // the fourth column holds the translation
    matrix[3] = glm::vec4(1.0f, 2.0f, 3.0f, 1.0f);

    EXPECT_EQ(
      neon::mat4_to_string(matrix),
      "1.00 0.00 0.00 1.00 \n"
      "0.00 1.00 0.00 2.00 \n"
      "0.00 0.00 1.00 3.00 \n"
      "0.00 0.00 0.00 1.00 \n");
  }
}
