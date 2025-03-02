#ifndef LIGHT_HPP
#define LIGHT_HPP

#include <string>
#include <glm/vec3.hpp>

namespace core
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
    glm::vec3 position{};
    glm::vec3 direction{};
    glm::vec3 ambient{};
    glm::vec3 diffuse{};
    glm::vec3 specular{};
  };
} // core

#endif //LIGHT_HPP
