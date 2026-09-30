#include "component-format.hpp"

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/light.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>
#include <neon/world-system/ecs/components/sound-listener.hpp>
#include <neon/world-system/ecs/components/sound-source.hpp>
#include <neon/world-system/ecs/components/spectator.hpp>

// Every component is written with the values that differ from its defaults,
// so that a scene file holds what was decided and nothing else.

namespace neon
{
  namespace
  {
    const std::vector<std::string> render_targets = {"window", "texture"};
    const std::vector<std::string> light_types = {"direction", "point", "spot"};

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

    void Put(DataValue &map, const std::string &name, const std::string &value, const std::string &standard)
    {
      if (value != standard) { map.Set(name, DataValue::Text(value)); }
    }

    void Put(DataValue &map, const std::string &name, const glm::vec3 &value, const glm::vec3 &standard)
    {
      if (value != standard) { map.Set(name, List(value)); }
    }

    void Put(DataValue &map, const std::string &name, const Color &value, const Color &standard)
    {
      if (value.r == standard.r && value.g == standard.g && value.b == standard.b && value.a == standard.a)
      {
        return;
      }

      auto list = List({value.r, value.g, value.b});
      if (value.a != 1.0f) { list.Add(DataValue::Number(value.a)); }
      map.Set(name, list);
    }

    ComponentFormat TransformFormat()
    {
      return ComponentFormat::Of<Transform>(
        "Transform",
        [](const DataReader &reader, Transform &transform)
        {
          reader.Read("position", transform.position);

          if (glm::vec3 rotation{0.0f}; reader.Read("rotation", rotation))
          {
            transform.rotation = {rotation.x, rotation.y, rotation.z};
          }

          reader.ReadScale("scale", transform.scale);
        },
        [](const Transform &transform, DataValue &map)
        {
          const Transform standard{};
          Put(map, "position", transform.position, standard.position);
          Put(
            map,
            "rotation",
            glm::vec3{transform.rotation.pitch, transform.rotation.yaw, transform.rotation.roll},
            glm::vec3{0.0f});
          Put(map, "scale", transform.scale, standard.scale);
        });
    }

    ComponentFormat RenderableFormat()
    {
      return ComponentFormat::Of<Renderable>(
        "Renderable",
        [](const DataReader &reader, Renderable &renderable)
        {
          auto &info = renderable.render_info;
          reader.Read("model", info.model_path);
          reader.Read("shader", info.shader_path);
          reader.Read("textures", info.texture_paths);
          reader.Read("scale_textures", info.scale_textures);

          bool found = false;
          const auto material = reader.ReadMap("material", found);
          material.Read("shininess", info.material_info.shininess);
          material.Read("color", info.material_info.color);
          material.Read("use_textures", info.material_info.use_textures);
          material.Finish();

          if (info.model_path.empty() || info.shader_path.empty())
          {
            reader.Report({}, reader.GetWhere() + " needs a 'model' and a 'shader'");
          }
        },
        [](const Renderable &renderable, DataValue &map)
        {
          const RenderInfo standard{};
          const auto &info = renderable.render_info;

          Put(map, "model", info.model_path, standard.model_path);
          Put(map, "shader", info.shader_path, standard.shader_path);

          if (!info.texture_paths.empty())
          {
            auto textures = DataValue::List();
            for (const auto &path : info.texture_paths) { textures.Add(DataValue::Text(path)); }
            map.Set("textures", textures);
          }

          Put(map, "scale_textures", info.scale_textures, false);

          auto material = DataValue::Map();
          Put(material, "shininess", info.material_info.shininess, standard.material_info.shininess);
          Put(material, "color", info.material_info.color, standard.material_info.color);
          Put(material, "use_textures", info.material_info.use_textures, standard.material_info.use_textures);
          if (!material.GetEntries().empty()) { map.Set("material", material); }
        });
    }

    ComponentFormat CameraFormat()
    {
      return ComponentFormat::Of<Camera>(
        "Camera",
        [](const DataReader &reader, Camera &camera)
        {
          if (std::size_t target = 0; reader.ReadChoice("target", render_targets, target))
          {
            camera.target = static_cast<RenderTarget>(target);
          }

          reader.Read("fov", camera.fov);
          reader.Read("near", camera.near_plane);
          reader.Read("far", camera.far_plane);
          reader.Read("up", camera.up);
        },
        [](const Camera &camera, DataValue &map)
        {
          const Camera standard{};

          if (camera.target != standard.target)
          {
            map.Set("target", DataValue::Text(render_targets[static_cast<std::size_t>(camera.target)]));
          }

          Put(map, "fov", camera.fov, standard.fov);
          Put(map, "near", camera.near_plane, standard.near_plane);
          Put(map, "far", camera.far_plane, standard.far_plane);
          Put(map, "up", camera.up, standard.up);
        });
    }

