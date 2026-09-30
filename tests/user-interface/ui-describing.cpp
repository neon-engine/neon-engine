#include "ui-fixture.hpp"

#include <neon/reflection/field-text.hpp>

// Describing kinds of elements through reflection, which is what an
// inspector and a binding for scripts read.

namespace
{
  using neon::FieldKind;
  using neon::FieldLength;
  using neon::FieldValue;
  using neon::TypeInfo;
  using neon::UiHandle;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;
  using ::testing::ElementsAre;
  using ::testing::IsSupersetOf;

  class UiDescribingTest : public UiTest
  {
  protected:
    void ShowOne(const std::string &element)
    {
      ASSERT_GE(ShowUnderRoot(element), 0) << _logger->Messages(LogLevel::Error);
      Frame();
    }

    [[nodiscard]] std::vector<std::string> NamesOf(const std::vector<neon::FieldInfo> &fields)
    {
      std::vector<std::string> names;
      for (const auto &field : fields) { names.push_back(field.name); }
      return names;
    }
  };

  TEST_F(UiDescribingTest, DescribesAKindWithWhatEveryElementHasAndItsOwnFields)
  {
    TypeInfo button;
    ASSERT_TRUE(_ui->DescribeElement("button", button));

    EXPECT_EQ(button.name, "button");
    EXPECT_EQ(button.description, "What a player chooses with the pointer, the keys, or a controller");
    EXPECT_THAT(NamesOf(button.fields), ElementsAre(
      "name", "class", "title", "tab_index", "hidden", "text", "enabled", "autofocus", "style"));

    const neon::FieldInfo *text = button.Find("text");
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(text->kind, FieldKind::Text);
    EXPECT_EQ(text->description, "What is written on it, which may refer to values of the game as {name}");

    const neon::FieldInfo *kind = nullptr;
    TypeInfo input;
    ASSERT_TRUE(_ui->DescribeElement("input", input));
    kind = input.Find("kind");
    ASSERT_NE(kind, nullptr);
    EXPECT_EQ(kind->kind, FieldKind::Choice);
    EXPECT_THAT(kind->choices, ElementsAre("text", "password", "number"));
  }

  TEST_F(UiDescribingTest, DoesNotDescribeAKindThatIsNotKnown)
  {
    TypeInfo description;
    EXPECT_FALSE(_ui->DescribeElement("lable", description));
  }

  TEST_F(UiDescribingTest, TheStyleIsAGroupWithEveryPropertyOfTheTable)
  {
    TypeInfo panel;
    ASSERT_TRUE(_ui->DescribeElement("panel", panel));

    const neon::FieldInfo *style = panel.Find("style");
    ASSERT_NE(style, nullptr);
    EXPECT_EQ(style->kind, FieldKind::Group);

    EXPECT_THAT(NamesOf(style->fields), IsSupersetOf({
      "width", "height", "background_color", "opacity", "flex_direction", "padding_top", "font_size", "overflow_x"
    }));

    // shorthands are not fields: they are what their longhands are
    EXPECT_EQ(panel.Find("style.margin"), nullptr);
    EXPECT_NE(panel.Find("style.margin_top"), nullptr);

    const neon::FieldInfo *width = panel.Find("style.width");
    ASSERT_NE(width, nullptr);
    EXPECT_EQ(width->kind, FieldKind::Length);

    const neon::FieldInfo *direction = panel.Find("style.flex_direction");
    ASSERT_NE(direction, nullptr);
    EXPECT_EQ(direction->kind, FieldKind::Choice);
    EXPECT_THAT(direction->choices, ElementsAre("row", "row-reverse", "column", "column-reverse"));

    const neon::FieldInfo *color = panel.Find("style.background_color");
    ASSERT_NE(color, nullptr);
    EXPECT_EQ(color->kind, FieldKind::Color);

    const neon::FieldInfo *padding = panel.Find("style.padding_top");
    ASSERT_NE(padding, nullptr);
    EXPECT_EQ(padding->kind, FieldKind::Length);

    const neon::FieldInfo *opacity = panel.Find("style.opacity");
    ASSERT_NE(opacity, nullptr);
    EXPECT_EQ(opacity->kind, FieldKind::Number);
  }

  TEST_F(UiDescribingTest, TheFieldsOfADescriptionReadAndChangeAnElement)
  {
    ASSERT_GE(ShowUnderRoot(
      "- {type: button, name: start, text: Start, width: 200, background_color: \"#336699\"}\n",
      "align_items: flex-start\n"), 0) << _logger->Messages(LogLevel::Error);
    Frame();

    TypeInfo button;
    ASSERT_TRUE(_ui->DescribeElement("button", button));

    // the description works on an element through a pointer to it, as
    // reflection does for every type
    const neon::UiElement &element = Element("start");
    const auto *object = static_cast<const void *>(&element);

    EXPECT_EQ(std::get<std::string>(button.Find("name")->get(object)), "start");
    EXPECT_EQ(std::get<std::string>(button.Find("text")->get(object)), "Start");
    EXPECT_EQ(std::get<bool>(button.Find("enabled")->get(object)), true);

    const auto width = std::get<FieldLength>(button.Find("style.width")->get(object));
    EXPECT_FALSE(width.is_auto);
    EXPECT_FLOAT_EQ(width.pixels, 200.0f);

    const auto color = std::get<neon::Color>(button.Find("style.background_color")->get(object));
    EXPECT_NEAR(color.r, 0.2f, 0.01f);
    EXPECT_NEAR(color.g, 0.4f, 0.01f);
    EXPECT_NEAR(color.b, 0.6f, 0.01f);

    // and changes it
    auto *mutable_object = const_cast<void *>(object);
    button.Find("text")->set(mutable_object, std::string("Go"));
    button.Find("style.width")->set(mutable_object, FieldLength{false, 300.0f, 0.0f});
    button.Find("style.background_color")->set(mutable_object, neon::Color{1.0f, 0.0f, 0.0f, 1.0f});
    Frame();

    EXPECT_EQ(std::get<std::string>(button.Find("text")->get(object)), "Go");
    ExpectBox("start", 0.0f, 0.0f, 332.0f, 32.0f);
    EXPECT_EQ(_ui->GetComputed(_ui->FindByName("start"), "background-color"), "rgb(255, 0, 0)");
  }

  TEST_F(UiDescribingTest, AStyleIsReachedAsAFieldByItsPath)
  {
    ShowOne("- {type: label, name: title, text: Hello, opacity: 0.5}\n");
    const UiHandle title = _ui->FindByName("title");

    FieldValue value;
    ASSERT_TRUE(_ui->GetField(title, "style.opacity", value));
    EXPECT_FLOAT_EQ(std::get<float>(value), 0.5f);

    ASSERT_TRUE(_ui->GetField(title, "style.text_align", value));
    EXPECT_EQ(std::get<std::string>(value), "left");

    EXPECT_TRUE(_ui->SetField(title, "style.opacity", 0.25f));
    EXPECT_TRUE(_ui->SetField(title, "style.text_align", std::string("center")));
    Frame();

    EXPECT_EQ(_ui->GetComputed(title, "opacity"), "0.25");
    EXPECT_EQ(_ui->GetComputed(title, "text-align"), "center");

    EXPECT_FALSE(_ui->GetField(title, "style.colour", value));
    EXPECT_FALSE(_ui->SetField(title, "style.colour", std::string("#fff")));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn, "'style.colour' of label 'title' is no property that is known. Nothing is set"));
  }
}
