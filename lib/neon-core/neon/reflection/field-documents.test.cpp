#include <format>
#include <cstdint>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "field-documents.hpp"

namespace
{
  using neon::Color;
  using neon::DataReader;
  using neon::DataValue;
  using neon::TypeBuilder;
  using neon::TypeInfo;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  enum class Fuel
  {
    Petrol = 0,
    Diesel,
    Electric
  };

  struct Car
  {
    bool locked = false;
    int seats = 5;
    float top_speed = 180.0f;
    std::string plate;
    glm::vec3 size{1.0f};
    Color paint;
    std::vector<std::string> owners;
    Fuel fuel = Fuel::Petrol;
    float tire_pressure = 2.2f;
    std::string tire_brand = "none";
    std::vector<float> gears;
    double odometer = 0.0;
  };

  void Describe(TypeBuilder<Car> &type)
  {
    type.Named("Car");
    type.Field("locked", &Car::locked);
    type.Field("seats", &Car::seats).AtLeast(1);
    type.Field("top_speed", &Car::top_speed).Above(0);
    type.Field("plate", &Car::plate).Required();
    type.Field("size", &Car::size).OneNumberForAll();
    type.Field("paint", &Car::paint);
    type.Field("owners", &Car::owners);
    type.Choice("fuel", &Car::fuel, {"petrol", "diesel", "electric"}).AlwaysWritten();

    type.Group("tires", [](TypeBuilder<Car> &tires)
    {
      tires.Field("pressure", &Car::tire_pressure).Above(0);
      tires.Field("brand", &Car::tire_brand);
    });

    type.Field("gears", &Car::gears).Above(0);
    type.Field("odometer", &Car::odometer).AtLeast(0);
  }

  DataValue Numbers(const std::vector<float> &numbers)
  {
    auto list = DataValue::List();
    for (const float number : numbers) { list.Add(DataValue::Number(number)); }
    return list;
  }

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

  class FieldDocumentsTest : public ::testing::Test
  {
  protected:
    TypeInfo _type = TypeInfo::Of<Car>();
    std::vector<std::string> _errors;
    Car _car;

    void Read(const DataValue &map)
    {
      const DataReader reader(map, "cars.yml", "Car of entity 'taxi'", _errors);
      neon::ReadFields(_type, reader, &_car);
      reader.Finish();
    }

    /// A map that holds what a car needs, and nothing else.
    static DataValue Plated()
    {
      auto map = DataValue::Map();
      map.Set("plate", DataValue::Text("NEON 1"));
      return map;
    }

    DataValue Write() const
    {
      const Car standard;
      auto map = DataValue::Map();
      neon::WriteFields(_type, &_car, &standard, map);
      return map;
    }
  };

  // reading

  TEST_F(FieldDocumentsTest, ReadsEveryKindOfField)
  {
    auto tires = DataValue::Map();
    tires.Set("pressure", DataValue::Number(2.5f));
    tires.Set("brand", DataValue::Text("Grip"));

    auto owners = DataValue::List();
    owners.Add(DataValue::Text("Ada"));
    owners.Add(DataValue::Text("Bo"));

    auto map = Plated();
    map.Set("locked", DataValue::Bool(true));
    map.Set("seats", DataValue::Number(2));
    map.Set("top_speed", DataValue::Number(250.0f));
    map.Set("size", Numbers({4.0f, 1.5f, 1.8f}));
    map.Set("paint", Numbers({1.0f, 0.0f, 0.0f, 0.5f}));
    map.Set("owners", owners);
    map.Set("fuel", DataValue::Text("electric"));
    map.Set("tires", tires);
    map.Set("gears", Numbers({3.5f, 2.1f, 1.4f}));
    map.Set("odometer", DataValue::Number(123456.789012345));

    Read(map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_TRUE(_car.locked);
    EXPECT_EQ(_car.seats, 2);
    EXPECT_EQ(_car.top_speed, 250.0f);
    EXPECT_EQ(_car.plate, "NEON 1");
    EXPECT_EQ(_car.size, glm::vec3(4.0f, 1.5f, 1.8f));
    EXPECT_EQ(_car.paint.a, 0.5f);
    EXPECT_THAT(_car.owners, ElementsAre("Ada", "Bo"));
    EXPECT_EQ(_car.fuel, Fuel::Electric);
    EXPECT_EQ(_car.tire_pressure, 2.5f);
    EXPECT_EQ(_car.tire_brand, "Grip");
    EXPECT_THAT(_car.gears, ElementsAre(3.5f, 2.1f, 1.4f));
    EXPECT_EQ(_car.odometer, 123456.789012345);
  }

  TEST_F(FieldDocumentsTest, WhatIsNotWrittenKeepsItsValue)
  {
    Read(Plated());

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_car.seats, 5);
    EXPECT_EQ(_car.top_speed, 180.0f);
    EXPECT_EQ(_car.tire_brand, "none");
  }

