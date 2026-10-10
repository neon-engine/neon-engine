#include "data-buffer.hpp"

#include <string>

#include <gtest/gtest.h>

namespace
{
  using neon::DataBuffer;

  /// Has no default constructor, which a DataBuffer does not ask for.
  struct Named
  {
    std::string name;

    explicit Named(const std::string &name) : name(name) {}
  };

  TEST(DataBuffer, StartsEmpty)
  {
    const DataBuffer<int> buffer(4);

    EXPECT_EQ(buffer.Size(), 0);
    EXPECT_EQ(buffer.Capacity(), 4);
    EXPECT_FALSE(buffer.Contains(0));
  }

  TEST(DataBuffer, HandsOutIdsCountedFromZero)
  {
    DataBuffer<int> buffer(4);

    EXPECT_EQ(buffer.Add(10), 0);
    EXPECT_EQ(buffer.Add(20), 1);
    EXPECT_EQ(buffer.Add(30), 2);
    EXPECT_EQ(buffer.Size(), 3);
  }

  TEST(DataBuffer, ReturnsWhatWasStoredUnderAnId)
  {
    DataBuffer<std::string> buffer(4);
    const int first = buffer.Add("first");
    const int second = buffer.Add("second");

    EXPECT_EQ(*buffer.Find(first), "first");
    EXPECT_EQ(*buffer.Find(second), "second");
  }

  TEST(DataBuffer, StoresACopy)
  {
    DataBuffer<std::string> buffer(4);
    std::string original = "original";

    const int id = buffer.Add(original);
    original = "changed";

    EXPECT_EQ(*buffer.Find(id), "original");
  }

  TEST(DataBuffer, LetsAnElementBeChangedInPlace)
  {
    DataBuffer<int> buffer(4);
    const int id = buffer.Add(1);

    *buffer.Find(id) = 2;

    EXPECT_EQ(*buffer.Find(id), 2);
  }

  TEST(DataBuffer, ReadsThroughAConstBuffer)
  {
    DataBuffer<int> buffer(4);
    const int id = buffer.Add(7);

    const DataBuffer<int> &read_only = buffer;

    EXPECT_EQ(*read_only.Find(id), 7);
    EXPECT_EQ(read_only.Find(id + 1), nullptr);
  }

  TEST(DataBuffer, RefusesAnElementWhenEverySlotIsTaken)
  {
    DataBuffer<int> buffer(2);
    buffer.Add(1);
    buffer.Add(2);

    EXPECT_EQ(buffer.Add(3), -1);
    EXPECT_EQ(buffer.Size(), 2);
  }

  TEST(DataBuffer, HoldsNothingWithACapacityOfZero)
  {
    DataBuffer<int> buffer(0);

    EXPECT_EQ(buffer.Capacity(), 0);
    EXPECT_EQ(buffer.Add(1), -1);
  }

  TEST(DataBuffer, TreatsACapacityBelowZeroAsZero)
  {
    DataBuffer<int> buffer(-5);

    EXPECT_EQ(buffer.Capacity(), 0);
    EXPECT_EQ(buffer.Add(1), -1);
  }

  TEST(DataBuffer, RemoveReturnsTheElement)
  {
    DataBuffer<std::string> buffer(4);
    const int id = buffer.Add("element");

    EXPECT_EQ(buffer.Remove(id), std::optional<std::string>("element"));
    EXPECT_EQ(buffer.Size(), 0);
    EXPECT_FALSE(buffer.Contains(id));
  }

  TEST(DataBuffer, RemoveLeavesTheOtherElementsUnderTheirIds)
  {
    DataBuffer<int> buffer(4);
    const int first = buffer.Add(10);
    const int second = buffer.Add(20);
    const int third = buffer.Add(30);

    buffer.Remove(second);

    EXPECT_EQ(*buffer.Find(first), 10);
    EXPECT_EQ(*buffer.Find(third), 30);
    EXPECT_EQ(buffer.Size(), 2);
  }

  TEST(DataBuffer, GivesTheIdOfARemovedElementOutAgain)
  {
    DataBuffer<int> buffer(2);
    const int first = buffer.Add(10);
    buffer.Add(20);

    buffer.Remove(first);
    const int reused = buffer.Add(30);

    EXPECT_EQ(reused, first);
    EXPECT_EQ(*buffer.Find(reused), 30);
    EXPECT_EQ(buffer.Size(), 2);
  }

  TEST(DataBuffer, IsFullAgainOnceTheFreedSlotsAreTaken)
  {
    DataBuffer<int> buffer(2);
    buffer.Add(10);
    const int second = buffer.Add(20);
    buffer.Remove(second);

    EXPECT_NE(buffer.Add(30), -1);
    EXPECT_EQ(buffer.Add(40), -1);
  }

  TEST(DataBuffer, RemovesNothingWhenTheSameIdIsRemovedTwice)
  {
    DataBuffer<int> buffer(4);
    const int id = buffer.Add(10);
    buffer.Remove(id);

    EXPECT_EQ(buffer.Remove(id), std::nullopt);
    EXPECT_EQ(buffer.Size(), 0);
  }

  TEST(DataBuffer, RemovesNothingForAnIdThatHoldsNothing)
  {
    DataBuffer<int> buffer(4);
    buffer.Add(10);

    EXPECT_EQ(buffer.Remove(-1), std::nullopt);
    EXPECT_EQ(buffer.Remove(1), std::nullopt);
    EXPECT_EQ(buffer.Remove(4), std::nullopt);
    EXPECT_EQ(buffer.Size(), 1) << "what is there stays";
  }

  TEST(DataBuffer, FindsNothingForAnIdThatHoldsNothing)
  {
    DataBuffer<int> buffer(4);
    const int id = buffer.Add(10);
    buffer.Remove(id);

    EXPECT_EQ(buffer.Find(id), nullptr);
    EXPECT_EQ(buffer.Find(-1), nullptr);
    EXPECT_EQ(buffer.Find(3), nullptr);
  }

  TEST(DataBuffer, ContainsOnlyTheIdsThatHoldAnElement)
  {
    DataBuffer<int> buffer(4);
    const int first = buffer.Add(10);
    const int second = buffer.Add(20);
    buffer.Remove(first);

    EXPECT_FALSE(buffer.Contains(first));
    EXPECT_TRUE(buffer.Contains(second));
    EXPECT_FALSE(buffer.Contains(-1));
    EXPECT_FALSE(buffer.Contains(2));
    EXPECT_FALSE(buffer.Contains(100));
  }

  TEST(DataBuffer, KeepsElementsWhereTheyAreWhenMoreAreAdded)
  {
    DataBuffer<int> buffer(64);
    const int first = buffer.Add(10);
    const int *address = buffer.Find(first);

    for (int i = 0; i < 63; i++) { buffer.Add(i); }

    EXPECT_EQ(buffer.Find(first), address);
    EXPECT_EQ(*buffer.Find(first), 10);
  }

  TEST(DataBuffer, StoresATypeWithoutADefaultConstructor)
  {
    DataBuffer<Named> buffer(2);
    const int first = buffer.Add(Named("first"));
    buffer.Remove(first);
    const int second = buffer.Add(Named("second"));

    EXPECT_EQ(buffer.Find(second)->name, "second");
  }
}
