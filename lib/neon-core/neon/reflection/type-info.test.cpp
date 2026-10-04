#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "type-info.hpp"

namespace
{
  using neon::Color;
  using neon::FieldKind;
  using neon::FieldValue;
  using neon::TypeBuilder;
  using neon::TypeInfo;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  enum class Mood
  {
    Calm = 0,
    Angry,
    Asleep
  };

  struct Armor
  {
    float thickness = 1.0f;
    Color tint;
  };

  struct Monster
  {
    bool alive = true;
    int legs = 4;
    float speed = 2.5f;
    std::string name = "unnamed";
    glm::vec3 home{0.0f};
    Color skin;
    std::vector<std::string> sounds;
    Mood mood = Mood::Calm;
    Armor armor;

    // the width of each of its two stripes
    std::vector<float> stripes{1.0f, 1.0f};

    // kept as radians, seen as degrees
    float turn = 0.0f;

    // precise, as seconds that add up are
    double age = 0.0;

    // not described
    int handle = -1;
  };

  // found by TypeInfo::Of through the type it is about
  void Describe(TypeBuilder<Monster> &type)
  {
    type.Named("Monster", "Something to run from");

    type.Field("alive", &Monster::alive);
    type.Field("legs", &Monster::legs).AtLeast(0).AtMost(100);
    type.Field("speed", &Monster::speed).Above(0).Describe("Units per second");
    type.Field("name", &Monster::name).Required();
    type.Field("home", &Monster::home).OneNumberForAll();
    type.Field("skin", &Monster::skin);
    type.Field("sounds", &Monster::sounds);
    type.Choice("mood", &Monster::mood, {"calm", "angry", "asleep"}).AlwaysWritten();

    type.Group("armor", [](TypeBuilder<Monster> &armor)
    {
      armor.Field("thickness", [](Monster &monster) -> float & { return monster.armor.thickness; });
      armor.Field("tint", [](Monster &monster) -> Color & { return monster.armor.tint; });
    }).Describe("What it wears");

    type.Field("stripes", &Monster::stripes).Count(2).Above(0);

    type.Field<float>(
      "turn",
      [](const Monster &monster) { return glm::degrees(monster.turn); },
      [](Monster &monster, const float &degrees) { monster.turn = glm::radians(degrees); });

    type.Field("age", &Monster::age).AtLeast(0);
  }

  struct Ramp
  {
    glm::vec3 size{1.0f};
    float slope = 10.0f;
  };

  void Describe(TypeBuilder<Ramp> &type)
  {
    type.Named("Ramp");
    type.Field("size", &Ramp::size).Above(0);
    type.Field("slope", &Ramp::slope).AtLeast(0).AtMost(90).Unit("degrees");
  }

  struct Lamp
  {
    std::uint32_t lights = 1;
  };

  void Describe(TypeBuilder<Lamp> &type)
  {
    type.Named("Lamp");
    type.Layers("lights", &Lamp::lights);
  }

  struct Nameless
  {
    float value = 0.0f;
  };

  void Describe(TypeBuilder<Nameless> &type)
  {
    type.Field("value", &Nameless::value);
  }

  struct Twice
  {
    float value = 0.0f;
  };

  void Describe(TypeBuilder<Twice> &type)
  {
    type.Named("Twice");
    type.Field("value", &Twice::value);
    type.Field("value", &Twice::value);
  }

  struct Early
  {
    float value = 0.0f;
  };

  void Describe(TypeBuilder<Early> &type)
  {
    type.Named("Early");
    type.Required();
  }

  struct Unsure
  {
    float value = 0.0f;
  };

  void Describe(TypeBuilder<Unsure> &type)
  {
    type.Named("Unsure");
    type.Field("value", &Unsure::value).OnlyWhen("kind", {"one"});
  }

  enum class Size
  {
    Small = 0,
    Large
  };

  struct Misworded
  {
    Size size = Size::Small;
    float value = 0.0f;
  };

  void Describe(TypeBuilder<Misworded> &type)
  {
    type.Named("Misworded");
    type.Choice("size", &Misworded::size, {"small", "large"});
    type.Field("value", &Misworded::value).OnlyWhen("size", {"huge"});
  }

  struct Grouped
  {
    float value = 0.0f;
  };

