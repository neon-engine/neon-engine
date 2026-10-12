#include "project-template-text.hpp"

#include <string>

#include <gtest/gtest.h>

namespace
{
  using neon::editor::ProjectTemplateText;

  const neon::editor::NewProject project{
    .name = "my-game",
    .organization = "acme",
    .parent_directory = "/home/someone/Dev",
    .engine_directory = "/home/someone/Dev/neon-engine",
  };

  TEST(ProjectTemplateText, NothingIsLeftToFillIn)
  {
    for (const std::string &text : {
           ProjectTemplateText::ProjectFile(project), ProjectTemplateText::Settings(project),
           ProjectTemplateText::StartScene(project), ProjectTemplateText::InputMap(project),
           ProjectTemplateText::ExtensionRecipe(project), ProjectTemplateText::ExtensionSource(project),
           ProjectTemplateText::CMakeLists(project), ProjectTemplateText::CMakePresets(),
           ProjectTemplateText::GitIgnore(), ProjectTemplateText::ReadMe(project),
         })
    {
      // what is filled in stands between two @, as @name@
      EXPECT_EQ(text.find("@name@"), std::string::npos) << text;
      EXPECT_EQ(text.find("@organization@"), std::string::npos) << text;
      EXPECT_EQ(text.find("@engine@"), std::string::npos) << text;
      EXPECT_EQ(text.back(), '\n');
    }
  }

  TEST(ProjectTemplateText, TheProjectNamesItsSceneAndItsInputMap)
  {
    const std::string text = ProjectTemplateText::ProjectFile(project);
    EXPECT_NE(text.find("entry_scene: assets://scenes/start.scene.yml"), std::string::npos);
    EXPECT_NE(text.find("input: assets://input/game.input.yml"), std::string::npos);
  }

  TEST(ProjectTemplateText, TheSceneHasABoxTheExtensionTurns)
  {
    const std::string text = ProjectTemplateText::StartScene(project);
    EXPECT_NE(text.find("shape: box"), std::string::npos);
    EXPECT_NE(text.find("Spinner:"), std::string::npos);
    EXPECT_EQ(text.find("{}"), std::string::npos);
  }

  TEST(ProjectTemplateText, TheExtensionRegistersTheSpinner)
  {
    const std::string text = ProjectTemplateText::ExtensionSource(project);
    EXPECT_NE(text.find("#include <neon/extension/neon-extension.hpp>"), std::string::npos);
    EXPECT_NE(text.find("RegisterComponent<Spinner>(\"Spinner\""), std::string::npos);
    EXPECT_NE(text.find("NEON_EXTENSION(game::Game)"), std::string::npos);
  }

  TEST(ProjectTemplateText, TheListsNameTheEngineWithForwardSlashes)
  {
    neon::editor::NewProject on_windows = project;
    on_windows.engine_directory = "C:\\Users\\someone\\neon-engine";

    const std::string text = ProjectTemplateText::CMakeLists(on_windows);
    EXPECT_NE(text.find("set(NEON_ENGINE_DIRECTORY \"C:/Users/someone/neon-engine\""), std::string::npos);
    EXPECT_NE(text.find("include(\"${NEON_ENGINE_DIRECTORY}/cmake/NeonSdk.cmake\")"), std::string::npos);
  }

  TEST(ProjectTemplateText, ThePresetsAreTheSameForEveryProject)
  {
    const std::string text = ProjectTemplateText::CMakePresets();
    for (const char *preset : {
           "\"name\": \"macos-arm64-debug\"", "\"name\": \"macos-arm64-release\"",
           "\"name\": \"linux-x64-debug\"", "\"name\": \"linux-x64-release\"",
           "\"name\": \"windows-x64-debug\"", "\"name\": \"windows-x64-release\"",
         })
    {
      EXPECT_NE(text.find(preset), std::string::npos) << preset;
    }
    EXPECT_EQ(text.find("my-game"), std::string::npos);
    EXPECT_EQ(text.find("neon-engine"), std::string::npos);
  }

  TEST(ProjectTemplateText, TheReadMeSaysHowToBuildAndRun)
  {
    const std::string text = ProjectTemplateText::ReadMe(project);
    EXPECT_NE(text.find("cmake --preset macos-arm64-debug"), std::string::npos);
    EXPECT_NE(text.find("build/macos-arm64-debug/my-game/NeonRuntime"), std::string::npos);
  }
}
