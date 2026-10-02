#include "file-system.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::FileSystem;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  /// A file system in memory with a folder behind every scheme.
  /// `assets://` is `/assets/`, `user://` is `/user/`, and `output://` is
  /// `/chosen/`.
  class FileSystemTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _file_system{SettingsConfig{.output_directory = "/chosen"}, _logger};

    void SetUp() override
    {
      _file_system.Initialize();
      _file_system.AddNativeFile("/assets/models/cube.obj", "a cube");
    }

    /// Expects that the path was refused, and that the one error that was
    /// logged names the path and holds the reason.
    void ExpectRefused(const std::string &path, const std::string &reason) const
    {
      EXPECT_EQ(_logger->Count(LogLevel::Error), 1u) << _logger->Messages(LogLevel::Error);
      EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Invalid path '" + path + "': "))
        << _logger->Messages(LogLevel::Error);
      EXPECT_TRUE(_logger->Contains(LogLevel::Error, reason)) << _logger->Messages(LogLevel::Error);
    }

    /// Expects that the path cannot be read, for the reason given.
    void ExpectNotReadable(const std::string &path, const std::string &reason)
    {
      std::string contents = "untouched";

      EXPECT_FALSE(_file_system.ReadText(path, contents)) << path;
      EXPECT_EQ(contents, "untouched");
      ExpectRefused(path, reason);

      _logger->Clear();
      EXPECT_FALSE(_file_system.Exists(path)) << path;
      ExpectRefused(path, reason);
    }

    /// Expects that the path cannot be written, for the reason given.
    void ExpectNotWritable(const std::string &path, const std::string &reason)
    {
      EXPECT_FALSE(_file_system.WriteText(path, "contents")) << path;
      ExpectRefused(path, reason);
      EXPECT_THAT(_file_system.MadeDirectories(), IsEmpty());
    }
  };

  constexpr auto unknown_scheme =
    "it does not start with a known scheme, which are assets://, user:// or output://";
  constexpr auto backslash = "it contains a backslash, folders are separated by forward slashes on every platform";
  constexpr auto forbidden_character =
    "it contains a character that is not allowed in file names on every platform";
  constexpr auto leaves_the_folder = "it contains '..', a path may not leave the folder of its scheme";
  constexpr auto ends_badly = "a file or folder name ends with a dot or a space, which not every platform keeps";
  constexpr auto names_no_file = "it names no file";
  constexpr auto read_only = "its scheme is read-only";

  TEST(FileSystem, NamesItsSchemes)
  {
    EXPECT_EQ(FileSystem::assets_scheme, "assets://");
    EXPECT_EQ(FileSystem::user_scheme, "user://");
    EXPECT_EQ(FileSystem::output_scheme, "output://");
  }

  // reading

  TEST_F(FileSystemTest, ReadsAFileAsText)
  {
    std::string contents;

    EXPECT_TRUE(_file_system.Exists("assets://models/cube.obj"));
    EXPECT_TRUE(_file_system.ReadText("assets://models/cube.obj", contents));
    EXPECT_EQ(contents, "a cube");
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(FileSystemTest, ReadsAFileAsBytes)
  {
    std::vector<unsigned char> contents;

    EXPECT_TRUE(_file_system.ReadBytes("assets://models/cube.obj", contents));
    EXPECT_THAT(contents, ElementsAre('a', ' ', 'c', 'u', 'b', 'e'));
  }

  TEST_F(FileSystemTest, ReadsTextWithEveryByteKept)
  {
    const std::string written{'a', '\0', 'b', '\n', '\r', '\xff', '\x80'};
    _file_system.AddNativeFile("/assets/bytes.bin", written);

    std::string contents;
    ASSERT_TRUE(_file_system.ReadText("assets://bytes.bin", contents));

    EXPECT_EQ(contents, written);
  }

  TEST_F(FileSystemTest, ReadsAnEmptyFile)
  {
    _file_system.AddNativeFile("/assets/empty.txt", "");

    std::string contents = "something";
    ASSERT_TRUE(_file_system.ReadText("assets://empty.txt", contents));

    EXPECT_EQ(contents, "");
  }

  TEST_F(FileSystemTest, ReplacesWhatTheTextHeldBefore)
  {
    std::string contents = "something that was there before";
    ASSERT_TRUE(_file_system.ReadText("assets://models/cube.obj", contents));

    EXPECT_EQ(contents, "a cube");
  }

  TEST_F(FileSystemTest, ReadsFromEveryScheme)
  {
    _file_system.AddNativeFile("/user/save.dat", "a save");
    _file_system.AddNativeFile("/chosen/frame.png", "a frame");

    std::string contents;
    EXPECT_TRUE(_file_system.ReadText("user://save.dat", contents));
    EXPECT_EQ(contents, "a save");
    EXPECT_TRUE(_file_system.ReadText("output://frame.png", contents));
    EXPECT_EQ(contents, "a frame");
  }

  TEST_F(FileSystemTest, KeepsTheSchemesApart)
  {
    EXPECT_FALSE(_file_system.Exists("user://models/cube.obj"));
    EXPECT_FALSE(_file_system.Exists("output://models/cube.obj"));
  }

  TEST_F(FileSystemTest, DoesNotFindAFileThatIsMissingAndLogsNoError)
  {
    std::string contents = "untouched";

    EXPECT_FALSE(_file_system.Exists("assets://models/sphere.obj"));
    EXPECT_FALSE(_file_system.ReadText("assets://models/sphere.obj", contents));
    EXPECT_EQ(contents, "untouched");
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(FileSystemTest, DoesNotFindAFileInAFolderThatIsMissingAndLogsNoError)
  {
    EXPECT_FALSE(_file_system.Exists("assets://textures/wood/oak.png"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(FileSystemTest, DoesNotFindAFileBelowAFile)
  {
    EXPECT_FALSE(_file_system.Exists("assets://models/cube.obj/cube.obj"));
  }

  TEST_F(FileSystemTest, DoesNotFindAFileWhenAFolderCannotBeListed)
  {
    _file_system.fail_to_list = true;

    EXPECT_FALSE(_file_system.Exists("assets://models/cube.obj"));
  }

  // the native path

  TEST_F(FileSystemTest, LocatesAFileBelowTheFolderOfItsScheme)
  {
    _file_system.AddNativeFile("/user/saves/slot-1/save.dat");
    _file_system.AddNativeFile("/chosen/frame.png");

    std::string native_path;
    EXPECT_TRUE(_file_system.LocateNative("assets://models/cube.obj", native_path));
    EXPECT_EQ(native_path, "/assets/models/cube.obj");
    EXPECT_TRUE(_file_system.LocateNative("user://saves/slot-1/save.dat", native_path));
    EXPECT_EQ(native_path, "/user/saves/slot-1/save.dat");
    EXPECT_TRUE(_file_system.LocateNative("output://frame.png", native_path));
    EXPECT_EQ(native_path, "/chosen/frame.png");
  }

  TEST_F(FileSystemTest, LeavesTheNativePathAloneWhenNothingIsFound)
  {
    std::string native_path = "untouched";

    EXPECT_FALSE(_file_system.LocateNative("assets://models/sphere.obj", native_path));
    EXPECT_FALSE(_file_system.LocateNative("models/cube.obj", native_path));
    EXPECT_EQ(native_path, "untouched");
  }

  TEST(FileSystem, JoinsFoldersWithTheSeparatorOfThePlatform)
  {
    const auto logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem file_system(SettingsConfig{.output_directory = "C:\\chosen"}, logger, '\\');
    file_system.Initialize();
    file_system.AddNativeFile("\\assets\\models\\cube.obj", "a cube");

    std::string native_path;
    EXPECT_TRUE(file_system.LocateNative("assets://models/cube.obj", native_path));
    EXPECT_EQ(native_path, "\\assets\\models\\cube.obj");

    EXPECT_TRUE(file_system.LocateNativeForWriting("output://shots/frame.png", native_path));
    EXPECT_EQ(native_path, "C:\\chosen\\shots\\frame.png");
    EXPECT_EQ(logger->Count(LogLevel::Error), 0u) << logger->Messages(LogLevel::Error);
  }

  // doubled slashes and "."

  class FileSystemHarmlessPath : public FileSystemTest, public ::testing::WithParamInterface<const char *> {};

  TEST_P(FileSystemHarmlessPath, FindsTheFile)
  {
    std::string native_path;

    EXPECT_TRUE(_file_system.LocateNative(GetParam(), native_path));
    EXPECT_EQ(native_path, "/assets/models/cube.obj");
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  INSTANTIATE_TEST_SUITE_P(
    FileSystem,
    FileSystemHarmlessPath,
    ::testing::Values(
      "assets://models/cube.obj",
      "assets://models//cube.obj",
      "assets://models////cube.obj",
      "assets:///models/cube.obj",
      "assets://./models/cube.obj",
      "assets://models/./cube.obj",
      "assets://models/././cube.obj",
      "assets://models/cube.obj/",
      "assets://models/cube.obj/."));

  TEST_F(FileSystemTest, AcceptsDotsInsideAName)
  {
    _file_system.AddNativeFile("/assets/shaders/basic-lit.vert.spv");
    _file_system.AddNativeFile("/assets/.hidden/..two-dots");
    _file_system.AddNativeFile("/assets/a..b");

    EXPECT_TRUE(_file_system.Exists("assets://shaders/basic-lit.vert.spv"));
    EXPECT_TRUE(_file_system.Exists("assets://.hidden/..two-dots"));
    EXPECT_TRUE(_file_system.Exists("assets://a..b"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(FileSystemTest, AcceptsSpacesInsideAName)
  {
    _file_system.AddNativeFile("/assets/my models/ a cube.obj");

    EXPECT_TRUE(_file_system.Exists("assets://my models/ a cube.obj"));
  }

  TEST_F(FileSystemTest, AcceptsCharactersOutsideAscii)
  {
    _file_system.AddNativeFile("/assets/modelle/w\xc3\xbcrfel.obj");

    EXPECT_TRUE(_file_system.Exists("assets://modelle/w\xc3\xbcrfel.obj"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  // the rules, for reading and for writing

  struct RefusedPath
  {
    std::string path;
    std::string reason;
  };

  void PrintTo(const RefusedPath &refused, std::ostream *stream)
  {
    *stream << ::testing::PrintToString(refused.path);
  }

  class FileSystemRefusedPath : public FileSystemTest, public ::testing::WithParamInterface<RefusedPath> {};

  TEST_P(FileSystemRefusedPath, CannotBeRead)
  {
    ExpectNotReadable(GetParam().path, GetParam().reason);
  }

  TEST_P(FileSystemRefusedPath, CannotBeWritten)
  {
    ExpectNotWritable(GetParam().path, GetParam().reason);
  }

  INSTANTIATE_TEST_SUITE_P(
    WithoutAKnownScheme,
    FileSystemRefusedPath,
    ::testing::Values(
      RefusedPath{"", unknown_scheme},
      RefusedPath{"cube.obj", unknown_scheme},
      RefusedPath{"assets/models/cube.obj", unknown_scheme},
      RefusedPath{"/assets/models/cube.obj", unknown_scheme},
      RefusedPath{"/usr/share/x.obj", unknown_scheme},
      RefusedPath{"C:\\game\\x.obj", unknown_scheme},
      RefusedPath{"./models/cube.obj", unknown_scheme},
      RefusedPath{"../models/cube.obj", unknown_scheme},
      RefusedPath{"file://models/cube.obj", unknown_scheme},
      RefusedPath{"res://models/cube.obj", unknown_scheme},
      RefusedPath{"assets:/models/cube.obj", unknown_scheme},
      RefusedPath{"assets:models/cube.obj", unknown_scheme},
      RefusedPath{"Assets://models/cube.obj", unknown_scheme},
      RefusedPath{"ASSETS://models/cube.obj", unknown_scheme},
      RefusedPath{"User://save.dat", unknown_scheme},
      RefusedPath{" assets://models/cube.obj", unknown_scheme},
      RefusedPath{"://models/cube.obj", unknown_scheme}));

  INSTANTIATE_TEST_SUITE_P(
    WithABackslash,
    FileSystemRefusedPath,
    ::testing::Values(
      RefusedPath{"user://models\\cube.obj", backslash},
      RefusedPath{"user://models/cube.obj\\", backslash},
      RefusedPath{"user://\\models/cube.obj", backslash},
      RefusedPath{"user://..\\settings.ini", backslash}));

  INSTANTIATE_TEST_SUITE_P(
    WithAForbiddenCharacter,
    FileSystemRefusedPath,
    ::testing::Values(
      RefusedPath{"user://models/what?.obj", forbidden_character},
      RefusedPath{"user://models/a<b.obj", forbidden_character},
      RefusedPath{"user://models/a>b.obj", forbidden_character},
      RefusedPath{"user://models/a:b.obj", forbidden_character},
      RefusedPath{"user://models/a\"b.obj", forbidden_character},
      RefusedPath{"user://models/a|b.obj", forbidden_character},
      RefusedPath{"user://models/*.obj", forbidden_character},
      RefusedPath{"user://C:/models/cube.obj", forbidden_character},
      RefusedPath{"user://assets://cube.obj", forbidden_character},
      RefusedPath{"user://models/a\tb.obj", forbidden_character},
      RefusedPath{"user://models/cube.obj\n", forbidden_character},
      RefusedPath{"user://models/a\x01" "b.obj", forbidden_character},
      RefusedPath{"user://models/a\x1f" "b.obj", forbidden_character}));

  INSTANTIATE_TEST_SUITE_P(
    ThatLeavesItsFolder,
    FileSystemRefusedPath,
    ::testing::Values(
      RefusedPath{"user://../settings.ini", leaves_the_folder},
      RefusedPath{"user://..", leaves_the_folder},
      RefusedPath{"user://models/../cube.obj", leaves_the_folder},
      RefusedPath{"user://models/..", leaves_the_folder},
      RefusedPath{"user://models/../../../etc/passwd", leaves_the_folder},
      RefusedPath{"user://./../cube.obj", leaves_the_folder}));

  INSTANTIATE_TEST_SUITE_P(
    WithANameThatEndsBadly,
    FileSystemRefusedPath,
    ::testing::Values(
      RefusedPath{"user://models/name./cube.obj", ends_badly},
      RefusedPath{"user://models/cube.obj.", ends_badly},
      RefusedPath{"user://models/cube.obj ", ends_badly},
      RefusedPath{"user://models /cube.obj", ends_badly},
      RefusedPath{"user:// ", ends_badly},
      RefusedPath{"user://models/...", ends_badly}));

  INSTANTIATE_TEST_SUITE_P(
    ThatNamesNoFile,
    FileSystemRefusedPath,
    ::testing::Values(
      RefusedPath{"user://", names_no_file},
      RefusedPath{"user:///", names_no_file},
      RefusedPath{"user://.", names_no_file},
      RefusedPath{"user://./", names_no_file},
      RefusedPath{"user://.//./", names_no_file}));

  TEST_F(FileSystemTest, ChecksTheRulesOfAssetsToo)
  {
    ExpectNotReadable("assets://../settings.ini", leaves_the_folder);
    _logger->Clear();
    ExpectNotReadable("assets://models\\cube.obj", backslash);
    _logger->Clear();
    ExpectNotReadable("assets://", names_no_file);
  }

  // letter case, when reading

  TEST_F(FileSystemTest, RefusesAFolderInAnotherLetterCase)
  {
    ExpectNotReadable(
      "assets://Models/cube.obj",
      "'Models' is named 'models' on disk, letter case has to match on every platform");
  }

  TEST_F(FileSystemTest, RefusesAFileInAnotherLetterCase)
  {
    ExpectNotReadable(
      "assets://models/Cube.OBJ",
      "'Cube.OBJ' is named 'cube.obj' on disk, letter case has to match on every platform");
  }

  TEST_F(FileSystemTest, NamesTheFirstPartOfThePathThatIsInAnotherLetterCase)
  {
    ExpectNotReadable(
      "assets://MODELS/CUBE.OBJ",
      "'MODELS' is named 'models' on disk, letter case has to match on every platform");
  }

  TEST_F(FileSystemTest, FindsANameInAnyLetterCaseWhenThePathMatchesIt)
  {
    _file_system.AddNativeFile("/assets/Textures/Wood-Oak.PNG");

    EXPECT_TRUE(_file_system.Exists("assets://Textures/Wood-Oak.PNG"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(FileSystemTest, FindsEachOfTwoNamesThatDifferInLetterCaseOnly)
  {
    // what a file system that tells letter case apart can hold
    _file_system.AddNativeFile("/assets/models/Cube.obj", "the upper one");

    std::string contents;
    EXPECT_TRUE(_file_system.ReadText("assets://models/cube.obj", contents));
    EXPECT_EQ(contents, "a cube");
    EXPECT_TRUE(_file_system.ReadText("assets://models/Cube.obj", contents));
    EXPECT_EQ(contents, "the upper one");
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  // writing

  TEST_F(FileSystemTest, WritesTextToTheFolderOfTheUser)
  {
    ASSERT_TRUE(_file_system.WriteText("user://save.dat", "a save"));

    EXPECT_TRUE(_file_system.HasNativeFile("/user/save.dat"));

    std::string contents;
    EXPECT_TRUE(_file_system.ReadText("user://save.dat", contents));
    EXPECT_EQ(contents, "a save");
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(FileSystemTest, WritesTextWithEveryByteKept)
  {
    const std::string written{'a', '\0', 'b', '\n', '\r', '\xff', '\x80'};
    ASSERT_TRUE(_file_system.WriteText("user://bytes.bin", written));

    std::vector<unsigned char> contents;
    ASSERT_TRUE(_file_system.ReadBytes("user://bytes.bin", contents));

    EXPECT_THAT(contents, ElementsAre('a', 0, 'b', '\n', '\r', 0xff, 0x80));
  }

  TEST_F(FileSystemTest, WritesAnEmptyFile)
  {
    ASSERT_TRUE(_file_system.WriteText("user://empty.txt", ""));

    EXPECT_TRUE(_file_system.Exists("user://empty.txt"));
  }

  TEST_F(FileSystemTest, WritesToTheOutputFolder)
  {
    ASSERT_TRUE(_file_system.WriteText("output://frame.png", "a frame"));

    EXPECT_TRUE(_file_system.HasNativeFile("/chosen/frame.png"));
  }

  TEST_F(FileSystemTest, ReplacesAFileThatExists)
  {
    ASSERT_TRUE(_file_system.WriteText("user://save.dat", "the first save, which is long"));
    ASSERT_TRUE(_file_system.WriteText("user://save.dat", "the second"));

    std::string contents;
    ASSERT_TRUE(_file_system.ReadText("user://save.dat", contents));
    EXPECT_EQ(contents, "the second");
  }

  TEST_F(FileSystemTest, CreatesTheFoldersThatLeadToAFileOneByOne)
  {
    ASSERT_TRUE(_file_system.WriteText("user://saves/slot-1/today/save.dat", "a save"));

    EXPECT_THAT(
      _file_system.MadeDirectories(),
      ElementsAre("/user/saves", "/user/saves/slot-1", "/user/saves/slot-1/today"));
    EXPECT_TRUE(_file_system.HasNativeFile("/user/saves/slot-1/today/save.dat"));
  }

  TEST_F(FileSystemTest, CreatesOnlyTheFoldersThatAreMissing)
  {
    _file_system.AddNativeFile("/user/saves/other.dat");

    ASSERT_TRUE(_file_system.WriteText("user://saves/slot-1/save.dat", "a save"));

    EXPECT_THAT(_file_system.MadeDirectories(), ElementsAre("/user/saves/slot-1"));
  }

  TEST_F(FileSystemTest, CreatesNoFolderForAFileAtTheTop)
  {
    ASSERT_TRUE(_file_system.WriteText("user://save.dat", "a save"));

    EXPECT_THAT(_file_system.MadeDirectories(), IsEmpty());
  }

  TEST_F(FileSystemTest, IgnoresDoubledSlashesAndDotsWhenWriting)
  {
    ASSERT_TRUE(_file_system.WriteText("user://./saves//slot-1/./save.dat", "a save"));

    EXPECT_TRUE(_file_system.HasNativeFile("/user/saves/slot-1/save.dat"));
  }

  TEST_F(FileSystemTest, LocatesAFileToWriteBelowTheFolderOfItsScheme)
  {
    std::string native_path;

    EXPECT_TRUE(_file_system.LocateNativeForWriting("user://saves/save.dat", native_path));
    EXPECT_EQ(native_path, "/user/saves/save.dat");
    EXPECT_TRUE(_file_system.LocateNativeForWriting("output://shots/frame.png", native_path));
    EXPECT_EQ(native_path, "/chosen/shots/frame.png");
  }

  TEST_F(FileSystemTest, RefusesToWriteToAssets)
  {
    ExpectNotWritable("assets://models/cube.obj", read_only);

    std::string contents;
    ASSERT_TRUE(_file_system.ReadText("assets://models/cube.obj", contents));
    EXPECT_EQ(contents, "a cube");
  }

  TEST_F(FileSystemTest, RefusesToWriteANewFileToAssets)
  {
    ExpectNotWritable("assets://new/file.txt", read_only);

    EXPECT_FALSE(_file_system.HasNativeFile("/assets/new/file.txt"));
    EXPECT_FALSE(_file_system.HasNativeDirectory("/assets/new"));
  }

  TEST_F(FileSystemTest, RefusesToWriteBytesToAssets)
  {
    EXPECT_FALSE(_file_system.WriteBytes("assets://file.bin", {1, 2, 3}));
    ExpectRefused("assets://file.bin", read_only);
  }

  TEST_F(FileSystemTest, RefusesToWriteAFileThatDiffersFromAnotherInLetterCaseOnly)
  {
    _file_system.AddNativeFile("/user/Save.dat", "the one on disk");

    ExpectNotWritable(
      "user://save.dat",
      "'save.dat' is named 'Save.dat' on disk, letter case has to match on every platform");
    EXPECT_FALSE(_file_system.HasNativeFile("/user/save.dat"));
  }

  TEST_F(FileSystemTest, RefusesToWriteIntoAFolderThatDiffersFromAnotherInLetterCaseOnly)
  {
    _file_system.AddNativeFile("/user/Saves/save.dat");

    ExpectNotWritable(
      "user://saves/slot-1/save.dat",
      "'saves' is named 'Saves' on disk, letter case has to match on every platform");
    EXPECT_FALSE(_file_system.HasNativeDirectory("/user/saves"));
  }

  TEST_F(FileSystemTest, ReportsAFolderThatCannotBeCreated)
  {
    _file_system.fail_to_make_directories = true;

    EXPECT_FALSE(_file_system.WriteText("user://saves/save.dat", "a save"));

    EXPECT_TRUE(
      _logger->Contains(
        LogLevel::Error,
        "Could not write 'user://saves/save.dat': a folder on the way to it cannot be created"))
      << _logger->Messages(LogLevel::Error);
    EXPECT_FALSE(_file_system.HasNativeFile("/user/saves/save.dat"));
  }

  TEST_F(FileSystemTest, ReportsAFolderThatCannotBeListedWhenWriting)
  {
    _file_system.fail_to_list = true;

    EXPECT_FALSE(_file_system.WriteText("user://save.dat", "a save"));

    EXPECT_TRUE(
      _logger->Contains(
        LogLevel::Error,
        "Could not write 'user://save.dat': a folder on the way to it cannot be listed"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(FileSystemTest, LeavesTheNativePathAloneWhenWritingIsRefused)
  {
    std::string native_path = "untouched";

    EXPECT_FALSE(_file_system.LocateNativeForWriting("assets://file.txt", native_path));
    EXPECT_FALSE(_file_system.LocateNativeForWriting("user://../file.txt", native_path));
    EXPECT_EQ(native_path, "untouched");
  }

  // output:// without a folder

  class FileSystemWithoutOutputTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _file_system{SettingsConfig{}, _logger};

    void SetUp() override
    {
      _file_system.Initialize();
    }

    void ExpectRefused(const std::string &path) const
    {
      EXPECT_EQ(_logger->Count(LogLevel::Error), 1u) << _logger->Messages(LogLevel::Error);
      EXPECT_TRUE(
        _logger->Contains(
          LogLevel::Error,
          "Invalid path '" + path + "': output:// has no folder. None was chosen with --output-dir, "
          "or the chosen one cannot be used"))
        << _logger->Messages(LogLevel::Error);
    }
  };

  TEST_F(FileSystemWithoutOutputTest, RefusesToWriteToOutput)
  {
    EXPECT_FALSE(_file_system.WriteText("output://frame.png", "a frame"));

    ExpectRefused("output://frame.png");
  }

  TEST_F(FileSystemWithoutOutputTest, RefusesToReadFromOutput)
  {
    std::string contents = "untouched";

    EXPECT_FALSE(_file_system.ReadText("output://frame.png", contents));

    EXPECT_EQ(contents, "untouched");
    ExpectRefused("output://frame.png");
  }

  TEST_F(FileSystemWithoutOutputTest, DoesNotFindAnythingInOutput)
  {
    EXPECT_FALSE(_file_system.Exists("output://frame.png"));

    ExpectRefused("output://frame.png");
  }

  TEST_F(FileSystemWithoutOutputTest, ReportsTheMissingFolderBeforeAnyOtherRule)
  {
    EXPECT_FALSE(_file_system.Exists("output://../frame.png"));

    ExpectRefused("output://../frame.png");
  }

  TEST_F(FileSystemWithoutOutputTest, StillReadsAndWritesTheOtherSchemes)
  {
    _file_system.AddNativeFile("/assets/models/cube.obj", "a cube");

    EXPECT_TRUE(_file_system.Exists("assets://models/cube.obj"));
    EXPECT_TRUE(_file_system.WriteText("user://save.dat", "a save"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  // the log file

  /// Writes down where it was told to put its log file.
  class RecordingLogFileTarget final : public neon::LogFileTarget
  {
  public:
    std::vector<std::string> opened;
    int gone_without = 0;
    bool can_open = true;

    bool OpenLogFile(const std::string &native_path) override
    {
      opened.push_back(native_path);
      return can_open;
    }

    void GoWithoutLogFile() override
    {
      gone_without++;
    }
  };

  TEST_F(FileSystemTest, PlacesTheLogFileBelowTheFolderOfUser)
  {
    RecordingLogFileTarget target;

    EXPECT_TRUE(_file_system.PlaceLogFile("user://logs/neon-engine.log", target));

    EXPECT_THAT(target.opened, ElementsAre("/user/logs/neon-engine.log"));
    EXPECT_EQ(target.gone_without, 0);
    EXPECT_THAT(_file_system.MadeDirectories(), ElementsAre("/user/logs"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(FileSystemTest, PlacesTheLogFileWithTheSeparatorOfThePlatform)
  {
    MemoryFileSystem file_system(SettingsConfig{}, _logger, '\\');
    file_system.Initialize();
    RecordingLogFileTarget target;

    EXPECT_TRUE(file_system.PlaceLogFile("user://logs/neon-engine.log", target));

    EXPECT_THAT(target.opened, ElementsAre("\\user\\logs\\neon-engine.log"));
  }

  TEST_F(FileSystemTest, SaysWhenTheTargetCannotOpenTheLogFile)
  {
    RecordingLogFileTarget target;
    target.can_open = false;

    EXPECT_FALSE(_file_system.PlaceLogFile("user://logs/neon-engine.log", target));

    EXPECT_THAT(target.opened, ElementsAre("/user/logs/neon-engine.log"));
  }

  TEST_F(FileSystemTest, TellsTheTargetToGoWithoutALogFileInAFolderThatCannotBeCreated)
  {
    _file_system.fail_to_make_directories = true;
    RecordingLogFileTarget target;

    EXPECT_FALSE(_file_system.PlaceLogFile("user://logs/neon-engine.log", target));

    EXPECT_THAT(target.opened, IsEmpty());
    EXPECT_EQ(target.gone_without, 1);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "The log file cannot be placed at 'user://logs/neon-engine.log'"))
      << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "a folder on the way to it cannot be created"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(FileSystemTest, TellsTheTargetToGoWithoutALogFileInAReadOnlyScheme)
  {
    RecordingLogFileTarget target;

    EXPECT_FALSE(_file_system.PlaceLogFile("assets://neon-engine.log", target));

    EXPECT_THAT(target.opened, IsEmpty());
    EXPECT_EQ(target.gone_without, 1);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, read_only)) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(FileSystemTest, TellsTheTargetToGoWithoutALogFileAtAPathThatBreaksARule)
  {
    RecordingLogFileTarget target;

    EXPECT_FALSE(_file_system.PlaceLogFile("logs/neon-engine.log", target));

    EXPECT_THAT(target.opened, IsEmpty());
    EXPECT_EQ(target.gone_without, 1);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, unknown_scheme)) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(FileSystemTest, ListsTheFilesOfAFolderAndOfTheFoldersBelowItInTheOrderOfTheirNames)
  {
    _file_system.AddNativeFile("/assets/scripts/zed.lua", "");
    _file_system.AddNativeFile("/assets/scripts/door.lua", "");
    _file_system.AddNativeFile("/assets/scripts/lib/tween.lua", "");
    _file_system.AddNativeFile("/assets/scripts/lib/a/deep.lua", "");
    _file_system.AddNativeFile("/assets/scripts/enemies/turret.lua", "");

    std::vector<std::string> paths;
    EXPECT_TRUE(_file_system.ListFiles("assets://scripts", paths));
    EXPECT_THAT(
      paths,
      ElementsAre(
        "assets://scripts/door.lua",
        "assets://scripts/zed.lua",
        "assets://scripts/enemies/turret.lua",
        "assets://scripts/lib/tween.lua",
        "assets://scripts/lib/a/deep.lua"));

    // a slash at the end names the same folder
    paths.clear();
    EXPECT_TRUE(_file_system.ListFiles("assets://scripts/", paths));
    EXPECT_THAT(paths, ::testing::SizeIs(5));
  }

  TEST_F(FileSystemTest, CannotListAFolderThatIsNotThereOrAFile)
  {
    std::vector<std::string> paths;
    EXPECT_FALSE(_file_system.ListFiles("assets://scripts", paths));
    EXPECT_FALSE(_file_system.ListFiles("assets://models/cube.obj", paths));
    EXPECT_THAT(paths, IsEmpty());
  }

  TEST_F(FileSystemTest, ListsTheWholeOfASchemeWhenGivenTheSchemeAlone)
  {
    _file_system.AddNativeFile("/assets/scripts/door.lua", "");
    _file_system.AddNativeFile("/assets/README.md", "");

    std::vector<std::string> paths;
    EXPECT_TRUE(_file_system.ListFiles("assets://", paths));
    EXPECT_THAT(paths, ElementsAre("assets://README.md", "assets://models/cube.obj", "assets://scripts/door.lua"));
  }
}
