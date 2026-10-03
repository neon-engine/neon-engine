#ifndef BENCH_NATIVE_NO_COPY_HPP
#define BENCH_NATIVE_NO_COPY_HPP

#include <cstdint>

namespace bench
{
  /// A crate that counts how long it has fallen, and in how many frames it
  /// was below a line. The twin of NoCopy in Lua.
  struct NativeNoCopy
  {
    float floor = -5.0f;
    float air_time = 0.0f;
    std::int32_t passes = 0;
  };
} // bench

#endif //BENCH_NATIVE_NO_COPY_HPP
