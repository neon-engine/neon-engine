#include "native-path.hpp"

namespace neon
{
  std::filesystem::path NativePath::FromUtf8(const std::string &utf8)
  {
    // A path built from char8_t is decoded as UTF-8 on every platform. On
    // Windows that gives the wide characters the operating system takes;
    // elsewhere the bytes are kept as they are.
    return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
  }

  std::string NativePath::ToUtf8(const std::filesystem::path &path)
  {
    const std::u8string utf8 = path.u8string();
    return {utf8.begin(), utf8.end()};
  }
} // neon
