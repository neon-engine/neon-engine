#include "os-native-files.hpp"

#include <memory>
#include <string>

#include <gtest/gtest.h>
#include <neon/testing/recording-logger.hpp>
#include <neon/testing/temporary-directory.hpp>

namespace
{
  class OSNativeFilesTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<neon::testing::RecordingLogger> logger = std::make_shared<neon::testing::RecordingLogger>();
    neon::OS_NativeFiles files{logger};
    neon::testing::TemporaryDirectory directory;
  };

  TEST_F(OSNativeFilesTest, KnowsTheHomeFolder)
  {
    std::string home;
    ASSERT_TRUE(files.HomeDirectory(home));
    EXPECT_FALSE(home.empty());
    EXPECT_TRUE(files.IsDirectory(home));
    EXPECT_NE(home.back(), '/');
  }

  TEST_F(OSNativeFilesTest, JoinsAFolderAndAName)
  {
    const std::string joined = files.Join(directory.Native(), "projects");
    EXPECT_EQ(joined, directory.Native("projects"));
  }

  TEST_F(OSNativeFilesTest, MakesEveryFolderOnTheWay)
  {
    const std::string nested = files.Join(files.Join(directory.Native(), "a"), "b");

    ASSERT_TRUE(files.MakeDirectories(nested));

    EXPECT_TRUE(directory.HasDirectory("a/b"));
    EXPECT_TRUE(files.IsDirectory(nested));
    EXPECT_TRUE(files.Exists(nested));

    // a folder that is there already is fine
    EXPECT_TRUE(files.MakeDirectories(nested));
  }

  TEST_F(OSNativeFilesTest, RefusesAFolderWhereAFileIs)
  {
    directory.Write("taken", "");

    EXPECT_FALSE(files.MakeDirectories(directory.Native("taken")));
    EXPECT_TRUE(logger->Contains(neon::testing::LogLevel::Error, "taken"));
  }

  TEST_F(OSNativeFilesTest, WritesTextAndReplacesIt)
  {
    const std::string path = files.Join(directory.Native(), "notes.txt");

    ASSERT_TRUE(files.WriteText(path, "first"));
    ASSERT_TRUE(files.WriteText(path, "second"));

    EXPECT_EQ(directory.Read("notes.txt"), "second");
    EXPECT_TRUE(files.Exists(path));
    EXPECT_FALSE(files.IsDirectory(path));
  }

  TEST_F(OSNativeFilesTest, RefusesToWriteIntoAFolderThatIsNotThere)
  {
    EXPECT_FALSE(files.WriteText(directory.Native("missing/notes.txt"), "text"));
    EXPECT_TRUE(logger->Contains(neon::testing::LogLevel::Error, "notes.txt"));
  }

  TEST_F(OSNativeFilesTest, KnowsWhatIsNotThere)
  {
    EXPECT_FALSE(files.Exists(directory.Native("nothing")));
    EXPECT_FALSE(files.IsDirectory(directory.Native("nothing")));
  }
}
