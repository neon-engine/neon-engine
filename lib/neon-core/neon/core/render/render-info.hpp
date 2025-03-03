#ifndef RENDER_INFO_HPP
#define RENDER_INFO_HPP

#include <string>
#include <vector>
#include <neon/core/common/color.hpp>

#include "material-info.hpp"


namespace core
{
  struct RenderInfo
  {
    std::string model_path;
    std::string shader_path;
    std::vector<std::string> texture_paths;
    bool scale_textures;
    MaterialInfo material_info;
  };
} // core

#endif //RENDER_INFO_HPP
