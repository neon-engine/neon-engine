#ifndef BENCH_COUNTING_HPP
#define BENCH_COUNTING_HPP

#include <array>
#include <string>
#include <vector>

#include <neon/extension/neon-extension.hpp>

#include "crate.hpp"
#include "native-light-copy.hpp"
#include "native-no-copy.hpp"
#include "native-heavy-copy.hpp"
#include "native-spawner.hpp"

namespace bench
{
  /// Counts the crates on the scene, whatever they do and whichever
  /// language does it, and says so on the HUD in every frame and in the log
  /// once a second. Above the count the HUD names the scene and says what
  /// its crates do, which is read off the spawner of the scene.
  class Counting final : public neon::extension::System
  {
    /// What the HUD says of a behavior: the part of the name of its scenes
    /// that says what its crates do, and that in words.
    struct Label
    {
      const char *name;
      const char *about;
    };

    std::vector<neon::extension::BlockQuery> _crates;
    neon::extension::Query<NativeSpawner> _spawners;
    bool _labeled = false;
    double _since_log = 0.0;

    /// The label of a behavior, see NativeSpawner::behavior. The names
    /// are those of the scenes under assets/scenes, and change with them.
    static const Label &LabelOf(const std::int32_t behavior)
    {
      static const std::array<Label, 7> labels = {{
        {"", "No script and no system: each crate is a body alone"},
        {"cpp-heavy-copy", "C++, heavy copy: each crate copies its position 64 times a frame"},
        {"lua-heavy-copy", "Lua, heavy copy: each crate copies its position 64 times a frame"},
        {"cpp-light-copy", "C++, light copy: each crate adds the time of the frame to one field"},
        {"lua-light-copy", "Lua, light copy: each crate adds the time of the frame to one field"},
        {"cpp-no-copy", "C++, no copy: each crate reads its height in place once a frame"},
        {"lua-no-copy", "Lua, no copy: each crate reads its height in place once a frame"},
      }};
      return labels[behavior < 0 || behavior >= static_cast<std::int32_t>(labels.size()) ? 0 : behavior];
    }

    /// Names the scene on the HUD, once, when its spawner is there.
    void LabelScene(neon::extension::World &world)
    {
      _spawners.Each([this, &world](neon::extension::Entity, NativeSpawner &spawner)
      {
        if (_labeled) { return; }

        const Label &label = LabelOf(spawner.behavior);

        // rain-lua-heavy-copy, falling-lua-no-copy-2000; and with no
        // behavior rain-no-copy, falling-none-2000
        const bool none = label.name[0] == '\0';
        const std::string scene = spawner.burst > 0
                                    ? std::string("falling-") + (none ? "none" : label.name) + "-" + std::to_string(spawner.burst)
                                    : std::string("rain-") + (none ? "no-copy" : label.name);
        world.SetUiText("scene", scene);
        world.SetUiText("about", label.about);
        _labeled = true;
      });
    }

  public:
    void Start(neon::extension::World &world) override
    {
      using neon::extension::ComponentOf;

      // the crates of the extension, and those of the scripts, which are
      // only there when their files are
      for (const NeonComponent component : {
             ComponentOf<Crate>::id,
             ComponentOf<NativeLightCopy>::id,
             ComponentOf<NativeNoCopy>::id,
             ComponentOf<NativeHeavyCopy>::id,
             world.FindComponent("LightCopy"),
             world.FindComponent("NoCopy"),
             world.FindComponent("HeavyCopy"),
           })
      {
        if (component != 0) { _crates.push_back(world.CreateBlockQuery({component})); }
      }

      _spawners = world.CreateQuery<NativeSpawner>();
    }

    void Update(neon::extension::World &world, const double delta_time) override
    {
      if (!_labeled) { LabelScene(world); }

      std::uint64_t count = 0;
      for (const auto &crates : _crates)
      {
        crates.Each([&count](const NeonEntityBlock &block) { count += block.count; });
      }

      world.SetUiNumber("crates", static_cast<double>(count));

      _since_log += delta_time;
      if (_since_log >= 1.0)
      {
        _since_log = 0.0;
        world.Info("Crates on the scene: " + std::to_string(count));
      }
    }
  };
} // bench

#endif //BENCH_COUNTING_HPP
