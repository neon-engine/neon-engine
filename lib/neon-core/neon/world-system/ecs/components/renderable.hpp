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

    /// The `mesh_version` of the mesh the renderer has. Filled in by the
    /// engine.
    unsigned int mesh_version_drawn = 0;

    /// The `version` of what the renderer was last told the entity looks
    /// like. Filled in by the engine.
    unsigned int version_drawn = 0;
  };

  /// What the renderer knows the entity as is not described. It belongs to
  /// one run and means nothing in a file.
  inline void Describe(TypeBuilder<Renderable> &type)
  {
    type.Named("Renderable", "Makes an entity visible");

    // The renderer keeps what an entity looks like from when it was first
    // drawn. A field that is written afterwards counts the version up, and
    // the entity is drawn with what it says now.
    type.Written([](Renderable &renderable) { renderable.render_info.version++; });

    type.Field("model", [](Renderable &renderable) -> std::string & { return renderable.render_info.model_path; })
        .Describe("Virtual path of the model. Left out when the entity has a Geometry, which is drawn instead");

    type.Choice("fit", [](Renderable &renderable) -> ModelFit & { return renderable.render_info.fit; }, {"none", "unit"})
        .Describe(
          "How the model is sized: none draws it at its own size and origin, as the file says; unit moves its middle "
          "to the origin and scales it so that its longest side is 1");

    type.Field("shader", [](Renderable &renderable) -> std::string & { return renderable.render_info.shader_path; })
        .Required()
        .Describe("Virtual path of the shader, without an extension");

    type.Field(
          "textures",
          [](Renderable &renderable) -> std::vector<std::string> & { return renderable.render_info.texture_paths; })
        .Describe("Virtual paths of the textures");

    type.Field(
          "preload",
          [](Renderable &renderable) -> std::vector<std::string> & { return renderable.render_info.preload_paths; })
        .Describe("Virtual paths of textures the entity will show later in place of its first one, made ready ahead");

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
            "metallic",
            [](Renderable &renderable) -> float & { return renderable.render_info.material_info.metallic; })
          .AtLeast(0.0f).AtMost(1.0f)
          .Describe("How much of a metal the surface is, 0 for plastic or stone and 1 for metal. Read by the pbr shader");

      material.Field(
            "roughness",
            [](Renderable &renderable) -> float & { return renderable.render_info.material_info.roughness; })
          .AtLeast(0.0f).AtMost(1.0f)
          .Describe("How rough the surface is, 0 for a mirror and 1 for matte. Read by the pbr shader");

      material.Field(
        "use_textures",
        [](Renderable &renderable) -> bool & { return renderable.render_info.material_info.use_textures; });

      material.Choice(
            "alpha_mode",
            [](Renderable &renderable) -> AlphaMode & { return renderable.render_info.material_info.alpha_mode; },
            {"opaque", "blend"})
          .Describe("Whether the alpha of the color lets what is behind show through");

      material.Choice(
            "double_sided",
            [](Renderable &renderable) -> DoubleSided & { return renderable.render_info.material_info.double_sided; },
            {"model", "always", "never"})
          .Describe(
            "Whether the back of every triangle is drawn too: model takes what the model file says, always draws "
            "it, never leaves it out");

      material.Field(
            "emissive",
            [](Renderable &renderable) -> Color & { return renderable.render_info.material_info.emissive; })
          .Describe(
            "The light the surface gives off itself, shown in the dark by the pbr and basic-lit shaders. Black, "
            "the default, takes what the model file says");

      material.Field(
            "emissive_strength",
            [](Renderable &renderable) -> float & { return renderable.render_info.material_info.emissive_strength; })
          .AtLeast(0.0f)
          .Describe("What the emissive light is multiplied by: above 1 is brighter than white, 0 turns it off");

      material.Field(
            "emissive_texture",
            [](Renderable &renderable) -> std::string &
            {
              return renderable.render_info.material_info.emissive_texture;
            })
          .Describe(
            "Virtual path of a texture of what the surface gives off, or surface://<name> for a render target. "
            "Left out takes the model file's");

      material.Field(
            "lightmap",
            [](Renderable &renderable) -> std::string & { return renderable.render_info.material_info.lightmap; })
          .Describe(
            "Virtual path of a lightmap, light worked out ahead of time, which lies over the mesh by the second "
            "set of coordinates of its vertices");

      material.Field(
            "lightmap_strength",
            [](Renderable &renderable) -> float & { return renderable.render_info.material_info.lightmap_strength; })
          .AtLeast(0.0f)
          .Describe("What the lightmap is multiplied by, in linear light");
    });
  }
} // neon

#endif //RENDERABLE_HPP
