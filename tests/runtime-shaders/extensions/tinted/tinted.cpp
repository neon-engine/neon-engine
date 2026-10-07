// Draws a square with a shader the extension brings, and sets the numbers
// that shader reads. Nothing of it is a shader of the engine.

#include <string>

#include <neon/extension/neon-extension.hpp>

namespace
{
  using neon::extension::Entity;
  using neon::extension::World;

  /// Tells a small square, drawn red, another texture once it was drawn
  /// for two frames: it is blue from then on.
  class Flipping final : public neon::extension::System
  {
    int _frames = 0;

  public:
    void Update(World &world, const double delta_time) override
    {
      if (++_frames != 3) { return; }

      const Entity flipped = world.FindEntity("flipped");
      if (flipped == 0) { return; }

      const bool told = world.SetTexts(flipped, world.FindField("Renderable", "textures"), {"image://tinted/blue"});
      world.Info(told ? "The small square is told another texture" : "The small square could not be told a texture");
    }
  };

  class Tinted final : public neon::extension::Extension
  {
  public:
    bool Initialize(World &world) override
    {
      AddSystem<Flipping>("Flipping");
      return true;
    }

    void Start(World &world) override
    {
      // The numbers at place 3: a colour in linear light, which a screen
      // writes as 255 188 0.
      const bool numbered = world.SetShaderNumbers(3, {1.0f, 0.5f, 0.0f, 0.0f});

      const std::string white = world.SetImage("white", 1, 1, {255, 255, 255, 255});

      const Entity square = world.CreateEntity("square");
      world.AddComponent(square, "Transform");
      world.AddComponent(square, "Renderable");
      world.SetVector3(square, world.FindField("Transform", "position"), {0.0f, 0.0f, -3.0f});
      world.SetText(square, world.FindField("Renderable", "shader"), "extensions://tinted/shaders/tinted");
      world.SetTexts(square, world.FindField("Renderable", "textures"), {white});

      // two metres each way, facing the camera, which looks down z
      const NeonVector3 towards{0.0f, 0.0f, 1.0f};
      const NeonColor plain{1.0f, 1.0f, 1.0f, 1.0f};
      const bool shown = world.SetMesh(
        square,
        {
          {{-1.0f, -1.0f, 0.0f}, towards, {0.0f, 1.0f}, plain},
          {{1.0f, -1.0f, 0.0f}, towards, {1.0f, 1.0f}, plain},
          {{1.0f, 1.0f, 0.0f}, towards, {1.0f, 0.0f}, plain},
          {{-1.0f, 1.0f, 0.0f}, towards, {0.0f, 0.0f}, plain},
        },
        {0, 1, 2, 0, 2, 3});

      // A small square below and to the left, drawn with a shader of the
      // engine and a red picture. The blue one it is told later is made
      // ready ahead, with `preload`.
      const std::string red = world.SetImage("red", 1, 1, {255, 0, 0, 255});
      const std::string blue = world.SetImage("blue", 1, 1, {0, 0, 255, 255});
      const Entity flipped = world.CreateEntity("flipped");
      world.AddComponent(flipped, "Transform");
      world.AddComponent(flipped, "Renderable");
      world.SetVector3(flipped, world.FindField("Transform", "position"), {-1.6f, -0.9f, -3.0f});
      world.SetText(flipped, world.FindField("Renderable", "shader"), "engine://shaders/unlit");
      world.SetTexts(flipped, world.FindField("Renderable", "textures"), {red});
      world.SetTexts(flipped, world.FindField("Renderable", "preload"), {blue});
      world.SetMesh(
        flipped,
        {
          {{-0.25f, -0.25f, 0.0f}, towards, {0.0f, 1.0f}, plain},
          {{0.25f, -0.25f, 0.0f}, towards, {1.0f, 1.0f}, plain},
          {{0.25f, 0.25f, 0.0f}, towards, {1.0f, 0.0f}, plain},
          {{-0.25f, 0.25f, 0.0f}, towards, {0.0f, 0.0f}, plain},
        },
        {0, 1, 2, 0, 2, 3});

      world.Info(numbered && shown ? "The square is drawn with the shader of the extension"
                                   : "The square could not be drawn");
    }
  };
}

NEON_EXTENSION(Tinted)