  TEST_F(FieldDocumentsTest, ANameWithNothingBehindItCountsAsNotWritten)
  {
    auto map = Plated();
    map.Set("seats", DataValue{});

    Read(map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_car.seats, 5);
  }

  TEST_F(FieldDocumentsTest, OneNumberStandsForAllThreeWhereTheFieldAllowsIt)
  {
    auto map = Plated();
    map.Set("size", DataValue::Number(2.0f));

    Read(map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_car.size, glm::vec3(2.0f));
  }

  TEST_F(FieldDocumentsTest, ANameThatIsNotDescribedIsReportedWithThoseThatAre)
  {
    auto map = Plated();
    map.Set("wings", DataValue::Number(2));

    Read(map);

    EXPECT_THAT(_errors, ElementsAre(
                  "cars.yml: 'wings' is not known to Car of entity 'taxi'. Known are: "
                  "locked, seats, top_speed, plate, size, paint, owners, fuel, tires, gears, odometer"));
  }

  TEST_F(FieldDocumentsTest, ANameInAGroupThatIsNotDescribedIsReported)
  {
    auto tires = DataValue::Map();
    tires.Set("colour", DataValue::Text("black"));
    auto map = Plated();
    map.Set("tires", tires);

    Read(map);

    EXPECT_THAT(_errors, ElementsAre(
                  "cars.yml: 'colour' is not known to 'tires' of Car of entity 'taxi'. "
                  "Known are: pressure, brand"));
  }

  TEST_F(FieldDocumentsTest, AGroupThatIsNotAMapIsReported)
  {
    auto map = Plated();
    map.Set("tires", DataValue::Number(4));

    Read(map);

    EXPECT_THAT(_errors, ElementsAre(
                  "cars.yml: 'tires' of Car of entity 'taxi' is a number, where a map was expected"));
    EXPECT_EQ(_car.tire_pressure, 2.2f);
  }

  TEST_F(FieldDocumentsTest, AValueOfTheWrongKindIsReportedAndLeavesTheFieldAlone)
  {
    auto map = Plated();
    map.Set("top_speed", DataValue::Text("fast"));

    Read(map);

    EXPECT_THAT(_errors, ElementsAre(
                  "cars.yml: 'top_speed' of Car of entity 'taxi' is text, where a number was expected"));
    EXPECT_EQ(_car.top_speed, 180.0f);
  }

  TEST_F(FieldDocumentsTest, ANumberWithAFractionIsReportedWhereAWholeOneIsExpected)
  {
    auto map = Plated();
    map.Set("seats", DataValue::Number(4.5f));

    Read(map);

    EXPECT_THAT(_errors, ElementsAre(
                  "cars.yml: 'seats' of Car of entity 'taxi' is 4.5, where a whole number was expected"));
    EXPECT_EQ(_car.seats, 5);
  }

  TEST_F(FieldDocumentsTest, ANumberThatIsNotAboveWhatItHasToBeAboveIsReported)
  {
    auto map = Plated();
    map.Set("top_speed", DataValue::Number(0.0f));

    Read(map);

    EXPECT_THAT(_errors, ElementsAre(
                  "cars.yml: 'top_speed' of Car of entity 'taxi' is 0, where a number above 0 was expected"));
    EXPECT_EQ(_car.top_speed, 180.0f);
  }

  TEST_F(FieldDocumentsTest, ANumberInAGroupIsCheckedAsWell)
  {
    auto tires = DataValue::Map();
    tires.Set("pressure", DataValue::Number(-1.0f));
    auto map = Plated();
    map.Set("tires", tires);

    Read(map);

    EXPECT_THAT(_errors, ElementsAre(
                  "cars.yml: 'pressure' of 'tires' of Car of entity 'taxi' is -1, where a number above 0 was expected"));
  }

