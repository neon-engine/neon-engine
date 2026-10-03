#ifndef RENDER_INFO_HPP
#define RENDER_INFO_HPP

#include <memory>
#include <string>
#include <vector>
#include <neon/common/color.hpp>
#include <neon/geometry/mesh-data.hpp>

#include "material-info.hpp"
#include "model-fit.hpp"

namespace neon
{
  struct RenderInfo
  {
    /// Virtual path of the model file. Not read when `mesh` is set.
    std::string model_path;

    /// How the model file is sized when it is drawn: as it says, or moved
    /// to the origin and scaled to a longest side of 1. Not read when
    /// `mesh` is set, which is drawn as it is.
    ModelFit fit = ModelFit::None;

    /// A mesh built at run time, in place of a model file: by a Geometry
    /// component, a tool, or an importer. Drawn as it is, in metres, without
    /// the centring and scaling a model from a file gets. Shared, so that
    /// the physics may hold the same mesh.
    std::shared_ptr<const MeshData> mesh;

    /// Counted up by whoever changes `mesh` after it was first drawn, a
    /// rope that moves or a tool that edits a level, so that the renderer
    /// is handed the mesh again. A mesh that keeps as many vertices and
    /// triangles is written over the one before, which costs a copy.
    unsigned int mesh_version = 0;
    std::string shader_path;
    std::vector<std::string> texture_paths;
    /// Whether the textures repeat as the entity grows, see docs/scenes.md.
    /// Off unless a recipe says so: without a value here, a Renderable read
    /// without it took whatever the memory held, in a release build.
    bool scale_textures = false;
    MaterialInfo material_info;
  };
} // neon

#endif //RENDER_INFO_HPP
