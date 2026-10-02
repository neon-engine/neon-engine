#ifndef ENTROPY_CONTEXT_HPP
#define ENTROPY_CONTEXT_HPP

#include <cstddef>
#include <span>

namespace neon
{
  /// What the rest of the engine sees of the random numbers of the operating
  /// system: bytes that nobody can predict or repeat, from its secure
  /// generator.
  ///
  /// It has no seed and is never the same twice. A game starts its own
  /// generators from it, and keeps the seed when a run is to be repeated.
  /// The engine itself draws nothing from it: a run that is to repeat does so
  /// through the fixed time step and a script of input, and no option of the
  /// command line or the settings steers it.
  ///
  /// When the operating system cannot give entropy, `Fill()` says so by
  /// returning false, and the backend logs why. It never falls back to the
  /// clock in silence. A 64-bit number for a seed is made of its bytes with
  /// `entropy_seed()`.
  class EntropyContext
  {
  protected:
    ~EntropyContext() = default;

  public:
    /// Fills the bytes from the secure generator of the operating system.
    /// Returns false, and leaves the bytes as they were, when it cannot.
    /// Nothing to fill is always filled.
    virtual bool Fill(std::span<std::byte> bytes) = 0;
  };
} // neon

#endif //ENTROPY_CONTEXT_HPP
