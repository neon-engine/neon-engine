#ifndef LIGHT_HPP
#define LIGHT_HPP

#include <string>
#include <glm/vec3.hpp>

namespace neon
{
  enum class LightType
  {
    Direction,
    Point,
    SpotLight
  };

  struct LightSource
  {
    std::string id;
    LightType light_type;
    float constant;
    float linear;
    float quadratic;
    float cutoff;
    float outer_cutoff;
    glm::vec3 position{};
    glm::vec3 direction{};
    glm::vec3 ambient{};
    glm::vec3 diffuse{};
    glm::vec3 specular{};
  };
} // neon

#endif //LIGHT_HPP