  TEST_F(FieldDocumentsTest, AWholeNumberBelowWhatItMayBeIsReported)
  {
    auto map = Plated();
    map.Set("seats", DataValue::Number(0));

    Read(map);

    EXPECT_THAT(_errors, ElementsAre(
                  "cars.yml: 'seats' of Car of entity 'taxi' is 0, where a number of 1 or above was expected"));
  }

  TEST_F(FieldDocumentsTest, EveryNumberOfAListIsCheckedAndTheListIsLeftAlone)
  {
    _car.gears = {1.0f};
    auto map = Plated();
    map.Set("gears", Numbers({3.5f, 0.0f}));

    Read(map);

    EXPECT_THAT(_errors, ElementsAre(
                  "cars.yml: 'gears' of Car of entity 'taxi' is [3.5, 0], where numbers above 0 were expected"));
    EXPECT_THAT(_car.gears, ElementsAre(1.0f));
  }

  TEST_F(FieldDocumentsTest, AWordThatIsNotAmongTheChoicesIsReported)
  {
    auto map = Plated();
    map.Set("fuel", DataValue::Text("coal"));

    Read(map);

    EXPECT_THAT(_errors, ElementsAre(
                  "cars.yml: 'fuel' of Car of entity 'taxi' is 'coal', "
                  "where one of these was expected: petrol, diesel, electric"));
    EXPECT_EQ(_car.fuel, Fuel::Petrol);
  }

  TEST_F(FieldDocumentsTest, ATextThatIsRequiredAndNotWrittenIsReported)
  {
    Read(DataValue::Map());

    EXPECT_THAT(_errors, ElementsAre("cars.yml: Car of entity 'taxi' needs a 'plate'"));
  }

  TEST_F(FieldDocumentsTest, ATextThatIsRequiredAndWrittenEmptyIsReported)
  {
    auto map = DataValue::Map();
    map.Set("plate", DataValue::Text(""));

    Read(map);

    EXPECT_THAT(_errors, ElementsAre(
                  "cars.yml: 'plate' of Car of entity 'taxi' must not be empty",
                  "cars.yml: Car of entity 'taxi' needs a 'plate'"));
  }

