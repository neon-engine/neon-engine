#include "rope-drawing.hpp"

#include <algorithm>

#include <neon/common/transform.hpp>
#include <neon/curves/hanging-curve.hpp>
#include <neon/curves/polyline.hpp>
#include <neon/geometry/tube.hpp>
#include <neon/world-system/ecs/components/joint.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>

namespace neon
{
  RopeDrawing::RopeDrawing(const std::shared_ptr<Logger> &logger)
  {
    _logger = logger;
  }

  void RopeDrawing::Register(EntityStore &store)
  {
    store.Register<Rope>("Rope");
  }

  void RopeDrawing::Initialize(EntityStore &store)
  {
    _query = store.Query<Rope, Renderable, Transform>();
    _joints_known = store.FindComponent("Joint") != No_Component;
  }

  bool RopeDrawing::Place(
    EntityStore &store,
    const std::string &path,
    const glm::vec3 &anchor,
    Entity &found,
    glm::vec3 &place) const
  {
    if (path.empty())
    {
      place = anchor;
      return true;
    }

    if (found == No_Entity || !store.IsAlive(found)) { found = store.FindEntity(path); }
    if (found == No_Entity) { return false; }

    const auto *transform = store.Get<Transform>(found);
    if (transform == nullptr) { return false; }

    // where it is drawn, which carries its scale as an anchor is sized by
    place = glm::vec3(transform->world_coordinates * glm::vec4(anchor, 1.0f));
    return true;
  }

  bool RopeDrawing::FindEnds(
    EntityStore &store,
    const Entity entity,
    Rope &rope,
    glm::vec3 &from,
    glm::vec3 &to,
    float &length) const
  {
    const auto refuse = [&](const std::string &why)
    {
      rope.failed = true;
      const auto name = store.GetName(entity);
      _logger->Error("The Rope of entity '{}' cannot be drawn: {}", name, why);
      return false;
    };

    if (rope.joint.empty())
    {
      if (!Place(store, rope.from, rope.from_anchor, rope.from_entity, from))
      {
        return refuse("'from' names '" + rope.from + "', which is no entity with a Transform");
      }
      if (!Place(store, rope.to, rope.to_anchor, rope.to_entity, to))
      {
        return refuse("'to' names '" + rope.to + "', which is no entity with a Transform");
      }
      length = rope.length;
      return true;
    }

    // the rope of a joint: one end on the entity of the joint, the other
    // where the joint is held
    if (rope.from_entity == No_Entity || !store.IsAlive(rope.from_entity))
    {
      rope.from_entity = store.FindEntity(rope.joint);
    }
    if (rope.from_entity == No_Entity) { return refuse("'joint' names '" + rope.joint + "', which is no entity"); }

    const auto *joint = _joints_known ? store.Get<Joint>(rope.from_entity) : nullptr;
    if (joint == nullptr || joint->type != JointKind::Rope)
    {
      return refuse("'joint' names '" + rope.joint + "', which has no Joint of type rope");
    }

    if (!Place(store, rope.joint, joint->anchor, rope.from_entity, from))
    {
      return refuse("'" + rope.joint + "' has no Transform");
    }
    if (!Place(store, joint->other, joint->other_anchor, rope.to_entity, to))
    {
      return refuse("its joint is held by '" + joint->other + "', which is no entity with a Transform");
    }

    // a joint without a length is as long as its ends were apart when it
    // was made, which is when the rope first sees them
    if (rope.found_length < 0.0f) { rope.found_length = glm::length(to - from); }
    length = joint->length > 0.0f ? joint->length : rope.found_length;
    return true;
  }

  void RopeDrawing::Interpolate(EntityStore &store, const double blend)
  {
    store.Each(_query, [this, &store](const EntityBlock &block)
    {
      auto *ropes = block.Column<Rope>(0);
      auto *renderables = block.Column<Renderable>(1);
      const auto *transforms = block.Column<Transform>(2);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &rope = ropes[i];
        auto &info = renderables[i].render_info;
        if (rope.failed) { continue; }

        // the mesh is the rope's own, and what the Renderable draws
        if (rope.mesh == nullptr) { rope.mesh = std::make_shared<MeshData>(); }
        if (info.mesh != rope.mesh) { info.mesh = rope.mesh; }

        glm::vec3 from;
        glm::vec3 to;
        float length = 0.0f;
        if (!FindEnds(store, block.entities[i], rope, from, to, length)) { continue; }

        // The mesh is written where the entity of the rope is, so that it
        // is drawn where its ends are wherever that entity was put.
        const glm::mat4 into_entity = inverse(transforms[i].world_coordinates);
        from = glm::vec3(into_entity * glm::vec4(from, 1.0f));
        to = glm::vec3(into_entity * glm::vec4(to, 1.0f));

        // nothing moved: what is drawn already is right
        if (!rope.mesh->IsEmpty() && from == rope.drawn_from && to == rope.drawn_to && length == rope.drawn_length)
        {
          continue;
        }

        // down in the world, seen from the entity
        const glm::vec3 turned = glm::vec3(into_entity * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
        const glm::vec3 down = dot(turned, turned) > 0.0f ? normalize(turned) : glm::vec3(0.0f, -1.0f, 0.0f);

        // the curve, flattened finely, then walked in even steps: a ring
        // of the tube every so far along the rope
        const auto curve = HangingCurve(from, to, length, down);
        _flat.clear();
        _flat.push_back(curve.p0);
        curve.Flatten(std::max(rope.thickness * 0.05f, 0.0005f), _flat);
        ResamplePolyline(_flat, static_cast<std::size_t>(std::max(rope.segments, 1)) + 1, _centres);

        // written over the mesh of the frame before, which has as many
        // vertices, so nothing is allocated here or in the renderer
        rope.mesh->vertices.clear();
        rope.mesh->indices.clear();
        AppendTube(_centres, rope.thickness * 0.5f, rope.sides, rope.texels_per_metre, *rope.mesh);

        rope.drawn_from = from;
        rope.drawn_to = to;
        rope.drawn_length = length;
        info.mesh_version++;
      }
    });
  }
} // neon
