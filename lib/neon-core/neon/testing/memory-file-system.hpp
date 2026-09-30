#ifndef MEMORY_FILE_SYSTEM_HPP
#define MEMORY_FILE_SYSTEM_HPP

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <neon/filesystem/file-system.hpp>

namespace neon::testing
{
  /// A file system backend that keeps its files in memory. Nothing touches
  /// the disk.
  ///
  /// It is a backend like any other: the path rules and the letter case
  /// check are those of FileSystem, which is what makes it fit for testing
  /// them. It also serves as the file system of anything that reads files.
  ///
  /// Its native paths are made up. `assets://` is the folder `assets` at the
  /// top, `user://` is `user`, and `output://` is what the settings name as
  /// output directory. With the separator `\` that gives `\assets\`.
  class MemoryFileSystem final : public FileSystem
  {
    std::map<std::string, std::vector<unsigned char>> _files;
    std::set<std::string> _directories;
    std::vector<std::string> _made_directories;

    [[nodiscard]] std::string WithoutLastSeparator(const std::string &native_path) const
    {
      if (!native_path.empty() && native_path.back() == _native_separator)
      {
        return native_path.substr(0, native_path.size() - 1);
      }
      return native_path;
    }

    /// The folder a native path lies in, without a separator at its end.
    [[nodiscard]] std::string ParentOf(const std::string &native_path) const
    {
      const std::size_t separator = native_path.rfind(_native_separator);
      return separator == std::string::npos ? "" : native_path.substr(0, separator);
    }

    [[nodiscard]] std::string NameOf(const std::string &native_path) const
    {
      const std::size_t separator = native_path.rfind(_native_separator);
      return separator == std::string::npos ? native_path : native_path.substr(separator + 1);
    }

    void AddDirectories(const std::string &native_directory)
    {
      std::string directory = WithoutLastSeparator(native_directory);
      while (!directory.empty())
      {
        _directories.insert(directory);
        directory = ParentOf(directory);
      }
    }

  protected:
    bool ListDirectory(const std::string &native_directory, std::vector<std::string> &names) override
    {
      if (fail_to_list) { return false; }

      const std::string directory = WithoutLastSeparator(native_directory);
      if (!_directories.contains(directory)) { return false; }

      for (const auto &candidate : _directories)
      {
        if (ParentOf(candidate) == directory) { names.push_back(NameOf(candidate)); }
      }
      for (const auto &[file, contents] : _files)
      {
        if (ParentOf(file) == directory) { names.push_back(NameOf(file)); }
      }
      return true;
    }

    bool MakeDirectory(const std::string &native_directory) override
    {
      if (fail_to_make_directories) { return false; }

      // like a real one, it creates a single folder whose parent exists
      if (!_directories.contains(ParentOf(native_directory))) { return false; }

      _directories.insert(native_directory);
      _made_directories.push_back(native_directory);
      return true;
    }

  public:
    /// Makes every folder refuse to be listed.
    bool fail_to_list = false;

    /// Makes every folder refuse to be created.
    bool fail_to_make_directories = false;

    /// Call Initialize() before anything else, as with every backend.
    MemoryFileSystem(
      const SettingsConfig &settings_config,
      const std::shared_ptr<Logger> &logger,
      const char native_separator = '/')
      : FileSystem(settings_config, logger)
    {
      _native_separator = native_separator;
    }

    void Initialize() override
    {
      const std::string separator(1, _native_separator);

      _assets_directory = separator + "assets" + separator;
      _user_directory = separator + "user" + separator;
      AddDirectories(_assets_directory);
      AddDirectories(_user_directory);

      _output_directory.clear();
      if (!_settings_config.output_directory.empty())
      {
        _output_directory = _settings_config.output_directory + separator;
        AddDirectories(_output_directory);
      }
    }

    void CleanUp() override
    {
      _files.clear();
      _directories.clear();
      _made_directories.clear();
    }

    bool Exists(const std::string &path) override
    {
      std::string native_path;
      return Locate(path, native_path) && _files.contains(native_path);
    }

    bool ReadBytes(const std::string &path, std::vector<unsigned char> &contents) override
    {
      std::string native_path;
      if (!Locate(path, native_path)) { return false; }

      const auto file = _files.find(native_path);
      if (file == _files.end()) { return false; }

      contents = file->second;
      return true;
    }

    bool WriteBytes(const std::string &path, const std::vector<unsigned char> &contents) override
    {
      std::string native_path;
      if (!LocateForWriting(path, native_path)) { return false; }

      // a folder of that name is in the way
      if (_directories.contains(native_path)) { return false; }

      _files[native_path] = contents;
      return true;
    }

    /// Puts a file where a native path says, as if it had always been there.
    /// The folders that lead to it are created.
    void AddNativeFile(const std::string &native_path, const std::string &contents = "")
    {
      AddDirectories(ParentOf(native_path));
      _files[native_path] = {contents.begin(), contents.end()};
    }

    [[nodiscard]] bool HasNativeFile(const std::string &native_path) const
    {
      return _files.contains(native_path);
    }

    [[nodiscard]] bool HasNativeDirectory(const std::string &native_path) const
    {
      return _directories.contains(native_path);
    }

    /// The folders that were created through MakeDirectory, in order.
    [[nodiscard]] const std::vector<std::string> &MadeDirectories() const
    {
      return _made_directories;
    }

    /// FileSystem::Locate, which only backends can call.
    bool LocateNative(const std::string &path, std::string &native_path)
    {
      return Locate(path, native_path);
    }

    /// FileSystem::LocateForWriting, which only backends can call.
    bool LocateNativeForWriting(const std::string &path, std::string &native_path)
    {
      return LocateForWriting(path, native_path);
    }
  };
} // neon::testing

#endif //MEMORY_FILE_SYSTEM_HPP
