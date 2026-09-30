#include "component-format.hpp"

#include <cmath>
#include <format>

#include <neon/world-system/ecs/components/character-body.hpp>
#include <neon/world-system/ecs/components/collider.hpp>
#include <neon/world-system/ecs/components/rigid-body.hpp>
#include <neon/world-system/ecs/components/trigger.hpp>

// How the components of the physics are written in a scene file. As with
// every component, a value that is the default is left out. What the engine
// fills in while it runs, such as the body of a RigidBody, is never written.

namespace neon
{
  namespace
  {
    const std::vector<std::string> body_kinds = {"static", "kinematic", "dynamic"};

    // in the order of ShapeKind
    const std::vector<std::string> shape_kinds = {
      "box",
      "sphere",
      "capsule",
      "cylinder",
      "tapered_capsule",
      "tapered_cylinder",
      "plane",
      "convex_hull",
      "mesh"
    };

    DataValue List(const glm::vec3 &value)
    {
      auto list = DataValue::List();
      list.Add(DataValue::Number(value.x));
      list.Add(DataValue::Number(value.y));
      list.Add(DataValue::Number(value.z));
      return list;
    }

    void Put(DataValue &map, const std::string &name, const float value, const float standard)
    {
      if (value != standard) { map.Set(name, DataValue::Number(value)); }
    }

    void Put(DataValue &map, const std::string &name, const bool value, const bool standard)
    {
      if (value != standard) { map.Set(name, DataValue::Bool(value)); }
    }

    void Put(DataValue &map, const std::string &name, const glm::vec3 &value, const glm::vec3 &standard)
    {
      if (value != standard) { map.Set(name, List(value)); }
    }

    /// Layers are written as the numbers they are called by, from 1 to 32.
    void PutLayers(DataValue &map, const std::string &name, const std::uint32_t value, const std::uint32_t standard)
    {
      if (value == standard) { return; }

      auto list = DataValue::List();
      for (int layer = 1; layer <= 32; layer++)
      {
        if ((value & (1u << (layer - 1))) != 0) { list.Add(DataValue::Number(layer)); }
      }
      map.Set(name, list);
    }

    bool ReadLayer(const DataReader &reader, const std::string &name, const DataValue &value, std::uint32_t &layers)
    {
      float number = 0.0f;
      if (!value.GetNumber(number))
      {
        reader.Report(value, std::format(
                        "'{}' of {} holds {}, where a layer from 1 to 32 was expected",
                        name, reader.GetWhere(), DataValue::Describe(value.GetKind())));
        return false;
      }

      if (number < 1.0f || number > 32.0f || number != std::floor(number))
      {
        reader.Report(value, std::format(
                        "'{}' of {} holds {}, where a layer from 1 to 32 was expected",
                        name, reader.GetWhere(), number));
        return false;
      }

      layers |= 1u << (static_cast<int>(number) - 1);
      return true;
    }

    /// One layer, or a list of layers. An empty list is no layer at all.
    bool ReadLayers(const DataReader &reader, const std::string &name, std::uint32_t &value)
    {
      const auto *found = reader.ReadValue(name);
      if (found == nullptr) { return false; }

      std::uint32_t layers = 0;

      if (found->IsList())
      {
        for (const auto &item : found->GetItems())
        {
          if (!ReadLayer(reader, name, item, layers)) { return false; }
        }
      } else if (float number = 0.0f; found->GetNumber(number))
      {
        if (!ReadLayer(reader, name, *found, layers)) { return false; }
      } else
      {
        reader.Report(*found, std::format(
                        "'{}' of {} is {}, where a layer from 1 to 32 or a list of layers was expected",
                        name, reader.GetWhere(), DataValue::Describe(found->GetKind())));
        return false;
      }

      value = layers;
      return true;
    }

