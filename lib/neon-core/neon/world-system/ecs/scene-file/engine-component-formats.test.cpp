#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/light.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>
#include <neon/world-system/ecs/components/sound-listener.hpp>
#include <neon/world-system/ecs/components/sound-source.hpp>
#include <neon/world-system/ecs/components/spectator.hpp>

#include "component-format.hpp"

namespace
{
  using neon::AlphaMode;
  using neon::Camera;
  using neon::ComponentFormats;
  using neon::DataReader;
  using neon::DataValue;
  using neon::Entity;
  using neon::Light;
  using neon::LightType;
  using neon::Renderable;
  using neon::RenderTarget;
  using neon::SoundListener;
  using neon::SoundSource;
  using neon::Spectator;
  using neon::Transform;
  using neon::testing::FakeEntityStore;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;
  using ::testing::SizeIs;

  DataValue Numbers(const std::vector<float> &numbers)
  {
    auto list = DataValue::List();
    for (const float number : numbers) { list.Add(DataValue::Number(number)); }
    return list;
  }

  DataValue Texts(const std::vector<std::string> &texts)
  {
    auto list = DataValue::List();
    for (const auto &text : texts) { list.Add(DataValue::Text(text)); }
    return list;
  }

  /// The names of a map, in the order they were written.
  std::vector<std::string> NamesOf(const DataValue &map)
  {
    std::vector<std::string> names;
    for (const auto &[name, value] : map.GetEntries()) { names.push_back(name); }
    return names;
  }

  std::vector<float> NumbersOf(const DataValue &list)
  {
    std::vector<float> numbers;
    for (const auto &item : list.GetItems())
    {
      float number = 0.0f;
      EXPECT_TRUE(item.GetNumber(number));
      numbers.push_back(number);
    }
    return numbers;
  }

  std::string TextOf(const DataValue &map, const std::string &name)
  {
    std::string text;
    const auto *value = map.Find(name);
    EXPECT_NE(value, nullptr) << name;
    if (value != nullptr) { EXPECT_TRUE(value->GetText(text)) << name; }
    return text;
  }

  class EngineComponentFormatsTest : public ::testing::Test
  {
  protected:
    FakeEntityStore _store;
    ComponentFormats _formats;
    std::vector<std::string> _errors;
    Entity _entity = neon::No_Entity;

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Transform>("Transform");
      _store.Register<Renderable>("Renderable");
      _store.Register<Camera>("Camera");
      _store.Register<Light>("Light");
      _store.Register<Spectator>("Spectator");
      _store.Register<SoundSource>("SoundSource");
      _store.Register<SoundListener>("SoundListener");

      _formats.AddEngineComponents();
      _entity = _store.CreateEntity("thing");
    }

    void TearDown() override
    {
      _store.CleanUp();
    }

    /// Reads a component the way a scene file does.
    void Read(const std::string &name, const DataValue &value)
    {
      const auto *format = _formats.Find(name);
      ASSERT_NE(format, nullptr) << name;

      const DataReader reader(value, "scene.yml", name + " of entity 'thing'", _errors);
      format->read(reader, _store, _entity);
      reader.Finish();
    }

