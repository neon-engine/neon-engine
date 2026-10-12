#include "project-template.hpp"

#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <neon/testing/fake-native-file-context.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  class ProjectTemplateTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<neon::testing::RecordingLogger> logger = std::make_shared<neon::testing::RecordingLogger>();
    neon::testing::FakeNativeFileContext files;
    neon::editor::ProjectTemplate project_template{&files, logger};

    neon::editor::NewProject project{
      .name = "my-game",
      .organization = "acme",
      .parent_directory = "/home/someone/Dev",
      .engine_directory = "/home/someone/Dev/neon-engine",
    };
    std::string directory;
    std::string error;

    void SetUp() override
    {
      // a checkout of the engine is there, and the parent folder is not yet
      files.MakeDirectories("/home/someone/Dev/neon-engine/cmake");
      files.WriteText("/home/someone/Dev/neon-engine/cmake/NeonSdk.cmake", "");
    }
  };

  TEST_F(ProjectTemplateTest, DefaultParentIsDevUnderTheHomeFolder)
  {
    std::string parent;
    ASSERT_TRUE(project_template.DefaultParentDirectory(parent));
    EXPECT_EQ(parent, "/home/someone/Dev");
  }

  TEST_F(ProjectTemplateTest, DefaultParentNeedsTheHomeFolder)
  {
    files.home.clear();

    std::string parent;
    EXPECT_FALSE(project_template.DefaultParentDirectory(parent));
  }

  TEST_F(ProjectTemplateTest, PlainNamesAreLowercaseLettersDigitsAndDashes)
  {
    EXPECT_TRUE(neon::editor::ProjectTemplate::IsPlainName("my-game-2"));
    EXPECT_FALSE(neon::editor::ProjectTemplate::IsPlainName(""));
    EXPECT_FALSE(neon::editor::ProjectTemplate::IsPlainName("My Game"));
    EXPECT_FALSE(neon::editor::ProjectTemplate::IsPlainName("2games"));
    EXPECT_FALSE(neon::editor::ProjectTemplate::IsPlainName("my_game"));
  }

  TEST_F(ProjectTemplateTest, MakesTheFolderWithEverythingAProjectNeeds)
  {
    ASSERT_TRUE(project_template.Create(project, directory, error)) << error;

    EXPECT_EQ(directory, "/home/someone/Dev/my-game");
    for (const char *file : {
           "CMakeLists.txt", "CMakePresets.json", ".gitignore", "README.md",
           "assets/project.yml", "assets/settings.yml", "assets/scenes/start.scene.yml",
           "assets/input/game.input.yml",
           "extensions/my-game/extension.yml", "extensions/my-game/my-game.cpp",
         })
    {
      EXPECT_TRUE(files.files.contains("/home/someone/Dev/my-game/" + std::string(file))) << file;
    }
    // the ten files, and the NeonSdk.cmake of the engine
    EXPECT_EQ(files.files.size(), 11u);
    EXPECT_TRUE(logger->Contains(neon::testing::LogLevel::Info, "Made the project my-game of acme"));
  }

  TEST_F(ProjectTemplateTest, TheFilesCarryTheNamesAndTheEngine)
  {
    ASSERT_TRUE(project_template.Create(project, directory, error)) << error;

    const std::string &project_file = files.files.at("/home/someone/Dev/my-game/assets/project.yml");
    EXPECT_NE(project_file.find("name: my-game\n"), std::string::npos);
    EXPECT_NE(project_file.find("organization: acme\n"), std::string::npos);

    const std::string &lists = files.files.at("/home/someone/Dev/my-game/CMakeLists.txt");
    EXPECT_NE(lists.find("set(NEON_ENGINE_DIRECTORY \"/home/someone/Dev/neon-engine\" CACHE PATH"), std::string::npos);
    EXPECT_NE(lists.find("neon_add_project(my-game"), std::string::npos);
    EXPECT_NE(lists.find("extensions/my-game/my-game.cpp"), std::string::npos);

    const std::string &recipe = files.files.at("/home/someone/Dev/my-game/extensions/my-game/extension.yml");
    EXPECT_NE(recipe.find("name: my-game\n"), std::string::npos);
    EXPECT_NE(recipe.find("macos-arm64: my-game-macos-arm64.dylib"), std::string::npos);
  }

  TEST_F(ProjectTemplateTest, GoesUnderDevByDefault)
  {
    project.parent_directory.clear();

    ASSERT_TRUE(project_template.Create(project, directory, error)) << error;
    EXPECT_EQ(directory, "/home/someone/Dev/my-game");
  }

  TEST_F(ProjectTemplateTest, MakesTheParentFolderWhenItIsMissing)
  {
    project.parent_directory = "/home/someone/Games";

    ASSERT_TRUE(project_template.Create(project, directory, error)) << error;
    EXPECT_TRUE(files.IsDirectory("/home/someone/Games"));
    EXPECT_TRUE(files.IsDirectory("/home/someone/Games/my-game/extensions/my-game"));
  }

  TEST_F(ProjectTemplateTest, RefusesANameThatIsNotPlain)
  {
    project.name = "My Game";

    EXPECT_FALSE(project_template.Create(project, directory, error));
    EXPECT_NE(error.find("'My Game' is not a plain name for a project"), std::string::npos);
    EXPECT_FALSE(files.Exists("/home/someone/Dev/My Game"));
  }

  TEST_F(ProjectTemplateTest, RefusesAnOrganizationThatIsNotPlain)
  {
    project.organization = "ACME Inc.";

    EXPECT_FALSE(project_template.Create(project, directory, error));
    EXPECT_NE(error.find("'ACME Inc.' is not a plain name for an organization"), std::string::npos);
  }

  TEST_F(ProjectTemplateTest, RefusesAnEngineFolderWithoutTheSdk)
  {
    project.engine_directory = "/home/someone/elsewhere";

    EXPECT_FALSE(project_template.Create(project, directory, error));
    EXPECT_NE(error.find("The engine is not at '/home/someone/elsewhere'"), std::string::npos);
  }

  TEST_F(ProjectTemplateTest, RefusesAFolderThatIsThereAlready)
  {
    files.MakeDirectories("/home/someone/Dev/my-game");

    EXPECT_FALSE(project_template.Create(project, directory, error));
    EXPECT_NE(error.find("There is already something at /home/someone/Dev/my-game"), std::string::npos);
    EXPECT_FALSE(files.Exists("/home/someone/Dev/my-game/CMakeLists.txt"));
  }

  TEST_F(ProjectTemplateTest, SaysWhichFileCouldNotBeWritten)
  {
    files.refuses_writes = true;

    EXPECT_FALSE(project_template.Create(project, directory, error));
    EXPECT_NE(error.find("could not be created"), std::string::npos);
  }

  TEST_F(ProjectTemplateTest, JoinsWithTheSeparatorOfThePlatform)
  {
    files.separator = '\\';
    files.home = "C:\\Users\\someone";
    files.files.clear();
    files.directories.clear();
    files.MakeDirectories("C:\\Users\\someone\\Dev\\neon-engine\\cmake");
    files.WriteText("C:\\Users\\someone\\Dev\\neon-engine\\cmake\\NeonSdk.cmake", "");
    project.parent_directory.clear();
    project.engine_directory = "C:\\Users\\someone\\Dev\\neon-engine";

    ASSERT_TRUE(project_template.Create(project, directory, error)) << error;

    EXPECT_EQ(directory, "C:\\Users\\someone\\Dev\\my-game");
    EXPECT_TRUE(files.files.contains("C:\\Users\\someone\\Dev\\my-game\\extensions\\my-game\\my-game.cpp"));

    // CMake reads forward slashes on every platform
    const std::string &lists = files.files.at("C:\\Users\\someone\\Dev\\my-game\\CMakeLists.txt");
    EXPECT_NE(lists.find("\"C:/Users/someone/Dev/neon-engine\""), std::string::npos);
  }
}
