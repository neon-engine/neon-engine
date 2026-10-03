#ifndef BENCH_NATIVE_HEAVY_COPY_HPP
#define BENCH_NATIVE_HEAVY_COPY_HPP

#include <neon/extension/neon-extension.hpp>

namespace bench
{
  /// A crate that copies its position sixty-four times in every frame. The
  /// twin of HeavyCopy in Lua, where each copy crosses between the script and
  /// the store.
  struct NativeHeavyCopy
  {
    float age = 0.0f;
    neon::extension::Vector3 last{0.0f, 0.0f, 0.0f};
  };
} // bench

#endif //BENCH_NATIVE_HEAVY_COPY_HPP
