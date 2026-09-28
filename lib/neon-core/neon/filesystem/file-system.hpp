#ifndef FILE_SYSTEM_HPP
#define FILE_SYSTEM_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "file-system-context.hpp"
#include "neon/application/settings-config.hpp"
#include "neon/logging/logger.hpp"

namespace neon
{
  /// Base class for file system backends.
  ///
  /// It owns everything that is the same for every backend: checking that a
  /// virtual path follows the rules, and turning it into the native path the
  /// backend opens. A backend supplies the native folders in Initialize() and
  /// implements the actual file access.
  class FileSystem : public FileSystemContext
  {
  protected:
    SettingsConfig _settings_config;
    std::shared_ptr<Logger> _logger;

    /// Native folder behind `assets://`, ending with a native separator.
    /// Backends set this in Initialize().
    std::string _assets_directory;

    /// Native folder behind `user://`, ending with a native separator.
    /// Backends set this in Initialize() and make sure it exists.
    std::string _user_directory;

    /// Separator the platform uses between folders in a native path.
    /// Backends set this in Initialize().
    char _native_separator = '/';

    /// Lists the names of the files and folders directly inside a native
    /// folder, spelled exactly as they are stored. Returns false if the folder
    /// cannot be listed.
    virtual bool ListDirectory(const std::string &native_directory, std::vector<std::string> &names) = 0;

    /// Creates one native folder whose parent exists. Returns false if it
    /// cannot be created.
    virtual bool MakeDirectory(const std::string &native_directory) = 0;

    /// Finds the file a virtual path names and produces the native path to
    /// open. Returns false if the path breaks a rule or names nothing.
    ///
    /// Every folder and file name is compared with what is stored on disk and
    /// has to match exactly, letter case included. This holds on platforms
    /// whose own file system ignores case, so a path cannot work on one
    /// platform and fail on another.
    ///
    /// This is the only place a native path is produced, and it is only
    /// available to backends.
    bool Locate(const std::string &path, std::string &native_path);

    /// Produces the native path to write a virtual path to, creating the
    /// folders that lead to it. Returns false if the path breaks a rule or
    /// its scheme is read-only.
    ///
    /// Letter case is held to the same standard as when reading. A name that
    /// matches an existing one in everything but case is refused, since the
    /// two would be the same file on some platforms and different files on
    /// others.
    bool LocateForWriting(const std::string &path, std::string &native_path);

    ~FileSystem() = default;

  public:
    static constexpr std::string_view assets_scheme = "assets://";
    static constexpr std::string_view user_scheme = "user://";

    explicit FileSystem(const SettingsConfig &settings_config, const std::shared_ptr<Logger> &logger)
    {
      _settings_config = settings_config;
      _logger = logger;
    }

    virtual void Initialize() = 0;

    virtual void CleanUp() = 0;

    bool ReadText(const std::string &path, std::string &contents) override;

    bool WriteText(const std::string &path, const std::string &contents) override;

  private:
    /// Splits a virtual path into the native folder of its scheme and the
    /// names that follow, after checking the rules that need no disk access.
    bool Parse(
      const std::string &path,
      std::string &scheme_directory,
      bool &writable,
      std::vector<std::string_view> &segments) const;
  };
} // neon

#endif //FILE_SYSTEM_HPP
