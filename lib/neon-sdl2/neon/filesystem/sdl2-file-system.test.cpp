#include "sdl2-file-system.hpp"

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/recording-logger.hpp>
#include <neon/testing/temporary-directory.hpp>

// The path rules are tested in neon-core, against a file system in memory.
// These tests are about what this backend adds: real folders and real files.
//
// assets:// is the folder `assets` next to the test, which the build fills,
// see CMakeLists.txt. user:// and output:// are sent to a temporary folder
// that is removed after each test.

namespace
{
  using neon::SDL2_FileSystem;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;
  using neon::testing::TemporaryDirectory;
  using ::testing::ElementsAre;

  /// Sends the folder of the user to a place of our choice, for this process.
  /// Returns false where that is not possible.
  bool RedirectUserDirectory(const std::string &home, const std::string &data)
  {
#if defined(_WIN32)
    // Windows is asked for the folder directly and takes no hint
    return false;
#else
    // Linux follows XDG_DATA_HOME, macOS follows the home folder
    return setenv("XDG_DATA_HOME", data.c_str(), 1) == 0 &&
           setenv("CFFIXED_USER_HOME", home.c_str(), 1) == 0 &&
           setenv("HOME", home.c_str(), 1) == 0;
#endif
  }

  class Sdl2FileSystemTest : public ::testing::Test
  {
  protected:
    TemporaryDirectory _directory;
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    std::unique_ptr<SDL2_FileSystem> _file_system;

    // where user:// is, relative to the temporary folder
    std::string _user;

    void SetUp() override
    {
      if (!RedirectUserDirectory(_directory.Native("home"), _directory.Native("data")))
      {
        GTEST_SKIP() << "The folder of the user cannot be sent elsewhere on this platform, "
                        "and a test must not write into the real one";
      }

      Start(SettingsConfig{
        .organization = "neon-engine-tests",
        .application = "sdl2-file-system",
        .output_directory = _directory.Native("output")
      });

      // make sure of it, before anything is written
      const std::string prefix = "user:// is " + _directory.Native();
      if (!_logger->Contains(LogLevel::Info, prefix))
      {
        _file_system.reset();
        GTEST_SKIP() << "user:// did not follow to the temporary folder: " << _logger->Messages(LogLevel::Info);
      }

      for (const auto &[level, message] : _logger->Entries())
      {
        if (!message.starts_with(prefix)) { continue; }

        // what follows the temporary folder and its separator, with forward slashes
        _user = message.substr(prefix.size() + 1);
        for (char &character : _user)
        {
          if (character == '\\') { character = '/'; }
        }
      }
    }

    void TearDown() override
    {
      if (_file_system != nullptr) { _file_system->CleanUp(); }
    }

    /// Starts a file system with the settings, in place of the one before.
    void Start(const SettingsConfig &settings)
    {
      if (_file_system != nullptr) { _file_system->CleanUp(); }
      _logger->Clear();

      _file_system = std::make_unique<SDL2_FileSystem>(settings, _logger);
      _file_system->Initialize();
    }

    void ExpectNoErrors() const
    {
      EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
      EXPECT_EQ(_logger->Count(LogLevel::Critical), 0u) << _logger->Messages(LogLevel::Critical);
    }

    void ExpectError(const std::string &text) const
    {
      EXPECT_TRUE(_logger->Contains(LogLevel::Error, text)) << _logger->Messages(LogLevel::Error);
    }
  };

  // starting