  TEST_F(FieldDocumentsTest, ATextThatIsRequiredMayComeFromTheObject)
  {
    _car.plate = "SET IN CODE";

    Read(DataValue::Map());

    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(FieldDocumentsTest, EveryProblemIsReportedAndNotOnlyTheFirst)
  {
    auto map = DataValue::Map();
    map.Set("seats", DataValue::Text("many"));
    map.Set("fuel", DataValue::Text("coal"));
    map.Set("wings", DataValue::Number(2));

    Read(map);

    EXPECT_EQ(_errors.size(), 4u);
  }

  TEST_F(FieldDocumentsTest, AValueKnowsItsLineInWhatIsReported)
  {
    auto speed = DataValue::Text("fast");
    speed.SetLine(12);
    auto map = Plated();
    map.Set("top_speed", speed);

    Read(map);

    EXPECT_THAT(_errors, ElementsAre(
                  "cars.yml:12: 'top_speed' of Car of entity 'taxi' is text, where a number was expected"));
  }

  TEST_F(FieldDocumentsTest, AValueThatIsRefusedAfterItWasReadKnowsItsLineToo)
  {
    auto speed = DataValue::Number(-5.0f);
    speed.SetLine(7);
    auto map = Plated();
    map.Set("top_speed", speed);

    Read(map);

    EXPECT_THAT(_errors, ElementsAre(
                  "cars.yml:7: 'top_speed' of Car of entity 'taxi' is -5, where a number above 0 was expected"));
  }

  // writing

  TEST_F(FieldDocumentsTest, WritesOnlyWhatIsAlwaysWrittenForAnObjectWithItsDefaults)
  {
    const auto written = Write();

    EXPECT_THAT(NamesOf(written), ElementsAre("fuel"));
  }

  TEST_F(FieldDocumentsTest, WritesWhatDiffersFromTheDefaultsInTheOrderOfTheDescription)
  {
    _car.tire_brand = "Grip";
    _car.plate = "NEON 1";
    _car.locked = true;

    const auto written = Write();

    EXPECT_THAT(NamesOf(written), ElementsAre("locked", "plate", "fuel", "tires"));
    EXPECT_THAT(NamesOf(*written.Find("tires")), ElementsAre("brand"));
  }

  TEST_F(FieldDocumentsTest, WritesAChoiceAsItsWord)
  {
    _car.fuel = Fuel::Diesel;

    std::string word;
    EXPECT_TRUE(Write().Find("fuel")->GetText(word));
    EXPECT_EQ(word, "diesel");
  }

  TEST_F(FieldDocumentsTest, WritesAVectorAsThreeNumbersEvenWhenTheyAreTheSame)
  {
    _car.size = glm::vec3{2.0f};

    EXPECT_THAT(NumbersOf(*Write().Find("size")), ElementsAre(2.0f, 2.0f, 2.0f));
  }

  TEST_F(FieldDocumentsTest, WritesTheAlphaOfAColorOnlyWhenItIsNotOne)
  {
    _car.paint = {0.5f, 0.5f, 0.5f, 1.0f};
    EXPECT_THAT(NumbersOf(*Write().Find("paint")), ElementsAre(0.5f, 0.5f, 0.5f));

    _car.paint = {0.5f, 0.5f, 0.5f, 0.25f};
    EXPECT_THAT(NumbersOf(*Write().Find("paint")), ElementsAre(0.5f, 0.5f, 0.5f, 0.25f));
  }

  TEST_F(FieldDocumentsTest, WritesANumberAsOneThatCameFromAFloat)
  {
    _car.top_speed = 0.31f;

    EXPECT_TRUE(Write().Find("top_speed")->IsSinglePrecision());
  }

  TEST_F(FieldDocumentsTest, WritesAPreciseNumberWithEveryDigitItHolds)
  {
    _car.odometer = 123456.789012345;

    const auto written = Write();
    const auto *odometer = written.Find("odometer");
    ASSERT_NE(odometer, nullptr);
    EXPECT_FALSE(odometer->IsSinglePrecision());

    double number = 0.0;
    EXPECT_TRUE(odometer->GetNumber(number));
    EXPECT_EQ(number, 123456.789012345);
  }

  TEST_F(FieldDocumentsTest, WhatIsWrittenReadsBackAsTheSame)
  {
    _car.locked = true;
    _car.seats = 7;
    _car.top_speed = 99.5f;
    _car.plate = "NEON 1";
    _car.size = {4.0f, 1.5f, 1.8f};
    _car.paint = {0.1f, 0.2f, 0.3f, 0.4f};
    _car.owners = {"Ada", "Bo"};
    _car.fuel = Fuel::Electric;
    _car.tire_pressure = 3.0f;
    _car.tire_brand = "Grip";
    _car.gears = {3.5f, 2.1f};
    _car.odometer = 0.1 + 0.2;
    const auto written = Write();
    const Car before = _car;

    _car = Car{};
    Read(written);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_car.locked, before.locked);
    EXPECT_EQ(_car.seats, before.seats);
    EXPECT_EQ(_car.top_speed, before.top_speed);
    EXPECT_EQ(_car.plate, before.plate);
    EXPECT_EQ(_car.size, before.size);
    EXPECT_EQ(_car.paint.a, before.paint.a);
    EXPECT_EQ(_car.owners, before.owners);
    EXPECT_EQ(_car.fuel, before.fuel);
    EXPECT_EQ(_car.tire_pressure, before.tire_pressure);
    EXPECT_EQ(_car.tire_brand, before.tire_brand);
    EXPECT_EQ(_car.gears, before.gears);
    EXPECT_EQ(_car.odometer, before.odometer);
  }
}

// Fields that belong to an object only while a choice holds one of a few
// words.

namespace
{
  enum class Form
  {
    Square = 0,
    Circle,
    Picture
  };

  struct Shape
  {
    Form form = Form::Square;
    float side = 1.0f;
    float radius = 0.5f;
    std::string image;
  };

  void Describe(TypeBuilder<Shape> &type)
  {
    type.Named("Shape");
    type.Choice("form", &Shape::form, {"square", "circle", "picture"}).AlwaysWritten();
    type.Field("side", &Shape::side).OnlyWhen("form", {"square", "picture"});
    type.Field("radius", &Shape::radius).Above(0).OnlyWhen("form", {"circle"});
    type.Field("image", &Shape::image).Required().AlwaysWritten().OnlyWhen("form", {"picture"});
  }

