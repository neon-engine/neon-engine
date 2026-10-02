#include "os-entropy.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include <neon/random/entropy-seed.hpp>
#include <neon/testing/recording-logger.hpp>

// What comes out is random, so a test can only check that something came
// out, and that it is not the same thing twice. The odds that 32 random bytes
// are all zero, or equal to the 32 before, are too small to see in a
// lifetime of running them.

namespace
{
  using neon::entropy_seed;
  using neon::OS_Entropy;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;

  class OsEntropyTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    OS_Entropy _entropy{_logger};

    static bool AllZero(const std::span<const std::byte> bytes)
    {
      return std::ranges::all_of(bytes, [](const std::byte byte) { return byte == std::byte{0}; });
    }
  };

  TEST_F(OsEntropyTest, FillsTheBytes)
  {
    std::array<std::byte, 32> bytes{};

    ASSERT_TRUE(_entropy.Fill(bytes));
    EXPECT_FALSE(AllZero(bytes));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(OsEntropyTest, TwoFillsDiffer)
  {
    std::array<std::byte, 32> first{};
    std::array<std::byte, 32> second{};

    ASSERT_TRUE(_entropy.Fill(first));
    ASSERT_TRUE(_entropy.Fill(second));
    EXPECT_NE(first, second);
  }

  TEST_F(OsEntropyTest, FillsNothingWithoutComplaint)
  {
    std::vector<std::byte> none;

    EXPECT_TRUE(_entropy.Fill(none));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(OsEntropyTest, FillsMoreThanTheOperatingSystemGivesInOneCall)
  {
    // getentropy stops at 256 bytes a call, so the end of this is filled by
    // a later call than the start
    std::vector<std::byte> bytes(1000);

    ASSERT_TRUE(_entropy.Fill(bytes));
    EXPECT_FALSE(AllZero(std::span(bytes).subspan(0, 256)));
    EXPECT_FALSE(AllZero(std::span(bytes).subspan(256, 256)));
    EXPECT_FALSE(AllZero(std::span(bytes).subspan(512)));
  }

  TEST_F(OsEntropyTest, GivesASeed)
  {
    const auto first = entropy_seed(_entropy);
    const auto second = entropy_seed(_entropy);

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_NE(first, second);
  }
}
