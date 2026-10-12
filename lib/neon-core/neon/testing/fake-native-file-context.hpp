#ifndef FAKE_NATIVE_FILE_CONTEXT_HPP
#define FAKE_NATIVE_FILE_CONTEXT_HPP

#include <map>
#include <set>
#include <string>

#include <neon/filesystem/native-file-context.hpp>

namespace neon::testing
{
  /// The files of a machine, kept in memory, so that a test reads what a
  /// tool wrote without touching the disk. Folders are joined with a forward
  /// slash, as on macOS and Linux; a test that wants the separator of
  /// Windows sets `separator`.
  class FakeNativeFileContext final : public NativeFileContext
  {
  public:
    /// What HomeDirectory answers. Empty makes it fail.
    std::string home = "/home/someone";

    char separator = '/';

    /// Every file that was written, by its path.
    std::map<std::string, std::string> files;

    /// Every folder that is there, by its path.
    std::set<std::string> directories;

    /// Makes every write fail, as a disk that is full or read-only does.
    bool refuses_writes = false;

    bool HomeDirectory(std::string &native_directory) override
    {
      if (home.empty()) { return false; }
      native_directory = home;
      return true;
    }

    [[nodiscard]] std::string Join(const std::string &native_directory, const std::string &name) override
    {
      if (native_directory.empty()) { return name; }
      if (native_directory.back() == separator) { return native_directory + name; }
      return native_directory + separator + name;
    }

    bool Exists(const std::string &native_path) override
    {
      return files.contains(native_path) || directories.contains(native_path);
    }

    bool IsDirectory(const std::string &native_path) override
    {
      return directories.contains(native_path);
    }

    bool MakeDirectories(const std::string &native_directory) override
    {
      if (refuses_writes || files.contains(native_directory)) { return false; }

      // every folder on the way is there afterwards, as on a disk
      for (std::size_t end = native_directory.find(separator, 1);
           end != std::string::npos;
           end = native_directory.find(separator, end + 1))
      {
        directories.insert(native_directory.substr(0, end));
      }
      directories.insert(native_directory);
      return true;
    }

    bool WriteText(const std::string &native_path, const std::string &contents) override
    {
      if (refuses_writes) { return false; }

      const std::size_t last = native_path.rfind(separator);
      if (last != std::string::npos && last > 0 && !directories.contains(native_path.substr(0, last)))
      {
        return false;
      }

      files[native_path] = contents;
      return true;
    }
  };
} // neon::testing

#endif //FAKE_NATIVE_FILE_CONTEXT_HPP
