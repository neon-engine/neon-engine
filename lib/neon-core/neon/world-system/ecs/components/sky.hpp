#ifndef SKY_COMPONENT_HPP
#define SKY_COMPONENT_HPP

#include <string>

#include <neon/reflection/type-builder.hpp>
#include <neon/render/sky-info.hpp>

namespace neon
{
  /// Makes an entity the sky of its scene: what every camera sees behind
  /// everything else. A scene has one.
  struct Sky
  {
    SkyInfo info;
  };

  inline void Describe(TypeBuilder<Sky> &type)
  {
    type.Named("Sky", "What is seen behind everything else: the six faces of a cube, or a panorama around a sphere");

    // the type is what a sky is, so it is written even when it is the
    // default
    type.Choice("type", [](Sky &sky) -> SkyType & { return sky.info.type; }, {"box", "sphere"})
        .AlwaysWritten()
        .Describe("box shows the six images of faces, sphere wraps the one image of texture around the camera");

    type.Group("faces", [](TypeBuilder<Sky> &faces)
    {
      faces.Field("right", [](Sky &sky) -> std::string & { return sky.info.right; })
           .Describe("For a box: virtual path of the image seen along positive x");
      faces.Field("left", [](Sky &sky) -> std::string & { return sky.info.left; })
           .Describe("For a box: virtual path of the image seen along negative x");
      faces.Field("top", [](Sky &sky) -> std::string & { return sky.info.top; })
           .Describe("For a box: virtual path of the image seen straight up");
      faces.Field("bottom", [](Sky &sky) -> std::string & { return sky.info.bottom; })
           .Describe("For a box: virtual path of the image seen straight down");
      faces.Field("front", [](Sky &sky) -> std::string & { return sky.info.front; })
           .Describe("For a box: virtual path of the image seen along negative z, where a camera looks that was not turned");
      faces.Field("back", [](Sky &sky) -> std::string & { return sky.info.back; })
           .Describe("For a box: virtual path of the image seen along positive z");
    });

    type.Field("texture", [](Sky &sky) -> std::string & { return sky.info.texture; })
        .Describe("For a sphere: virtual path of a panorama twice as wide as high, its middle seen along negative z");

    type.Field("rotation", [](Sky &sky) -> float & { return sky.info.rotation; })
        .Describe("Degrees the sky is turned around the direction that is up");

    type.Field("brightness", [](Sky &sky) -> float & { return sky.info.brightness; })
        .AtLeast(0.0f)
        .Describe("What the light of the images is multiplied by");
  }
} // neon

#endif //SKY_COMPONENT_HPP
