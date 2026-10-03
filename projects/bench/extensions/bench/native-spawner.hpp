#ifndef BENCH_NATIVE_SPAWNER_HPP
#define BENCH_NATIVE_SPAWNER_HPP

#include <cstdint>

namespace bench
{
  /// Places crates: `burst` of them at once in the first frame, and
  /// `per_second` of them for ever, above the floor.
  struct NativeSpawner
  {
    /// Which crate, by what it does: 0 nothing, 1 NativeHeavyCopy, 2 HeavyCopy in
    /// Lua, 3 NativeLightCopy, 4 LightCopy in Lua, 5 NativeNoCopy, 6 NoCopy in Lua.
    std::int32_t behaviour = 0;

    std::int32_t burst = 0;
    float per_second = 0.0f;

    /// What is left of a crate from the frames before.
    float owed = 0.0f;

    std::int32_t spawned = 0;
  };
} // bench

#endif //BENCH_NATIVE_SPAWNER_HPP