  void Describe(TypeBuilder<Grouped> &type)
  {
    type.Named("Grouped");
    type.Group("group", [](TypeBuilder<Grouped> &group)
    {
      group.Field("value", &Grouped::value);
      group.Rule([](const Grouped &, const std::string &) { return std::string(); });
    });
  }

  class TypeInfoTest : public ::testing::Test
  {
  protected:
    TypeInfo _type = TypeInfo::Of<Monster>();
    Monster _monster;

    template<typename V>
    V Get(const std::string &path)
    {
      const auto *field = _type.Find(path);
      EXPECT_NE(field, nullptr) << path;
      return std::get<V>(field->get(&_monster));
    }

    void Set(const std::string &path, const FieldValue &value)
    {
      const auto *field = _type.Find(path);
      ASSERT_NE(field, nullptr) << path;
      ASSERT_EQ(field->Check(value, path), "") << path;
      field->set(&_monster, value);
    }

    std::string Check(const std::string &path, const FieldValue &value)
    {
      const auto *field = _type.Find(path);
      EXPECT_NE(field, nullptr) << path;
      return field == nullptr ? "" : field->Check(value, "'" + path + "' of Monster");
    }
  };

  // what is described

  TEST_F(TypeInfoTest, ATypeSaysWhatIsDoneWithAnObjectThatWasWritten)
  {
    struct Counted
    {
      float size = 1.0f;
      int writes = 0;
    };

    neon::TypeBuilder<Counted> builder;
    builder.Named("Counted");
    builder.Field("size", &Counted::size);
    builder.Written([](Counted &counted) { counted.writes++; });
    const TypeInfo type = builder.Build();

    // what writes a field calls it afterwards, with the object
    Counted counted;
    ASSERT_TRUE(static_cast<bool>(type.written));
    type.Find("size")->set(&counted, 2.0f);
    type.written(&counted);
    EXPECT_EQ(counted.size, 2.0f);
    EXPECT_EQ(counted.writes, 1);
  }

  TEST_F(TypeInfoTest, KnowsTheNameAndTheDescriptionOfTheType)
  {
    EXPECT_EQ(_type.name, "Monster");
    EXPECT_EQ(_type.description, "Something to run from");
  }

  TEST_F(TypeInfoTest, KeepsFieldsInTheOrderTheyWereDescribedIn)
  {
    std::vector<std::string> names;
    for (const auto &field : _type.fields) { names.push_back(field.name); }

    EXPECT_THAT(names, ElementsAre(
                  "alive", "legs", "speed", "name", "home", "skin", "sounds", "mood", "armor", "stripes", "turn",
                  "age"));
  }

  TEST_F(TypeInfoTest, DeducesWhatAFieldHoldsFromTheMember)
  {
    EXPECT_EQ(_type.Find("alive")->kind, FieldKind::Boolean);
    EXPECT_EQ(_type.Find("legs")->kind, FieldKind::Integer);
    EXPECT_EQ(_type.Find("speed")->kind, FieldKind::Float);
    EXPECT_EQ(_type.Find("name")->kind, FieldKind::String);
    EXPECT_EQ(_type.Find("home")->kind, FieldKind::Vector3);
    EXPECT_EQ(_type.Find("skin")->kind, FieldKind::Color);
    EXPECT_EQ(_type.Find("sounds")->kind, FieldKind::StringList);
    EXPECT_EQ(_type.Find("mood")->kind, FieldKind::Choice);
    EXPECT_EQ(_type.Find("armor")->kind, FieldKind::Group);
    EXPECT_EQ(_type.Find("stripes")->kind, FieldKind::FloatList);
    EXPECT_EQ(_type.Find("turn")->kind, FieldKind::Float);
    EXPECT_EQ(_type.Find("age")->kind, FieldKind::Double);
  }

  TEST_F(TypeInfoTest, WhatFollowsAFieldIsAboutThatField)
  {
    EXPECT_EQ(_type.Find("speed")->description, "Units per second");
    EXPECT_EQ(_type.Find("speed")->above, 0.0f);
    EXPECT_FALSE(_type.Find("legs")->above.has_value());
    EXPECT_EQ(_type.Find("legs")->at_least, 0.0f);
    EXPECT_EQ(_type.Find("legs")->at_most, 100.0f);
    EXPECT_TRUE(_type.Find("name")->required);
    EXPECT_FALSE(_type.Find("alive")->required);
    EXPECT_TRUE(_type.Find("home")->one_number_for_all);
    EXPECT_TRUE(_type.Find("mood")->always_written);
    EXPECT_EQ(_type.Find("stripes")->count, 2);
    EXPECT_FALSE(_type.Find("sounds")->count.has_value());
  }