    /// A number that has to be above 0, such as a radius.
    void ReadAboveZero(const DataReader &reader, const std::string &name, float &value)
    {
      float written = value;
      if (!reader.Read(name, written)) { return; }

      if (!(written > 0.0f))
      {
        reader.Report(*reader.ReadValue(name), std::format(
                        "'{}' of {} is {}, where a number above 0 was expected",
                        name, reader.GetWhere(), written));
        return;
      }

      value = written;
    }

    /// A number that has to be 0 or above, such as a friction.
    void ReadFromZero(const DataReader &reader, const std::string &name, float &value, const float most = -1.0f)
    {
      float written = value;
      if (!reader.Read(name, written)) { return; }

      if (written < 0.0f || (most >= 0.0f && written > most) || written != written)
      {
        const std::string expected = most >= 0.0f
          ? std::format("a number from 0 to {}", most)
          : std::string("a number of 0 or above");

        reader.Report(*reader.ReadValue(name), std::format(
                        "'{}' of {} is {}, where {} was expected",
                        name, reader.GetWhere(), written, expected));
        return;
      }

      value = written;
    }

    /// The format of a component that the physics registers. A world
    /// without physics does not know the component, which is said instead
    /// of giving the entity something the store cannot hold.
    template<typename T>
    ComponentFormat Format(
      const std::string &name,
      const std::function<void(const DataReader &reader, T &component)> &read,
      const std::function<void(const T &component, DataValue &map)> &write)
    {
      auto format = ComponentFormat::Of<T>(name, read, write);

      format.read = [name, read](const DataReader &reader, EntityStore &store, const Entity entity)
      {
        T component{};
        read(reader, component);

        if (store.FindComponent(name) == No_Component)
        {
          reader.Report({}, std::format(
                          "{} needs the physics, which is not part of this world",
                          reader.GetWhere()));
          return;
        }

        store.Set(entity, component);
      };

      return format;
    }

    ComponentFormat RigidBodyFormat()
    {
      return Format<RigidBody>(
        "RigidBody",
        [](const DataReader &reader, RigidBody &body)
        {
          if (std::size_t kind = 0; reader.ReadChoice("kind", body_kinds, kind))
          {
            body.kind = static_cast<BodyKind>(kind);
          }

          ReadAboveZero(reader, "mass", body.mass);
          ReadFromZero(reader, "friction", body.friction);
          ReadFromZero(reader, "bounce", body.bounce, 1.0f);
          ReadFromZero(reader, "linear_damping", body.linear_damping);
          ReadFromZero(reader, "angular_damping", body.angular_damping);
          reader.Read("gravity_scale", body.gravity_scale);
          reader.Read("linear_velocity", body.linear_velocity);
          reader.Read("angular_velocity", body.angular_velocity);
          reader.Read("continuous", body.continuous);
          reader.Read("can_sleep", body.can_sleep);
          ReadLayers(reader, "layers", body.layers);
          ReadLayers(reader, "mask", body.mask);
        },
        [](const RigidBody &body, DataValue &map)
        {
          const RigidBody standard{};

          // the kind is what a body is, so it is written even when it is
          // the default
          map.Set("kind", DataValue::Text(body_kinds[static_cast<std::size_t>(body.kind)]));

          Put(map, "mass", body.mass, standard.mass);
          Put(map, "friction", body.friction, standard.friction);
          Put(map, "bounce", body.bounce, standard.bounce);
          Put(map, "linear_damping", body.linear_damping, standard.linear_damping);
          Put(map, "angular_damping", body.angular_damping, standard.angular_damping);
          Put(map, "gravity_scale", body.gravity_scale, standard.gravity_scale);
          Put(map, "linear_velocity", body.linear_velocity, standard.linear_velocity);
          Put(map, "angular_velocity", body.angular_velocity, standard.angular_velocity);
          Put(map, "continuous", body.continuous, standard.continuous);
          Put(map, "can_sleep", body.can_sleep, standard.can_sleep);
          PutLayers(map, "layers", body.layers, standard.layers);
          PutLayers(map, "mask", body.mask, standard.mask);
        });
    }