  class ShapeDocumentsTest : public ::testing::Test
  {
  protected:
    TypeInfo _type = TypeInfo::Of<Shape>();
    std::vector<std::string> _errors;
    Shape _shape;

    void Read(const DataValue &map)
    {
      const DataReader reader(map, "shape.yml", "Shape", _errors);
      neon::ReadFields(_type, reader, &_shape);
      reader.Finish();
    }

    DataValue Write() const
    {
      const Shape standard;
      auto map = DataValue::Map();
      neon::WriteFields(_type, &_shape, &standard, map);
      return map;
    }

    static DataValue Of(const std::string &form)
    {
      auto map = DataValue::Map();
      map.Set("form", DataValue::Text(form));
      return map;
    }
  };

  TEST_F(ShapeDocumentsTest, ReadsTheFieldsThatBelongToWhatTheChoiceHolds)
  {
    auto map = Of("circle");
    map.Set("radius", DataValue::Number(2.0f));

    Read(map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_shape.form, Form::Circle);
    EXPECT_EQ(_shape.radius, 2.0f);
  }

  TEST_F(ShapeDocumentsTest, SaysThatAFieldThatDoesNotBelongIsNotKnown)
  {
    auto map = Of("square");
    auto radius = DataValue::Number(2.0f);
    radius.SetLine(3);
    map.Set("radius", radius);

    Read(map);

    EXPECT_THAT(_errors, ElementsAre("shape.yml:3: 'radius' is not known to Shape. Known are: form, side"));
    EXPECT_EQ(_shape.radius, 0.5f);
  }

  TEST_F(ShapeDocumentsTest, ChecksAFieldThatBelongsAsAnyOther)
  {
    auto map = Of("circle");
    map.Set("radius", DataValue::Number(0.0f));

    Read(map);

    EXPECT_THAT(_errors, ElementsAre("shape.yml: 'radius' of Shape is 0, where a number above 0 was expected"));
  }

  TEST_F(ShapeDocumentsTest, SaysWhatARequiredFieldIsNeededFor)
  {
    Read(Of("picture"));

    EXPECT_THAT(_errors, ElementsAre("shape.yml: Shape needs an 'image' for the form picture"));
  }

  TEST_F(ShapeDocumentsTest, DoesNotNeedARequiredFieldThatDoesNotBelong)
  {
    Read(Of("circle"));

    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(ShapeDocumentsTest, WritesOnlyTheFieldsThatBelong)
  {
    _shape.side = 2.0f;
    _shape.radius = 3.0f;
    _shape.image = "assets://images/cat.png";

    _shape.form = Form::Square;
    EXPECT_THAT(NamesOf(Write()), ElementsAre("form", "side"));

    _shape.form = Form::Circle;
    EXPECT_THAT(NamesOf(Write()), ElementsAre("form", "radius"));

    _shape.form = Form::Picture;
    EXPECT_THAT(NamesOf(Write()), ElementsAre("form", "side", "image"));
  }

  TEST_F(ShapeDocumentsTest, KnowsWhetherAFieldBelongsToAnObject)
  {
    _shape.form = Form::Circle;

    EXPECT_TRUE(_type.Belongs(*_type.Find("form"), &_shape));
    EXPECT_TRUE(_type.Belongs(*_type.Find("radius"), &_shape));
    EXPECT_FALSE(_type.Belongs(*_type.Find("side"), &_shape));
  }
}

// Rules that the fields of a type have to follow together.

namespace
{
  struct Frame
  {
    float inner = 1.0f;
    float outer = 2.0f;
    bool hollow = true;
  };

  void Describe(TypeBuilder<Frame> &type)
  {
    type.Named("Frame");
    type.Field("inner", &Frame::inner);
    type.Field("outer", &Frame::outer);
    type.Field("hollow", &Frame::hollow);

    type.Rule("inner", [](const Frame &frame, const std::string &where) -> std::string
    {
      if (frame.inner < frame.outer) { return {}; }
      return std::format("'inner' of {} is {}, which is not less than its 'outer'", where, frame.inner);
    });

    type.Rule([](const Frame &frame, const std::string &where) -> std::string
    {
      if (frame.hollow || frame.inner == 0.0f) { return {}; }
      return std::format("{} is not hollow and has an inside", where);
    });
  }

  class FrameDocumentsTest : public ::testing::Test
  {
  protected:
    TypeInfo _type = TypeInfo::Of<Frame>();
    std::vector<std::string> _errors;
    Frame _frame;