  TEST_F(TypeInfoTest, WhatFollowsAGroupIsAboutTheGroup)
  {
    EXPECT_EQ(_type.Find("armor")->description, "What it wears");
    EXPECT_EQ(_type.Find("armor.tint")->description, "");
  }

  TEST_F(TypeInfoTest, KnowsTheWordsOfAChoice)
  {
    EXPECT_THAT(_type.Find("mood")->choices, ElementsAre("calm", "angry", "asleep"));
  }

  TEST_F(TypeInfoTest, FindsAFieldOfAGroupByItsPath)
  {
    ASSERT_NE(_type.Find("armor.thickness"), nullptr);
    EXPECT_EQ(_type.Find("armor.thickness")->kind, FieldKind::Float);
  }

  TEST_F(TypeInfoTest, DoesNotFindWhatWasNotDescribed)
  {
    EXPECT_EQ(_type.Find("handle"), nullptr);
    EXPECT_EQ(_type.Find("armor.weight"), nullptr);
    EXPECT_EQ(_type.Find("thickness"), nullptr);
    EXPECT_EQ(_type.Find("speed.more"), nullptr);
    EXPECT_EQ(_type.Find(""), nullptr);
  }

  TEST_F(TypeInfoTest, ListsEveryFieldThatHoldsAValue)
  {
    EXPECT_THAT(_type.GetPaths(), ElementsAre(
                  "alive", "legs", "speed", "name", "home", "skin", "sounds", "mood",
                  "armor.thickness", "armor.tint", "stripes", "turn", "age"));
  }

  // reading

