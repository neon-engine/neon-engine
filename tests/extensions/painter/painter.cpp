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
    }
  };
}

NEON_EXTENSION(PainterExtension)