    void Read(const DataValue &map)
    {
      const DataReader reader(map, "frame.yml", "Frame", _errors);
      neon::ReadFields(_type, reader, &_frame);
      reader.Finish();
    }
  };

  TEST_F(FrameDocumentsTest, ReportsARuleThatIsBrokenAtTheLineOfItsField)
  {
    auto inner = DataValue::Number(3.0f);
    inner.SetLine(4);
    auto map = DataValue::Map();
    map.Set("inner", inner);

    Read(map);

    EXPECT_THAT(_errors, ElementsAre("frame.yml:4: 'inner' of Frame is 3, which is not less than its 'outer'"));
  }

  TEST_F(FrameDocumentsTest, KeepsWhatWasReadWhenARuleIsBroken)
  {
    auto map = DataValue::Map();
    map.Set("inner", DataValue::Number(3.0f));

    Read(map);

    EXPECT_EQ(_frame.inner, 3.0f);
  }

  TEST_F(FrameDocumentsTest, ReportsARuleThatIsAboutNoOneFieldWithoutALine)
  {
    auto hollow = DataValue::Bool(false);
    hollow.SetLine(5);
    auto map = DataValue::Map();
    map.Set("hollow", hollow);

    Read(map);

    EXPECT_THAT(_errors, ElementsAre("frame.yml: Frame is not hollow and has an inside"));
  }

  TEST_F(FrameDocumentsTest, ChecksTheRulesWithWhatTheFileLeavesOut)
  {
    _frame.outer = 0.5f;

    Read(DataValue::Map());

    EXPECT_THAT(_errors, ElementsAre("frame.yml: 'inner' of Frame is 1, which is not less than its 'outer'"));
  }

  TEST_F(FrameDocumentsTest, ReportsNothingWhenEveryRuleIsFollowed)
  {
    auto map = DataValue::Map();
    map.Set("inner", DataValue::Number(0.0f));
    map.Set("hollow", DataValue::Bool(false));

    Read(map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_type.rules.size(), 2u);
  }
}

// Layers.

namespace
{
  struct Lamp
  {
    std::uint32_t lights = 1;
  };

  void Describe(TypeBuilder<Lamp> &type)
  {
    type.Named("Lamp");
    type.Layers("lights", &Lamp::lights);
  }

  class LayerDocumentsTest : public ::testing::Test
  {
  protected:
    TypeInfo _type = TypeInfo::Of<Lamp>();
    std::vector<std::string> _errors;
    Lamp _lamp;

    void Read(const DataValue &value)
    {
      auto map = DataValue::Map();
      map.Set("lights", value);

      const DataReader reader(map, "lamp.yml", "Lamp", _errors);
      neon::ReadFields(_type, reader, &_lamp);
      reader.Finish();
    }

    DataValue Write() const
    {
      const Lamp standard;
      auto map = DataValue::Map();
      neon::WriteFields(_type, &_lamp, &standard, map);
      return map;
    }

    static DataValue At(const std::size_t line, DataValue value)
    {
      value.SetLine(line);
      return value;
    }
  };

  TEST_F(LayerDocumentsTest, ReadsAListOfLayers)
  {
    Read(Numbers({2.0f, 3.0f, 32.0f}));

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_lamp.lights, 0x80000006u);
  }

