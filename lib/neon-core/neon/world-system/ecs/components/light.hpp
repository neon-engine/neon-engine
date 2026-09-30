#ifndef LIGHT_COMPONENT_HPP
#define LIGHT_COMPONENT_HPP

#include <neon/reflection/type-builder.hpp>
#include <neon/render/light-source.hpp>

namespace neon
{
  /// Makes an entity a source of light. The position of the light follows
  /// the Transform of the entity.
  struct Light
  {
    LightSource source;
  };

  /// The position of a light follows the Transform of its entity, and the
  /// renderer names it after its entity, so neither is described.
  inline void Describe(TypeBuilder<Light> &type)
  {
    type.Named("Light", "Makes an entity a source of light");

    // the type is what a light is, so it is written even when it is the
    // default
    type.Choice(
          "type",
          [](Light &light) -> LightType & { return light.source.light_type; },
          {"direction", "point", "spot"})
        .AlwaysWritten();

    type.Field("direction", [](Light &light) -> glm::vec3 & { return light.source.direction; });
    type.Field("ambient", [](Light &light) -> glm::vec3 & { return light.source.ambient; });
    type.Field("diffuse", [](Light &light) -> glm::vec3 & { return light.source.diffuse; });
    type.Field("specular", [](Light &light) -> glm::vec3 & { return light.source.specular; });
    type.Field("constant", [](Light &light) -> float & { return light.source.constant; });
    type.Field("linear", [](Light &light) -> float & { return light.source.linear; });
    type.Field("quadratic", [](Light &light) -> float & { return light.source.quadratic; });
    type.Field("cutoff", [](Light &light) -> float & { return light.source.cutoff; });
    type.Field("outer_cutoff", [](Light &light) -> float & { return light.source.outer_cutoff; });
  }
} // neon

#endif //LIGHT_COMPONENT_HPP
