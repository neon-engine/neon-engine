#ifndef BENCH_LIGHT_COPYING_HPP
#define BENCH_LIGHT_COPYING_HPP

#include <neon/extension/neon-extension.hpp>

#include "native-light-copy.hpp"

namespace bench
{
  /// The system of NativeLightCopy, the same work as LightCopySystem in Lua.
  class LightCopying final : public neon::extension::System
  {
    neon::extension::Query<NativeLightCopy> _crates;

  public:
    void Start(neon::extension::World &world) override
    {
      _crates = world.CreateQuery<NativeLightCopy>();
    }

    void Update(neon::extension::World &world, const double delta_time) override
    {
      const auto time = static_cast<float>(delta_time);
      _crates.Each([time](neon::extension::Entity, NativeLightCopy &crate) { crate.age += time; });
    }
  };
} // bench

#endif //BENCH_LIGHT_COPYING_HPP
