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
                  "locked, seats, top_speed, plate, size, paint, owners, fuel, tires"));
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

    EXPECT_THAT(_errors, ElementsAre("cars.yml: 'top_speed' of Car of entity 'taxi' has to be above 0"));
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
                  "cars.yml: 'pressure' of 'tires' of Car of entity 'taxi' has to be above 0"));
  }

  TEST_F(FieldDocumentsTest, AWholeNumberBelowWhatItMayBeIsReported)
  {
    auto map = Plated();
    map.Set("seats", DataValue::Number(0));

    Read(map);

    EXPECT_THAT(_errors, ElementsAre("cars.yml: 'seats' of Car of entity 'taxi' has to be at least 1"));
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

    EXPECT_THAT(_errors, ElementsAre("cars.yml:7: 'top_speed' of Car of entity 'taxi' has to be above 0"));
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
  }
}
