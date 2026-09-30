#include "ui-fixture.hpp"

// Shaders of elements: which shader a call is drawn with, what it is told,
// and what happens when it cannot be used. What a shader makes of a pixel
// is looked at in the frames the runtime writes.

namespace
{
  using neon::MaterialValue2D;
  using neon::No_Material;
  using neon::Triangles2D;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;
  using ::testing::ElementsAre;

  class UiShaderTest : public UiTest
  {
  protected:
    static constexpr const char *kShine = "assets://shaders/ui/shine";

    /// A box of 200 by 100 at 100, 100 with the properties that are given.
    void ShowBox(const std::string &properties, const std::string &children = "")
    {
      std::string element =
        "- type: panel\n"
        "  name: box\n"
        "  position: absolute\n"
        "  left: 100\n"
        "  top: 100\n"
        "  width: 200\n"
        "  height: 100\n"
        "  background_color: \"#ff0000\"\n" + Indented(properties, "  ");

      if (!children.empty()) { element += "  children:\n" + Indented(children, "    "); }

      ASSERT_GE(ShowUnderRoot(element), 0) << _logger->Messages(LogLevel::Error);
      Frame();
    }

    [[nodiscard]] static const MaterialValue2D *ValueOf(const Triangles2D &batch, const std::string &name)
    {
      for (const auto &value : batch.material_values)
      {
        if (value.name == name) { return &value; }
      }
      return nullptr;
    }

    [[nodiscard]] std::vector<std::string> ProblemsOf(const std::string &properties)
    {
      _logger->Clear();
      EXPECT_EQ(ShowUnderRoot("- type: panel\n  name: box\n" + Indented(properties, "  ")), -1);

      auto errors = Errors();
      if (!errors.empty()) { errors.pop_back(); }
      return errors;
    }
  };

  TEST_F(UiShaderTest, AnElementWithoutAShaderIsDrawnWithThatOfTheRenderer)
  {
    ShowBox("");

    ASSERT_EQ(_renderer.batches.size(), 1u);
    EXPECT_EQ(_renderer.batches[0].material, No_Material);
    EXPECT_TRUE(_renderer.batches[0].material_values.empty());
    EXPECT_TRUE(_renderer.materials.empty()) << "and no shader is asked for";
  }

  TEST_F(UiShaderTest, DrawsAnElementWithTheShaderItNames)
  {
    ShowBox("shader: assets://shaders/ui/shine\n");

    EXPECT_EQ(_renderer.materials, (std::vector<std::string>{kShine}));

    ASSERT_EQ(_renderer.batches.size(), 1u);
    EXPECT_EQ(_renderer.batches[0].material, 0);

    // the shader is told the box of the element, in pixels
    EXPECT_FLOAT_EQ(_renderer.batches[0].material_box[0], 100);
    EXPECT_FLOAT_EQ(_renderer.batches[0].material_box[1], 100);
    EXPECT_FLOAT_EQ(_renderer.batches[0].material_box[2], 200);
    EXPECT_FLOAT_EQ(_renderer.batches[0].material_box[3], 100);
  }

  TEST_F(UiShaderTest, TheBoxAShaderIsToldGrowsWithTheFrame)
  {
    _renderer.SetResolution(3840, 2160);
    ShowBox("shader: assets://shaders/ui/shine\n");

    EXPECT_FLOAT_EQ(_renderer.batches.at(0).material_box[0], 200);
    EXPECT_FLOAT_EQ(_renderer.batches.at(0).material_box[2], 400);
  }

  TEST_F(UiShaderTest, HandsTheValuesOfTheFileToTheShader)
  {
    ShowBox(
      "shader: assets://shaders/ui/shine\n"
      "shader_values:\n"
      "  speed: 0.5\n"
      "  tint: \"#ff800080\"\n"
      "  offset: [4, -2]\n"
      "  steps: 3\n"
      "  lit: true\n");

    ASSERT_EQ(_renderer.batches.size(), 1u);
    const Triangles2D &batch = _renderer.batches[0];
    ASSERT_EQ(batch.material_values.size(), 5u);

    const MaterialValue2D *speed = ValueOf(batch, "speed");
    ASSERT_NE(speed, nullptr);
    EXPECT_EQ(speed->count, 1);
    EXPECT_FLOAT_EQ(speed->numbers[0], 0.5f);

    // a colour is four numbers, and its alpha is not multiplied in
    const MaterialValue2D *tint = ValueOf(batch, "tint");
    ASSERT_NE(tint, nullptr);
    EXPECT_EQ(tint->count, 4);
    EXPECT_NEAR(tint->numbers[0], 1.0f, 0.002f);
    EXPECT_NEAR(tint->numbers[1], 0.502f, 0.002f);
    EXPECT_NEAR(tint->numbers[2], 0.0f, 0.002f);
    EXPECT_NEAR(tint->numbers[3], 0.502f, 0.002f);

    const MaterialValue2D *offset = ValueOf(batch, "offset");
    ASSERT_NE(offset, nullptr);
    EXPECT_EQ(offset->count, 2);
    EXPECT_FLOAT_EQ(offset->numbers[0], 4);
    EXPECT_FLOAT_EQ(offset->numbers[1], -2);

    EXPECT_FLOAT_EQ(ValueOf(batch, "steps")->numbers[0], 3);
    EXPECT_FLOAT_EQ(ValueOf(batch, "lit")->numbers[0], 1);
  }

