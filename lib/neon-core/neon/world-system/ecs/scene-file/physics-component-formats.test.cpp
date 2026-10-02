#include "component-format.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/fake-entity-store.hpp>
#include <neon/world-system/ecs/components/character-body.hpp>
#include <neon/world-system/ecs/components/collider.hpp>
#include <neon/world-system/ecs/components/joint.hpp>
#include <neon/world-system/ecs/components/rigid-body.hpp>
#include <neon/world-system/ecs/components/trigger.hpp>

namespace
{
  using neon::BodyKind;
  using neon::CharacterBody;
  using neon::Collider;
  using neon::ComponentFormats;
  using neon::DataReader;
  using neon::DataValue;
  using neon::Entity;
  using neon::Joint;
  using neon::JointKind;
  using neon::RigidBody;
  using neon::ShapeKind;
  using neon::Trigger;
  using neon::testing::FakeEntityStore;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  /// A value as it is after a file was read, which knows its line.
  DataValue At(const std::size_t line, DataValue value)
  {
    value.SetLine(line);
    return value;
  }

  DataValue Numbers(const std::vector<double> &numbers)
  {
    auto list = DataValue::List();
    for (const double number : numbers) { list.Add(DataValue::Number(number)); }
    return list;
  }

  DataValue Texts(const std::vector<std::string> &texts)
  {
    auto list = DataValue::List();
    for (const auto &text : texts) { list.Add(DataValue::Text(text)); }
    return list;
  }

  std::vector<std::string> TextsOf(const DataValue &map, const std::string &name)
  {
    std::vector<std::string> texts;
    const auto *value = map.Find(name);
    if (value == nullptr) { return texts; }

    for (const auto &item : value->GetItems())
    {
      std::string text;
      (void) item.GetText(text);
      texts.push_back(text);
    }
    return texts;
  }

  /// The names of a map, in the order they were written in.
  std::vector<std::string> NamesOf(const DataValue &map)
  {
    std::vector<std::string> names;
    for (const auto &[name, value] : map.GetEntries()) { names.push_back(name); }
    return names;
  }

  double NumberOf(const DataValue &map, const std::string &name)
  {
    double number = -12345.0;
    const auto *value = map.Find(name);
    if (value != nullptr) { (void) value->GetNumber(number); }
    return number;
  }

  std::string TextOf(const DataValue &map, const std::string &name)
  {
    std::string text = "(none)";
    const auto *value = map.Find(name);
    if (value != nullptr) { (void) value->GetText(text); }
    return text;
  }

  std::vector<double> NumbersOf(const DataValue &map, const std::string &name)
  {
    std::vector<double> numbers;
    const auto *value = map.Find(name);
    if (value == nullptr) { return numbers; }

    for (const auto &item : value->GetItems())
    {
      double number = -12345.0;
      (void) item.GetNumber(number);
      numbers.push_back(number);
    }
    return numbers;
  }

  class PhysicsComponentFormatsTest : public ::testing::Test
  {
  protected:
    FakeEntityStore _store;
    ComponentFormats _formats;
    std::vector<std::string> _errors;
    Entity _entity = neon::No_Entity;

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<RigidBody>("RigidBody");
      _store.Register<Trigger>("Trigger");
      _store.Register<CharacterBody>("CharacterBody");
      _store.Register<Collider>("Collider");
      _store.Register<Joint>("Joint");

      _formats.AddPhysicsComponents();
      _entity = _store.CreateEntity("crate");
    }

    /// Reads a component the way a scene file does.
    void Read(const std::string &name, const DataValue &map)
    {
      const auto *format = _formats.Find(name);
      ASSERT_NE(format, nullptr);

      const DataReader reader(map, "test.scene.yml", name + " of entity 'crate'", _errors);
      format->read(reader, _store, _entity);
      reader.Finish();
    }

    /// Writes the component of the entity the way a scene file does.
    DataValue Write(const std::string &name)
    {
      DataValue value;
      const auto *format = _formats.Find(name);
      if (format == nullptr) { return value; }

      EXPECT_TRUE(format->write(_store, _entity, value));
      return value;
    }

