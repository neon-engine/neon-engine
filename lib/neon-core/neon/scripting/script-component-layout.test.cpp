#include "script-component-layout.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/data-reader.hpp>
#include <neon/testing/fake-entity-store.hpp>

namespace
{
  using neon::Color;
  using neon::ComponentId;
  using neon::DataReader;
  using neon::DataValue;
  using neon::Entity;
  using neon::FieldKind;
  using neon::FieldValue;
  using neon::ScriptComponentLayout;
  using neon::ScriptField;
  using neon::testing::FakeEntityStore;
  using ::testing::ElementsAre;
  using ::testing::HasSubstr;

  /// The fields of a door, as a script would declare them.
  std::vector<ScriptField> door_fields()
  {
    return {
      {"open", FieldKind::Boolean, false, "Whether it stands open"},
      {"speed", FieldKind::Float, 2.0f, ""},
      {"count", FieldKind::Integer, 3, ""},
      {"sound", FieldKind::String, std::string("assets://sounds/door.wav"), ""},
      {"at", FieldKind::Vector3, glm::vec3{1.0f, 2.0f, 3.0f}, ""},
      {"tint", FieldKind::Color, Color{0.5f, 0.25f, 1.0f, 1.0f}, ""},
      {"seconds", FieldKind::Double, 0.5, ""},
    };
  }

  TEST(ScriptComponentLayoutTest, LaysTheFieldsOutAtTheirAlignmentsAndDescribesThem)
  {
    const ScriptComponentLayout layout("Door", "A door", door_fields());

    EXPECT_EQ(layout.GetProblem(), "");
    EXPECT_EQ(layout.GetName(), "Door");
    EXPECT_EQ(layout.GetAlignment(), alignof(std::string) > alignof(double) ? alignof(std::string) : alignof(double));
    EXPECT_EQ(layout.GetSize() % layout.GetAlignment(), 0u);

    const auto type = layout.GetTypeInfo();
    EXPECT_EQ(type->name, "Door");
    EXPECT_EQ(type->description, "A door");
    EXPECT_THAT(type->GetPaths(), ElementsAre("open", "speed", "count", "sound", "at", "tint", "seconds"));
    EXPECT_EQ(type->Find("open")->description, "Whether it stands open");
    EXPECT_EQ(type->Find("sound")->kind, FieldKind::String);
  }

  TEST(ScriptComponentLayoutTest, ConstructsTheDefaultsAndReadsAndChangesEveryField)
  {
    const ScriptComponentLayout layout("Door", "", door_fields());
    const auto info = layout.GetComponentInfo();
    const auto type = layout.GetTypeInfo();

    std::vector<std::max_align_t> memory(layout.GetSize() / sizeof(std::max_align_t) + 1);
    info.construct(memory.data(), 1);

    EXPECT_TRUE(neon::Same(type->Find("open")->get(memory.data()), FieldValue{false}));
    EXPECT_TRUE(neon::Same(type->Find("speed")->get(memory.data()), FieldValue{2.0f}));
    EXPECT_TRUE(neon::Same(type->Find("count")->get(memory.data()), FieldValue{3}));
    EXPECT_TRUE(neon::Same(type->Find("sound")->get(memory.data()), FieldValue{std::string("assets://sounds/door.wav")}));
    EXPECT_EQ(std::get<glm::vec3>(type->Find("at")->get(memory.data())), (glm::vec3{1.0f, 2.0f, 3.0f}));
    EXPECT_EQ(std::get<Color>(type->Find("tint")->get(memory.data())).g, 0.25f);
    EXPECT_TRUE(neon::Same(type->Find("seconds")->get(memory.data()), FieldValue{0.5}));

    type->Find("open")->set(memory.data(), true);
    type->Find("sound")->set(memory.data(), std::string("none"));
    type->Find("at")->set(memory.data(), glm::vec3{9.0f});
    EXPECT_TRUE(neon::Same(type->Find("open")->get(memory.data()), FieldValue{true}));
    EXPECT_TRUE(neon::Same(type->Find("sound")->get(memory.data()), FieldValue{std::string("none")}));
    EXPECT_EQ(std::get<glm::vec3>(type->Find("at")->get(memory.data())), glm::vec3{9.0f});

    // a value of another type is left out, not written over the field
    type->Find("speed")->set(memory.data(), std::string("fast"));
    EXPECT_TRUE(neon::Same(type->Find("speed")->get(memory.data()), FieldValue{2.0f}));

    info.destruct(memory.data(), 1);
  }

