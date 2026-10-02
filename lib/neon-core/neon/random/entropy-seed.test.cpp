#include "entropy-seed.hpp"

#include <cstddef>
#include <initializer_list>
#include <vector>

#include <gtest/gtest.h>

#include <neon/testing/fake-entropy-context.hpp>

namespace
{
  using neon::entropy_seed;
  using neon::testing::FakeEntropyContext;

  std::vector<std::byte> bytes_of(const std::initializer_list<unsigned char> values)
  {
    std::vector<std::byte> bytes;
    for (const auto value : values) { bytes.push_back(std::byte{value}); }
    return bytes;
  }

  TEST(EntropySeed, IsMadeOfEightBytesWithTheFirstLowest)
  {
    FakeEntropyContext entropy(bytes_of({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}));

    EXPECT_EQ(entropy_seed(entropy), 0x0807060504030201u);
    EXPECT_EQ(entropy.fills, 1u);
  }

  TEST(EntropySeed, TakesTheNextBytesForTheNextSeed)
  {
    FakeEntropyContext entropy(bytes_of({0x11, 0, 0, 0, 0, 0, 0, 0, 0x22, 0, 0, 0, 0, 0, 0, 0}));

    EXPECT_EQ(entropy_seed(entropy), 0x11u);
    EXPECT_EQ(entropy_seed(entropy), 0x22u);
  }

  TEST(EntropySeed, UsesEveryBitOfTheNumber)
  {
    FakeEntropyContext entropy(bytes_of({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}));

    EXPECT_EQ(entropy_seed(entropy), 0xFFFFFFFFFFFFFFFFu);
  }

  TEST(EntropySeed, IsEmptyWhenTheOperatingSystemGivesNoEntropy)
  {
    FakeEntropyContext entropy;
    entropy.refuses = true;

    EXPECT_FALSE(entropy_seed(entropy).has_value());
    EXPECT_EQ(entropy.fills, 1u);
  }
}
