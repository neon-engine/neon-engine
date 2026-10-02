#include "entropy-seed.hpp"

#include <array>
#include <cstddef>

namespace neon
{
  std::optional<std::uint64_t> entropy_seed(EntropyContext &entropy)
  {
    std::array<std::byte, sizeof(std::uint64_t)> bytes{};
    if (!entropy.Fill(bytes)) { return std::nullopt; }

    // put together by hand rather than copied, so that the byte order does
    // not depend on the machine
    std::uint64_t seed = 0;
    for (std::size_t i = 0; i < bytes.size(); ++i)
    {
      seed |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[i])) << (8 * i);
    }
    return seed;
  }
} // neon
