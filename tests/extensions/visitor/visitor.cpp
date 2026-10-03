// An extension that uses what is not its own: the Transform of the engine,
// by the names of its fields, the input, a file, a prefab, and a scene.

#include <string>
#include <vector>

#include <neon/extension/neon-extension.hpp>

namespace
{
  using neon::extension::Entity;
  using neon::extension::Vector3;
  using neon::extension::World;

  /// Moves the entity `visited` by the action `move`, lifts it when `jump`
  /// is pressed, and does what the other actions ask for.
  class Visiting final : public neon::extension::System
  {
    NeonField _position = 0;
    NeonField _scale = 0;

    // the position of a Transform in place, for every entity that has one
    neon::extension::BlockQuery _transforms;
    neon::extension::FieldPlace<Vector3> _position_place;

  public:
    void Start(World &world) override
    {
      _position = world.FindField("Transform", "position");
      _scale = world.FindField("Transform", "scale");

      const bool unknown = world.FindField("Transform", "weight") != 0 || world.FindField("Nothing", "position") != 0;
      world.Info(std::string("Found position and scale of Transform: ") + (_position != 0 && _scale != 0 ? "yes" : "no")
                 + ", and what is not there: " + (unknown ? "yes" : "no"));

      _transforms = world.CreateBlockQuery({world.FindComponent("Transform")});
      _position_place = world.PlaceField<Vector3>("Transform", "position");

      // a rotation is worked out when it is read, and a position is no number
      const bool misplaced = world.PlaceField<Vector3>("Transform", "rotation").IsValid()
                             || world.PlaceField<float>("Transform", "position").IsValid()
                             || world.CreateBlockQuery({world.FindComponent("Nothing")}).IsValid();
      world.Info(std::string("The position has a place: ") + (_position_place.IsValid() ? "yes" : "no")
                 + ", and what has none: " + (misplaced ? "yes" : "no"));

      std::vector<std::uint8_t> recipe;
      const bool read = world.ReadFile("extensions://visitor/extension.yml", recipe);
      const std::string text(recipe.begin(), recipe.end());
      world.Info(std::string("Read its own recipe: ") + (read && text.find("name: visitor") != std::string::npos ? "yes" : "no")
                 + ", a file that is not there: "
                 + (world.FileExists("extensions://visitor/nothing.txt") || world.ReadFile("extensions://visitor/nothing.txt", recipe) ? "yes" : "no"));
    }

    void OnPhysicsEvent(World &world, const NeonPhysicsEvent &event) override
    {
      world.Info(std::string(event.trigger != 0 ? "A trigger " : "A body ") + std::to_string(event.first)
                 + (event.began != 0 ? " was entered by " : " was left by ") + std::to_string(event.second)
                 + " at height " + std::to_string(static_cast<int>(event.point.y)));
    }

    void Update(World &world, const double delta_time) override
    {
      if (world.WasActionPressed("lift"))
      {
        // every Transform a metre up, written in place, and how many on the HUD
        std::uint64_t count = 0;
        _transforms.Each([&](const NeonEntityBlock &block)
        {
          for (std::uint64_t i = 0; i < block.count; i++) { _position_place.At(block.columns[0], i).y += 1.0f; }
          count += block.count;
        });
        world.SetUiNumber("lifted", static_cast<double>(count));
        world.SetUiText("who", "visitor");
      }

      if (world.WasActionPressed("drop"))
      {
        const Entity crate = world.SpawnAt("assets://prefabs/crate.prefab.yml", {1.0f, 30.0f, 2.0f}, {0.0f, 90.0f, 0.0f});
        world.Info("Dropped " + std::to_string(crate));
      }

      if (world.WasActionPressed("look"))
      {
        NeonRayHit hit{};
        const bool found = world.CastRay({0.0f, 5.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, 100.0f, hit, 9);
        world.Info(found
                     ? "Looked down and saw " + std::to_string(hit.entity) + " at a distance of "
                       + std::to_string(static_cast<int>(hit.distance))
                     : "Looked down and saw nothing");
      }

      const Entity visited = world.FindEntity("visited");
      if (visited == 0) { return; }

      const neon::extension::Vector2 move = world.ActionAxis2("move");
      Vector3 position = world.GetVector3(visited, _position);
      position.x += move.x;
      position.z += move.y;
      if (world.WasActionPressed("jump")) { position.y += 1.0f; }
      world.SetVector3(visited, _position, position);

      if (world.IsActionDown("grow")) { world.SetVector3(visited, _scale, {2.0f, 2.0f, 2.0f}); }

      // a scale is three numbers, which the field says
      if (world.IsActionDown("vanish") && world.SetText(visited, _scale, "nothing"))
      {
        world.Error("A scale of text was taken");
      }

      if (world.WasActionPressed("spawn"))
      {
        const Entity crate = world.Spawn("assets://prefabs/crate.prefab.yml", visited);
        world.Info("Spawned " + std::to_string(crate));
      }
      if (world.WasActionPressed("leave")) { world.LoadScene("assets://scenes/next.scene.yml"); }
    }
  };

  class VisitorExtension final : public neon::extension::Extension
  {
  public:
    bool Initialize(World &world) override
    {
      AddSystem<Visiting>("Visiting");
      return true;
    }
  };
}

NEON_EXTENSION(VisitorExtension)