    ComponentFormat LightFormat()
    {
      return ComponentFormat::Of<Light>(
        "Light",
        [](const DataReader &reader, Light &light)
        {
          auto &source = light.source;

          if (std::size_t type = 0; reader.ReadChoice("type", light_types, type))
          {
            source.light_type = static_cast<LightType>(type);
          }

          reader.Read("direction", source.direction);
          reader.Read("ambient", source.ambient);
          reader.Read("diffuse", source.diffuse);
          reader.Read("specular", source.specular);
          reader.Read("constant", source.constant);
          reader.Read("linear", source.linear);
          reader.Read("quadratic", source.quadratic);
          reader.Read("cutoff", source.cutoff);
          reader.Read("outer_cutoff", source.outer_cutoff);
        },
        [](const Light &light, DataValue &map)
        {
          const auto &source = light.source;

          // the type is what a light is, so it is written even when it is
          // the default
          map.Set("type", DataValue::Text(light_types[static_cast<std::size_t>(source.light_type)]));

          Put(map, "direction", source.direction, glm::vec3{0.0f});
          Put(map, "ambient", source.ambient, glm::vec3{0.0f});
          Put(map, "diffuse", source.diffuse, glm::vec3{0.0f});
          Put(map, "specular", source.specular, glm::vec3{0.0f});
          Put(map, "constant", source.constant, 0.0f);
          Put(map, "linear", source.linear, 0.0f);
          Put(map, "quadratic", source.quadratic, 0.0f);
          Put(map, "cutoff", source.cutoff, 0.0f);
          Put(map, "outer_cutoff", source.outer_cutoff, 0.0f);
        });
    }

    ComponentFormat SpectatorFormat()
    {
      return ComponentFormat::Of<Spectator>(
        "Spectator",
        [](const DataReader &reader, Spectator &spectator)
        {
          reader.Read("move_speed", spectator.move_speed);
          reader.Read("look_speed", spectator.look_speed);
        },
        [](const Spectator &spectator, DataValue &map)
        {
          const Spectator standard{};
          Put(map, "move_speed", spectator.move_speed, standard.move_speed);
          Put(map, "look_speed", spectator.look_speed, standard.look_speed);
        });
    }

    ComponentFormat SoundSourceFormat()
    {
      return ComponentFormat::Of<SoundSource>(
        "SoundSource",
        [](const DataReader &reader, SoundSource &source)
        {
          auto &sound = source.sound;
          reader.Read("sound", sound.path);
          reader.Read("playing", source.playing);
          reader.Read("looping", sound.looping);
          reader.Read("volume", sound.volume);
          reader.Read("pitch", sound.pitch);
          reader.Read("spatial", sound.spatial);
          reader.Read("min_distance", sound.min_distance);
          reader.Read("max_distance", sound.max_distance);

          if (sound.path.empty())
          {
            reader.Report({}, reader.GetWhere() + " needs a 'sound'");
          }

          if (sound.pitch <= 0.0f)
          {
            reader.Report({}, "'pitch' of " + reader.GetWhere() + " has to be above 0");
          }
        },
        [](const SoundSource &source, DataValue &map)
        {
          const SoundSource standard{};
          const auto &sound = source.sound;

          Put(map, "sound", sound.path, standard.sound.path);
          Put(map, "playing", source.playing, standard.playing);
          Put(map, "looping", sound.looping, standard.sound.looping);
          Put(map, "volume", sound.volume, standard.sound.volume);
          Put(map, "pitch", sound.pitch, standard.sound.pitch);
          Put(map, "spatial", sound.spatial, standard.sound.spatial);
          Put(map, "min_distance", sound.min_distance, standard.sound.min_distance);
          Put(map, "max_distance", sound.max_distance, standard.sound.max_distance);
        });
    }

    ComponentFormat SoundListenerFormat()
    {
      return ComponentFormat::Of<SoundListener>(
        "SoundListener",
        [](const DataReader &reader, SoundListener &listener)
        {
          reader.Read("volume", listener.volume);
        },
        [](const SoundListener &listener, DataValue &map)
        {
          Put(map, "volume", listener.volume, SoundListener{}.volume);
        });
    }
  }

  void ComponentFormats::AddEngineComponents()
  {
    Add(TransformFormat());
    Add(RenderableFormat());
    Add(CameraFormat());
    Add(LightFormat());
    Add(SpectatorFormat());
    Add(SoundSourceFormat());
    Add(SoundListenerFormat());
    AddPhysicsComponents();
  }
} // neon
