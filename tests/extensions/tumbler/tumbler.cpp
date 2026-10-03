// An extension in C++, over neon-extension.hpp: a component with its
// defaults in the struct, and a system that turns what carries it in every
// step of the world and counts the frames.

#include <string>

#include <neon/extension/neon-extension.hpp>

namespace
{
  using neon::extension::Entity;
  using neon::extension::Field;
  using neon::extension::Query;
  using neon::extension::Vector3;
  using neon::extension::World;

  struct Tumbler
  {
    float speed = 90.0f;
    Vector3 axis{0.0f, 1.0f, 0.0f};
    float angle = 0.0f;
    std::int32_t frames = 0;
    bool enabled = true;
  };

  class Tumbling final : public neon::extension::System
  {
    Query<Tumbler> _tumblers;

    // how fast everything turns, which the extension chose when it added
    // the system
    float _pace;

  public:
    explicit Tumbling(const float pace) { _pace = pace; }

    void Start(World &world) override
    {
      _tumblers = world.CreateQuery<Tumbler>();
    }

    void Update(World &world, const double delta_time) override
    {
      _tumblers.Each([](Entity, Tumbler &tumbler) { tumbler.frames += 1; });
    }

    void FixedUpdate(World &world, const double fixed_delta_time) override
    {
      const auto step = static_cast<float>(fixed_delta_time) * _pace;
      _tumblers.Each([step](Entity, Tumbler &tumbler)
      {
        if (tumbler.enabled) { tumbler.angle += tumbler.speed * step; }
      });
    }

    void Interpolate(World &world, const double blend) override
    {
      if (blend == 0.25) { world.Info("Between two steps, a quarter of the way"); }
    }
  };

  class TumblerExtension final : public neon::extension::Extension
  {
  public:
    bool Initialize(World &world) override
    {
      AddSystem<Tumbling>("Tumbling", 2.0f);
      return true;
    }

    void RegisterComponents(World &world) override
    {
      world.RegisterComponent<Tumbler>("Tumbler", "Turns what carries it, step by step", {
        Field("speed", &Tumbler::speed, "How far it turns in a second, in degrees"),
        Field("axis", &Tumbler::axis),
        Field("angle", &Tumbler::angle),
        Field("frames", &Tumbler::frames),
        Field("enabled", &Tumbler::enabled),
      });
    }

    void Start(World &world) override
    {
      const Entity still = world.CreateEntity("made-by-tumbler");
      world.Set(still, Tumbler{.speed = 10.0f, .enabled = false});

      world.Info(std::string("A tumbler that stands still was made: ") + (world.Has<Tumbler>(still) ? "yes" : "no"));
    }

    void CleanUp(World &world) override
    {
      world.Info("The tumblers are put away");
    }
  };
}

NEON_EXTENSION(TumblerExtension)