  TEST_F(Sdl2FileSystemTest, SaysWhereEveryFolderIs)
  {
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "assets:// is "));
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "user:// is " + _directory.Native()));
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "output:// is " + _directory.Native("output")));
    ExpectNoErrors();
  }

  TEST_F(Sdl2FileSystemTest, PutsTheFolderOfTheUserUnderOrganizationAndApplication)
  {
    EXPECT_NE(_user.find("neon-engine-tests"), std::string::npos) << _user;
    EXPECT_NE(_user.find("sdl2-file-system"), std::string::npos) << _user;
    EXPECT_LT(_user.find("neon-engine-tests"), _user.find("sdl2-file-system"));
  }

  TEST_F(Sdl2FileSystemTest, CreatesTheFolderOfTheUser)
  {
    EXPECT_TRUE(_directory.HasDirectory(_user));
  }

  TEST_F(Sdl2FileSystemTest, GivesEveryApplicationAFolderOfItsOwn)
  {
    const std::string first = _user;
    _directory.Write(first + "save.dat", "of the first");

    Start(SettingsConfig{.organization = "neon-engine-tests", .application = "another-application"});

    EXPECT_FALSE(_file_system->Exists("user://save.dat"));
    EXPECT_TRUE(_file_system->WriteText("user://save.dat", "of the second"));
    EXPECT_EQ(_directory.Read(first + "save.dat"), "of the first");
  }

  // assets://

  TEST_F(Sdl2FileSystemTest, ReadsAnAsset)
  {
    std::string contents;

    EXPECT_TRUE(_file_system->Exists("assets://models/cube.obj"));
    ASSERT_TRUE(_file_system->ReadText("assets://models/cube.obj", contents));
    EXPECT_EQ(contents, "a cube\n");
    ExpectNoErrors();
  }

  TEST_F(Sdl2FileSystemTest, ReadsAssetsAtTheTopAndInsideFolders)
  {
    std::string contents;

    ASSERT_TRUE(_file_system->ReadText("assets://readme.txt", contents));
    EXPECT_EQ(contents, "at the top\n");
    ASSERT_TRUE(_file_system->ReadText("assets://models/props/chair.obj", contents));
    EXPECT_EQ(contents, "a chair\n");
    ExpectNoErrors();
  }

  TEST_F(Sdl2FileSystemTest, ReadsAnAssetAsBytes)
  {
    std::vector<unsigned char> contents;

    ASSERT_TRUE(_file_system->ReadBytes("assets://models/cube.obj", contents));

    EXPECT_THAT(contents, ElementsAre('a', ' ', 'c', 'u', 'b', 'e', '\n'));
  }

  TEST_F(Sdl2FileSystemTest, ReadsAnEmptyFile)
  {
    std::string contents = "something";

    EXPECT_TRUE(_file_system->Exists("assets://empty.txt"));
    ASSERT_TRUE(_file_system->ReadText("assets://empty.txt", contents));
    EXPECT_EQ(contents, "");
    ExpectNoErrors();
  }

  TEST_F(Sdl2FileSystemTest, DoesNotFindAnAssetThatIsMissingAndLogsNoError)
  {
    std::string contents = "untouched";

    EXPECT_FALSE(_file_system->Exists("assets://models/sphere.obj"));
    EXPECT_FALSE(_file_system->Exists("assets://missing/sphere.obj"));
    EXPECT_FALSE(_file_system->ReadText("assets://models/sphere.obj", contents));
    EXPECT_EQ(contents, "untouched");
    ExpectNoErrors();
  }

  TEST_F(Sdl2FileSystemTest, FindsAnAssetWhoseNameHasCapitals)
  {
    std::string contents;

    ASSERT_TRUE(_file_system->ReadText("assets://Textures/Wood-Oak.PNG", contents));
    EXPECT_EQ(contents, "a texture\n");
    ExpectNoErrors();
  }

  // This is the test that matters most on macOS and Windows, whose own file
  // systems would find the file.
  TEST_F(Sdl2FileSystemTest, RefusesAnAssetInAnotherLetterCase)
  {
    std::string contents = "untouched";

    EXPECT_FALSE(_file_system->Exists("assets://Models/cube.obj"));
    ExpectError("'Models' is named 'models' on disk, letter case has to match on every platform");

    _logger->Clear();
    EXPECT_FALSE(_file_system->ReadText("assets://models/Cube.obj", contents));
    ExpectError("'Cube.obj' is named 'cube.obj' on disk, letter case has to match on every platform");

    _logger->Clear();
    EXPECT_FALSE(_file_system->Exists("assets://textures/Wood-Oak.PNG"));
    ExpectError("'textures' is named 'Textures' on disk, letter case has to match on every platform");

    _logger->Clear();
    EXPECT_FALSE(_file_system->Exists("assets://Textures/wood-oak.png"));
    ExpectError("'wood-oak.png' is named 'Wood-Oak.PNG' on disk, letter case has to match on every platform");

    EXPECT_EQ(contents, "untouched");
  }

  TEST_F(Sdl2FileSystemTest, RefusesToWriteToAssets)
  {
    EXPECT_FALSE(_file_system->WriteText("assets://models/cube.obj", "changed"));
    ExpectError("Invalid path 'assets://models/cube.obj': its scheme is read-only");

    EXPECT_FALSE(_file_system->WriteText("assets://written-by-a-test.txt", "new"));
    EXPECT_FALSE(_file_system->Exists("assets://written-by-a-test.txt"));

    std::string contents;
    ASSERT_TRUE(_file_system->ReadText("assets://models/cube.obj", contents));
    EXPECT_EQ(contents, "a cube\n");
  }

  TEST_F(Sdl2FileSystemTest, RefusesAPathThatBreaksARule)
  {
    EXPECT_FALSE(_file_system->Exists("assets://models/../models/cube.obj"));
    EXPECT_FALSE(_file_system->Exists("assets://models\\cube.obj"));
    EXPECT_FALSE(_file_system->Exists("models/cube.obj"));
    EXPECT_FALSE(_file_system->Exists(_directory.Native("output")));

    EXPECT_EQ(_logger->Count(LogLevel::Error), 4u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(Sdl2FileSystemTest, DoesNotTakeAFolderForAFile)
  {
    std::vector<unsigned char> contents;

    EXPECT_FALSE(_file_system->Exists("assets://models"));
    EXPECT_FALSE(_file_system->ReadBytes("assets://models", contents));
  }

  // user://

  TEST_F(Sdl2FileSystemTest, WritesAndReadsAFileOfTheUser)
  {
    ASSERT_TRUE(_file_system->WriteText("user://settings.ini", "volume=11\n"));

    std::string contents;
    EXPECT_TRUE(_file_system->Exists("user://settings.ini"));
    ASSERT_TRUE(_file_system->ReadText("user://settings.ini", contents));
    EXPECT_EQ(contents, "volume=11\n");
    EXPECT_EQ(_directory.Read(_user + "settings.ini"), "volume=11\n");
    ExpectNoErrors();
  }

  TEST_F(Sdl2FileSystemTest, WritesAndReadsEveryByte)
  {
    std::vector<unsigned char> written;
    for (int i = 0; i < 256; i++) { written.push_back(static_cast<unsigned char>(i)); }
    // line endings are left as they are
    written.insert(written.end(), {'\r', '\n', '\n', '\r', 0x1a, 0});

    ASSERT_TRUE(_file_system->WriteBytes("user://bytes.bin", written));

    std::vector<unsigned char> contents;
    ASSERT_TRUE(_file_system->ReadBytes("user://bytes.bin", contents));
    EXPECT_EQ(contents, written);
    ExpectNoErrors();
  }

  TEST_F(Sdl2FileSystemTest, WritesAndReadsALargeFile)
  {
    std::vector<unsigned char> written(3 * 1024 * 1024 + 17);
    for (std::size_t i = 0; i < written.size(); i++) { written[i] = static_cast<unsigned char>(i * 31 + i / 251); }

    ASSERT_TRUE(_file_system->WriteBytes("user://large.bin", written));

    std::vector<unsigned char> contents;
    ASSERT_TRUE(_file_system->ReadBytes("user://large.bin", contents));
    EXPECT_TRUE(contents == written);
  }

  TEST_F(Sdl2FileSystemTest, WritesAnEmptyFile)
  {
    ASSERT_TRUE(_file_system->WriteText("user://empty.txt", ""));

    std::string contents = "something";
    EXPECT_TRUE(_file_system->Exists("user://empty.txt"));
    ASSERT_TRUE(_file_system->ReadText("user://empty.txt", contents));
    EXPECT_EQ(contents, "");
  }

  TEST_F(Sdl2FileSystemTest, ReplacesAFileWithAShorterOne)
  {
    ASSERT_TRUE(_file_system->WriteText("user://save.dat", "the first save, which is the longer one"));
    ASSERT_TRUE(_file_system->WriteText("user://save.dat", "the second"));

    std::string contents;
    ASSERT_TRUE(_file_system->ReadText("user://save.dat", contents));
    EXPECT_EQ(contents, "the second");
  }

  TEST_F(Sdl2FileSystemTest, CreatesTheFoldersThatLeadToAFile)
  {
    ASSERT_TRUE(_file_system->WriteText("user://saves/slot-1/today/save.dat", "a save"));

    EXPECT_TRUE(_directory.HasDirectory(_user + "saves/slot-1/today"));
    EXPECT_EQ(_directory.Read(_user + "saves/slot-1/today/save.dat"), "a save");

    std::string contents;
    ASSERT_TRUE(_file_system->ReadText("user://saves/slot-1/today/save.dat", contents));
    EXPECT_EQ(contents, "a save");
    ExpectNoErrors();
  }

  TEST_F(Sdl2FileSystemTest, WritesIntoAFolderThatExists)
  {
    ASSERT_TRUE(_file_system->WriteText("user://saves/first.dat", "first"));
    ASSERT_TRUE(_file_system->WriteText("user://saves/second.dat", "second"));

    EXPECT_EQ(_directory.Read(_user + "saves/first.dat"), "first");
    EXPECT_EQ(_directory.Read(_user + "saves/second.dat"), "second");
    ExpectNoErrors();
  }

  TEST_F(Sdl2FileSystemTest, FindsWhatWasPutIntoTheFolderOfTheUserFromOutside)
  {
    _directory.Write(_user + "mods/Big Mod/readme.txt", "installed by hand");

    std::string contents;
    ASSERT_TRUE(_file_system->ReadText("user://mods/Big Mod/readme.txt", contents));
    EXPECT_EQ(contents, "installed by hand");
  }

  TEST_F(Sdl2FileSystemTest, RefusesToWriteAFileThatDiffersFromAnotherInLetterCaseOnly)
  {
    ASSERT_TRUE(_file_system->WriteText("user://Save.dat", "the one on disk"));
    _logger->Clear();

    EXPECT_FALSE(_file_system->WriteText("user://save.dat", "the other"));

    ExpectError("'save.dat' is named 'Save.dat' on disk, letter case has to match on every platform");
    EXPECT_EQ(_directory.Read(_user + "Save.dat"), "the one on disk");
  }

  TEST_F(Sdl2FileSystemTest, RefusesToWriteIntoAFolderThatDiffersFromAnotherInLetterCaseOnly)
  {
    ASSERT_TRUE(_file_system->WriteText("user://Saves/save.dat", "the one on disk"));
    _logger->Clear();

    EXPECT_FALSE(_file_system->WriteText("user://saves/other.dat", "the other"));

    ExpectError("'saves' is named 'Saves' on disk, letter case has to match on every platform");
    EXPECT_FALSE(_directory.HasFile(_user + "Saves/other.dat"));
  }

  TEST_F(Sdl2FileSystemTest, RefusesToWriteAFileWhereAFolderIs)
  {
    ASSERT_TRUE(_file_system->WriteText("user://saves/save.dat", "a save"));
    _logger->Clear();

    EXPECT_FALSE(_file_system->WriteText("user://saves", "in the way"));

    EXPECT_GE(_logger->Count(LogLevel::Error), 1u);
    EXPECT_TRUE(_directory.HasDirectory(_user + "saves"));
  }

  TEST_F(Sdl2FileSystemTest, RefusesToWriteBelowAFile)
  {
    ASSERT_TRUE(_file_system->WriteText("user://save.dat", "a save"));
    _logger->Clear();

    EXPECT_FALSE(_file_system->WriteText("user://save.dat/inside.dat", "below a file"));

    EXPECT_GE(_logger->Count(LogLevel::Error), 1u);
    EXPECT_EQ(_directory.Read(_user + "save.dat"), "a save");
  }

  // output://

  TEST_F(Sdl2FileSystemTest, CreatesTheOutputFolderWhenItIsMissing)
  {
    EXPECT_TRUE(_directory.HasDirectory("output"));

    Start(SettingsConfig{
      .organization = "neon-engine-tests",
      .application = "sdl2-file-system",
      .output_directory = _directory.Native("not/there/yet")
    });

    EXPECT_TRUE(_directory.HasDirectory("not/there/yet"));
    ExpectNoErrors();
  }

  TEST_F(Sdl2FileSystemTest, WritesToTheOutputFolder)
  {
    ASSERT_TRUE(_file_system->WriteText("output://frame.png", "a frame"));
    ASSERT_TRUE(_file_system->WriteText("output://shots/today/frame-0001.png", "another"));

    EXPECT_EQ(_directory.Read("output/frame.png"), "a frame");
    EXPECT_EQ(_directory.Read("output/shots/today/frame-0001.png"), "another");

    std::string contents;
    EXPECT_TRUE(_file_system->Exists("output://frame.png"));
    ASSERT_TRUE(_file_system->ReadText("output://frame.png", contents));
    EXPECT_EQ(contents, "a frame");
    ExpectNoErrors();
  }

  TEST_F(Sdl2FileSystemTest, UsesAnOutputFolderThatExistsAndKeepsWhatIsInIt)
  {
    _directory.Write("kept/earlier.png", "from an earlier run");

    Start(SettingsConfig{
      .organization = "neon-engine-tests",
      .application = "sdl2-file-system",
      .output_directory = _directory.Native("kept")
    });

    std::string contents;
    ASSERT_TRUE(_file_system->ReadText("output://earlier.png", contents));
    EXPECT_EQ(contents, "from an earlier run");
    ASSERT_TRUE(_file_system->WriteText("output://frame.png", "a frame"));
    EXPECT_EQ(_directory.Read("kept/earlier.png"), "from an earlier run");
  }

  TEST_F(Sdl2FileSystemTest, AcceptsAnOutputFolderThatEndsWithASeparator)
  {
    Start(SettingsConfig{
      .organization = "neon-engine-tests",
      .application = "sdl2-file-system",
      .output_directory = _directory.Native("shots") + "/"
    });

    ASSERT_TRUE(_file_system->WriteText("output://frame.png", "a frame"));
    EXPECT_EQ(_directory.Read("shots/frame.png"), "a frame");
    ExpectNoErrors();
  }

  TEST_F(Sdl2FileSystemTest, AcceptsAnOutputFolderThatIsWrittenWithDots)
  {
    Start(SettingsConfig{
      .organization = "neon-engine-tests",
      .application = "sdl2-file-system",
      .output_directory = _directory.Native("one") + "/../two/./shots"
    });

    ASSERT_TRUE(_file_system->WriteText("output://frame.png", "a frame"));
    EXPECT_EQ(_directory.Read("two/shots/frame.png"), "a frame");
    EXPECT_FALSE(_directory.HasDirectory("one"));
    ExpectNoErrors();
  }

  TEST_F(Sdl2FileSystemTest, AcceptsAnOutputFolderWithSpacesAndCapitals)
  {
    Start(SettingsConfig{
      .organization = "neon-engine-tests",
      .application = "sdl2-file-system",
      .output_directory = _directory.Native("My Shots")
    });

    ASSERT_TRUE(_file_system->WriteText("output://frame.png", "a frame"));
    EXPECT_EQ(_directory.Read("My Shots/frame.png"), "a frame");
  }

  TEST_F(Sdl2FileSystemTest, HoldsOutputToTheLetterCaseOfWhatIsInIt)
  {
    ASSERT_TRUE(_file_system->WriteText("output://Frame.png", "a frame"));
    _logger->Clear();

    EXPECT_FALSE(_file_system->Exists("output://frame.png"));
    ExpectError("'frame.png' is named 'Frame.png' on disk, letter case has to match on every platform");
  }

  TEST_F(Sdl2FileSystemTest, LeavesOutputWithoutAFolderWhenNoneWasChosen)
  {
    Start(SettingsConfig{.organization = "neon-engine-tests", .application = "sdl2-file-system"});

    EXPECT_FALSE(_logger->Contains(LogLevel::Info, "output:// is"));
    ExpectNoErrors();

    EXPECT_FALSE(_file_system->WriteText("output://frame.png", "a frame"));
    ExpectError("Invalid path 'output://frame.png': output:// has no folder");
    EXPECT_FALSE(_file_system->Exists("output://frame.png"));
  }

  TEST_F(Sdl2FileSystemTest, LeavesOutputWithoutAFolderWhenAFileIsInTheWay)
  {
    _directory.Write("taken", "a file, not a folder");

    Start(SettingsConfig{
      .organization = "neon-engine-tests",
      .application = "sdl2-file-system",
      .output_directory = _directory.Native("taken")
    });

    ExpectError("The output folder '" + _directory.Native("taken") + "' cannot be used");
    EXPECT_FALSE(_logger->Contains(LogLevel::Info, "output:// is"));

    _logger->Clear();
    EXPECT_FALSE(_file_system->WriteText("output://frame.png", "a frame"));
    ExpectError("Invalid path 'output://frame.png': output:// has no folder");
    EXPECT_EQ(_directory.Read("taken"), "a file, not a folder");
  }

  TEST_F(Sdl2FileSystemTest, LeavesOutputWithoutAFolderWhenItWouldBeBelowAFile)
  {
    _directory.Write("taken", "a file, not a folder");

    Start(SettingsConfig{
      .organization = "neon-engine-tests",
      .application = "sdl2-file-system",
      .output_directory = _directory.Native("taken/shots")
    });

    ExpectError("cannot be used");
    EXPECT_FALSE(_file_system->WriteText("output://frame.png", "a frame"));
  }

  TEST_F(Sdl2FileSystemTest, StillServesTheOtherSchemesWhenOutputHasNoFolder)
  {
    Start(SettingsConfig{.organization = "neon-engine-tests", .application = "sdl2-file-system"});

    EXPECT_TRUE(_file_system->Exists("assets://models/cube.obj"));
    EXPECT_TRUE(_file_system->WriteText("user://save.dat", "a save"));
    ExpectNoErrors();
  }

  // stopping

  TEST_F(Sdl2FileSystemTest, CanBeCleanedUpTwiceAndStartedAgain)
  {
    ASSERT_TRUE(_file_system->WriteText("user://save.dat", "a save"));

    _file_system->CleanUp();
    _file_system->CleanUp();
    _file_system->Initialize();

    std::string contents;
    ASSERT_TRUE(_file_system->ReadText("user://save.dat", contents));
    EXPECT_EQ(contents, "a save");
    EXPECT_TRUE(_file_system->Exists("assets://models/cube.obj"));
  }

  TEST_F(Sdl2FileSystemTest, IsAFileSystemLikeAnyOther)
  {
    neon::FileSystemContext &context = *_file_system;

    std::string contents;
    EXPECT_TRUE(context.WriteText("user://save.dat", "a save"));
    EXPECT_TRUE(context.ReadText("user://save.dat", contents));
    EXPECT_EQ(contents, "a save");
  }
}
