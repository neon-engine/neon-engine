#ifndef RENDER_INFO_HPP
#define RENDER_INFO_HPP

#include <string>
#include <vector>
#include <neon/common/color.hpp>

#include "material-info.hpp"


namespace neon
{
  struct RenderInfo
  {
    std::string model_path;
    std::string shader_path;
    std::vector<std::string> texture_paths;
    bool scale_textures;
    MaterialInfo material_info;
  };
} // neon

#endif //RENDER_INFO_HPP
