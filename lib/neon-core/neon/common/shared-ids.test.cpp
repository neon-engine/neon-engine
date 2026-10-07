#include "shared-ids.hpp"

#include <string>

#include <gtest/gtest.h>

namespace
{
  using neon::SharedIds;

  TEST(SharedIdsTest, HasNothingUnderAKeyThatWasNeverAdded)
  {
    SharedIds<std::string> ids;

    EXPECT_EQ(ids.Take("assets://models/crate.obj"), -1);
    EXPECT_EQ(ids.Shares(), 0u);
  }

  TEST(SharedIdsTest, HandsTheSameIdToEveryoneWhoAsksForTheSameKey)
  {
    SharedIds<std::string> ids;
    ids.Add("assets://models/crate.obj", 4);

    EXPECT_EQ(ids.Take("assets://models/crate.obj"), 4);
    EXPECT_EQ(ids.Take("assets://models/crate.obj"), 4);
    EXPECT_EQ(ids.CountOf(4), 3);
    EXPECT_EQ(ids.Shares(), 2u);
  }

  TEST(SharedIdsTest, KeepsKeysApart)
  {
    SharedIds<std::string> ids;
    ids.Add("geometry:box_1_1_1", 0);
    ids.Add("geometry:box_1_2_1", 1);

    EXPECT_EQ(ids.Take("geometry:box_1_1_1"), 0);
    EXPECT_EQ(ids.Take("geometry:box_1_2_1"), 1);
    EXPECT_EQ(ids.Size(), 2u);
  }

  TEST(SharedIdsTest, FreesAnIdWhenTheLastThatHeldItGivesItBack)
  {
    SharedIds<std::string> ids;
    ids.Add("assets://models/crate.obj", 4);
    (void) ids.Take("assets://models/crate.obj");

    EXPECT_FALSE(ids.Release(4));
    EXPECT_TRUE(ids.Contains(4));
    EXPECT_TRUE(ids.Release(4));
    EXPECT_FALSE(ids.Contains(4));
    EXPECT_EQ(ids.Take("assets://models/crate.obj"), -1);
  }

  TEST(SharedIdsTest, KnowsTheKeyOfAnIdThatWasAdded)
  {
    SharedIds<std::string> ids;
    ids.Add("assets://models/crate.obj", 4);

    ASSERT_NE(ids.KeyOf(4), nullptr);
    EXPECT_EQ(*ids.KeyOf(4), "assets://models/crate.obj");
    EXPECT_EQ(ids.KeyOf(5), nullptr);
  }

  TEST(SharedIdsTest, FreesAnIdThatWasNeverAddedAtOnce)
  {
    SharedIds<std::string> ids;

    EXPECT_TRUE(ids.Release(9));
    EXPECT_EQ(ids.CountOf(9), 0);
  }

  TEST(SharedIdsTest, AKeyThatWasFreedCanBeAddedAgainUnderAnotherId)
  {
    SharedIds<std::string> ids;
    ids.Add("assets://models/crate.obj", 4);
    (void) ids.Release(4);

    ids.Add("assets://models/crate.obj", 7);

    EXPECT_EQ(ids.Take("assets://models/crate.obj"), 7);
  }

  TEST(SharedIdsTest, ForgetsEveryIdWhenCleared)
  {
    SharedIds<std::string> ids;
    ids.Add("assets://models/crate.obj", 4);
    (void) ids.Take("assets://models/crate.obj");

    ids.Clear();

    EXPECT_EQ(ids.Size(), 0u);
    EXPECT_EQ(ids.Take("assets://models/crate.obj"), -1);
    EXPECT_EQ(ids.Shares(), 1u);
  }
} // namespace
