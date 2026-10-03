// The extension of the bench: its components, and the systems that run over
// them. See ../README.md.

#include <neon/extension/neon-extension.hpp>

#include "light-copying.hpp"
#include "counting.hpp"
#include "crate.hpp"
#include "no-copying.hpp"
#include "native-light-copy.hpp"
#include "native-no-copy.hpp"
#include "native-heavy-copy.hpp"
#include "native-spawner.hpp"
#include "heavy-copying.hpp"
#include "spawning.hpp"

namespace bench
{
  class Bench final : public neon::extension::Extension
  {
  public:
    bool Initialize(neon::extension::World &world) override
    {
      AddSystem<Spawning>("Spawning");
      AddSystem<LightCopying>("LightCopying");
      AddSystem<NoCopying>("NoCopying");
      AddSystem<HeavyCopying>("HeavyCopying");
      AddSystem<Counting>("Counting");
      return true;
    }

    void RegisterComponents(neon::extension::World &world) override
    {
      using neon::extension::Field;

      world.RegisterComponent<Crate>("Crate", "A crate with no behaviour", {
        Field("spawned", &Crate::spawned),
      });

      world.RegisterComponent<NativeLightCopy>("NativeLightCopy", "A crate that adds the time of the frame to one field", {
        Field("age", &NativeLightCopy::age),
      });

      world.RegisterComponent<NativeNoCopy>("NativeNoCopy", "A crate that reads its height once a frame, and counts its time and its frames below a line", {
        Field("floor", &NativeNoCopy::floor, "The height it counts its frames below"),
        Field("air_time", &NativeNoCopy::air_time),
        Field("passes", &NativeNoCopy::passes),
      });

      world.RegisterComponent<NativeHeavyCopy>("NativeHeavyCopy", "A crate that copies its position 64 times a frame", {
        Field("age", &NativeHeavyCopy::age),
        Field("last", &NativeHeavyCopy::last),
      });

      world.RegisterComponent<NativeSpawner>("NativeSpawner", "Places crates, at once and for ever", {
        Field("behaviour", &NativeSpawner::behaviour,
              "0 nothing, 1 NativeHeavyCopy, 2 HeavyCopy, 3 NativeLightCopy, 4 LightCopy, 5 NativeNoCopy, 6 NoCopy"),
        Field("burst", &NativeSpawner::burst, "How many are placed at once in the first frame"),
        Field("per_second", &NativeSpawner::per_second, "How many are dropped in a second, for ever"),
        Field("owed", &NativeSpawner::owed),
        Field("spawned", &NativeSpawner::spawned),
      });
    }
  };
} // bench

NEON_EXTENSION(bench::Bench)
