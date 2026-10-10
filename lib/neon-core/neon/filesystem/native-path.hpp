#ifndef NATIVE_PATH_HPP
#define NATIVE_PATH_HPP

#include <filesystem>
#include <string>

namespace neon
{
  /// The one place a native path, which the engine keeps as UTF-8 in a
  /// std::string, is turned into what the operating system opens, and back.
  ///
  /// On Windows the operating system takes wide characters, and a
  /// std::filesystem::path built from a std::string would read the bytes in
  /// the ANSI code page, not as UTF-8, so a folder such as C:\Users\Jose with
  /// an accent in the name would not be found. Here the bytes are decoded as
  /// UTF-8. On macOS and Linux the bytes are the native path already, and
  /// both directions are the identity.
  class NativePath final
  {
  public:
    NativePath() = delete;

    /// The path the operating system opens, for a native path in UTF-8.
    [[nodiscard]] static std::filesystem::path FromUtf8(const std::string &utf8);

    /// The native path in UTF-8, the inverse of FromUtf8.
    [[nodiscard]] static std::string ToUtf8(const std::filesystem::path &path);
  };
} // neon

#endif //NATIVE_PATH_HPP
