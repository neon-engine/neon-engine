#include "geometry-building.hpp"

#include <algorithm>
#include <format>
#include <iterator>
#include <utility>

#include <neon/curves/bezier-path.hpp>
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
    MeshBuilder builder(geometry.texels_per_meter);

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
      case GeometryShape::Sphere: builder.AddSphere(geometry.size, geometry.sides, geometry.smooth); break;
      case GeometryShape::Cylinder: builder.AddCylinder(geometry.size, geometry.sides, geometry.smooth); break;
      case GeometryShape::Quad: builder.AddUprightQuad({geometry.size.x, geometry.size.y}); break;
      case GeometryShape::Tube:
      {
        std::vector<glm::vec3> points;
        for (std::size_t i = 0; i + 2 < geometry.points.size(); i += 3)
        {
          points.emplace_back(geometry.points[i], geometry.points[i + 1], geometry.points[i + 2]);
        }

        // straight pieces that stay within a fiftieth of the thickness of
        // the curve, which is below what shows
        const float tolerance = std::max(geometry.size.x * 0.02f, 0.0005f);
        const BezierPath<glm::vec3> path(std::move(points));
        builder.AddTube(path.Flatten(tolerance), geometry.size.x * 0.5f, geometry.sides);
        break;
      }
    }

    MeshData mesh = builder.InsideOut(geometry.inside).Build();

    // a tube is built round, and faceted when it is not to be smooth
    if (geometry.shape == GeometryShape::Tube)
    {
      if (!geometry.smooth) { ComputeFlatNormals(mesh); }
      return mesh;
    }

    // a sphere and a cylinder are built round where they are to be, with
    // the edges of a cylinder's caps kept hard, which smoothing would undo
    const bool is_built_smooth = geometry.shape == GeometryShape::Sphere || geometry.shape == GeometryShape::Cylinder;
    if (geometry.smooth && !is_built_smooth) { ComputeSmoothNormals(mesh); }
    return mesh;
  }

  std::string GeometryBuilding::KeyOf(const Geometry &geometry)
  {
    // a float is written as the shortest text that reads back as the same
    // float, so that no two values share a key; the parts are joined with
    // underscores, so that the key has no spaces in it
    std::string key = "geometry:";
    auto out = std::back_inserter(key);
    std::format_to(
      out, "{}_size_{}_{}_{}_segments_{}_sides_{}_texels_{}_smooth_{}_inside_{}",
      static_cast<int>(geometry.shape), geometry.size.x, geometry.size.y, geometry.size.z, geometry.segments,
      geometry.sides, geometry.texels_per_meter, geometry.smooth, geometry.inside);

    key += "_outline";
    for (const float value : geometry.outline) { std::format_to(out, "_{}", value); }
    key += "_points";
    for (const float value : geometry.points) { std::format_to(out, "_{}", value); }
    return key;
  }

  std::shared_ptr<const MeshData> GeometryBuilding::MeshOf(const Geometry &geometry, const std::string &key)
  {
    if (const auto it = _meshes.find(key); it != _meshes.end())
    {
      if (auto mesh = it->second.lock(); mesh != nullptr) { return mesh; }
    }

    auto mesh = std::make_shared<const MeshData>(Build(geometry));
    _meshes[key] = mesh;

    // what nothing holds any more is forgotten, now and then
    if (_meshes.size() >= _sweep_at)
    {
      std::erase_if(_meshes, [](const auto &entry) { return entry.second.expired(); });
      _sweep_at = std::max<std::size_t>(64, _meshes.size() * 2);
    }
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

        std::string key = KeyOf(geometries[i]);
        auto mesh = MeshOf(geometries[i], key);
        if (mesh->IsEmpty())
        {
          const auto name = store.GetName(block.entities[i]);
          _logger->Error("The Geometry of entity '{}' builds nothing, so nothing is drawn", name);
        }
        renderables[i].render_info.mesh = std::move(mesh);
        renderables[i].render_info.mesh_key = std::move(key);
      }
    });
  }
} // neon