  TEST_F(UiShaderTest, AValueOfAShaderFollowsAValueOfTheGame)
  {
    _ui->SetNumber("charge", 0.25);

    ShowBox("shader: assets://shaders/ui/cooldown\nshader_values: { progress: \"{charge}\" }\n");

    const MaterialValue2D *progress = ValueOf(_renderer.batches.at(0), "progress");
    ASSERT_NE(progress, nullptr);
    EXPECT_FLOAT_EQ(progress->numbers[0], 0.25f);

    _ui->SetNumber("charge", 0.75);
    Frame();

    EXPECT_FLOAT_EQ(ValueOf(_renderer.batches.at(0), "progress")->numbers[0], 0.75f);

    // a flag counts as 1 or 0
    _ui->SetFlag("charge", true);
    Frame();

    EXPECT_FLOAT_EQ(ValueOf(_renderer.batches.at(0), "progress")->numbers[0], 1.0f);
  }

  TEST_F(UiShaderTest, AValueOfTheGameThatIsNotSetCountsAsZero)
  {
    ShowBox("shader: assets://shaders/ui/cooldown\nshader_values: { progress: \"{nothing}\" }\n");

    EXPECT_FLOAT_EQ(ValueOf(_renderer.batches.at(0), "progress")->numbers[0], 0.0f);
    EXPECT_TRUE(Errors().empty());
  }

  TEST_F(UiShaderTest, TellsAShaderTheTime)
  {
    ShowBox("shader: assets://shaders/ui/shine\n");
    EXPECT_FLOAT_EQ(_renderer.batches.at(0).time, 0.0f);

    _ui->AdvanceTime(0.5);
    _ui->AdvanceTime(0.25);
    Frame();

    EXPECT_FLOAT_EQ(_renderer.batches.at(0).time, 0.75f);
    EXPECT_DOUBLE_EQ(_ui->GetTime(), 0.75);

    // time does not run backwards
    _ui->AdvanceTime(-1.0);
    EXPECT_DOUBLE_EQ(_ui->GetTime(), 0.75);
  }

