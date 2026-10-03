#ifndef BENCH_HEAVY_COPYING_HPP
#define BENCH_HEAVY_COPYING_HPP

#include <neon/extension/neon-extension.hpp>

#include "native-heavy-copy.hpp"

namespace bench
{
  /// The system of NativeHeavyCopy, the same work as HeavyCopySystem in Lua: the
  /// position is copied sixty-four times, which costs a script a crossing
  /// each time and costs this nothing but the copies.
  class HeavyCopying final : public neon::extension::System
  {
    neon::extension::BlockQuery _crates;
    neon::extension::FieldPlace<neon::extension::Vector3> _position;

    static void Churn(NativeHeavyCopy &crate, const neon::extension::Vector3 &position)
    {
      for (int i = 1; i <= 64; i++)
      {
        // volatile, so that the copies are made and not worked out away
        const volatile neon::extension::Vector3 copy{position.x + static_cast<float>(i), position.y, position.z};
        crate.last = neon::extension::Vector3{copy.x, copy.y, copy.z};
      }
    }

  public:
    void Start(neon::extension::World &world) override
    {
      _crates = world.CreateBlockQuery(
        {neon::extension::ComponentOf<NativeHeavyCopy>::id, world.FindComponent("Transform")});
      _position = world.PlaceField<neon::extension::Vector3>("Transform", "position");
      if (!_crates.IsValid() || !_position.IsValid()) { world.Error("HeavyCopying cannot reach the Transform"); }
    }

    void Update(neon::extension::World &world, const double delta_time) override
    {
      if (!_position.IsValid()) { return; }

      const auto time = static_cast<float>(delta_time);
      _crates.Each([this, time](const NeonEntityBlock &block)
      {
        auto *crates = static_cast<NativeHeavyCopy *>(block.columns[0]);
        for (std::uint64_t i = 0; i < block.count; i++)
        {
          crates[i].age += time;
          Churn(crates[i], _position.At(block.columns[1], i));
        }
      });
    }
  };
} // bench

#endif //BENCH_HEAVY_COPYING_HPP
