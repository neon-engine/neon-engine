#ifndef RENDER_INFO_HPP
#define RENDER_INFO_HPP

#include <memory>
#include <string>
#include <vector>
#include <neon/common/color.hpp>
#include <neon/geometry/mesh-data.hpp>

#include "material-info.hpp"


namespace neon
{
  struct RenderInfo
  {
    /// Virtual path of the model file. Not read when `mesh` is set.
    std::string model_path;

    /// A mesh built at run time, in place of a model file: by a Geometry
    /// component, a tool, or an importer. Drawn as it is, in metres, without
    /// the centring and scaling a model from a file gets. Shared, so that
    /// the physics may hold the same mesh.
    std::shared_ptr<const MeshData> mesh;
    std::string shader_path;
    std::vector<std::string> texture_paths;
    bool scale_textures;
    MaterialInfo material_info;
  };
} // neon

#endif //RENDER_INFO_HPP
