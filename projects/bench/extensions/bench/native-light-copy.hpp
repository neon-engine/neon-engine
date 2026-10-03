#ifndef BENCH_NATIVE_LIGHT_COPY_HPP
#define BENCH_NATIVE_LIGHT_COPY_HPP

namespace bench
{
  /// A crate that does the least a system can: its age grows by the time
  /// of the frame. The twin of LightCopy in Lua.
  struct NativeLightCopy
  {
    float age = 0.0f;
  };
} // bench

#endif //BENCH_NATIVE_LIGHT_COPY_HPP