    /// The only problem that was found with a value.
    std::string ProblemOf(const std::string &component, const std::string &name, const DataValue &value)
    {
      _errors.clear();

      auto map = DataValue::Map();
      map.Set(name, At(7, value));
      Read(component, map);

      if (_errors.size() != 1) { return std::to_string(_errors.size()) + " problems"; }
      return _errors.front();
    }
  };

  // what is known

  TEST_F(PhysicsComponentFormatsTest, KnowsTheComponentsOfThePhysics)
  {
    EXPECT_NE(_formats.Find("RigidBody"), nullptr);
    EXPECT_NE(_formats.Find("Trigger"), nullptr);
    EXPECT_NE(_formats.Find("CharacterBody"), nullptr);
    EXPECT_NE(_formats.Find("Collider"), nullptr);
    EXPECT_NE(_formats.Find("Joint"), nullptr);
  }

  TEST_F(PhysicsComponentFormatsTest, AreAmongTheComponentsOfTheEngine)
  {
    ComponentFormats formats;
    formats.AddEngineComponents();

    EXPECT_NE(formats.Find("Transform"), nullptr);
    EXPECT_NE(formats.Find("RigidBody"), nullptr);
    EXPECT_NE(formats.Find("Trigger"), nullptr);
    EXPECT_NE(formats.Find("CharacterBody"), nullptr);
    EXPECT_NE(formats.Find("Collider"), nullptr);
    EXPECT_NE(formats.Find("Joint"), nullptr);
  }

  TEST_F(PhysicsComponentFormatsTest, AreMadeFromTheDescriptionsOfTheComponents)
  {
    for (const std::string name : {"RigidBody", "Trigger", "CharacterBody", "Collider", "Joint"})
    {
      const auto *format = _formats.Find(name);
      ASSERT_NE(format, nullptr);
      ASSERT_NE(format->type, nullptr) << name;
      EXPECT_EQ(format->type->name, name);
    }

    EXPECT_THAT(_formats.Find("Collider")->type->GetPaths(), ElementsAre(
                  "shape", "size", "radius", "height", "top_radius", "bottom_radius", "model", "offset",
                  "rotation"));
    EXPECT_EQ(_formats.Find("RigidBody")->type->Find("layers")->kind, neon::FieldKind::Layers);
    EXPECT_EQ(_formats.Find("CharacterBody")->type->Find("max_slope")->unit, "degrees");
    EXPECT_THAT(_formats.Find("Joint")->type->GetPaths(), ElementsAre("type", "other", "anchor", "axis", "limits"));
  }

  TEST_F(PhysicsComponentFormatsTest, SaysThatAWorldWithoutPhysicsCannotHoldThem)
  {
    FakeEntityStore store;
    store.Initialize();
    const Entity entity = store.CreateEntity("crate");

    for (const std::string name : {"RigidBody", "Trigger", "CharacterBody", "Collider", "Joint"})
    {
      _errors.clear();
      const auto map = At(4, DataValue::Map());
      const DataReader reader(map, "test.scene.yml", name + " of entity 'crate'", _errors);

      _formats.Find(name)->read(reader, store, entity);

      EXPECT_THAT(_errors, ElementsAre(
                    "test.scene.yml: " + name
                    + " of entity 'crate' needs the physics, which is not part of this world"));
    }
  }

  // RigidBody

  TEST_F(PhysicsComponentFormatsTest, ReadsARigidBodyWithItsDefaults)
  {
    Read("RigidBody", DataValue::Map());

    const RigidBody standard{};
    const auto *body = _store.Get<RigidBody>(_entity);
    ASSERT_NE(body, nullptr);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(body->kind, BodyKind::Dynamic);
    EXPECT_EQ(body->mass, 1.0f);
    EXPECT_EQ(body->friction, 0.5f);
    EXPECT_EQ(body->bounce, 0.0f);
    EXPECT_EQ(body->linear_damping, standard.linear_damping);
    EXPECT_EQ(body->angular_damping, standard.angular_damping);
    EXPECT_EQ(body->gravity_scale, 1.0f);
    EXPECT_EQ(body->linear_velocity, glm::vec3(0.0f));
    EXPECT_EQ(body->angular_velocity, glm::vec3(0.0f));
    EXPECT_FALSE(body->continuous);
    EXPECT_TRUE(body->can_sleep);
    EXPECT_EQ(body->layers, 1u);
    EXPECT_EQ(body->mask, 1u);
    EXPECT_EQ(body->body, neon::No_Body);
    EXPECT_FALSE(body->failed);
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsEveryValueOfARigidBody)
  {
    auto map = DataValue::Map();
    map.Set("kind", DataValue::Text("kinematic"));
    map.Set("mass", DataValue::Number(10.0));
    map.Set("friction", DataValue::Number(0.25));
    map.Set("bounce", DataValue::Number(0.75));
    map.Set("linear_damping", DataValue::Number(0.5));
    map.Set("angular_damping", DataValue::Number(2.0));
    map.Set("gravity_scale", DataValue::Number(-1.0));
    map.Set("linear_velocity", Numbers({1.0, 2.0, 3.0}));
    map.Set("angular_velocity", Numbers({90.0, 0.0, -45.0}));
    map.Set("continuous", DataValue::Bool(true));
    map.Set("can_sleep", DataValue::Bool(false));
    map.Set("layers", Numbers({2.0, 3.0}));
    map.Set("mask", Numbers({1.0, 32.0}));

    Read("RigidBody", map);

    const auto *body = _store.Get<RigidBody>(_entity);
    ASSERT_NE(body, nullptr);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(body->kind, BodyKind::Kinematic);
    EXPECT_EQ(body->mass, 10.0f);
    EXPECT_EQ(body->friction, 0.25f);
    EXPECT_EQ(body->bounce, 0.75f);
    EXPECT_EQ(body->linear_damping, 0.5f);
    EXPECT_EQ(body->angular_damping, 2.0f);
    EXPECT_EQ(body->gravity_scale, -1.0f);
    EXPECT_EQ(body->linear_velocity, glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(body->angular_velocity, glm::vec3(90.0f, 0.0f, -45.0f));
    EXPECT_TRUE(body->continuous);
    EXPECT_FALSE(body->can_sleep);
    EXPECT_EQ(body->layers, 0b110u);
    EXPECT_EQ(body->mask, 0x80000001u);
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsEveryKindOfBody)
  {
    const std::vector<std::pair<std::string, BodyKind>> kinds = {
      {"static", BodyKind::Static},
      {"kinematic", BodyKind::Kinematic},
      {"dynamic", BodyKind::Dynamic}
    };

    for (const auto &[text, kind] : kinds)
    {
      auto map = DataValue::Map();
      map.Set("kind", DataValue::Text(text));

      Read("RigidBody", map);

      EXPECT_EQ(_store.Get<RigidBody>(_entity)->kind, kind) << text;
    }
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsOneLayerWithoutAList)
  {
    auto map = DataValue::Map();
    map.Set("layers", DataValue::Number(3.0));
    map.Set("mask", DataValue::Number(32.0));

    Read("RigidBody", map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_store.Get<RigidBody>(_entity)->layers, 0b100u);
    EXPECT_EQ(_store.Get<RigidBody>(_entity)->mask, 0x80000000u);
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsAnEmptyListAsNoLayer)
  {
    auto map = DataValue::Map();
    map.Set("mask", DataValue::List());

    Read("RigidBody", map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_store.Get<RigidBody>(_entity)->mask, 0u);
    EXPECT_EQ(_store.Get<RigidBody>(_entity)->layers, 1u);
  }

  TEST_F(PhysicsComponentFormatsTest, SaysWhatIsWrongWithARigidBody)
  {
    const std::string where = "test.scene.yml:7: ";
    const std::string of = " of RigidBody of entity 'crate' ";

    EXPECT_EQ(ProblemOf("RigidBody", "kind", DataValue::Text("rigid")),
              where + "'kind'" + of + "is 'rigid', where one of these was expected: static, kinematic, dynamic");
    EXPECT_EQ(ProblemOf("RigidBody", "kind", DataValue::Number(2.0)),
              where + "'kind'" + of + "is a number, where one of these was expected: static, kinematic, dynamic");
    EXPECT_EQ(ProblemOf("RigidBody", "mass", DataValue::Text("heavy")),
              where + "'mass'" + of + "is text, where a number was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "mass", DataValue::Number(0.0)),
              where + "'mass'" + of + "is 0, where a number above 0 was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "mass", DataValue::Number(-2.5)),
              where + "'mass'" + of + "is -2.5, where a number above 0 was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "friction", DataValue::Number(-1.0)),
              where + "'friction'" + of + "is -1, where a number of 0 or above was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "bounce", DataValue::Number(1.5)),
              where + "'bounce'" + of + "is 1.5, where a number from 0 to 1 was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "bounce", DataValue::Number(-0.5)),
              where + "'bounce'" + of + "is -0.5, where a number from 0 to 1 was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "linear_damping", DataValue::Number(-1.0)),
              where + "'linear_damping'" + of + "is -1, where a number of 0 or above was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "angular_damping", DataValue::Number(-1.0)),
              where + "'angular_damping'" + of + "is -1, where a number of 0 or above was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "gravity_scale", DataValue::Bool(true)),
              where + "'gravity_scale'" + of + "is true or false, where a number was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "linear_velocity", DataValue::Number(3.0)),
              where + "'linear_velocity'" + of + "is a number, where a list of 3 numbers was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "angular_velocity", Numbers({1.0, 2.0})),
              where + "'angular_velocity'" + of + "holds 2 values, where a list of 3 numbers was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "continuous", DataValue::Number(1.0)),
              where + "'continuous'" + of + "is a number, where true or false was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "can_sleep", DataValue::Text("yes")),
              where + "'can_sleep'" + of + "is text, where true or false was expected");
  }

  TEST_F(PhysicsComponentFormatsTest, SaysWhatIsWrongWithLayers)
  {
    const std::string where = "test.scene.yml:7: ";
    const std::string of = " of RigidBody of entity 'crate' ";

    EXPECT_EQ(ProblemOf("RigidBody", "layers", DataValue::Text("walls")),
              where + "'layers'" + of + "is text, where a layer from 1 to 32 or a list of layers was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "layers", DataValue::Number(0.0)),
              where + "'layers'" + of + "holds 0, where a layer from 1 to 32 was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "mask", DataValue::Number(33.0)),
              where + "'mask'" + of + "holds 33, where a layer from 1 to 32 was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "mask", DataValue::Number(1.5)),
              where + "'mask'" + of + "holds 1.5, where a layer from 1 to 32 was expected");

    auto mixed = DataValue::List();
    mixed.Add(At(8, DataValue::Number(1.0)));
    mixed.Add(At(9, DataValue::Text("two")));
    EXPECT_EQ(ProblemOf("RigidBody", "mask", mixed),
              "test.scene.yml:9: 'mask'" + of + "holds text, where a layer from 1 to 32 was expected");
  }

  TEST_F(PhysicsComponentFormatsTest, KeepsTheDefaultOfAValueThatIsWrong)
  {
    auto map = DataValue::Map();
    map.Set("mass", DataValue::Number(-1.0));
    map.Set("bounce", DataValue::Number(7.0));
    map.Set("layers", DataValue::Number(40.0));

    Read("RigidBody", map);

    EXPECT_EQ(_errors.size(), 3u);
    EXPECT_EQ(_store.Get<RigidBody>(_entity)->mass, 1.0f);
    EXPECT_EQ(_store.Get<RigidBody>(_entity)->bounce, 0.0f);
    EXPECT_EQ(_store.Get<RigidBody>(_entity)->layers, 1u);
  }

  TEST_F(PhysicsComponentFormatsTest, SaysThatANameOfARigidBodyIsNotKnown)
  {
    auto map = DataValue::Map();
    map.Set("weight", At(12, DataValue::Number(3.0)));

    Read("RigidBody", map);

    EXPECT_THAT(_errors, ElementsAre(
                  "test.scene.yml:12: 'weight' is not known to RigidBody of entity 'crate'. Known are: "
                  "kind, mass, friction, bounce, linear_damping, angular_damping, gravity_scale, "
                  "linear_velocity, angular_velocity, continuous, can_sleep, lock_position, lock_rotation, "
                  "layers, mask"));
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsTheAxesARigidBodyIsLockedOn)
  {
    auto map = DataValue::Map();
    map.Set("lock_position", Texts({"x", "z"}));
    map.Set("lock_rotation", Texts({"x", "y", "z"}));

    Read("RigidBody", map);

    const auto *body = _store.Get<RigidBody>(_entity);
    ASSERT_NE(body, nullptr);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_THAT(body->lock_position, ElementsAre("x", "z"));
    EXPECT_THAT(body->lock_rotation, ElementsAre("x", "y", "z"));
  }

  TEST_F(PhysicsComponentFormatsTest, SaysThatAnAxisARigidBodyIsLockedOnIsNotKnown)
  {
    EXPECT_EQ(ProblemOf("RigidBody", "lock_rotation", Texts({"y", "w"})),
              "test.scene.yml:7: 'lock_rotation' of RigidBody of entity 'crate' holds 'w', where x, y, or z was "
              "expected");
    EXPECT_EQ(ProblemOf("RigidBody", "lock_position", Texts({"up"})),
              "test.scene.yml:7: 'lock_position' of RigidBody of entity 'crate' holds 'up', where x, y, or z "
              "was expected");
    EXPECT_EQ(ProblemOf("RigidBody", "lock_position", DataValue::Text("x")),
              "test.scene.yml:7: 'lock_position' of RigidBody of entity 'crate' is text, where a list of texts "
              "was expected");
  }

  TEST_F(PhysicsComponentFormatsTest, WritesOnlyTheKindOfARigidBodyWithDefaults)
  {
    _store.Set(_entity, RigidBody{});

    const auto map = Write("RigidBody");

    EXPECT_THAT(NamesOf(map), ElementsAre("kind"));
    EXPECT_EQ(TextOf(map, "kind"), "dynamic");
  }

  TEST_F(PhysicsComponentFormatsTest, WritesEveryValueOfARigidBodyThatIsNotTheDefault)
  {
    RigidBody body;
    body.kind = BodyKind::Static;
    body.mass = 10.0f;
    body.friction = 0.25f;
    body.bounce = 0.75f;
    body.linear_damping = 0.5f;
    body.angular_damping = 2.0f;
    body.gravity_scale = 0.0f;
    body.linear_velocity = {1.0f, 2.0f, 3.0f};
    body.angular_velocity = {90.0f, 0.0f, -45.0f};
    body.continuous = true;
    body.can_sleep = false;
    body.lock_position = {"y"};
    body.lock_rotation = {"x", "z"};
    body.layers = 0b110;
    body.mask = 0x80000001u;
    _store.Set(_entity, body);

    const auto map = Write("RigidBody");

    EXPECT_THAT(NamesOf(map), ElementsAre(
                  "kind", "mass", "friction", "bounce", "linear_damping", "angular_damping", "gravity_scale",
                  "linear_velocity", "angular_velocity", "continuous", "can_sleep", "lock_position",
                  "lock_rotation", "layers", "mask"));
    EXPECT_THAT(TextsOf(map, "lock_position"), ElementsAre("y"));
    EXPECT_THAT(TextsOf(map, "lock_rotation"), ElementsAre("x", "z"));
    EXPECT_EQ(TextOf(map, "kind"), "static");
    EXPECT_EQ(NumberOf(map, "mass"), 10.0);
    EXPECT_EQ(NumberOf(map, "gravity_scale"), 0.0);
    EXPECT_THAT(NumbersOf(map, "linear_velocity"), ElementsAre(1.0, 2.0, 3.0));
    EXPECT_THAT(NumbersOf(map, "angular_velocity"), ElementsAre(90.0, 0.0, -45.0));
    EXPECT_THAT(NumbersOf(map, "layers"), ElementsAre(2.0, 3.0));
    EXPECT_THAT(NumbersOf(map, "mask"), ElementsAre(1.0, 32.0));
  }

  TEST_F(PhysicsComponentFormatsTest, NeverWritesWhatTheEngineFilledIn)
  {
    RigidBody body;
    body.body = 17;
    body.failed = true;
    _store.Set(_entity, body);
    Trigger trigger;
    trigger.body = 18;
    trigger.inside = 3;
    trigger.failed = true;
    _store.Set(_entity, trigger);
    CharacterBody character;
    character.character = 19;
    character.on_floor = true;
    character.on_wall = true;
    character.on_ceiling = true;
    character.floor = 1234;
    character.floor_normal = {1.0f, 0.0f, 0.0f};
    character.real_velocity = {1.0f, 2.0f, 3.0f};
    character.failed = true;
    _store.Set(_entity, character);

    EXPECT_THAT(NamesOf(Write("RigidBody")), ElementsAre("kind"));
    EXPECT_THAT(NamesOf(Write("Trigger")), IsEmpty());
    EXPECT_THAT(NamesOf(Write("CharacterBody")), IsEmpty());
  }

  TEST_F(PhysicsComponentFormatsTest, WritesNoLayerAsAnEmptyList)
  {
    RigidBody body;
    body.mask = 0;
    _store.Set(_entity, body);

    const auto map = Write("RigidBody");

    ASSERT_NE(map.Find("mask"), nullptr);
    EXPECT_TRUE(map.Find("mask")->IsList());
    EXPECT_THAT(NumbersOf(map, "mask"), IsEmpty());
  }

  TEST_F(PhysicsComponentFormatsTest, WritesNothingForAnEntityWithoutTheComponent)
  {
    DataValue value;

    EXPECT_FALSE(_formats.Find("RigidBody")->write(_store, _entity, value));
    EXPECT_FALSE(_formats.Find("Trigger")->write(_store, _entity, value));
    EXPECT_FALSE(_formats.Find("CharacterBody")->write(_store, _entity, value));
    EXPECT_FALSE(_formats.Find("Collider")->write(_store, _entity, value));
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsARigidBodyAsItWasWritten)
  {
    RigidBody body;
    body.kind = BodyKind::Kinematic;
    body.mass = 12.5f;
    body.friction = 0.31f;
    body.bounce = 0.2f;
    body.gravity_scale = 0.5f;
    body.linear_velocity = {0.1f, -2.0f, 3.5f};
    body.angular_velocity = {10.0f, 20.0f, 30.0f};
    body.continuous = true;
    body.layers = 0b1010;
    body.mask = 0;
    _store.Set(_entity, body);
    const auto written = Write("RigidBody");
    _store.Remove<RigidBody>(_entity);

    Read("RigidBody", written);

    const auto *read = _store.Get<RigidBody>(_entity);
    ASSERT_NE(read, nullptr);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(read->kind, body.kind);
    EXPECT_EQ(read->mass, body.mass);
    EXPECT_EQ(read->friction, body.friction);
    EXPECT_EQ(read->bounce, body.bounce);
    EXPECT_EQ(read->gravity_scale, body.gravity_scale);
    EXPECT_EQ(read->linear_velocity, body.linear_velocity);
    EXPECT_EQ(read->angular_velocity, body.angular_velocity);
    EXPECT_EQ(read->continuous, body.continuous);
    EXPECT_EQ(read->layers, body.layers);
    EXPECT_EQ(read->mask, body.mask);
    EXPECT_THAT(NamesOf(Write("RigidBody")), NamesOf(written));
  }

  // Trigger

  TEST_F(PhysicsComponentFormatsTest, ReadsATriggerWithItsDefaults)
  {
    Read("Trigger", DataValue::Map());

    const auto *trigger = _store.Get<Trigger>(_entity);
    ASSERT_NE(trigger, nullptr);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(trigger->layers, 1u);
    EXPECT_EQ(trigger->mask, 1u);
    EXPECT_EQ(trigger->inside, 0u);
    EXPECT_EQ(trigger->body, neon::No_Body);
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsEveryValueOfATrigger)
  {
    auto map = DataValue::Map();
    map.Set("layers", DataValue::Number(4.0));
    map.Set("mask", Numbers({1.0, 2.0}));

    Read("Trigger", map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_store.Get<Trigger>(_entity)->layers, 0b1000u);
    EXPECT_EQ(_store.Get<Trigger>(_entity)->mask, 0b11u);
  }

  TEST_F(PhysicsComponentFormatsTest, SaysWhatIsWrongWithATrigger)
  {
    EXPECT_EQ(ProblemOf("Trigger", "mask", DataValue::Bool(true)),
              "test.scene.yml:7: 'mask' of Trigger of entity 'crate' is true or false, "
              "where a layer from 1 to 32 or a list of layers was expected");
    EXPECT_EQ(ProblemOf("Trigger", "radius", DataValue::Number(2.0)),
              "test.scene.yml:7: 'radius' is not known to Trigger of entity 'crate'. Known are: layers, mask");
  }

  TEST_F(PhysicsComponentFormatsTest, WritesATriggerAndReadsItBack)
  {
    Trigger trigger;
    trigger.layers = 0b10;
    trigger.mask = 0b101;
    _store.Set(_entity, trigger);

    const auto written = Write("Trigger");
    _store.Remove<Trigger>(_entity);
    Read("Trigger", written);

    EXPECT_THAT(NamesOf(written), ElementsAre("layers", "mask"));
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_store.Get<Trigger>(_entity)->layers, 0b10u);
    EXPECT_EQ(_store.Get<Trigger>(_entity)->mask, 0b101u);
  }

  TEST_F(PhysicsComponentFormatsTest, WritesATriggerWithDefaultsAsAnEmptyMap)
  {
    _store.Set(_entity, Trigger{});

    const auto map = Write("Trigger");

    EXPECT_TRUE(map.IsMap());
    EXPECT_THAT(NamesOf(map), IsEmpty());
  }

  // Collider

  TEST_F(PhysicsComponentFormatsTest, ReadsAColliderWithItsDefaults)
  {
    Read("Collider", DataValue::Map());

    const auto *collider = _store.Get<Collider>(_entity);
    ASSERT_NE(collider, nullptr);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(collider->shape, ShapeKind::Box);
    EXPECT_EQ(collider->size, glm::vec3(1.0f));
    EXPECT_EQ(collider->radius, 0.5f);
    EXPECT_EQ(collider->height, 2.0f);
    EXPECT_EQ(collider->top_radius, 0.25f);
    EXPECT_EQ(collider->bottom_radius, 0.5f);
    EXPECT_TRUE(collider->model.empty());
    EXPECT_EQ(collider->offset, glm::vec3(0.0f));
    EXPECT_EQ(collider->rotation.pitch, 0.0f);
    EXPECT_EQ(collider->rotation.yaw, 0.0f);
    EXPECT_EQ(collider->rotation.roll, 0.0f);
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsEveryShape)
  {
    const std::vector<std::pair<std::string, ShapeKind>> shapes = {
      {"box", ShapeKind::Box},
      {"sphere", ShapeKind::Sphere},
      {"capsule", ShapeKind::Capsule},
      {"cylinder", ShapeKind::Cylinder},
      {"tapered_capsule", ShapeKind::TaperedCapsule},
      {"tapered_cylinder", ShapeKind::TaperedCylinder},
      {"plane", ShapeKind::Plane},
      {"convex_hull", ShapeKind::ConvexHull},
      {"mesh", ShapeKind::Mesh}
    };

    for (const auto &[text, shape] : shapes)
    {
      auto map = DataValue::Map();
      map.Set("shape", DataValue::Text(text));
      map.Set("model", DataValue::Text("assets://models/bunny.obj"));
      _errors.clear();

      Read("Collider", map);

      EXPECT_EQ(_store.Get<Collider>(_entity)->shape, shape) << text;
      // the model belongs to the last two alone
      const bool has_model = shape == ShapeKind::ConvexHull || shape == ShapeKind::Mesh;
      EXPECT_EQ(_errors.empty(), has_model) << text;
    }
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsABox)
  {
    auto map = DataValue::Map();
    map.Set("shape", DataValue::Text("box"));
    map.Set("size", Numbers({1.0, 2.0, 3.0}));
    map.Set("offset", Numbers({0.0, 0.5, 0.0}));
    map.Set("rotation", Numbers({10.0, 20.0, 30.0}));

    Read("Collider", map);

    const auto *collider = _store.Get<Collider>(_entity);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(collider->size, glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(collider->offset, glm::vec3(0.0f, 0.5f, 0.0f));
    EXPECT_EQ(collider->rotation.pitch, 10.0f);
    EXPECT_EQ(collider->rotation.yaw, 20.0f);
    EXPECT_EQ(collider->rotation.roll, 30.0f);
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsOneNumberAsTheSizeOfEverySideOfABox)
  {
    auto map = DataValue::Map();
    map.Set("size", DataValue::Number(4.0));

    Read("Collider", map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_store.Get<Collider>(_entity)->size, glm::vec3(4.0f));
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsTheValuesOfTheRoundShapes)
  {
    auto sphere = DataValue::Map();
    sphere.Set("shape", DataValue::Text("sphere"));
    sphere.Set("radius", DataValue::Number(2.0));
    Read("Collider", sphere);
    EXPECT_EQ(_store.Get<Collider>(_entity)->radius, 2.0f);

    auto capsule = DataValue::Map();
    capsule.Set("shape", DataValue::Text("capsule"));
    capsule.Set("radius", DataValue::Number(0.3));
    capsule.Set("height", DataValue::Number(1.8));
    Read("Collider", capsule);
    EXPECT_EQ(_store.Get<Collider>(_entity)->radius, 0.3f);
    EXPECT_EQ(_store.Get<Collider>(_entity)->height, 1.8f);

    auto cylinder = DataValue::Map();
    cylinder.Set("shape", DataValue::Text("cylinder"));
    cylinder.Set("radius", DataValue::Number(3.0));
    cylinder.Set("height", DataValue::Number(0.1));
    Read("Collider", cylinder);
    EXPECT_EQ(_store.Get<Collider>(_entity)->radius, 3.0f);
    EXPECT_EQ(_store.Get<Collider>(_entity)->height, 0.1f);

    for (const std::string shape : {"tapered_capsule", "tapered_cylinder"})
    {
      auto tapered = DataValue::Map();
      tapered.Set("shape", DataValue::Text(shape));
      tapered.Set("height", DataValue::Number(3.0));
      tapered.Set("top_radius", DataValue::Number(0.1));
      tapered.Set("bottom_radius", DataValue::Number(0.9));
      Read("Collider", tapered);
      EXPECT_EQ(_store.Get<Collider>(_entity)->height, 3.0f);
      EXPECT_EQ(_store.Get<Collider>(_entity)->top_radius, 0.1f);
      EXPECT_EQ(_store.Get<Collider>(_entity)->bottom_radius, 0.9f);
    }

    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsAConeAsATaperedCylinderThatEndsInAPoint)
  {
    auto map = DataValue::Map();
    map.Set("shape", DataValue::Text("tapered_cylinder"));
    map.Set("top_radius", DataValue::Number(0.0));

    Read("Collider", map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_store.Get<Collider>(_entity)->top_radius, 0.0f);
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsTheModelOfAHullAndOfAMesh)
  {
    for (const std::string shape : {"convex_hull", "mesh"})
    {
      auto map = DataValue::Map();
      map.Set("shape", DataValue::Text(shape));
      map.Set("model", DataValue::Text("assets://models/bear.obj"));

      Read("Collider", map);

      EXPECT_EQ(_store.Get<Collider>(_entity)->model, "assets://models/bear.obj");
    }
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(PhysicsComponentFormatsTest, SaysWhatIsWrongWithACollider)
  {
    const std::string where = "test.scene.yml:7: ";
    const std::string of = " of Collider of entity 'crate' ";

    EXPECT_EQ(ProblemOf("Collider", "shape", DataValue::Text("cube")),
              where + "'shape'" + of + "is 'cube', where one of these was expected: box, sphere, capsule, "
              "cylinder, tapered_capsule, tapered_cylinder, plane, convex_hull, mesh");
    EXPECT_EQ(ProblemOf("Collider", "size", DataValue::Text("big")),
              where + "'size'" + of + "is text, where a list of 3 numbers was expected");
    EXPECT_EQ(ProblemOf("Collider", "size", Numbers({1.0, 2.0})),
              where + "'size'" + of + "holds 2 values, where a list of 3 numbers was expected");
    EXPECT_EQ(ProblemOf("Collider", "size", Numbers({1.0, 0.0, 1.0})),
              where + "'size'" + of + "is [1, 0, 1], where numbers above 0 were expected");
    EXPECT_EQ(ProblemOf("Collider", "size", DataValue::Number(-2.0)),
              where + "'size'" + of + "is [-2, -2, -2], where numbers above 0 were expected");
    EXPECT_EQ(ProblemOf("Collider", "offset", DataValue::Number(1.0)),
              where + "'offset'" + of + "is a number, where a list of 3 numbers was expected");
    EXPECT_EQ(ProblemOf("Collider", "rotation", Numbers({1.0, 2.0, 3.0, 4.0})),
              where + "'rotation'" + of + "holds 4 values, where a list of 3 numbers was expected");
  }

  TEST_F(PhysicsComponentFormatsTest, SaysWhatIsWrongWithARoundShape)
  {
    const std::string of = " of Collider of entity 'crate' ";

    const auto problem = [&](const std::string &shape, const std::string &name, const DataValue &value)
    {
      _errors.clear();
      auto map = DataValue::Map();
      map.Set("shape", DataValue::Text(shape));
      map.Set(name, At(7, value));
      Read("Collider", map);
      return _errors.size() == 1 ? _errors.front() : std::to_string(_errors.size()) + " problems";
    };

    EXPECT_EQ(problem("sphere", "radius", DataValue::Number(0.0)),
              "test.scene.yml:7: 'radius'" + of + "is 0, where a number above 0 was expected");
    EXPECT_EQ(problem("sphere", "radius", DataValue::Text("wide")),
              "test.scene.yml:7: 'radius'" + of + "is text, where a number was expected");
    EXPECT_EQ(problem("cylinder", "height", DataValue::Number(-1.0)),
              "test.scene.yml:7: 'height'" + of + "is -1, where a number above 0 was expected");
    EXPECT_EQ(problem("capsule", "height", DataValue::Number(0.5)),
              "test.scene.yml: 'height'" + of + "is 0.5, which is less than twice its 'radius' of 0.5");
    EXPECT_EQ(problem("tapered_capsule", "top_radius", DataValue::Number(0.0)),
              "test.scene.yml:7: 'top_radius'" + of + "is 0, where a number above 0 was expected");
    EXPECT_EQ(problem("tapered_capsule", "height", DataValue::Number(0.5)),
              "test.scene.yml: 'height'" + of
              + "is 0.5, which is less than its 'top_radius' and 'bottom_radius' together");
    EXPECT_EQ(problem("tapered_cylinder", "bottom_radius", DataValue::Number(-1.0)),
              "test.scene.yml:7: 'bottom_radius'" + of + "is -1, where a number of 0 or above was expected");
  }

  TEST_F(PhysicsComponentFormatsTest, SaysThatACylinderCannotEndInAPointOnBothSides)
  {
    auto map = DataValue::Map();
    map.Set("shape", DataValue::Text("tapered_cylinder"));
    map.Set("top_radius", DataValue::Number(0.0));
    map.Set("bottom_radius", DataValue::Number(0.0));

    Read("Collider", map);

    EXPECT_THAT(_errors, ElementsAre(
                  "test.scene.yml: 'top_radius' and 'bottom_radius' of Collider of entity 'crate' are both 0, "
                  "where one above 0 was expected"));
  }

  TEST_F(PhysicsComponentFormatsTest, AHullAndAMeshNeedNoModelSinceAGeometryMayShapeThem)
  {
    for (const std::string shape : {"convex_hull", "mesh"})
    {
      _errors.clear();
      auto map = DataValue::Map();
      map.Set("shape", DataValue::Text(shape));

      Read("Collider", map);

      EXPECT_TRUE(_store.Get<Collider>(_entity)->model.empty());
      EXPECT_THAT(_errors, IsEmpty()) << shape;
    }
  }

  TEST_F(PhysicsComponentFormatsTest, SaysThatAValueDoesNotBelongToTheShape)
  {
    auto box = DataValue::Map();
    box.Set("shape", DataValue::Text("box"));
    box.Set("radius", At(9, DataValue::Number(2.0)));
    Read("Collider", box);

    auto sphere = DataValue::Map();
    sphere.Set("shape", DataValue::Text("sphere"));
    sphere.Set("size", At(14, Numbers({1.0, 1.0, 1.0})));
    Read("Collider", sphere);

    auto plane = DataValue::Map();
    plane.Set("shape", DataValue::Text("plane"));
    plane.Set("model", At(21, DataValue::Text("assets://models/cube.obj")));
    Read("Collider", plane);

    EXPECT_THAT(_errors, ElementsAre(
                  "test.scene.yml:9: 'radius' is not known to Collider of entity 'crate'. Known are: "
                  "shape, size, offset, rotation",
                  "test.scene.yml:14: 'size' is not known to Collider of entity 'crate'. Known are: "
                  "shape, radius, offset, rotation",
                  "test.scene.yml:21: 'model' is not known to Collider of entity 'crate'. Known are: "
                  "shape, offset, rotation"));
  }

  TEST_F(PhysicsComponentFormatsTest, WritesOnlyTheShapeOfAColliderWithDefaults)
  {
    _store.Set(_entity, Collider{});

    const auto map = Write("Collider");

    EXPECT_THAT(NamesOf(map), ElementsAre("shape"));
    EXPECT_EQ(TextOf(map, "shape"), "box");
  }

  TEST_F(PhysicsComponentFormatsTest, WritesWhatBelongsToTheShapeAndNothingElse)
  {
    Collider collider;
    collider.size = {2.0f, 3.0f, 4.0f};
    collider.radius = 0.7f;
    collider.height = 3.0f;
    collider.top_radius = 0.1f;
    collider.bottom_radius = 0.2f;
    collider.model = "assets://models/bunny.obj";
    collider.offset = {0.0f, 1.0f, 0.0f};
    collider.rotation = {.pitch = 90.0f};

    const auto names_of = [&](const ShapeKind shape)
    {
      collider.shape = shape;
      _store.Set(_entity, collider);
      return NamesOf(Write("Collider"));
    };

    EXPECT_THAT(names_of(ShapeKind::Box), ElementsAre("shape", "size", "offset", "rotation"));
    EXPECT_THAT(names_of(ShapeKind::Sphere), ElementsAre("shape", "radius", "offset", "rotation"));
    EXPECT_THAT(names_of(ShapeKind::Capsule), ElementsAre("shape", "radius", "height", "offset", "rotation"));
    EXPECT_THAT(names_of(ShapeKind::Cylinder), ElementsAre("shape", "radius", "height", "offset", "rotation"));
    EXPECT_THAT(names_of(ShapeKind::TaperedCapsule),
                ElementsAre("shape", "height", "top_radius", "bottom_radius", "offset", "rotation"));
    EXPECT_THAT(names_of(ShapeKind::TaperedCylinder),
                ElementsAre("shape", "height", "top_radius", "bottom_radius", "offset", "rotation"));
    EXPECT_THAT(names_of(ShapeKind::Plane), ElementsAre("shape", "offset", "rotation"));
    EXPECT_THAT(names_of(ShapeKind::ConvexHull), ElementsAre("shape", "model", "offset", "rotation"));
    EXPECT_THAT(names_of(ShapeKind::Mesh), ElementsAre("shape", "model", "offset", "rotation"));
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsEveryShapeAsItWasWritten)
  {
    Collider collider;
    collider.size = {2.0f, 3.0f, 4.0f};
    collider.radius = 0.7f;
    collider.height = 3.0f;
    collider.top_radius = 0.1f;
    collider.bottom_radius = 0.2f;
    collider.model = "assets://models/bunny.obj";
    collider.offset = {0.0f, 1.0f, 0.0f};
    collider.rotation = {.pitch = 90.0f, .yaw = 0.31f, .roll = -5.0f};

    for (int shape = 0; shape <= static_cast<int>(ShapeKind::Mesh); shape++)
    {
      collider.shape = static_cast<ShapeKind>(shape);
      _store.Set(_entity, collider);
      const auto written = Write("Collider");
      _store.Remove<Collider>(_entity);

      Read("Collider", written);

      const auto *read = _store.Get<Collider>(_entity);
      ASSERT_NE(read, nullptr);
      EXPECT_THAT(_errors, IsEmpty()) << shape;
      EXPECT_EQ(read->shape, collider.shape);
      EXPECT_EQ(read->offset, collider.offset);
      EXPECT_EQ(read->rotation.pitch, collider.rotation.pitch);
      EXPECT_EQ(read->rotation.yaw, collider.rotation.yaw);
      EXPECT_EQ(read->rotation.roll, collider.rotation.roll);

      // written a second time, it is the same
      EXPECT_THAT(NamesOf(Write("Collider")), NamesOf(written));
    }
  }

  // CharacterBody

  TEST_F(PhysicsComponentFormatsTest, ReadsACharacterBodyWithItsDefaults)
  {
    Read("CharacterBody", DataValue::Map());

    const auto *character = _store.Get<CharacterBody>(_entity);
    ASSERT_NE(character, nullptr);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(character->velocity, glm::vec3(0.0f));
    EXPECT_EQ(character->fall_velocity, glm::vec3(0.0f));
    EXPECT_EQ(character->gravity_scale, 1.0f);
    EXPECT_EQ(character->max_slope, 45.0f);
    EXPECT_EQ(character->step_height, 0.25f);
    EXPECT_EQ(character->mass, 70.0f);
    EXPECT_EQ(character->push_strength, 100.0f);
    EXPECT_EQ(character->layers, 1u);
    EXPECT_EQ(character->mask, 1u);
    EXPECT_EQ(character->character, neon::No_Character);
    EXPECT_FALSE(character->on_floor);
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsEveryValueOfACharacterBody)
  {
    auto map = DataValue::Map();
    map.Set("velocity", Numbers({1.0, 0.0, -2.0}));
    map.Set("fall_velocity", Numbers({0.0, 5.0, 0.0}));
    map.Set("gravity_scale", DataValue::Number(0.0));
    map.Set("max_slope", DataValue::Number(60.0));
    map.Set("step_height", DataValue::Number(0.4));
    map.Set("mass", DataValue::Number(80.0));
    map.Set("push_strength", DataValue::Number(0.0));
    map.Set("layers", DataValue::Number(2.0));
    map.Set("mask", Numbers({1.0, 2.0, 3.0}));

    Read("CharacterBody", map);

    const auto *character = _store.Get<CharacterBody>(_entity);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(character->velocity, glm::vec3(1.0f, 0.0f, -2.0f));
    EXPECT_EQ(character->fall_velocity, glm::vec3(0.0f, 5.0f, 0.0f));
    EXPECT_EQ(character->gravity_scale, 0.0f);
    EXPECT_EQ(character->max_slope, 60.0f);
    EXPECT_EQ(character->step_height, 0.4f);
    EXPECT_EQ(character->mass, 80.0f);
    EXPECT_EQ(character->push_strength, 0.0f);
    EXPECT_EQ(character->layers, 0b10u);
    EXPECT_EQ(character->mask, 0b111u);
  }

  TEST_F(PhysicsComponentFormatsTest, SaysWhatIsWrongWithACharacterBody)
  {
    const std::string where = "test.scene.yml:7: ";
    const std::string of = " of CharacterBody of entity 'crate' ";

    EXPECT_EQ(ProblemOf("CharacterBody", "velocity", DataValue::Number(1.0)),
              where + "'velocity'" + of + "is a number, where a list of 3 numbers was expected");
    EXPECT_EQ(ProblemOf("CharacterBody", "fall_velocity", DataValue::Text("fast")),
              where + "'fall_velocity'" + of + "is text, where a list of 3 numbers was expected");
    EXPECT_EQ(ProblemOf("CharacterBody", "gravity_scale", DataValue::Text("none")),
              where + "'gravity_scale'" + of + "is text, where a number was expected");
    EXPECT_EQ(ProblemOf("CharacterBody", "max_slope", DataValue::Number(120.0)),
              where + "'max_slope'" + of + "is 120, where degrees from 0 to 90 were expected");
    EXPECT_EQ(ProblemOf("CharacterBody", "max_slope", DataValue::Number(-1.0)),
              where + "'max_slope'" + of + "is -1, where degrees from 0 to 90 were expected");
    EXPECT_EQ(ProblemOf("CharacterBody", "step_height", DataValue::Number(-0.1)),
              where + "'step_height'" + of + "is -0.1, where a number of 0 or above was expected");
    EXPECT_EQ(ProblemOf("CharacterBody", "mass", DataValue::Number(0.0)),
              where + "'mass'" + of + "is 0, where a number above 0 was expected");
    EXPECT_EQ(ProblemOf("CharacterBody", "push_strength", DataValue::Number(-5.0)),
              where + "'push_strength'" + of + "is -5, where a number of 0 or above was expected");
    EXPECT_EQ(ProblemOf("CharacterBody", "mask", DataValue::Number(64.0)),
              where + "'mask'" + of + "holds 64, where a layer from 1 to 32 was expected");
    EXPECT_EQ(ProblemOf("CharacterBody", "on_floor", DataValue::Bool(true)),
              where + "'on_floor' is not known to CharacterBody of entity 'crate'. Known are: velocity, "
              "fall_velocity, gravity_scale, max_slope, step_height, mass, push_strength, layers, mask");
  }

  TEST_F(PhysicsComponentFormatsTest, WritesACharacterBodyAndReadsItBack)
  {
    CharacterBody character;
    character.velocity = {1.0f, 0.0f, -2.0f};
    character.fall_velocity = {0.0f, 5.0f, 0.0f};
    character.gravity_scale = 0.5f;
    character.max_slope = 60.0f;
    character.step_height = 0.4f;
    character.mass = 80.0f;
    character.push_strength = 250.0f;
    character.layers = 0b10;
    character.mask = 0b111;
    _store.Set(_entity, character);

    const auto written = Write("CharacterBody");
    _store.Remove<CharacterBody>(_entity);
    Read("CharacterBody", written);

    EXPECT_THAT(NamesOf(written), ElementsAre(
                  "velocity", "fall_velocity", "gravity_scale", "max_slope", "step_height", "mass",
                  "push_strength", "layers", "mask"));
    const auto *read = _store.Get<CharacterBody>(_entity);
    ASSERT_NE(read, nullptr);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(read->velocity, character.velocity);
    EXPECT_EQ(read->fall_velocity, character.fall_velocity);
    EXPECT_EQ(read->gravity_scale, character.gravity_scale);
    EXPECT_EQ(read->max_slope, character.max_slope);
    EXPECT_EQ(read->step_height, character.step_height);
    EXPECT_EQ(read->mass, character.mass);
    EXPECT_EQ(read->push_strength, character.push_strength);
    EXPECT_EQ(read->layers, character.layers);
    EXPECT_EQ(read->mask, character.mask);
  }

  // every problem of a component is found, not only the first

  TEST_F(PhysicsComponentFormatsTest, FindsEveryProblemOfAComponent)
  {
    auto map = DataValue::Map();
    map.Set("shape", At(3, DataValue::Text("capsule")));
    map.Set("radius", At(4, DataValue::Number(-1.0)));
    map.Set("height", At(5, DataValue::Text("tall")));
    map.Set("size", At(6, DataValue::Number(1.0)));
    map.Set("offset", At(7, Numbers({1.0})));

    Read("Collider", map);

    EXPECT_THAT(_errors, ElementsAre(
                  "test.scene.yml:4: 'radius' of Collider of entity 'crate' is -1, where a number above 0 "
                  "was expected",
                  "test.scene.yml:5: 'height' of Collider of entity 'crate' is text, where a number was "
                  "expected",
                  "test.scene.yml:7: 'offset' of Collider of entity 'crate' holds 1 values, where a list of 3 "
                  "numbers was expected",
                  "test.scene.yml:6: 'size' is not known to Collider of entity 'crate'. Known are: shape, "
                  "radius, height, offset, rotation"));
  }

  // Joint

  TEST_F(PhysicsComponentFormatsTest, ReadsAJointWithItsDefaults)
  {
    Read("Joint", DataValue::Map());

    const auto *joint = _store.Get<Joint>(_entity);
    ASSERT_NE(joint, nullptr);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(joint->type, JointKind::Fixed);
    EXPECT_EQ(joint->other, "");
    EXPECT_EQ(joint->anchor, glm::vec3(0.0f));
    EXPECT_EQ(joint->axis, glm::vec3(0.0f, 1.0f, 0.0f));
    EXPECT_THAT(joint->limits, IsEmpty());
    EXPECT_EQ(joint->joint, neon::No_Joint);
    EXPECT_FALSE(joint->failed);
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsEveryValueOfAJoint)
  {
    auto map = DataValue::Map();
    map.Set("type", DataValue::Text("hinge"));
    map.Set("other", DataValue::Text("house/frame"));
    map.Set("anchor", Numbers({-0.8, 0.0, 0.0}));
    map.Set("axis", Numbers({0.0, 0.0, 1.0}));
    map.Set("limits", Numbers({-90.0, 10.0}));

    Read("Joint", map);

    const auto *joint = _store.Get<Joint>(_entity);
    ASSERT_NE(joint, nullptr);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(joint->type, JointKind::Hinge);
    EXPECT_EQ(joint->other, "house/frame");
    EXPECT_EQ(joint->anchor, glm::vec3(-0.8f, 0.0f, 0.0f));
    EXPECT_EQ(joint->axis, glm::vec3(0.0f, 0.0f, 1.0f));
    EXPECT_THAT(joint->limits, ElementsAre(-90.0f, 10.0f));
  }

  TEST_F(PhysicsComponentFormatsTest, ReadsEveryTypeOfJoint)
  {
    const std::vector<std::pair<std::string, JointKind>> types = {
      {"fixed", JointKind::Fixed},
      {"hinge", JointKind::Hinge},
      {"slider", JointKind::Slider},
      {"point", JointKind::Point}
    };

    for (const auto &[word, kind] : types)
    {
      auto map = DataValue::Map();
      map.Set("type", DataValue::Text(word));
      Read("Joint", map);

      EXPECT_THAT(_errors, IsEmpty()) << word;
      EXPECT_EQ(_store.Get<Joint>(_entity)->type, kind) << word;
    }
  }

  TEST_F(PhysicsComponentFormatsTest, SaysWhatIsWrongWithAJoint)
  {
    const std::string where = "test.scene.yml:7: ";
    const std::string of = " of Joint of entity 'crate' ";

    EXPECT_EQ(ProblemOf("Joint", "type", DataValue::Text("rope")),
              where + "'type'" + of + "is 'rope', where one of these was expected: fixed, hinge, slider, point");
    EXPECT_EQ(ProblemOf("Joint", "other", DataValue::Number(3.0)),
              where + "'other'" + of + "is a number, where text was expected");
    EXPECT_EQ(ProblemOf("Joint", "anchor", Numbers({1.0, 2.0})),
              where + "'anchor'" + of + "holds 2 values, where a list of 3 numbers was expected");
  }

  TEST_F(PhysicsComponentFormatsTest, SaysWhatIsWrongWithTheAxisAndTheLimitsOfAJoint)
  {
    const auto problem = [this](const std::string &type, const std::string &name, const DataValue &value)
    {
      _errors.clear();
      auto map = DataValue::Map();
      map.Set("type", DataValue::Text(type));
      map.Set(name, At(9, value));
      Read("Joint", map);
      return _errors.size() == 1 ? _errors.front() : std::to_string(_errors.size()) + " problems";
    };
    const std::string where = "test.scene.yml:9: ";
    const std::string of = " of Joint of entity 'crate' ";

    EXPECT_EQ(problem("hinge", "axis", Numbers({0.0, 0.0, 0.0})),
              where + "'axis'" + of + "is [0, 0, 0], where a direction was expected");
    EXPECT_EQ(problem("slider", "limits", Numbers({-1.0})),
              where + "'limits'" + of + "holds 1 number, where [least, most] or an empty list was expected");
    EXPECT_EQ(problem("slider", "limits", Numbers({-1.0, 0.0, 1.0})),
              where + "'limits'" + of + "holds 3 numbers, where [least, most] or an empty list was expected");
    EXPECT_EQ(problem("slider", "limits", Numbers({1.0, 2.0})),
              where + "'limits'" + of + "is [1, 2], where the least is 0 or below and the most 0 or above");
    EXPECT_EQ(problem("hinge", "limits", Numbers({-200.0, 90.0})),
              where + "'limits'" + of + "is [-200, 90], where degrees from -180 to 180 were expected");
    EXPECT_EQ(problem("slider", "limits", Numbers({-200.0, 90.0})), "0 problems");
    EXPECT_EQ(problem("hinge", "limits", Numbers({})), "0 problems");
  }

  TEST_F(PhysicsComponentFormatsTest, SaysThatTheAxisAndTheLimitsBelongToAHingeAndASlider)
  {
    auto map = DataValue::Map();
    map.Set("type", DataValue::Text("point"));
    map.Set("axis", At(5, Numbers({0.0, 1.0, 0.0})));
    map.Set("limits", At(6, Numbers({-1.0, 1.0})));

    Read("Joint", map);

    EXPECT_THAT(_errors, ElementsAre(
                  "test.scene.yml:5: 'axis' is not known to Joint of entity 'crate'. Known are: type, other, anchor",
                  "test.scene.yml:6: 'limits' is not known to Joint of entity 'crate'. Known are: type, other, "
                  "anchor"));
  }

  TEST_F(PhysicsComponentFormatsTest, WritesOnlyTheTypeOfAJointWithDefaults)
  {
    _store.Set(_entity, Joint{});

    const auto map = Write("Joint");

    EXPECT_THAT(NamesOf(map), ElementsAre("type"));
    EXPECT_EQ(TextOf(map, "type"), "fixed");
  }

  TEST_F(PhysicsComponentFormatsTest, WritesAJointAndReadsItBack)
  {
    Joint joint;
    joint.type = JointKind::Slider;
    joint.other = "rail";
    joint.anchor = {0.0f, -0.5f, 0.0f};
    joint.axis = {1.0f, 0.0f, 0.0f};
    joint.limits = {-3.0f, 3.0f};
    joint.joint = 7;
    joint.failed = true;
    _store.Set(_entity, joint);

    const auto map = Write("Joint");
    EXPECT_THAT(NamesOf(map), ElementsAre("type", "other", "anchor", "axis", "limits"));
    EXPECT_EQ(TextOf(map, "other"), "rail");
    EXPECT_THAT(NumbersOf(map, "limits"), ElementsAre(-3.0, 3.0));

    _store.Set(_entity, Joint{});
    Read("Joint", map);

    const auto *read = _store.Get<Joint>(_entity);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(read->type, JointKind::Slider);
    EXPECT_EQ(read->other, "rail");
    EXPECT_EQ(read->anchor, glm::vec3(0.0f, -0.5f, 0.0f));
    EXPECT_EQ(read->axis, glm::vec3(1.0f, 0.0f, 0.0f));
    EXPECT_THAT(read->limits, ElementsAre(-3.0f, 3.0f));
    EXPECT_EQ(read->joint, neon::No_Joint);
    EXPECT_FALSE(read->failed);
  }
}
