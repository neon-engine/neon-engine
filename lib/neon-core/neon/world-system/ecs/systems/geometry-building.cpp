#include "geometry-building.hpp"

#include <neon/geometry/mesh-builder.hpp>
#include <neon/geometry/mesh-normals.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>

namespace neon
{
  GeometryBuilding::GeometryBuilding(const std::shared_ptr<Logger> &logger)
  {
    _logger = logger;
  }

  MeshData GeometryBuilding::Build(const Geometry &geometry)
  {
    MeshBuilder builder(geometry.texels_per_metre);

    switch (geometry.shape)
    {
      case GeometryShape::Box: builder.AddBox(geometry.size); break;
      case GeometryShape::Plane: builder.AddPlane({geometry.size.x, geometry.size.z}, geometry.segments); break;
      case GeometryShape::Ramp: builder.AddRamp(geometry.size); break;
      case GeometryShape::Prism:
      {
        std::vector<glm::vec2> outline;
        for (std::size_t i = 0; i + 1 < geometry.outline.size(); i += 2)
        {
          outline.emplace_back(geometry.outline[i], geometry.outline[i + 1]);
        }
        builder.AddPrism(outline, geometry.size.y);
        break;
      }
    }

    MeshData mesh = builder.InsideOut(geometry.inside).Build();
    if (geometry.smooth) { ComputeSmoothNormals(mesh); }
    return mesh;
  }

  void GeometryBuilding::Register(EntityStore &store)
  {
    store.Register<Geometry>("Geometry");
  }

  void GeometryBuilding::Initialize(EntityStore &store)
  {
    _query = store.Query<Geometry, Renderable>();
  }

  void GeometryBuilding::Update(EntityStore &store, double delta_time)
  {
    store.Each(_query, [this, &store](const EntityBlock &block)
    {
      const auto *geometries = block.Column<Geometry>(0);
      auto *renderables = block.Column<Renderable>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        // built once, before the renderer is first asked for the entity
        if (renderables[i].render_info.mesh != nullptr || renderables[i].render_object_id >= 0) { continue; }

        auto mesh = std::make_shared<MeshData>(Build(geometries[i]));
        if (mesh->IsEmpty())
        {
          const auto name = store.GetName(block.entities[i]);
          _logger->Error("The Geometry of entity '{}' builds nothing, so nothing is drawn", name);
        }
        renderables[i].render_info.mesh = std::move(mesh);
      }
    });
  }
} // neon
