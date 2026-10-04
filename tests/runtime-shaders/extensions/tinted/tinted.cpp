// Draws a square with a shader the extension brings, and sets the numbers
// that shader reads. Nothing of it is a shader of the engine.

#include <string>

#include <neon/extension/neon-extension.hpp>

namespace
{
  using neon::extension::Entity;
  using neon::extension::World;

  class Tinted final : public neon::extension::Extension
  {
  public:
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
      world.SetText(square, world.FindField("Renderable", "shader"), "extensions://tinted/assets/shaders/tinted");
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

      world.Info(numbered && shown ? "The square is drawn with the shader of the extension"
                                   : "The square could not be drawn");
    }
  };
}

NEON_EXTENSION(Tinted)
