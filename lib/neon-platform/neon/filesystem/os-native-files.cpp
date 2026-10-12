#include "os-native-files.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <system_error>

#include <neon/filesystem/native-path.hpp>

namespace neon
{
  // Helpers of OS_NativeFiles: the variable of the environment that names
  // the home folder on this platform.
  namespace
  {
#if defined(_WIN32)
    constexpr const char *home_variable = "USERPROFILE";
#else
    constexpr const char *home_variable = "HOME";
#endif
  }

  bool OS_NativeFiles::HomeDirectory(std::string &native_directory)
  {
    // the environment is read as bytes; on Windows they are in the code page
    // of the process, which is UTF-8 for the applications of the engine
    const char *home = std::getenv(home_variable);
    if (home == nullptr || home[0] == '\0')
    {
      _logger->Error("The home folder is not known: {} is not set", home_variable);
      return false;
    }

    const std::filesystem::path path = NativePath::FromUtf8(home);
    native_directory = NativePath::ToUtf8(path.lexically_normal());
    while (native_directory.size() > 1 && (native_directory.back() == '/' || native_directory.back() == '\\'))
    {
      native_directory.pop_back();
    }
    return true;
  }

  std::string OS_NativeFiles::Join(const std::string &native_directory, const std::string &name)
  {
    std::filesystem::path path = NativePath::FromUtf8(native_directory) / NativePath::FromUtf8(name);
    return NativePath::ToUtf8(path.make_preferred());
  }

  bool OS_NativeFiles::Exists(const std::string &native_path)
  {
    std::error_code error;
    return std::filesystem::exists(NativePath::FromUtf8(native_path), error) && !error;
  }

  bool OS_NativeFiles::IsDirectory(const std::string &native_path)
  {
    std::error_code error;
    return std::filesystem::is_directory(NativePath::FromUtf8(native_path), error) && !error;
  }

  bool OS_NativeFiles::MakeDirectories(const std::string &native_directory)
  {
    const std::filesystem::path directory = NativePath::FromUtf8(native_directory);

    std::error_code error;
    std::filesystem::create_directories(directory, error);

    // a file of that name is in the way
    if (!error && !std::filesystem::is_directory(directory, error))
    {
      error = std::make_error_code(std::errc::not_a_directory);
    }

    if (error)
    {
      const std::string reason = error.message();
      _logger->Error("The folder {} cannot be created: {}", native_directory, reason);
      return false;
    }
    return true;
  }

  bool OS_NativeFiles::WriteText(const std::string &native_path, const std::string &contents)
  {
    std::ofstream file(NativePath::FromUtf8(native_path), std::ios::binary | std::ios::trunc);
    if (!file)
    {
      _logger->Error("The file {} cannot be opened for writing", native_path);
      return false;
    }

    file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    file.close();
    if (!file)
    {
      _logger->Error("The file {} could not be written in full", native_path);
      return false;
    }
    return true;
  }
} // neon