  TEST(ScriptComponentLayoutTest, CopiesAndMovesSeveralComponentsAtOnce)
  {
    const ScriptComponentLayout layout("Door", "", door_fields());
    const auto info = layout.GetComponentInfo();
    const auto type = layout.GetTypeInfo();

    const std::size_t stride = layout.GetSize();
    std::vector<std::max_align_t> from(2 * stride / sizeof(std::max_align_t) + 1);
    std::vector<std::max_align_t> to(2 * stride / sizeof(std::max_align_t) + 1);
    info.construct(from.data(), 2);
    info.construct(to.data(), 2);

    auto *second = static_cast<char *>(static_cast<void *>(from.data())) + stride;
    type->Find("sound")->set(second, std::string("second"));
    type->Find("count")->set(second, 7);

    info.copy(to.data(), from.data(), 2);
    auto *copied = static_cast<char *>(static_cast<void *>(to.data())) + stride;
    EXPECT_TRUE(neon::Same(type->Find("sound")->get(copied), FieldValue{std::string("second")}));
    EXPECT_TRUE(neon::Same(type->Find("count")->get(copied), FieldValue{7}));
    EXPECT_TRUE(neon::Same(type->Find("sound")->get(second), FieldValue{std::string("second")}));

    info.move(from.data(), to.data(), 2);
    EXPECT_TRUE(neon::Same(type->Find("sound")->get(second), FieldValue{std::string("second")}));

    info.destruct(from.data(), 2);
    info.destruct(to.data(), 2);
  }

  TEST(ScriptComponentLayoutTest, AComponentWithoutFieldsMarksAnEntity)
  {
    const ScriptComponentLayout layout("Enemy", "", {});
    EXPECT_EQ(layout.GetProblem(), "");
    EXPECT_EQ(layout.GetSize(), 1u);
    EXPECT_EQ(layout.GetAlignment(), 1u);
  }

  TEST(ScriptComponentLayoutTest, RefusesWhatAFieldCannotBe)
  {
    EXPECT_THAT(
      ScriptComponentLayout("Door", "", {{"9lives", FieldKind::Float, 1.0f, ""}}).GetProblem(),
      HasSubstr("The field '9lives' needs a name of letters, digits, and underscores"));
    EXPECT_THAT(
      ScriptComponentLayout("Door", "", {{"a", FieldKind::Float, 1.0f, ""}, {"a", FieldKind::Boolean, true, ""}}).GetProblem(),
      HasSubstr("The field 'a' is declared twice"));
    EXPECT_THAT(
      ScriptComponentLayout("Door", "", {{"names", FieldKind::StringList, std::vector<std::string>{}, ""}}).GetProblem(),
      HasSubstr("The field 'names' holds a list of texts, which a script's component cannot"));
    EXPECT_THAT(
      ScriptComponentLayout("Door", "", {{"speed", FieldKind::Float, std::string("fast"), ""}}).GetProblem(),
      HasSubstr("The default of 'speed' is not a number"));
  }

  TEST(ScriptComponentLayoutTest, ReadsARecipeOntoTheEntityAndWritesBackWhatDiffers)
  {
    FakeEntityStore store;
    store.Initialize();

    const ScriptComponentLayout layout("Door", "", door_fields());
    const ComponentId id = store.RegisterComponent(layout.GetComponentInfo());
    const auto format = layout.GetComponentFormat();
    const Entity entity = store.CreateEntity("lab-door");

    auto map = DataValue::Map();
    map.Set("speed", DataValue::Number(3.5f));
    map.Set("open", DataValue::Bool(true));
    std::vector<std::string> errors;
    format.read(DataReader(map, "lab.scene.yml", "Door of entity 'lab-door'", errors), store, entity);

    EXPECT_THAT(errors, ::testing::IsEmpty());
    void *object = store.GetComponent(entity, id);
    ASSERT_NE(object, nullptr);
    EXPECT_TRUE(neon::Same(layout.GetTypeInfo()->Find("speed")->get(object), FieldValue{3.5f}));
    EXPECT_TRUE(neon::Same(layout.GetTypeInfo()->Find("open")->get(object), FieldValue{true}));
    EXPECT_TRUE(neon::Same(layout.GetTypeInfo()->Find("count")->get(object), FieldValue{3}));

    // reading again starts from what the entity has
    auto more = DataValue::Map();
    more.Set("count", DataValue::Number(9));
    format.read(DataReader(more, "lab.scene.yml", "Door of entity 'lab-door'", errors), store, entity);
    object = store.GetComponent(entity, id);
    EXPECT_TRUE(neon::Same(layout.GetTypeInfo()->Find("speed")->get(object), FieldValue{3.5f}));
    EXPECT_TRUE(neon::Same(layout.GetTypeInfo()->Find("count")->get(object), FieldValue{9}));

    DataValue written;
    EXPECT_TRUE(format.write(store, entity, written));
    EXPECT_NE(written.Find("speed"), nullptr);
    EXPECT_NE(written.Find("open"), nullptr);
    EXPECT_NE(written.Find("count"), nullptr);
    EXPECT_EQ(written.Find("sound"), nullptr);

    EXPECT_FALSE(format.write(store, store.CreateEntity("without"), written));
  }

