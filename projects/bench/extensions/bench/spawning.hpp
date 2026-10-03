#ifndef BENCH_SPAWNING_HPP
#define BENCH_SPAWNING_HPP

#include <array>
#include <random>
#include <string>

#include <neon/extension/neon-extension.hpp>

#include "native-spawner.hpp"

namespace bench
{
  /// The system of NativeSpawner. The crates of a burst are placed in
  /// layers of a grid, so that every run starts the same; those that rain
  /// are placed at random from a seed that is always the same.
  class Spawning final : public neon::extension::System
  {
    neon::extension::Query<NativeSpawner> _spawners;
    std::mt19937 _random{7};

    /// The prefab of a behaviour, see NativeSpawner::behaviour.
    static const std::string &PrefabOf(const std::int32_t behaviour)
    {
      static const std::array<std::string, 7> prefabs = {
        "extensions://bench/assets/prefabs/crate-none.prefab.yml",
        "extensions://bench/assets/prefabs/crate-cpp-heavy-copy.prefab.yml",
        "extensions://bench/assets/prefabs/crate-lua-heavy-copy.prefab.yml",
        "extensions://bench/assets/prefabs/crate-cpp-light-copy.prefab.yml",
        "extensions://bench/assets/prefabs/crate-lua-light-copy.prefab.yml",
        "extensions://bench/assets/prefabs/crate-cpp-no-copy.prefab.yml",
        "extensions://bench/assets/prefabs/crate-lua-no-copy.prefab.yml",
      };
      return prefabs[behaviour < 0 || behaviour >= static_cast<std::int32_t>(prefabs.size()) ? 0 : behaviour];
    }

  public:
    void Start(neon::extension::World &world) override
    {
      _spawners = world.CreateQuery<NativeSpawner>();
    }

    void Update(neon::extension::World &world, const double delta_time) override
    {
      std::uniform_real_distribution<float> place(-10.0f, 10.0f);
      std::uniform_real_distribution<float> turn(0.0f, 360.0f);

      _spawners.Each([&](neon::extension::Entity, NativeSpawner &spawner)
      {
        const std::string &prefab = PrefabOf(spawner.behaviour);

        // the burst: 20 by 20 to a layer, a metre and a bit apart
        while (spawner.spawned < spawner.burst)
        {
          const std::int32_t index = spawner.spawned;
          const auto x = static_cast<float>(index % 20) * 1.2f - 11.4f;
          const auto z = static_cast<float>(index / 20 % 20) * 1.2f - 11.4f;
          const auto y = static_cast<float>(index / 400) * 1.2f + 6.0f;
          (void) world.SpawnAt(prefab, {x, y, z});
          spawner.spawned++;
        }

        spawner.owed += spawner.per_second * static_cast<float>(delta_time);
        while (spawner.owed >= 1.0f)
        {
          spawner.owed -= 1.0f;
          const float x = place(_random);
          const float z = place(_random);
          (void) world.SpawnAt(prefab, {x, 30.0f, z}, {turn(_random), turn(_random), turn(_random)});
          spawner.spawned++;
        }
      });
    }
  };
} // bench

#endif //BENCH_SPAWNING_HPP