  TEST_F(LayerDocumentsTest, ReadsOneLayerWithoutAList)
  {
    Read(DataValue::Number(3.0f));

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_lamp.lights, 0b100u);
  }

  TEST_F(LayerDocumentsTest, ReadsAnEmptyListAsNoLayer)
  {
    Read(DataValue::List());

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_lamp.lights, 0u);
  }

  TEST_F(LayerDocumentsTest, SaysWhatIsNoLayerAndKeepsWhatWasThere)
  {
    Read(At(3, DataValue::Text("walls")));
    Read(At(4, DataValue::Number(0.0f)));
    Read(At(5, DataValue::Number(1.5f)));

    auto mixed = At(6, DataValue::List());
    mixed.Add(At(7, DataValue::Number(1.0f)));
    mixed.Add(At(8, DataValue::Text("two")));
    mixed.Add(At(9, DataValue::Number(40.0f)));
    Read(mixed);

    EXPECT_THAT(_errors, ElementsAre(
                  "lamp.yml:3: 'lights' of Lamp is text, where a layer from 1 to 32 or a list of layers was expected",
                  "lamp.yml:4: 'lights' of Lamp holds 0, where a layer from 1 to 32 was expected",
                  "lamp.yml:5: 'lights' of Lamp holds 1.5, where a layer from 1 to 32 was expected",
                  "lamp.yml:8: 'lights' of Lamp holds text, where a layer from 1 to 32 was expected"));
    EXPECT_EQ(_lamp.lights, 1u);
  }

  TEST_F(LayerDocumentsTest, WritesLayersAsTheirNumbersAndNoneAsAnEmptyList)
  {
    EXPECT_THAT(NamesOf(Write()), IsEmpty());

    _lamp.lights = 0b101u;
    ASSERT_NE(Write().Find("lights"), nullptr);
    EXPECT_THAT(NumbersOf(*Write().Find("lights")), ElementsAre(1.0f, 3.0f));

    _lamp.lights = 0u;
    ASSERT_NE(Write().Find("lights"), nullptr);
    EXPECT_TRUE(Write().Find("lights")->IsList());
    EXPECT_THAT(NumbersOf(*Write().Find("lights")), IsEmpty());
  }
}

// Lengths and lists of numbers.

namespace
{
  using neon::FieldLength;

  struct Box
  {
    FieldLength width;
    FieldLength height = FieldLength::Pixels(16.0f);
    std::vector<float> corners;
  };

  void Describe(TypeBuilder<Box> &type)
  {
    type.Named("Box");
    type.Field("width", &Box::width);
    type.Field("height", &Box::height);
    type.Field("corners", &Box::corners);
  }

  class LengthDocumentsTest : public ::testing::Test
  {
  protected:
    TypeInfo _type = TypeInfo::Of<Box>();
    std::vector<std::string> _errors;
    Box _box;

    void Read(const DataValue &map)
    {
      const DataReader reader(map, "box.yml", "Box", _errors);
      neon::ReadFields(_type, reader, &_box);
      reader.Finish();
    }
  };

  TEST_F(LengthDocumentsTest, DeducesTheKindsFromTheMembers)
  {
    EXPECT_EQ(_type.Find("width")->kind, neon::FieldKind::Length);
    EXPECT_EQ(_type.Find("corners")->kind, neon::FieldKind::NumberList);
  }

