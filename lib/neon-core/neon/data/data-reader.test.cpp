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
