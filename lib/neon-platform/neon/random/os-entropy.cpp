#include "os-entropy.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#else
// getentropy is declared here on macOS, and on Linux with glibc 2.25 and later
#include <sys/random.h>
#include <unistd.h>
#endif

namespace neon
{
#if defined(_WIN32)
  bool OS_Entropy::Fill(const std::span<std::byte> bytes)
  {
    if (bytes.empty()) { return true; }

    // the generator the system prefers, without a handle to an algorithm
    const NTSTATUS status = BCryptGenRandom(
      nullptr,
      reinterpret_cast<PUCHAR>(bytes.data()),
      static_cast<ULONG>(bytes.size()),
      BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    if (!BCRYPT_SUCCESS(status))
    {
      const auto code = static_cast<unsigned long>(status);
      _logger->Error("The operating system gave no entropy: BCryptGenRandom returned {:#x}", code);
      return false;
    }
    return true;
  }
#else
  bool OS_Entropy::Fill(const std::span<std::byte> bytes)
  {
    // getentropy fills at most 256 bytes in one call, and refuses more
    constexpr std::size_t most_per_call = 256;

    std::size_t filled = 0;
    while (filled < bytes.size())
    {
      const auto count = std::min(most_per_call, bytes.size() - filled);
      if (getentropy(bytes.data() + filled, count) != 0)
      {
        const std::string reason = std::strerror(errno);
        _logger->Error("The operating system gave no entropy: getentropy failed with {}", reason);
        return false;
      }
      filled += count;
    }
    return true;
  }
#endif
} // neon