  TEST_F(LengthDocumentsTest, ReadsALengthAsANumberAndAsText)
  {
    auto map = DataValue::Map();
    map.Set("width", DataValue::Number(200));
    map.Set("height", DataValue::Text("50%"));
    Read(map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_box.width, FieldLength::Pixels(200.0f));
    EXPECT_EQ(_box.height, FieldLength::Percent(50.0f));

    map.Set("width", DataValue::Text("auto"));
    map.Set("height", DataValue::Text("calc(100% + -20px)"));
    Read(map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_TRUE(_box.width.is_auto);
    EXPECT_EQ(_box.height, FieldLength::Sum(-20.0f, 100.0f));
  }

  TEST_F(LengthDocumentsTest, ReadsAListOfNumbers)
  {
    auto map = DataValue::Map();
    map.Set("corners", Numbers({4.0f, 8.0f, 0.5f}));
    Read(map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_THAT(_box.corners, ElementsAre(4.0f, 8.0f, 0.5f));

    map.Set("corners", DataValue::List());
    Read(map);
    EXPECT_THAT(_box.corners, IsEmpty());
  }

  TEST_F(LengthDocumentsTest, SaysWhatIsWrongAndLeavesTheFieldAlone)
  {
    auto words = DataValue::List();
    words.Add(DataValue::Number(1));
    words.Add(DataValue::Text("two"));

    auto wide = DataValue::Text("wide");
    wide.SetLine(3);

    auto map = DataValue::Map();
    map.Set("width", wide);
    map.Set("height", DataValue::Bool(true));
    map.Set("corners", words);
    Read(map);

    EXPECT_THAT(
      _errors,
      ElementsAre(
        "box.yml:3: 'width' of Box is 'wide', where a length such as 12, \"12px\", \"50%\", or auto was expected",
        "box.yml: 'height' of Box is true or false, where a length such as 12, \"12px\", \"50%\", or auto was "
        "expected",
        "box.yml: 'corners' of Box holds text, where a number was expected"));

    EXPECT_TRUE(_box.width.is_auto);
    EXPECT_EQ(_box.height, FieldLength::Pixels(16.0f));
    EXPECT_THAT(_box.corners, IsEmpty());
  }

  TEST_F(LengthDocumentsTest, WritesPixelsAsANumberAndEveryOtherLengthAsText)
  {
    _box.width = FieldLength::Pixels(200.0f);
    _box.height = FieldLength::Percent(50.0f);
    _box.corners = {4.0f, 8.0f};

    const Box standard;
    auto map = DataValue::Map();
    neon::WriteFields(_type, &_box, &standard, map);

    EXPECT_THAT(NamesOf(map), ElementsAre("width", "height", "corners"));

    float pixels = 0.0f;
    ASSERT_TRUE(map.Find("width")->GetNumber(pixels));
    EXPECT_FLOAT_EQ(pixels, 200.0f);

    std::string text;
    ASSERT_TRUE(map.Find("height")->GetText(text));
    EXPECT_EQ(text, "50%");

    EXPECT_THAT(NumbersOf(*map.Find("corners")), ElementsAre(4.0f, 8.0f));
  }

  TEST_F(LengthDocumentsTest, WritesNothingForWhatHoldsItsDefault)
  {
    const Box standard;
    auto map = DataValue::Map();
    neon::WriteFields(_type, &_box, &standard, map);

    EXPECT_THAT(NamesOf(map), IsEmpty());
  }

  TEST_F(LengthDocumentsTest, ReadsBackWhatItWrote)
  {
    _box.width = FieldLength::Sum(8.0f, 25.0f);
    _box.height = FieldLength::Auto();
    _box.corners = {1.0f, 2.0f, 3.0f, 4.0f};

    const Box standard;
    auto map = DataValue::Map();
    neon::WriteFields(_type, &_box, &standard, map);

    const Box written = _box;
    _box = Box{};
    Read(map);

    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_box.width, written.width);
    EXPECT_EQ(_box.height, written.height);
    EXPECT_EQ(_box.corners, written.corners);
  }

  TEST(FieldValueKindsTest, KnowsALengthAndAListOfNumbers)
  {
    using neon::FieldKind;
    using neon::FieldValue;

    EXPECT_EQ(neon::Describe(FieldKind::Length), "a length");
    EXPECT_EQ(neon::Describe(FieldKind::NumberList), "a list of numbers");

    EXPECT_TRUE(neon::Holds(FieldValue{FieldLength::Pixels(1.0f)}, FieldKind::Length));
    EXPECT_FALSE(neon::Holds(FieldValue{1.0f}, FieldKind::Length));
    EXPECT_TRUE(neon::Holds(FieldValue{std::vector<float>{1.0f}}, FieldKind::NumberList));
    EXPECT_FALSE(neon::Holds(FieldValue{std::vector<std::string>{"a"}}, FieldKind::NumberList));

    EXPECT_TRUE(neon::Same(FieldValue{FieldLength::Percent(5.0f)}, FieldValue{FieldLength::Percent(5.0f)}));
    EXPECT_FALSE(neon::Same(FieldValue{FieldLength::Percent(5.0f)}, FieldValue{FieldLength::Pixels(5.0f)}));
    EXPECT_FALSE(neon::Same(FieldValue{FieldLength::Auto()}, FieldValue{FieldLength::Pixels(0.0f)}));
    EXPECT_TRUE(neon::Same(FieldValue{std::vector<float>{1.0f, 2.0f}}, FieldValue{std::vector<float>{1.0f, 2.0f}}));
    EXPECT_FALSE(neon::Same(FieldValue{std::vector<float>{1.0f, 2.0f}}, FieldValue{std::vector<float>{1.0f}}));
  }

  TEST(FieldValueKindsTest, ChecksALengthAndAListOfNumbersByTheirKind)
  {
    const TypeInfo type = TypeInfo::Of<Box>();

    EXPECT_EQ(type.Find("width")->Check(neon::FieldValue{FieldLength::Pixels(1.0f)}, "'width' of Box"), "");
    EXPECT_EQ(type.Find("width")->Check(neon::FieldValue{1.0f}, "'width' of Box"), "'width' of Box takes a length");
    EXPECT_EQ(
      type.Find("corners")->Check(neon::FieldValue{1.0f}, "'corners' of Box"),
      "'corners' of Box takes a list of numbers");
  }
} // namespace
