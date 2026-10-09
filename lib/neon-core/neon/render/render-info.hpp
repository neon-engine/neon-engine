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
    /// component, a tool, or an importer. Drawn as it is, in meters, without
    /// the centering and scaling a model from a file gets. Shared, so that
    /// the physics may hold the same mesh.
    std::shared_ptr<const MeshData> mesh;

    /// What `mesh` was built from, by its values, such as those of a
    /// Geometry: render objects whose meshes have the same key share one
    /// model, as those that name the same model file do, see ModelKey. Empty
    /// for a mesh that is the entity's own, such as a rope's, which may
    /// change and is never shared. A mesh with a key never changes.
    std::string mesh_key;

    /// Counted up by whoever changes `mesh` after it was first drawn, a
    /// rope that moves or a tool that edits a level, so that the renderer
    /// is handed the mesh again. A mesh that keeps as many vertices and
    /// triangles is written over the one before, which costs a copy.
    unsigned int mesh_version = 0;

    /// Counted up whenever anything of this but the mesh was written after
    /// the entity was first drawn: its textures, its shader, its material,
    /// its model. The renderer is then told again what the entity looks
    /// like, see RenderContext::UpdateRenderObject(). The description of
    /// Renderable counts it for whoever writes a field through it; code
    /// that writes a member itself counts it itself.
    unsigned int version = 0;

    std::string shader_path;
    std::vector<std::string> texture_paths;
    /// Textures the entity will show in place of its first one, later: the
    /// faces of a character, the skins of what a player holds. Each is made
    /// ready when the entity is first drawn and stays loaded for as long as
    /// the entity is there, so that showing it for the first time costs
    /// nothing while the game runs. It changes nothing that is drawn.
    std::vector<std::string> preload_paths;

    /// Whether the textures repeat as the entity grows, see docs/scenes.md.
    /// Off unless a recipe says so: without a value here, a Renderable read
    /// without it took whatever the memory held, in a release build.
    bool scale_textures = false;
    MaterialInfo material_info;
  };
} // neon

#endif //RENDER_INFO_HPP
