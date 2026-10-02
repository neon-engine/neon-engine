#ifndef OS_ENTROPY_HPP
#define OS_ENTROPY_HPP

#include <cstddef>
#include <memory>
#include <span>

#include <neon/logging/logger.hpp>
#include <neon/random/entropy-context.hpp>

namespace neon
{
  /// The secure generator of the operating system: `getentropy()` on macOS
  /// and Linux, `BCryptGenRandom` on Windows. The platform is chosen when
  /// the backend is compiled, so main.cpp creates it as it creates the
  /// others.
  // ReSharper disable once CppInconsistentNaming
  class OS_Entropy final : public EntropyContext
  {
    std::shared_ptr<Logger> _logger;

  public:
    explicit OS_Entropy(const std::shared_ptr<Logger> &logger)
      : _logger(logger) {}

    /// Logs an error and returns false when the operating system refuses,
    /// so that a failure is never quiet.
    bool Fill(std::span<std::byte> bytes) override;
  };
} // neon

#endif //OS_ENTROPY_HPP
