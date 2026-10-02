#ifndef ENTROPY_SEED_HPP
#define ENTROPY_SEED_HPP

#include <cstdint>
#include <optional>

#include "entropy-context.hpp"

namespace neon
{
  /// A 64-bit number from the entropy, for seeding a generator of a game.
  /// Empty when the operating system could not give one, which the backend
  /// has logged.
  ///
  /// The first byte of the entropy is the lowest of the number, on every
  /// platform, so that the same bytes give the same seed everywhere.
  [[nodiscard]] std::optional<std::uint64_t> entropy_seed(EntropyContext &entropy);
} // neon

#endif //ENTROPY_SEED_HPP
