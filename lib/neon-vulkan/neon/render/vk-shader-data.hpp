#ifndef VK_SHADER_DATA_HPP
#define VK_SHADER_DATA_HPP

#include <glm/glm.hpp>

namespace neon
{
  // The data handed to the shaders. The layout has to match
  // app/<app>/shaders/vulkan/scene-data.glsl field for field. Every field is
  // a vec4 or a mat4, so the layout rules of the shading language add no
  // padding and the structures can be copied as they are.

  constexpr int kMax_Point_Lights = 64;
  constexpr int kMax_Spot_Lights = 64;

  // ReSharper disable once CppInconsistentNaming
  struct VK_DirectionLight
  {
    glm::vec4 direction{0.0f};
    glm::vec4 ambient{0.0f};
    glm::vec4 diffuse{0.0f};
    glm::vec4 specular{0.0f};
  };

  // ReSharper disable once CppInconsistentNaming
  struct VK_PointLight
  {
    glm::vec4 position{0.0f};
    glm::vec4 ambient{0.0f};
    glm::vec4 diffuse{0.0f};
    glm::vec4 specular{0.0f};
    // x constant, y linear, z quadratic
    glm::vec4 attenuation{0.0f};
  };

  // ReSharper disable once CppInconsistentNaming
  struct VK_SpotLight
  {
    glm::vec4 position{0.0f};
    glm::vec4 direction{0.0f};
    glm::vec4 ambient{0.0f};
    glm::vec4 diffuse{0.0f};
    glm::vec4 specular{0.0f};
    // x constant, y linear, z quadratic
    glm::vec4 attenuation{0.0f};
    // x cutoff, y outer cutoff
    glm::vec4 cutoff{0.0f};
  };

  /// What is the same for everything drawn with one camera and one set of
  /// lights.
  // ReSharper disable once CppInconsistentNaming
  struct VK_SceneData
  {
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
    glm::vec4 view_position{0.0f};
    VK_DirectionLight direction_light;
    // x number of point lights, y number of spot lights
    glm::ivec4 light_counts{0};
    VK_PointLight point_lights[kMax_Point_Lights];
    VK_SpotLight spot_lights[kMax_Spot_Lights];
  };

  /// What differs from one object to the next.
  // ReSharper disable once CppInconsistentNaming
  struct VK_ObjectData
  {
    glm::mat4 model{1.0f};
    glm::mat4 normal_matrix{1.0f};
    glm::vec4 color{1.0f};
    // x and y scale the texture coordinates
    glm::vec4 texture_scale{1.0f};
    // x shininess, y is 1 when textures are used and 0 when only the color is,
    // z is 1 when the object is see-through and 0 when it is opaque
    glm::vec4 material{0.0f};
  };

  static_assert(sizeof(VK_DirectionLight) == 64);
  static_assert(sizeof(VK_PointLight) == 80);
  static_assert(sizeof(VK_SpotLight) == 112);
  static_assert(sizeof(VK_SceneData) == 224 + kMax_Point_Lights * 80 + kMax_Spot_Lights * 112);
  static_assert(sizeof(VK_ObjectData) == 176);
} // neon

#endif //VK_SHADER_DATA_HPP
