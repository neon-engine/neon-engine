#include "native-path.hpp"

#include <filesystem>
#include <string>

#include <gtest/gtest.h>

// A native path in UTF-8 reaches the operating system with every letter it
// had, whatever the platform.

namespace
{
  using neon::NativePath;

  // the UTF-8 bytes of Jose with an accent on the e, spelled out so that the
  // test does not depend on how the compiler reads this file
  const std::string accented_user = "C:/Users/Jos\xC3\xA9/AppData/Roaming/neon-engine.log";

  TEST(NativePathTest, RoundTripsLettersOutsideAscii)
  {
    const std::filesystem::path native = NativePath::FromUtf8(accented_user);

    EXPECT_EQ(NativePath::ToUtf8(native), accented_user);
  }

  TEST(NativePathTest, RoundTripsAnAsciiPath)
  {
    const std::string plain = "/home/jose/.local/share/neon-engine/neon-sandbox/logs/neon-engine.log";

    EXPECT_EQ(NativePath::ToUtf8(NativePath::FromUtf8(plain)), plain);
  }

  TEST(NativePathTest, RoundTripsAnEmptyPath)
  {
    EXPECT_TRUE(NativePath::FromUtf8("").empty());
    EXPECT_EQ(NativePath::ToUtf8(std::filesystem::path()), "");
  }

#if defined(_WIN32)
  TEST(NativePathTest, DecodesUtf8ToWideCharactersOnWindows)
  {
    const std::filesystem::path native = NativePath::FromUtf8(accented_user);

    // one code unit for the accented e, not the two bytes of its UTF-8
    EXPECT_EQ(native.native(), L"C:/Users/Jos\u00E9/AppData/Roaming/neon-engine.log");
  }
#else
  TEST(NativePathTest, IsTheIdentityOutsideWindows)
  {
    const std::filesystem::path native = NativePath::FromUtf8(accented_user);

    // the bytes are the native path, untouched
    EXPECT_EQ(native.native(), accented_user);
  }
#endif
} // namespace
