#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP

#include <neon/reflection/type-builder.hpp>
#include <neon/render/render-info.hpp>

namespace neon
{
  /// Makes an entity visible. It is drawn where its Transform places it.
  struct Renderable
  {
    RenderInfo render_info;

    /// What the renderer knows the entity as. Filled in by the engine the
    /// first time the entity is drawn. -1 until then.
    int render_object_id = -1;
  };

  /// What the renderer knows the entity as is not described. It belongs to
  /// one run and means nothing in a file.
  inline void Describe(TypeBuilder<Renderable> &type)
  {
    type.Named("Renderable", "Makes an entity visible");

    type.Field("model", [](Renderable &renderable) -> std::string & { return renderable.render_info.model_path; })
        .Required()
        .Describe("Virtual path of the model");

    type.Field("shader", [](Renderable &renderable) -> std::string & { return renderable.render_info.shader_path; })
        .Required()
        .Describe("Virtual path of the shader, without an extension");

    type.Field(
          "textures",
          [](Renderable &renderable) -> std::vector<std::string> & { return renderable.render_info.texture_paths; })
        .Describe("Virtual paths of the textures");

    type.Field("scale_textures", [](Renderable &renderable) -> bool & { return renderable.render_info.scale_textures; })
        .Describe("Whether textures repeat as the entity grows");

    type.Group("material", [](TypeBuilder<Renderable> &material)
    {
      material.Field(
        "shininess",
        [](Renderable &renderable) -> float & { return renderable.render_info.material_info.shininess; });

      material.Field(
        "color",
        [](Renderable &renderable) -> Color & { return renderable.render_info.material_info.color; });

      material.Field(
        "use_textures",
        [](Renderable &renderable) -> bool & { return renderable.render_info.material_info.use_textures; });

      material.Choice(
            "alpha_mode",
            [](Renderable &renderable) -> AlphaMode & { return renderable.render_info.material_info.alpha_mode; },
            {"opaque", "blend"})
          .Describe("Whether the alpha of the colour lets what is behind show through");

      material.Field(
            "double_sided",
            [](Renderable &renderable) -> bool & { return renderable.render_info.material_info.double_sided; })
          .Describe("Whether the back of every triangle is drawn too. Off leaves it out");
    });
  }
} // neon

#endif //RENDERABLE_HPP
