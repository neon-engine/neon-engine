#ifndef BENCH_NO_COPYING_HPP
#define BENCH_NO_COPYING_HPP

#include <neon/extension/neon-extension.hpp>

#include "native-no-copy.hpp"

namespace bench
{
  /// The system of NativeNoCopy, the same work as NoCopySystem in Lua. It
  /// reads the position of the engine's Transform in place.
  class NoCopying final : public neon::extension::System
  {
    neon::extension::BlockQuery _crates;
    neon::extension::FieldPlace<neon::extension::Vector3> _position;

  public:
    void Start(neon::extension::World &world) override
    {
      _crates = world.CreateBlockQuery(
        {neon::extension::ComponentOf<NativeNoCopy>::id, world.FindComponent("Transform")});
      _position = world.PlaceField<neon::extension::Vector3>("Transform", "position");
      if (!_crates.IsValid() || !_position.IsValid()) { world.Error("NoCopying cannot reach the Transform"); }
    }

    void Update(neon::extension::World &world, const double delta_time) override
    {
      if (!_position.IsValid()) { return; }

      const auto time = static_cast<float>(delta_time);
      _crates.Each([this, time](const NeonEntityBlock &block)
      {
        auto *crates = static_cast<NativeNoCopy *>(block.columns[0]);
        for (std::uint64_t i = 0; i < block.count; i++)
        {
          crates[i].air_time += time;
          if (_position.At(block.columns[1], i).y < crates[i].floor) { crates[i].passes++; }
        }
      });
    }
  };
} // bench

#endif //BENCH_NO_COPYING_HPP