    ComponentFormat TriggerFormat()
    {
      return Format<Trigger>(
        "Trigger",
        [](const DataReader &reader, Trigger &trigger)
        {
          ReadLayers(reader, "layers", trigger.layers);
          ReadLayers(reader, "mask", trigger.mask);
        },
        [](const Trigger &trigger, DataValue &map)
        {
          const Trigger standard{};
          PutLayers(map, "layers", trigger.layers, standard.layers);
          PutLayers(map, "mask", trigger.mask, standard.mask);
        });
    }

    bool HasRadius(const ShapeKind shape)
    {
      return shape == ShapeKind::Sphere || shape == ShapeKind::Capsule || shape == ShapeKind::Cylinder;
    }

    bool HasHeight(const ShapeKind shape)
    {
      return shape == ShapeKind::Capsule || shape == ShapeKind::Cylinder
             || shape == ShapeKind::TaperedCapsule || shape == ShapeKind::TaperedCylinder;
    }

    bool IsTapered(const ShapeKind shape)
    {
      return shape == ShapeKind::TaperedCapsule || shape == ShapeKind::TaperedCylinder;
    }

    bool HasModel(const ShapeKind shape)
    {
      return shape == ShapeKind::ConvexHull || shape == ShapeKind::Mesh;
    }

    ComponentFormat ColliderFormat()
    {
      return Format<Collider>(
        "Collider",
        [](const DataReader &reader, Collider &collider)
        {
          if (std::size_t shape = 0; reader.ReadChoice("shape", shape_kinds, shape))
          {
            collider.shape = static_cast<ShapeKind>(shape);
          }

          const auto &name = shape_kinds[static_cast<std::size_t>(collider.shape)];

          // Only what belongs to the shape is asked for. A radius that is
          // written for a box is then reported as a name that is not known,
          // instead of being ignored without a word.
          if (collider.shape == ShapeKind::Box)
          {
            glm::vec3 size = collider.size;
            if (reader.ReadScale("size", size))
            {
              if (size.x > 0.0f && size.y > 0.0f && size.z > 0.0f)
              {
                collider.size = size;
              } else
              {
                reader.Report(*reader.ReadValue("size"), std::format(
                                "'size' of {} is [{}, {}, {}], where numbers above 0 were expected",
                                reader.GetWhere(), size.x, size.y, size.z));
              }
            }
          }

          if (HasRadius(collider.shape)) { ReadAboveZero(reader, "radius", collider.radius); }
          if (HasHeight(collider.shape)) { ReadAboveZero(reader, "height", collider.height); }

          if (IsTapered(collider.shape))
          {
            if (collider.shape == ShapeKind::TaperedCylinder)
            {
              // one of the two may be 0, which makes a cone
              ReadFromZero(reader, "top_radius", collider.top_radius);
              ReadFromZero(reader, "bottom_radius", collider.bottom_radius);

              if (collider.top_radius == 0.0f && collider.bottom_radius == 0.0f)
              {
                reader.Report({}, std::format(
                                "'top_radius' and 'bottom_radius' of {} are both 0, where one above 0 was expected",
                                reader.GetWhere()));
              }
            } else
            {
              ReadAboveZero(reader, "top_radius", collider.top_radius);
              ReadAboveZero(reader, "bottom_radius", collider.bottom_radius);
            }
          }

          if (collider.shape == ShapeKind::Capsule && collider.height < 2.0f * collider.radius)
          {
            reader.Report({}, std::format(
                            "'height' of {} is {}, which is less than twice its 'radius' of {}",
                            reader.GetWhere(), collider.height, collider.radius));
          }

          if (collider.shape == ShapeKind::TaperedCapsule
              && collider.height < collider.top_radius + collider.bottom_radius)
          {
            reader.Report({}, std::format(
                            "'height' of {} is {}, which is less than its 'top_radius' and 'bottom_radius' together",
                            reader.GetWhere(), collider.height));
          }

          if (HasModel(collider.shape))
          {
            reader.Read("model", collider.model);

            if (collider.model.empty())
            {
              reader.Report({}, std::format("{} needs a 'model' for the shape {}", reader.GetWhere(), name));
            }
          }

          reader.Read("offset", collider.offset);

          if (glm::vec3 rotation{0.0f}; reader.Read("rotation", rotation))
          {
            collider.rotation = {rotation.x, rotation.y, rotation.z};
          }
        },
        [](const Collider &collider, DataValue &map)
        {
          const Collider standard{};

          // the shape is what a collider is, so it is written even when it
          // is the default
          map.Set("shape", DataValue::Text(shape_kinds[static_cast<std::size_t>(collider.shape)]));

          // what does not belong to the shape is left out, since it would
          // be refused when the file is read
          if (collider.shape == ShapeKind::Box) { Put(map, "size", collider.size, standard.size); }
          if (HasRadius(collider.shape)) { Put(map, "radius", collider.radius, standard.radius); }
          if (HasHeight(collider.shape)) { Put(map, "height", collider.height, standard.height); }

          if (IsTapered(collider.shape))
          {
            Put(map, "top_radius", collider.top_radius, standard.top_radius);
            Put(map, "bottom_radius", collider.bottom_radius, standard.bottom_radius);
          }

          if (HasModel(collider.shape)) { map.Set("model", DataValue::Text(collider.model)); }

          Put(map, "offset", collider.offset, standard.offset);
          Put(
            map,
            "rotation",
            glm::vec3{collider.rotation.pitch, collider.rotation.yaw, collider.rotation.roll},
            glm::vec3{0.0f});
        });
    }

