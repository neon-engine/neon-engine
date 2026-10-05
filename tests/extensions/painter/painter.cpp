// An extension that shows something of its own making: a picture of four
// pixels and a square to show it on, given to an entity it creates with the
// engine's Transform and Renderable.

#include <string>
#include <vector>

#include <neon/extension/neon-extension.hpp>

namespace
{
  using neon::extension::Entity;
  using neon::extension::Vertex;
  using neon::extension::World;

  class PainterExtension final : public neon::extension::Extension
  {
  public:
    void Start(World &world) override
    {
      // red, green, blue, and white, from the top left
      const std::vector<std::uint8_t> pixels = {
        255, 0, 0, 255, 0, 255, 0, 255,
        0, 0, 255, 255, 255, 255, 255, 255,
      };
      const std::string picture = world.SetImage("checker", 2, 2, pixels);

      const Entity painted = world.CreateEntity("painted");
      const bool added = world.AddComponent(painted, "Transform") && world.AddComponent(painted, "Renderable");

      world.SetVector3(painted, world.FindField("Transform", "position"), {0.0f, 0.0f, -3.0f});
      world.SetText(painted, world.FindField("Renderable", "shader"), "assets://shaders/unlit");
      const bool textured = world.SetTexts(painted, world.FindField("Renderable", "textures"), {picture});

      // a square of two metres that faces along z, towards a camera at the
      // origin that looks down it
      const NeonVector3 towards{0.0f, 0.0f, 1.0f};
      const NeonColor white{1.0f, 1.0f, 1.0f, 1.0f};
      const std::vector<Vertex> corners = {
        {{-1.0f, -1.0f, 0.0f}, towards, {0.0f, 1.0f}, white},
        {{1.0f, -1.0f, 0.0f}, towards, {1.0f, 1.0f}, white},
        {{1.0f, 1.0f, 0.0f}, towards, {1.0f, 0.0f}, white},
        {{-1.0f, 1.0f, 0.0f}, towards, {0.0f, 0.0f}, white},
      };
      const bool meshed = world.SetMesh(painted, corners, {0, 1, 2, 0, 2, 3});

      // where each corner is in a lightmap, and the lightmap its material names
      const bool lit = world.SetMeshLightmap(painted, {{0.1f, 0.9f}, {0.9f, 0.9f}, {0.9f, 0.1f}, {0.1f, 0.1f}})
                       && world.SetText(painted, world.FindField("Renderable", "material.lightmap"), picture);
      world.Info(std::string("Lightmap: ") + (lit ? "yes" : "no"));

      world.Info("Painted " + picture + ": components " + (added ? "yes" : "no") + ", textures "
                 + (textured ? "yes" : "no") + ", mesh " + (meshed ? "yes" : "no"));

      // what is refused: a mesh for an entity that draws nothing, an index
      // that names no corner, a picture that is not its size, and a
      // component nobody knows
      const Entity bare = world.CreateEntity("bare");
      const bool refused = !world.SetMesh(bare, corners, {0, 1, 2})
                           && !world.SetMesh(painted, corners, {0, 1, 4})
                           && !world.SetMesh(painted, corners, {0, 1})
                           && !world.SetMeshLightmap(painted, {{0.0f, 0.0f}})
                           && !world.SetMeshLightmap(bare, {{0.0f, 0.0f}})
                           && world.SetImage("short", 2, 2, {255, 0, 0}).empty()
                           && !world.AddComponent(bare, "Nothing");
      world.Info(std::string("What is no mesh, picture, or component was refused: ") + (refused ? "yes" : "no"));

      // a sound from memory, which a sound source plays by the path it is
      // given; one without bytes is refused
      const std::string sound = world.SetSound("beep", {82, 73, 70, 70});
      world.Info("Handed over " + sound + ", and nothing without bytes: " +
        (world.SetSound("silence", {}).empty() ? "yes" : "no"));

      // how loud a group of sounds is, as a slider of a menu sets it; a
      // group without a name is refused
      const bool quieter = world.SetGroupVolume("music", 0.25f);
      world.Info("Turned the music down: " + std::string(quieter ? "yes" : "no") + ", to a quarter: " +
        (world.GetGroupVolume("music") == 0.25f ? "yes" : "no") + ", and no group without a name: " +
        (world.SetGroupVolume("", 1.0f) ? "no" : "yes"));

      // what nothing shows any more is freed when an extension says its
      // level is over
      world.Info(std::string("Asked for what nothing shows to be freed: ") + (world.FreeUnused() ? "yes" : "no"));

      // A component is turned off and on: off, it is not there for who
      // asks, and what it holds can still be written. One the entity does
      // not have is refused.
      const NeonComponent transform = world.FindComponent("Transform");
      const bool turned_off = world.SetEnabled(painted, transform, false) && !world.IsEnabled(painted, transform);
      const bool written = world.SetVector3(painted, world.FindField("Transform", "scale"), {2.0f, 2.0f, 2.0f});
      const bool turned_on = world.SetEnabled(painted, transform, true) && world.IsEnabled(painted, transform);
      const bool refused_off = !world.SetEnabled(painted, world.FindComponent("Camera"), false);
      world.Info(std::string("Turned a component off: ") + (turned_off ? "yes" : "no") + ", wrote it while it was off: " +
        (written ? "yes" : "no") + ", turned it on: " + (turned_on ? "yes" : "no") +
        ", and none that is not there: " + (refused_off ? "yes" : "no"));

      // A pool that is handed instances made in code: it hands them out in
      // the order they wait, hands out none when all are out, and takes
      // them back.
      const Entity pool = world.CreateEntity("sparks");
      const bool pooled = world.AddComponent(pool, "PoolManager");
      std::vector<Entity> sparks;
      for (int i = 0; i < 2; i++) { sparks.push_back(world.CreateEntity("spark " + std::to_string(i), pool)); }
      const bool taken = world.AddToPool(pool, "instance://spark", sparks) == 2;
      const Entity first = world.AcquireFromPool(pool, "instance://spark");
      const Entity second = world.AcquireFromPool(pool, "instance://spark");
      const bool ran_out = world.AcquireFromPool(pool, "instance://spark") == 0 && world.CountFreeInPool(pool, "instance://spark") == 0;
      const bool given_back = world.ReleaseToPool(first) && !world.ReleaseToPool(first)
                              && world.CountFreeInPool(pool, "instance://spark") == 1 && world.CountInPool(pool, "instance://spark") == 2;
      world.Info(std::string("A pool: ") + (pooled && taken ? "yes" : "no") + ", handed out in turn: " +
        (first == sparks[0] && second == sparks[1] ? "yes" : "no") + ", none when all were out: " +
        (ran_out ? "yes" : "no") + ", took one back: " + (given_back ? "yes" : "no"));

      // How the window is shown, as the video settings of a menu set it:
      // the mode, the size, the sizes the display offers, and whether a
      // frame waits for the screen. What is no mode and no size is refused.
      int wide = 0;
      int high = 0;
      const bool moded = world.SetWindowMode(1) && world.GetWindowMode() == 1;
      const bool sized = world.SetWindowSize(1280, 720) && world.GetWindowSize(wide, high) && wide == 1280 && high == 720;
      const auto offered = world.ListDisplaySizes();
      const bool listed = offered.size() == 2 && offered[0] == std::pair(3840, 2160) && offered[1] == std::pair(1920, 1080);
      const bool synced = world.SetVerticalSync(false) && !world.GetVerticalSync();
      const bool limited = world.SetFrameLimit(144) && world.GetFrameLimit() == 144;
      const bool refused_video = !world.SetWindowMode(7) && !world.SetWindowSize(0, 720);
      world.Info(std::string("The window: mode ") + (moded ? "yes" : "no") + ", size " + (sized ? "yes" : "no") +
        ", sizes of the display " + (listed ? "yes" : "no") + ", vertical sync " + (synced ? "yes" : "no") + ", frame limit " + (limited ? "yes" : "no") +
        ", and what is none of them refused: " + (refused_video ? "yes" : "no"));

      // numbers for the shaders an extension brings, at one of eight places;
      // a place that there is not is refused
      const bool numbered = world.SetShaderNumbers(2, {0.5f, 0.25f, 1.0f, 8.0f});
      world.Info(std::string("Numbers for the shaders: ") + (numbered ? "yes" : "no") + ", and none at place 8: " +
        (world.SetShaderNumbers(8, {0.0f, 0.0f, 0.0f, 0.0f}) ? "no" : "yes"));
    }
  };
}

NEON_EXTENSION(PainterExtension)
