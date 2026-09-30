#include "data-reader.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// What a reader tells about itself, and what it reports about the value it
// reads as a whole. What it reports about the values inside is covered by
// the tests of what reads scenes and user interfaces with it.

namespace
{
  using neon::DataReader;
  using neon::DataValue;
  using ::testing::ElementsAre;

  DataValue MapAt(const std::size_t line)
  {
    auto map = DataValue::Map();
    map.SetLine(line);
    return map;
  }

  TEST(DataReader, KnowsWhatItReadsAndWhere)
  {
    std::vector<std::string> errors;
    const DataValue map = MapAt(7);
    const DataReader reader(map, "assets://ui/hud.ui.yml", "label 'health'", errors);

    EXPECT_EQ(reader.GetDocument(), "assets://ui/hud.ui.yml");
    EXPECT_EQ(reader.GetWhere(), "label 'health'");
    EXPECT_EQ(&reader.GetErrors(), &errors);
  }

  TEST(DataReader, ReportsWhatIsMissingWithTheLineItStartsAt)
  {
    std::vector<std::string> errors;
    const DataValue map = MapAt(7);
    const DataReader reader(map, "hud.ui.yml", "image 'icon'", errors);

    reader.Report("image 'icon' has no 'src'");

    EXPECT_THAT(errors, ElementsAre("hud.ui.yml:7: image 'icon' has no 'src'"));
  }

  TEST(DataReader, ReportsWithoutALineWhenThereIsNone)
  {
    std::vector<std::string> errors;
    const DataValue map = DataValue::Map();
    const DataReader reader(map, "hud.ui.yml", "image 'icon'", errors);

    reader.Report("image 'icon' has no 'src'");

    EXPECT_THAT(errors, ElementsAre("hud.ui.yml: image 'icon' has no 'src'"));
  }

  TEST(DataReader, KeepsTheLineOfWhatIsNoMap)
  {
    std::vector<std::string> errors;
    auto text = DataValue::Text("panel");
    text.SetLine(3);

    const DataReader reader(text, "hud.ui.yml", "the root", errors);
    reader.Report("the root has no 'type'");

    EXPECT_THAT(errors, ElementsAre(
                  "hud.ui.yml:3: the root is text, where a map was expected",
                  "hud.ui.yml:3: the root has no 'type'"));
  }

  TEST(DataReader, ReadsAListOfNumbersOfAnyLength)
  {
    std::vector<std::string> errors;
    auto list = DataValue::List();
    list.Add(DataValue::Number(512.0f));
    list.Add(DataValue::Number(384.0f));
    auto map = DataValue::Map();
    map.Set("size", list);
    map.Set("none", DataValue::List());
    const DataReader reader(map, "hud.ui.yml", "the root", errors);

    std::vector<float> size;
    EXPECT_TRUE(reader.Read("size", size));
    EXPECT_THAT(size, ElementsAre(512.0f, 384.0f));

    std::vector<float> none{1.0f};
    EXPECT_TRUE(reader.Read("none", none));
    EXPECT_THAT(none, ElementsAre());

    EXPECT_FALSE(reader.Read("missing", size));
    EXPECT_THAT(errors, ElementsAre());
  }

  TEST(DataReader, ReportsWhatIsNoNumberInAListOfNumbers)
  {
    std::vector<std::string> errors;
    auto list = DataValue::List();
    list.Add(DataValue::Number(512.0f));
    auto word = DataValue::Text("wide");
    word.SetLine(5);
    list.Add(word);
    auto map = DataValue::Map();
    map.Set("size", list);
    map.Set("scale", DataValue::Number(2.0f));
    const DataReader reader(map, "hud.ui.yml", "the root", errors);

    std::vector<float> size{7.0f};
    EXPECT_FALSE(reader.Read("size", size));
    EXPECT_FALSE(reader.Read("scale", size));

    EXPECT_THAT(size, ElementsAre(7.0f));
    EXPECT_THAT(errors, ElementsAre(
                  "hud.ui.yml:5: 'size' of the root holds text, where a number was expected",
                  "hud.ui.yml: 'scale' of the root is a number, where a list of numbers was expected"));
  }

  TEST(DataReader, AReaderOfAPartWritesToTheSameErrors)
  {
    std::vector<std::string> errors;
    const DataValue map = MapAt(1);
    const DataReader reader(map, "hud.ui.yml", "the user interface", errors);

    const DataValue part = MapAt(4);
    const DataReader part_reader(part, reader.GetDocument(), "font 1", reader.GetErrors());
    part_reader.Report("font 1 has no 'src'");

    EXPECT_THAT(errors, ElementsAre("hud.ui.yml:4: font 1 has no 'src'"));
  }
}
