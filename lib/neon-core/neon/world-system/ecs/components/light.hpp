#ifndef LIGHT_COMPONENT_HPP
#define LIGHT_COMPONENT_HPP

#include <neon/render/light-source.hpp>

namespace neon
{
  /// Makes an entity a source of light. The position of the light follows
  /// the Transform of the entity.
  struct Light
  {
    LightSource source;
  };
} // neon

#endif //LIGHT_COMPONENT_HPP