    ComponentFormat CharacterBodyFormat()
    {
      return Format<CharacterBody>(
        "CharacterBody",
        [](const DataReader &reader, CharacterBody &character)
        {
          reader.Read("velocity", character.velocity);
          reader.Read("fall_velocity", character.fall_velocity);
          reader.Read("gravity_scale", character.gravity_scale);

          float max_slope = character.max_slope;
          if (reader.Read("max_slope", max_slope))
          {
            if (max_slope < 0.0f || max_slope > 90.0f || max_slope != max_slope)
            {
              reader.Report(*reader.ReadValue("max_slope"), std::format(
                              "'max_slope' of {} is {}, where degrees from 0 to 90 were expected",
                              reader.GetWhere(), max_slope));
            } else
            {
              character.max_slope = max_slope;
            }
          }

          ReadFromZero(reader, "step_height", character.step_height);
          ReadAboveZero(reader, "mass", character.mass);
          ReadFromZero(reader, "push_strength", character.push_strength);
          ReadLayers(reader, "layers", character.layers);
          ReadLayers(reader, "mask", character.mask);
        },
        [](const CharacterBody &character, DataValue &map)
        {
          const CharacterBody standard{};
          Put(map, "velocity", character.velocity, standard.velocity);
          Put(map, "fall_velocity", character.fall_velocity, standard.fall_velocity);
          Put(map, "gravity_scale", character.gravity_scale, standard.gravity_scale);
          Put(map, "max_slope", character.max_slope, standard.max_slope);
          Put(map, "step_height", character.step_height, standard.step_height);
          Put(map, "mass", character.mass, standard.mass);
          Put(map, "push_strength", character.push_strength, standard.push_strength);
          PutLayers(map, "layers", character.layers, standard.layers);
          PutLayers(map, "mask", character.mask, standard.mask);
        });
    }
  }

  void ComponentFormats::AddPhysicsComponents()
  {
    Add(RigidBodyFormat());
    Add(TriggerFormat());
    Add(CharacterBodyFormat());
    Add(ColliderFormat());
  }
} // neon