  TEST_F(UiShaderTest, TheContentOfAnElementIsDrawnWithItsShader)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n"
      "  text: Go\n"
      "  shader: assets://shaders/ui/shine\n"), 0);
    Frame();

    // the box of the button and its text, in one call
    ASSERT_EQ(_renderer.batches.size(), 1u);
    EXPECT_EQ(_renderer.batches[0].material, 0);
    EXPECT_EQ(_renderer.Quads().size(), 3u);
  }

  TEST_F(UiShaderTest, WhatIsInsideAnElementIsDrawnWithoutItsShader)
  {
    ShowBox(
      "shader: assets://shaders/ui/shine\n",
      "- type: panel\n  width: 20\n  height: 20\n  background_color: \"#00ff00\"\n");

    ASSERT_EQ(_renderer.batches.size(), 2u);
    EXPECT_EQ(_renderer.batches[0].material, 0);
    EXPECT_EQ(_renderer.batches[1].material, No_Material);
    EXPECT_TRUE(_renderer.batches[1].material_values.empty());
  }

  TEST_F(UiShaderTest, WhatFollowsAnElementIsDrawnWithoutItsShader)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n  width: 20\n  height: 20\n  background_color: \"#ff0000\"\n"
      "  shader: assets://shaders/ui/shine\n"
      "- type: panel\n  width: 20\n  height: 20\n  background_color: \"#00ff00\"\n"
      "- type: panel\n  width: 20\n  height: 20\n  background_color: \"#0000ff\"\n"), 0);
    Frame();

    ASSERT_EQ(_renderer.batches.size(), 2u);
    EXPECT_EQ(_renderer.batches[0].material, 0);
    EXPECT_EQ(_renderer.batches[1].material, No_Material);
    EXPECT_EQ(_renderer.batches[1].vertices.size(), 8u) << "the two that follow share a call";
  }

  TEST_F(UiShaderTest, AsksForAShaderOnce)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n  width: 20\n  height: 20\n  background_color: \"#ff0000\"\n"
      "  shader: assets://shaders/ui/shine\n"
      "- type: panel\n  width: 20\n  height: 20\n  background_color: \"#00ff00\"\n"
      "  shader: assets://shaders/ui/shine\n"
      "- type: panel\n  width: 20\n  height: 20\n  background_color: \"#0000ff\"\n"
      "  shader: assets://shaders/ui/dissolve\n"), 0);

    Frame();
    Frame();
    Frame();

    EXPECT_EQ(_renderer.materials, (std::vector<std::string>{kShine, "assets://shaders/ui/dissolve"}));

    // every element is a call of its own, since each has a box of its own
    ASSERT_EQ(_renderer.batches.size(), 3u);
    EXPECT_EQ(_renderer.batches[0].material, 0);
    EXPECT_EQ(_renderer.batches[1].material, 0);
    EXPECT_EQ(_renderer.batches[2].material, 1);
  }

  TEST_F(UiShaderTest, SaysOnceThatAShaderCannotBeUsedAndDrawsTheElementWithoutIt)
  {
    _renderer.broken_shaders.push_back("assets://shaders/ui/broken");

    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n  name: first\n  width: 20\n  height: 20\n  background_color: \"#ff0000\"\n"
      "  shader: assets://shaders/ui/broken\n"
      "  shader_values: { speed: 1 }\n"
      "- type: panel\n  name: second\n  width: 20\n  height: 20\n  background_color: \"#00ff00\"\n"
      "  shader: assets://shaders/ui/broken\n"), 0);

    Frame();
    Frame();
    Frame();

    // with the file and with the element
    EXPECT_EQ(Errors().size(), 1u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The shader assets://shaders/ui/broken of panel 'first' cannot be used, what asks for it is drawn "
      "without it"));

    EXPECT_EQ(_renderer.materials.size(), 1u) << "and it is not asked for again";

    // both are drawn, with the shader of the renderer, in one call
    ASSERT_EQ(_renderer.batches.size(), 1u);
    EXPECT_EQ(_renderer.batches[0].material, No_Material);
    EXPECT_TRUE(_renderer.batches[0].material_values.empty());
    EXPECT_EQ(_renderer.Quads().size(), 2u);
  }

  TEST_F(UiShaderTest, AStateCanGiveAnElementAShader)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n"
      "  name: start\n"
      "  position: absolute\n"
      "  left: 0\n"
      "  top: 0\n"
      "  width: 100\n"
      "  height: 100\n"
      "  hover:\n"
      "    shader: assets://shaders/ui/shine\n"
      "    shader_values: { speed: 2 }\n"), 0);
    Frame();

    EXPECT_EQ(_renderer.batches.at(0).material, No_Material);
    EXPECT_TRUE(_renderer.materials.empty()) << "a shader is asked for when it is first drawn with";

    PointAt(50, 50);
    Frame();
    Frame();

    EXPECT_EQ(_renderer.batches.at(0).material, 0);
    EXPECT_FLOAT_EQ(ValueOf(_renderer.batches.at(0), "speed")->numbers[0], 2);
  }

  TEST_F(UiShaderTest, NoneTakesTheShaderAway)
  {
    ShowBox("shader: none\n");

    EXPECT_EQ(_renderer.batches.at(0).material, No_Material);
    EXPECT_TRUE(_renderer.materials.empty());
  }

  TEST_F(UiShaderTest, ReleasesItsShadersWhenItIsCleanedUp)
  {
    _renderer.broken_shaders.push_back("assets://shaders/ui/broken");

    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n  width: 20\n  height: 20\n  background_color: \"#ff0000\"\n"
      "  shader: assets://shaders/ui/shine\n"
      "- type: panel\n  width: 20\n  height: 20\n  background_color: \"#ff0000\"\n"
      "  shader: assets://shaders/ui/broken\n"), 0);
    Frame();

    _ui->CleanUp();

    EXPECT_EQ(_renderer.materials_destroyed, 1u) << "the one that was made";
  }

  TEST_F(UiShaderTest, SaysWhatIsWrongWithAShader)
  {
    EXPECT_THAT(ProblemsOf("shader: shine\n"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'shader' of panel 'box' is 'shine', where a virtual path "
                  "without an extension such as assets://shaders/ui/shine, or none, was expected"));

    EXPECT_THAT(ProblemsOf("shader: [a]\n"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'shader' of panel 'box' is a list, where a virtual path such as "
                  "assets://ui/panel.png, or none was expected"));

    EXPECT_THAT(ProblemsOf("shader_values: 3\n"), ElementsAre(
                  "assets://ui/test.ui.yml:9: 'shader_values' of panel 'box' is a number, where a map of names "
                  "and values, such as { intensity: 0.5 } was expected"));

    EXPECT_THAT(ProblemsOf("shader_values:\n  speed: fast\n"), ElementsAre(
                  "assets://ui/test.ui.yml:10: 'speed' of 'shader_values' of panel 'box' is 'fast', where a "
                  "number, a colour, a list of 1 to 4 numbers, or a value such as \"{charge}\" was "
                  "expected"));

    EXPECT_THAT(ProblemsOf("shader_values:\n  offset: [1, 2, 3, 4, 5]\n"), ElementsAre(
                  "assets://ui/test.ui.yml:10: 'offset' of 'shader_values' of panel 'box' is a list, where a "
                  "number, a colour, a list of 1 to 4 numbers, or a value such as \"{charge}\" was "
                  "expected"));
  }
}
