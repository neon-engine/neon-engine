#ifndef VK_SKY_VALUES_HPP
#define VK_SKY_VALUES_HPP

#include <glm/glm.hpp>

namespace neon
{
  /// What the shaders of the sky are told about a draw, as push constants:
  /// the block `Sky` of sky.glsl, field for field.
  // ReSharper disable once CppInconsistentNaming
  struct VK_SkyValues
  {
    /// Turns a place on the screen, from -1 to 1 across and upward, at the
    /// far end of what the camera sees, into the direction it is seen in,
    /// in the space of the sky: the camera's turning undone, and the sky's
    /// own.
    glm::mat4 to_sky{1.0f};

    /// x: what the light of the images is multiplied by.
    glm::vec4 settings{1.0f, 0.0f, 0.0f, 0.0f};
  };
} // neon

#endif //VK_SKY_VALUES_HPP
