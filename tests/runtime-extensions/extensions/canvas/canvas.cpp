// Makes a picture of four plain colours and a square to show it on, and
// puts the square in front of the camera, with the engine's Transform and
// Renderable. Nothing of it is in a file.

#include <string>
#include <vector>

#include <neon/extension/neon-extension.hpp>

namespace
{
  using neon::extension::Entity;
  using neon::extension::Vertex;
  using neon::extension::World;

  // how many pixels the picture has each way, half of them one colour
  constexpr std::uint32_t side = 64;

  /// The picture: red at the top left, green at the top right, blue at the
  /// bottom left, and a grey at the bottom right.
  std::vector<std::uint8_t> MakePicture()
  {
    std::vector<std::uint8_t> pixels;
    pixels.reserve(side * side * 4);
    for (std::uint32_t y = 0; y < side; y++)
    {
      for (std::uint32_t x = 0; x < side; x++)
      {
        const bool right = x >= side / 2;
        const bool bottom = y >= side / 2;
        if (!right && !bottom) { pixels.insert(pixels.end(), {255, 0, 0, 255}); }
        else if (right && !bottom) { pixels.insert(pixels.end(), {0, 255, 0, 255}); }
        else if (!right && bottom) { pixels.insert(pixels.end(), {0, 0, 255, 255}); }
        else { pixels.insert(pixels.end(), {128, 128, 128, 255}); }
      }
    }
    return pixels;
  }

  class Canvas final : public neon::extension::Extension
  {
    /// A white square of a metre to the right of the canvas, lit by a
    /// lightmap of its own: half the light on its left half, all of it on
    /// its right.
    static void ShowLit(World &world)
    {
      const std::string white = world.SetImage("white", 1, 1, {255, 255, 255, 255});

      // Two pixels by two. 188 is how a screen writes half the light.
      const std::string light = world.SetImage(
        "light",
        2,
        2,
        {
          188, 188, 188, 255, 255, 255, 255, 255,
          188, 188, 188, 255, 255, 255, 255, 255,
        });

      const Entity lit = world.CreateEntity("lit");
      world.AddComponent(lit, "Transform");
      world.AddComponent(lit, "Renderable");
      world.SetVector3(lit, world.FindField("Transform", "position"), {1.65f, 0.0f, -3.0f});
      world.SetText(lit, world.FindField("Renderable", "shader"), "assets://shaders/unlit");
      world.SetTexts(lit, world.FindField("Renderable", "textures"), {white});
      world.SetText(lit, world.FindField("Renderable", "material.lightmap"), light);

      const NeonVector3 towards{0.0f, 0.0f, 1.0f};
      const NeonColor plain{1.0f, 1.0f, 1.0f, 1.0f};
      world.SetMesh(
        lit,
        {
          {{-0.5f, -0.5f, 0.0f}, towards, {0.0f, 1.0f}, plain},
          {{0.5f, -0.5f, 0.0f}, towards, {1.0f, 1.0f}, plain},
          {{0.5f, 0.5f, 0.0f}, towards, {1.0f, 0.0f}, plain},
          {{-0.5f, 0.5f, 0.0f}, towards, {0.0f, 0.0f}, plain},
        },
        {0, 1, 2, 0, 2, 3});

      // the lightmap lies over the square once, whatever its texture does
      const bool lightmapped = world.SetMeshLightmap(lit, {{0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}});
      world.Info(lightmapped ? "The square is lit by " + light : "The square could not be lit");
    }

  public:
    void Start(World &world) override
    {
      const std::string picture = world.SetImage("quarters", side, side, MakePicture());

      const Entity canvas = world.CreateEntity("canvas");
      world.AddComponent(canvas, "Transform");
      world.AddComponent(canvas, "Renderable");
      world.SetVector3(canvas, world.FindField("Transform", "position"), {0.0f, 0.0f, -3.0f});
      world.SetText(canvas, world.FindField("Renderable", "shader"), "assets://shaders/unlit");
      world.SetTexts(canvas, world.FindField("Renderable", "textures"), {picture});

      // two metres each way, facing the camera, which looks down z
      const NeonVector3 towards{0.0f, 0.0f, 1.0f};
      const NeonColor white{1.0f, 1.0f, 1.0f, 1.0f};
      const bool shown = world.SetMesh(
        canvas,
        {
          {{-1.0f, -1.0f, 0.0f}, towards, {0.0f, 1.0f}, white},
          {{1.0f, -1.0f, 0.0f}, towards, {1.0f, 1.0f}, white},
          {{1.0f, 1.0f, 0.0f}, towards, {1.0f, 0.0f}, white},
          {{-1.0f, 1.0f, 0.0f}, towards, {0.0f, 0.0f}, white},
        },
        {0, 1, 2, 0, 2, 3});

      world.Info(shown ? "The canvas shows " + picture : "The canvas could not be shown");

      ShowLit(world);
    }
  };
}

NEON_EXTENSION(Canvas)