    DataValue Write(const std::string &name)
    {
      DataValue value;
      const auto *format = _formats.Find(name);
      EXPECT_NE(format, nullptr) << name;
      if (format != nullptr) { EXPECT_TRUE(format->write(_store, _entity, value)) << name; }
      return value;
    }
  };

  // what is known

  TEST_F(EngineComponentFormatsTest, KnowsTheComponentsOfTheEngine)
  {
    for (const auto *name : {"Transform", "Renderable", "Camera", "Light", "Spectator", "SoundSource", "SoundListener"})
    {
      EXPECT_NE(_formats.Find(name), nullptr) << name;
    }
  }

  TEST_F(EngineComponentFormatsTest, DoesNotKnowWhatWasNotAdded)
  {
    EXPECT_EQ(_formats.Find("Health"), nullptr);
  }

  TEST_F(EngineComponentFormatsTest, WritesNothingForAComponentTheEntityDoesNotCarry)
  {
    DataValue value;

    EXPECT_FALSE(_formats.Find("Camera")->write(_store, _entity, value));
  }

  // Transform

  TEST_F(EngineComponentFormatsTest, ReadsATransform)
  {
    auto map = DataValue::Map();
    map.Set("position", Numbers({1.0f, 2.0f, 3.0f}));
    map.Set("rotation", Numbers({10.0f, 20.0f, 30.0f}));
    map.Set("scale", Numbers({4.0f, 5.0f, 6.0f}));

    Read("Transform", map);

    const auto *transform = _store.Get<Transform>(_entity);
    ASSERT_NE(transform, nullptr);
    EXPECT_EQ(transform->position, glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(transform->rotation.pitch, 10.0f);
    EXPECT_EQ(transform->rotation.yaw, 20.0f);
    EXPECT_EQ(transform->rotation.roll, 30.0f);
    EXPECT_EQ(transform->scale, glm::vec3(4.0f, 5.0f, 6.0f));
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EngineComponentFormatsTest, ATransformThatIsEmptyHasItsDefaults)
  {
    Read("Transform", DataValue::Map());

    const auto *transform = _store.Get<Transform>(_entity);
    ASSERT_NE(transform, nullptr);
    EXPECT_EQ(transform->position, glm::vec3(0.0f));
    EXPECT_EQ(transform->scale, glm::vec3(1.0f));
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EngineComponentFormatsTest, OneNumberForTheScaleStandsForAllThree)
  {
    auto map = DataValue::Map();
    map.Set("scale", DataValue::Number(0.2f));

    Read("Transform", map);

    EXPECT_EQ(_store.Get<Transform>(_entity)->scale, glm::vec3(0.2f));
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EngineComponentFormatsTest, OneNumberForThePositionIsReported)
  {
    auto map = DataValue::Map();
    map.Set("position", DataValue::Number(1.0f));

    Read("Transform", map);

    EXPECT_THAT(_errors, ElementsAre(
                  "scene.yml: 'position' of Transform of entity 'thing' is a number, "
                  "where a list of 3 numbers was expected"));
  }

  TEST_F(EngineComponentFormatsTest, TwoNumbersForThePositionAreReported)
  {
    auto map = DataValue::Map();
    map.Set("position", Numbers({1.0f, 2.0f}));

    Read("Transform", map);

    EXPECT_THAT(_errors, ElementsAre(
                  "scene.yml: 'position' of Transform of entity 'thing' holds 2 values, "
                  "where a list of 3 numbers was expected"));
  }

  TEST_F(EngineComponentFormatsTest, ANameATransformDoesNotKnowIsReportedWithThoseItKnows)
  {
    auto map = DataValue::Map();
    map.Set("postion", Numbers({1.0f, 2.0f, 3.0f}));

    Read("Transform", map);

    EXPECT_THAT(_errors, ElementsAre(
                  "scene.yml: 'postion' is not known to Transform of entity 'thing'. "
                  "Known are: position, rotation, scale"));
  }

  TEST_F(EngineComponentFormatsTest, WritesNothingOfATransformThatHasItsDefaults)
  {
    _store.Set(_entity, Transform{});

    EXPECT_THAT(NamesOf(Write("Transform")), IsEmpty());
    EXPECT_TRUE(Write("Transform").IsMap());
  }

  TEST_F(EngineComponentFormatsTest, WritesWhatOfATransformDiffersFromItsDefaults)
  {
    Transform transform;
    transform.position = {1.0f, 2.0f, 3.0f};
    transform.rotation.yaw = 90.0f;
    _store.Set(_entity, transform);

    const auto written = Write("Transform");

    EXPECT_THAT(NamesOf(written), ElementsAre("position", "rotation"));
    EXPECT_THAT(NumbersOf(*written.Find("position")), ElementsAre(1.0f, 2.0f, 3.0f));
    EXPECT_THAT(NumbersOf(*written.Find("rotation")), ElementsAre(0.0f, 90.0f, 0.0f));
  }

  TEST_F(EngineComponentFormatsTest, DoesNotWriteWhereATransformIsInTheWorld)
  {
    Transform transform;
    transform.world_coordinates[3] = {7.0f, 8.0f, 9.0f, 1.0f};
    _store.Set(_entity, transform);

    EXPECT_THAT(NamesOf(Write("Transform")), IsEmpty());
  }

  // Renderable

  TEST_F(EngineComponentFormatsTest, ReadsARenderable)
  {
    auto material = DataValue::Map();
    material.Set("shininess", DataValue::Number(32.0f));
    material.Set("color", Numbers({1.0f, 0.5f, 0.25f}));
    material.Set("use_textures", DataValue::Bool(false));

    auto map = DataValue::Map();
    map.Set("model", DataValue::Text("assets://models/cube.obj"));
    map.Set("shader", DataValue::Text("assets://shaders/basic-lit"));
    map.Set("textures", Texts({"assets://a.png", "assets://b.png"}));
    map.Set("scale_textures", DataValue::Bool(true));
    map.Set("material", material);

    Read("Renderable", map);

    const auto *renderable = _store.Get<Renderable>(_entity);
    ASSERT_NE(renderable, nullptr);
    const auto &info = renderable->render_info;
    EXPECT_EQ(info.model_path, "assets://models/cube.obj");
    EXPECT_EQ(info.shader_path, "assets://shaders/basic-lit");
    EXPECT_THAT(info.texture_paths, ElementsAre("assets://a.png", "assets://b.png"));
    EXPECT_TRUE(info.scale_textures);
    EXPECT_EQ(info.material_info.shininess, 32.0f);
    EXPECT_EQ(info.material_info.color.g, 0.5f);
    EXPECT_EQ(info.material_info.color.a, 1.0f);
    EXPECT_FALSE(info.material_info.use_textures);
    EXPECT_EQ(renderable->render_object_id, -1);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EngineComponentFormatsTest, AColorOfFourNumbersHasAnAlpha)
  {
    auto material = DataValue::Map();
    material.Set("color", Numbers({1.0f, 0.5f, 0.25f, 0.5f}));

    auto map = DataValue::Map();
    map.Set("model", DataValue::Text("m"));
    map.Set("shader", DataValue::Text("s"));
    map.Set("material", material);

    Read("Renderable", map);

    EXPECT_EQ(_store.Get<Renderable>(_entity)->render_info.material_info.color.a, 0.5f);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EngineComponentFormatsTest, AMaterialIsOpaqueUnlessItSaysItBlends)
  {
    auto map = DataValue::Map();
    map.Set("model", DataValue::Text("m"));
    map.Set("shader", DataValue::Text("s"));

    Read("Renderable", map);
    EXPECT_EQ(_store.Get<Renderable>(_entity)->render_info.material_info.alpha_mode, AlphaMode::Opaque);

    auto material = DataValue::Map();
    material.Set("alpha_mode", DataValue::Text("blend"));
    map.Set("material", material);

    Read("Renderable", map);
    EXPECT_EQ(_store.Get<Renderable>(_entity)->render_info.material_info.alpha_mode, AlphaMode::Blend);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EngineComponentFormatsTest, ARenderableWithoutAModelIsReported)
  {
    auto map = DataValue::Map();
    map.Set("shader", DataValue::Text("assets://shaders/basic-lit"));

    Read("Renderable", map);

    EXPECT_THAT(_errors, ElementsAre("scene.yml: Renderable of entity 'thing' needs a 'model'"));
  }

  TEST_F(EngineComponentFormatsTest, ARenderableWithoutAModelAndAShaderIsReportedForEach)
  {
    Read("Renderable", DataValue::Map());

    EXPECT_THAT(_errors, ElementsAre(
                  "scene.yml: Renderable of entity 'thing' needs a 'model'",
                  "scene.yml: Renderable of entity 'thing' needs a 'shader'"));
  }

  TEST_F(EngineComponentFormatsTest, ANameAMaterialDoesNotKnowIsReported)
  {
    auto material = DataValue::Map();
    material.Set("shine", DataValue::Number(3.0f));

    auto map = DataValue::Map();
    map.Set("model", DataValue::Text("m"));
    map.Set("shader", DataValue::Text("s"));
    map.Set("material", material);

    Read("Renderable", map);

    EXPECT_THAT(_errors, ElementsAre(
                  "scene.yml: 'shine' is not known to 'material' of Renderable of entity 'thing'. "
                  "Known are: shininess, color, use_textures, alpha_mode"));
  }

  TEST_F(EngineComponentFormatsTest, ATextureThatIsNotTextIsReported)
  {
    auto textures = DataValue::List();
    textures.Add(DataValue::Number(1.0f));

    auto map = DataValue::Map();
    map.Set("model", DataValue::Text("m"));
    map.Set("shader", DataValue::Text("s"));
    map.Set("textures", textures);

    Read("Renderable", map);

    EXPECT_THAT(_errors, ElementsAre(
                  "scene.yml: 'textures' of Renderable of entity 'thing' holds a number, "
                  "where text was expected"));
  }

  TEST_F(EngineComponentFormatsTest, WritesARenderableWithoutWhatTheRendererKnowsItAs)
  {
    Renderable renderable;
    renderable.render_info.model_path = "assets://models/cube.obj";
    renderable.render_info.shader_path = "assets://shaders/basic-lit";
    renderable.render_info.texture_paths = {"assets://a.png"};
    renderable.render_info.material_info.shininess = 32.0f;
    renderable.render_object_id = 7;
    _store.Set(_entity, renderable);

    const auto written = Write("Renderable");

    EXPECT_THAT(NamesOf(written), ElementsAre("model", "shader", "textures", "material"));
    EXPECT_EQ(TextOf(written, "model"), "assets://models/cube.obj");
    EXPECT_THAT(written.Find("textures")->GetItems(), SizeIs(1));
    EXPECT_THAT(NamesOf(*written.Find("material")), ElementsAre("shininess"));
  }

  TEST_F(EngineComponentFormatsTest, WritesNoMaterialThatHasItsDefaults)
  {
    Renderable renderable;
    renderable.render_info.model_path = "m";
    renderable.render_info.shader_path = "s";
    _store.Set(_entity, renderable);

    EXPECT_THAT(NamesOf(Write("Renderable")), ElementsAre("model", "shader"));
  }

  TEST_F(EngineComponentFormatsTest, WritesTheAlphaOfAColorThatHasOne)
  {
    Renderable renderable;
    renderable.render_info.model_path = "m";
    renderable.render_info.shader_path = "s";
    renderable.render_info.material_info.color = {0.5f, 0.5f, 0.5f, 0.25f};
    _store.Set(_entity, renderable);

    const auto written = Write("Renderable");

    EXPECT_THAT(
      NumbersOf(*written.Find("material")->Find("color")),
      ElementsAre(0.5f, 0.5f, 0.5f, 0.25f));
  }

  // Camera

  TEST_F(EngineComponentFormatsTest, ReadsACamera)
  {
    auto map = DataValue::Map();
    map.Set("target", DataValue::Text("texture"));
    map.Set("fov", DataValue::Number(60.0f));
    map.Set("near", DataValue::Number(0.5f));
    map.Set("far", DataValue::Number(500.0f));
    map.Set("up", Numbers({0.0f, 0.0f, 1.0f}));

    Read("Camera", map);

    const auto *camera = _store.Get<Camera>(_entity);
    ASSERT_NE(camera, nullptr);
    EXPECT_EQ(camera->target, RenderTarget::Texture);
    EXPECT_EQ(camera->fov, 60.0f);
    EXPECT_EQ(camera->near_plane, 0.5f);
    EXPECT_EQ(camera->far_plane, 500.0f);
    EXPECT_EQ(camera->up, glm::vec3(0.0f, 0.0f, 1.0f));
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EngineComponentFormatsTest, ATargetACameraDoesNotKnowIsReportedWithThoseItKnows)
  {
    auto map = DataValue::Map();
    map.Set("target", DataValue::Text("screen"));

    Read("Camera", map);

    EXPECT_THAT(_errors, ElementsAre(
                  "scene.yml: 'target' of Camera of entity 'thing' is 'screen', "
                  "where one of these was expected: window, texture"));
  }

  TEST_F(EngineComponentFormatsTest, AFieldOfViewThatIsTextIsReported)
  {
    auto map = DataValue::Map();
    map.Set("fov", DataValue::Text("wide"));

    Read("Camera", map);

    EXPECT_THAT(_errors, ElementsAre(
                  "scene.yml: 'fov' of Camera of entity 'thing' is text, where a number was expected"));
  }

  TEST_F(EngineComponentFormatsTest, WritesWhatOfACameraDiffersFromItsDefaults)
  {
    Camera camera;
    camera.target = RenderTarget::Texture;
    camera.far_plane = 50.0f;
    _store.Set(_entity, camera);

    const auto written = Write("Camera");

    EXPECT_THAT(NamesOf(written), ElementsAre("target", "far"));
    EXPECT_EQ(TextOf(written, "target"), "texture");
  }

  // Light

  TEST_F(EngineComponentFormatsTest, ReadsALight)
  {
    auto map = DataValue::Map();
    map.Set("type", DataValue::Text("spot"));
    map.Set("direction", Numbers({0.0f, -1.0f, 0.0f}));
    map.Set("ambient", Numbers({0.1f, 0.1f, 0.1f}));
    map.Set("diffuse", Numbers({0.2f, 0.2f, 0.2f}));
    map.Set("specular", Numbers({0.3f, 0.3f, 0.3f}));
    map.Set("constant", DataValue::Number(1.0f));
    map.Set("linear", DataValue::Number(0.09f));
    map.Set("quadratic", DataValue::Number(0.032f));
    map.Set("cutoff", DataValue::Number(12.5f));
    map.Set("outer_cutoff", DataValue::Number(17.5f));

    Read("Light", map);

    const auto *light = _store.Get<Light>(_entity);
    ASSERT_NE(light, nullptr);
    EXPECT_EQ(light->source.light_type, LightType::SpotLight);
    EXPECT_EQ(light->source.direction, glm::vec3(0.0f, -1.0f, 0.0f));
    EXPECT_EQ(light->source.specular, glm::vec3(0.3f));
    EXPECT_EQ(light->source.linear, 0.09f);
    EXPECT_EQ(light->source.outer_cutoff, 17.5f);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EngineComponentFormatsTest, AnIdIsNotKnownToALight)
  {
    auto map = DataValue::Map();
    map.Set("id", DataValue::Text("lamp"));

    Read("Light", map);

    EXPECT_THAT(_errors, ElementsAre(
                  "scene.yml: 'id' is not known to Light of entity 'thing'. Known are: "
                  "type, direction, ambient, diffuse, specular, constant, linear, quadratic, cutoff, outer_cutoff"));
  }

  TEST_F(EngineComponentFormatsTest, WritesTheTypeOfALightEvenWhenItIsTheDefault)
  {
    Light light{};
    light.source.diffuse = {0.4f, 0.4f, 0.4f};
    _store.Set(_entity, light);

    const auto written = Write("Light");

    EXPECT_THAT(NamesOf(written), ElementsAre("type", "diffuse"));
    EXPECT_EQ(TextOf(written, "type"), "direction");
  }

  // Spectator

  TEST_F(EngineComponentFormatsTest, ReadsASpectator)
  {
    auto map = DataValue::Map();
    map.Set("move_speed", DataValue::Number(5.0f));
    map.Set("look_speed", DataValue::Number(0.2f));

    Read("Spectator", map);

    EXPECT_EQ(_store.Get<Spectator>(_entity)->move_speed, 5.0f);
    EXPECT_EQ(_store.Get<Spectator>(_entity)->look_speed, 0.2f);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EngineComponentFormatsTest, WritesNothingOfASpectatorThatHasItsDefaults)
  {
    _store.Set(_entity, Spectator{});

    EXPECT_THAT(NamesOf(Write("Spectator")), IsEmpty());
  }

  // SoundSource

  TEST_F(EngineComponentFormatsTest, ReadsASoundSource)
  {
    auto map = DataValue::Map();
    map.Set("sound", DataValue::Text("assets://sounds/hum.wav"));
    map.Set("playing", DataValue::Bool(false));
    map.Set("looping", DataValue::Bool(true));
    map.Set("volume", DataValue::Number(0.6f));
    map.Set("pitch", DataValue::Number(1.5f));
    map.Set("spatial", DataValue::Bool(true));
    map.Set("min_distance", DataValue::Number(2.0f));
    map.Set("max_distance", DataValue::Number(30.0f));

    Read("SoundSource", map);

    const auto *source = _store.Get<SoundSource>(_entity);
    ASSERT_NE(source, nullptr);
    EXPECT_EQ(source->sound.path, "assets://sounds/hum.wav");
    EXPECT_FALSE(source->playing);
    EXPECT_TRUE(source->sound.looping);
    EXPECT_EQ(source->sound.volume, 0.6f);
    EXPECT_EQ(source->sound.pitch, 1.5f);
    EXPECT_TRUE(source->sound.spatial);
    EXPECT_EQ(source->sound.min_distance, 2.0f);
    EXPECT_EQ(source->sound.max_distance, 30.0f);
    EXPECT_EQ(source->sound_id, -1);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EngineComponentFormatsTest, ASoundSourceWithoutASoundIsReported)
  {
    Read("SoundSource", DataValue::Map());

    EXPECT_THAT(_errors, ElementsAre("scene.yml: SoundSource of entity 'thing' needs a 'sound'"));
  }

  TEST_F(EngineComponentFormatsTest, APitchThatIsNotAboveZeroIsReported)
  {
    auto map = DataValue::Map();
    map.Set("sound", DataValue::Text("s"));
    map.Set("pitch", DataValue::Number(0.0f));

    Read("SoundSource", map);

    EXPECT_THAT(_errors, ElementsAre("scene.yml: 'pitch' of SoundSource of entity 'thing' has to be above 0"));
  }

  TEST_F(EngineComponentFormatsTest, WritesASoundSourceWithoutWhatTheEngineKeepsForItself)
  {
    SoundSource source;
    source.sound.path = "assets://sounds/hum.wav";
    source.sound.looping = true;
    source.sound_id = 4;
    source.was_playing = true;
    _store.Set(_entity, source);

    EXPECT_THAT(NamesOf(Write("SoundSource")), ElementsAre("sound", "looping"));
  }

  // SoundListener

  TEST_F(EngineComponentFormatsTest, ReadsASoundListener)
  {
    auto map = DataValue::Map();
    map.Set("volume", DataValue::Number(0.5f));

    Read("SoundListener", map);

    EXPECT_EQ(_store.Get<SoundListener>(_entity)->volume, 0.5f);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EngineComponentFormatsTest, WritesNothingOfASoundListenerThatHasItsDefaults)
  {
    _store.Set(_entity, SoundListener{});

    EXPECT_THAT(NamesOf(Write("SoundListener")), IsEmpty());
  }

  // there and back

  TEST_F(EngineComponentFormatsTest, WhatIsWrittenReadsBackAsTheSame)
  {
    Camera camera;
    camera.fov = 70.0f;
    camera.up = {0.0f, 0.0f, 1.0f};
    _store.Set(_entity, camera);
    const auto written = Write("Camera");

    _store.Remove<Camera>(_entity);
    Read("Camera", written);

    EXPECT_EQ(_store.Get<Camera>(_entity)->fov, 70.0f);
    EXPECT_EQ(_store.Get<Camera>(_entity)->up, glm::vec3(0.0f, 0.0f, 1.0f));
    EXPECT_EQ(_store.Get<Camera>(_entity)->far_plane, Camera{}.far_plane);
    EXPECT_THAT(_errors, IsEmpty());
  }
}