  TEST(ScriptComponentLayoutTest, LaysOutEveryKindOfNumberAndVector)
  {
    const ScriptComponentLayout layout("Gauge", "", {
      {"level", FieldKind::Byte, std::uint8_t{7}, ""},
      {"mark", FieldKind::Char, 'a', ""},
      {"offset", FieldKind::Short, std::int16_t{-3}, ""},
      {"slots", FieldKind::UnsignedShort, std::uint16_t{4}, ""},
      {"flags", FieldKind::UnsignedInteger, std::uint32_t{5}, ""},
      {"ticks", FieldKind::Long, std::int64_t{-6}, ""},
      {"total", FieldKind::UnsignedLong, std::uint64_t{8}, ""},
      {"size", FieldKind::Vector2, glm::vec2{1.0f, 2.0f}, ""},
      {"tint", FieldKind::Vector4, glm::vec4{1.0f}, ""},
      {"cell", FieldKind::IntegerVector2, glm::ivec2{1, 2}, ""},
      {"voxel", FieldKind::IntegerVector3, glm::ivec3{1, 2, 3}, ""},
    });
    ASSERT_EQ(layout.GetProblem(), "");
    EXPECT_EQ(layout.GetAlignment(), alignof(glm::vec4) > alignof(std::int64_t) ? alignof(glm::vec4) : alignof(std::int64_t));

    const auto info = layout.GetComponentInfo();
    const auto type = layout.GetTypeInfo();
    std::vector<std::max_align_t> from(layout.GetSize() / sizeof(std::max_align_t) + 1);
    std::vector<std::max_align_t> to(layout.GetSize() / sizeof(std::max_align_t) + 1);
    info.construct(from.data(), 1);
    info.construct(to.data(), 1);

    EXPECT_TRUE(neon::Same(type->Find("level")->get(from.data()), FieldValue{std::uint8_t{7}}));
    EXPECT_TRUE(neon::Same(type->Find("ticks")->get(from.data()), FieldValue{std::int64_t{-6}}));
    EXPECT_TRUE(neon::Same(type->Find("voxel")->get(from.data()), FieldValue{glm::ivec3{1, 2, 3}}));

    type->Find("level")->set(from.data(), std::uint8_t{200});
    type->Find("mark")->set(from.data(), 'q');
    type->Find("size")->set(from.data(), glm::vec2{5.0f, 6.0f});
    type->Find("total")->set(from.data(), std::uint64_t{1} << 40);
    info.copy(to.data(), from.data(), 1);

    EXPECT_TRUE(neon::Same(type->Find("level")->get(to.data()), FieldValue{std::uint8_t{200}}));
    EXPECT_TRUE(neon::Same(type->Find("mark")->get(to.data()), FieldValue{'q'}));
    EXPECT_TRUE(neon::Same(type->Find("size")->get(to.data()), FieldValue{glm::vec2{5.0f, 6.0f}}));
    EXPECT_TRUE(neon::Same(type->Find("total")->get(to.data()), FieldValue{std::uint64_t{1} << 40}));

    info.destruct(from.data(), 1);
    info.destruct(to.data(), 1);
  }

  TEST(ScriptComponentLayoutTest, StillRefusesTheListsAScriptCannotHold)
  {
    EXPECT_THAT(
      ScriptComponentLayout("Door", "", {{"steps", FieldKind::IntegerList, std::vector<int>{}, ""}}).GetProblem(),
      HasSubstr("holds a list of whole numbers, which a script's component cannot"));
  }

  TEST(ScriptComponentLayoutTest, LaysOutAQuaternionAndTheMatrices)
  {
    const ScriptComponentLayout layout("Frame", "", {
      {"turn", FieldKind::Quaternion, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}, ""},
      {"basis", FieldKind::Matrix3, glm::mat3{2.0f}, ""},
      {"place", FieldKind::Matrix4, glm::mat4{1.0f}, ""},
    });
    ASSERT_EQ(layout.GetProblem(), "");

    const auto info = layout.GetComponentInfo();
    const auto type = layout.GetTypeInfo();
    std::vector<std::max_align_t> memory(layout.GetSize() / sizeof(std::max_align_t) + 1);
    info.construct(memory.data(), 1);

    EXPECT_TRUE(neon::Same(type->Find("basis")->get(memory.data()), FieldValue{glm::mat3{2.0f}}));
    type->Find("place")->set(memory.data(), glm::mat4{3.0f});
    EXPECT_TRUE(neon::Same(type->Find("place")->get(memory.data()), FieldValue{glm::mat4{3.0f}}));

    info.destruct(memory.data(), 1);
  }
}
