#ifndef FAKE_ENTROPY_CONTEXT_HPP
#define FAKE_ENTROPY_CONTEXT_HPP

#include <cstddef>
#include <span>
#include <utility>
#include <vector>

#include <neon/random/entropy-context.hpp>

namespace neon::testing
{
  /// An entropy whose bytes a test chooses, so that what is made of them can
  /// be checked against a known value.
  ///
  /// The bytes are handed out in order, one fill after the other, and start
  /// over from the first when they run out. Without any, every byte is zero.
  class FakeEntropyContext final : public EntropyContext
  {
    std::size_t _next = 0;

  public:
    /// What is handed out.
    std::vector<std::byte> bytes;

    /// Makes every fill fail, as an operating system without entropy does.
    bool refuses = false;

    /// How often a fill was asked for, including the ones that were refused.
    std::size_t fills = 0;

    FakeEntropyContext() = default;

    explicit FakeEntropyContext(std::vector<std::byte> known_bytes)
      : bytes(std::move(known_bytes)) {}

    bool Fill(const std::span<std::byte> target) override
    {
      ++fills;
      if (refuses) { return false; }

      for (auto &byte : target)
      {
        if (bytes.empty())
        {
          byte = std::byte{0};
          continue;
        }
        byte = bytes[_next];
        _next = (_next + 1) % bytes.size();
      }
      return true;
    }
  };
} // neon::testing

#endif //FAKE_ENTROPY_CONTEXT_HPP