  TEST_F(TypeInfoTest, ReadsEveryKindOfField)
  {
    _monster.alive = false;
    _monster.legs = 6;
    _monster.speed = 3.5f;
    _monster.name = "Grendel";
    _monster.home = {1.0f, 2.0f, 3.0f};
    _monster.skin = {0.1f, 0.2f, 0.3f, 0.4f};
    _monster.sounds = {"growl", "hiss"};
    _monster.mood = Mood::Angry;
    _monster.armor.thickness = 2.0f;
    _monster.stripes = {0.5f, 0.25f};
    _monster.age = 1234.56789012345;

    EXPECT_FALSE(Get<bool>("alive"));
    EXPECT_EQ(Get<int>("legs"), 6);
    EXPECT_EQ(Get<float>("speed"), 3.5f);
    EXPECT_EQ(Get<std::string>("name"), "Grendel");
    EXPECT_EQ(Get<glm::vec3>("home"), glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(Get<Color>("skin").a, 0.4f);
    EXPECT_THAT(Get<std::vector<std::string>>("sounds"), ElementsAre("growl", "hiss"));
    EXPECT_EQ(Get<std::string>("mood"), "angry");
    EXPECT_EQ(Get<float>("armor.thickness"), 2.0f);
    EXPECT_THAT(Get<std::vector<float>>("stripes"), ElementsAre(0.5f, 0.25f));
    EXPECT_EQ(Get<double>("age"), 1234.56789012345);
  }

  TEST_F(TypeInfoTest, ReadsAFieldThatIsKeptAsSomethingElse)
  {
    _monster.turn = glm::radians(90.0f);

    EXPECT_FLOAT_EQ(Get<float>("turn"), 90.0f);
  }

  TEST_F(TypeInfoTest, ReadsFromAnObjectThatMustNotChange)
  {
    const Monster monster;

    EXPECT_EQ(std::get<float>(_type.Find("speed")->get(&monster)), 2.5f);
  }

  // changing

  TEST_F(TypeInfoTest, ChangesEveryKindOfField)
  {
    Set("alive", false);
    Set("legs", 8);
    Set("speed", 9.0f);
    Set("name", std::string("Smaug"));
    Set("home", glm::vec3{4.0f, 5.0f, 6.0f});
    Set("skin", Color{1.0f, 0.0f, 0.0f, 0.5f});
    Set("sounds", std::vector<std::string>{"roar"});
    Set("mood", std::string("asleep"));
    Set("armor.tint", Color{0.0f, 1.0f, 0.0f, 1.0f});
    Set("stripes", std::vector{2.0f, 3.0f});
    Set("turn", 180.0f);
    Set("age", 1234.56789012345);

    EXPECT_FALSE(_monster.alive);
    EXPECT_EQ(_monster.legs, 8);
    EXPECT_EQ(_monster.speed, 9.0f);
    EXPECT_EQ(_monster.name, "Smaug");
    EXPECT_EQ(_monster.home, glm::vec3(4.0f, 5.0f, 6.0f));
    EXPECT_EQ(_monster.skin.a, 0.5f);
    EXPECT_THAT(_monster.sounds, ElementsAre("roar"));
    EXPECT_EQ(_monster.mood, Mood::Asleep);
    EXPECT_EQ(_monster.armor.tint.g, 1.0f);
    EXPECT_THAT(_monster.stripes, ElementsAre(2.0f, 3.0f));
    EXPECT_FLOAT_EQ(_monster.turn, glm::radians(180.0f));
    EXPECT_EQ(_monster.age, 1234.56789012345);
  }

  TEST_F(TypeInfoTest, LeavesWhatIsNotDescribedAlone)
  {
    _monster.handle = 7;

    Set("legs", 2);

    EXPECT_EQ(_monster.handle, 7);
  }

  // what cannot be taken

  TEST_F(TypeInfoTest, AcceptsAValueOfTheKindOfTheField)
  {
    EXPECT_EQ(Check("speed", 1.0f), "");
    EXPECT_EQ(Check("mood", std::string("calm")), "");
    EXPECT_EQ(Check("name", std::string("x")), "");
  }

  TEST_F(TypeInfoTest, RefusesAValueOfAnotherKind)
  {
    EXPECT_EQ(Check("speed", std::string("fast")), "'speed' of Monster takes a number");
    EXPECT_EQ(Check("speed", 3), "'speed' of Monster takes a number");
    EXPECT_EQ(Check("legs", 4.0f), "'legs' of Monster takes a whole number");
    EXPECT_EQ(Check("alive", 1), "'alive' of Monster takes true or false");
    EXPECT_EQ(Check("home", 1.0f), "'home' of Monster takes three numbers");
    EXPECT_EQ(Check("name", FieldValue{}), "'name' of Monster takes text");
    EXPECT_EQ(Check("stripes", 1.0f), "'stripes' of Monster takes a list of numbers");

    // a precise number is not a number of single precision, and the other way
    EXPECT_EQ(Check("age", 1.0f), "'age' of Monster takes a number");
    EXPECT_EQ(Check("speed", 1.0), "'speed' of Monster takes a number");
  }

  TEST_F(TypeInfoTest, ChecksAPreciseNumberAgainstItsLimitsAsADouble)
  {
    EXPECT_EQ(Check("age", 0.0), "");
    EXPECT_EQ(Check("age", 1.0e-300), "");
    EXPECT_EQ(Check("age", -0.5), "'age' of Monster is -0.5, where a number of 0 or above was expected");
  }

  TEST_F(TypeInfoTest, RefusesAListOfNumbersThatHoldsNotAsManyAsItHasTo)
  {
    EXPECT_EQ(Check("stripes", std::vector{1.0f}), "'stripes' of Monster holds 1 number, where 2 were expected");
    EXPECT_EQ(Check("stripes", std::vector{1.0f, 2.0f, 3.0f}), "'stripes' of Monster holds 3 numbers, where 2 were expected");
    EXPECT_EQ(Check("stripes", std::vector{1.0f, 2.0f}), "");
  }

  TEST_F(TypeInfoTest, RefusesAListOfNumbersWhereOneIsNotWhatItHasToBe)
  {
    EXPECT_EQ(
      Check("stripes", std::vector{1.0f, 0.0f}),
      "'stripes' of Monster is [1, 0], where numbers above 0 were expected");
    EXPECT_EQ(
      Check("stripes", std::vector{-1.0f, 1.0f}),
      "'stripes' of Monster is [-1, 1], where numbers above 0 were expected");
  }

  TEST_F(TypeInfoTest, RefusesAWordThatIsNotAmongTheChoices)
  {
    EXPECT_EQ(
      Check("mood", std::string("hungry")),
      "'mood' of Monster is 'hungry', where one of these was expected: calm, angry, asleep");
  }

  TEST_F(TypeInfoTest, RefusesANumberThatIsNotAboveWhatItHasToBeAbove)
  {
    EXPECT_EQ(Check("speed", 0.0f), "'speed' of Monster is 0, where a number above 0 was expected");
    EXPECT_EQ(Check("speed", -1.0f), "'speed' of Monster is -1, where a number above 0 was expected");
    EXPECT_EQ(Check("speed", 0.001f), "");
  }

  TEST_F(TypeInfoTest, RefusesANumberThatIsNotANumber)
  {
    EXPECT_EQ(Check("speed", std::nanf("")), "'speed' of Monster is nan, where a number above 0 was expected");
  }

  TEST_F(TypeInfoTest, RefusesAWholeNumberOutsideOfWhatItMayBe)
  {
    EXPECT_EQ(Check("legs", -1), "'legs' of Monster is -1, where a number from 0 to 100 was expected");
    EXPECT_EQ(Check("legs", 101), "'legs' of Monster is 101, where a number from 0 to 100 was expected");
    EXPECT_EQ(Check("legs", 0), "");
    EXPECT_EQ(Check("legs", 100), "");
  }

  TEST(TypeInfo, RefusesAVectorWhereOneNumberIsNotWhatItHasToBe)
  {
    const auto type = TypeInfo::Of<Ramp>();
    const auto *size = type.Find("size");

    EXPECT_EQ(
      size->Check(glm::vec3{1.0f, 0.0f, 1.0f}, "'size' of Ramp"),
      "'size' of Ramp is [1, 0, 1], where numbers above 0 were expected");
    EXPECT_EQ(size->Check(glm::vec3{0.5f}, "'size' of Ramp"), "");
  }

  TEST(TypeInfo, NamesWhatANumberCountsWhenItRefusesIt)
  {
    const auto type = TypeInfo::Of<Ramp>();
    const auto *slope = type.Find("slope");

    EXPECT_EQ(slope->unit, "degrees");
    EXPECT_EQ(
      slope->Check(120.0f, "'slope' of Ramp"),
      "'slope' of Ramp is 120, where degrees from 0 to 90 were expected");
    EXPECT_EQ(slope->Check(90.0f, "'slope' of Ramp"), "");
  }

  TEST(TypeInfo, SeesLayersAsTheirNumbers)
  {
    const auto type = TypeInfo::Of<Lamp>();
    const auto *lights = type.Find("lights");
    Lamp lamp;
    lamp.lights = 0x80000005u;

    EXPECT_EQ(lights->kind, FieldKind::Layers);
    EXPECT_THAT(std::get<std::vector<float>>(lights->get(&lamp)), ElementsAre(1.0f, 3.0f, 32.0f));

    lights->set(&lamp, std::vector{2.0f, 4.0f});
    EXPECT_EQ(lamp.lights, 0b1010u);

    lights->set(&lamp, std::vector<float>{});
    EXPECT_EQ(lamp.lights, 0u);
  }

  TEST(TypeInfo, RefusesANumberThatIsNoLayer)
  {
    const auto type = TypeInfo::Of<Lamp>();
    const auto *lights = type.Find("lights");

    EXPECT_EQ(lights->Check(std::vector{1.0f, 33.0f}, "'lights' of Lamp"),
              "'lights' of Lamp holds 33, where a layer from 1 to 32 was expected");
    EXPECT_EQ(lights->Check(std::vector{0.0f}, "'lights' of Lamp"),
              "'lights' of Lamp holds 0, where a layer from 1 to 32 was expected");
    EXPECT_EQ(lights->Check(std::vector{1.5f}, "'lights' of Lamp"),
              "'lights' of Lamp holds 1.5, where a layer from 1 to 32 was expected");
    EXPECT_EQ(lights->Check(2.0f, "'lights' of Lamp"), "'lights' of Lamp takes a list of layers");
    EXPECT_EQ(lights->Check(std::vector{1.0f, 32.0f}, "'lights' of Lamp"), "");
  }

  TEST_F(TypeInfoTest, RefusesAnEmptyTextWhereOneIsRequired)
  {
    EXPECT_EQ(Check("name", std::string()), "'name' of Monster must not be empty");
  }

  TEST_F(TypeInfoTest, RefusesAValueForAGroup)
  {
    EXPECT_EQ(Check("armor", 1.0f), "'armor' of Monster is a group and holds no value of its own");
  }

  // descriptions that are wrong

  TEST(TypeBuilder, RefusesADescriptionThatDoesNotNameItsType)
  {
    EXPECT_THROW((void) TypeInfo::Of<Nameless>(), std::logic_error);
  }

  TEST(TypeBuilder, RefusesAFieldThatIsDescribedTwice)
  {
    EXPECT_THROW((void) TypeInfo::Of<Twice>(), std::logic_error);
  }

  TEST(TypeBuilder, RefusesWhatIsAboutAFieldBeforeThereIsOne)
  {
    EXPECT_THROW((void) TypeInfo::Of<Early>(), std::logic_error);
  }

  TEST(TypeBuilder, RefusesAConditionOnWhatIsNoChoiceDescribedBefore)
  {
    EXPECT_THROW((void) TypeInfo::Of<Unsure>(), std::logic_error);
  }

  TEST(TypeBuilder, RefusesAConditionOnAWordTheChoiceDoesNotHave)
  {
    EXPECT_THROW((void) TypeInfo::Of<Misworded>(), std::logic_error);
  }

  TEST(TypeBuilder, RefusesARuleInAGroup)
  {
    EXPECT_THROW((void) TypeInfo::Of<Grouped>(), std::logic_error);
  }

  // values

  TEST(FieldValue, TwoValuesOfDifferentKindsAreNotTheSame)
  {
    EXPECT_FALSE(neon::Same(FieldValue{1}, FieldValue{1.0f}));
    EXPECT_FALSE(neon::Same(FieldValue{}, FieldValue{false}));
  }

  TEST(FieldValue, TwoColorsAreTheSameWhenAllOfTheirNumbersAre)
  {
    EXPECT_TRUE(neon::Same(FieldValue{Color{0.1f, 0.2f, 0.3f, 0.4f}}, FieldValue{Color{0.1f, 0.2f, 0.3f, 0.4f}}));
    EXPECT_FALSE(neon::Same(FieldValue{Color{0.1f, 0.2f, 0.3f, 0.4f}}, FieldValue{Color{0.1f, 0.2f, 0.3f, 0.5f}}));
  }

  TEST(FieldValue, TwoListsAreTheSameWhenTheirTextsAreInTheSameOrder)
  {
    using Texts = std::vector<std::string>;

    EXPECT_TRUE(neon::Same(FieldValue{Texts{"a", "b"}}, FieldValue{Texts{"a", "b"}}));
    EXPECT_FALSE(neon::Same(FieldValue{Texts{"a", "b"}}, FieldValue{Texts{"b", "a"}}));
    EXPECT_TRUE(neon::Same(FieldValue{Texts{}}, FieldValue{Texts{}}));
  }

  TEST(FieldValue, TwoListsAreTheSameWhenTheirNumbersAreInTheSameOrder)
  {
    using Numbers = std::vector<float>;

    EXPECT_TRUE(neon::Same(FieldValue{Numbers{1.0f, 2.0f}}, FieldValue{Numbers{1.0f, 2.0f}}));
    EXPECT_FALSE(neon::Same(FieldValue{Numbers{1.0f, 2.0f}}, FieldValue{Numbers{2.0f, 1.0f}}));
    EXPECT_FALSE(neon::Same(FieldValue{Numbers{1.0f}}, FieldValue{Numbers{1.0f, 1.0f}}));
  }

  TEST(FieldValue, TwoValuesThatHoldNothingAreTheSame)
  {
    EXPECT_TRUE(neon::Same(FieldValue{}, FieldValue{}));
  }

  TEST(FieldValue, ATextIsBothATextAndAChoice)
  {
    EXPECT_TRUE(neon::Holds(FieldValue{std::string("x")}, FieldKind::String));
    EXPECT_TRUE(neon::Holds(FieldValue{std::string("x")}, FieldKind::Choice));
    EXPECT_FALSE(neon::Holds(FieldValue{std::string("x")}, FieldKind::Float));
  }

  TEST(FieldValue, NothingIsAGroup)
  {
    EXPECT_FALSE(neon::Holds(FieldValue{}, FieldKind::Group));
    EXPECT_FALSE(neon::Holds(FieldValue{1.0f}, FieldKind::Group));
  }
}
