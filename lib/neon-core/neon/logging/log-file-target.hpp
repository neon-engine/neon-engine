#ifndef LOG_FILE_TARGET_HPP
#define LOG_FILE_TARGET_HPP

#include <string>

namespace neon
{
  /// Something that writes a log file, and is told where it goes after it has
  /// started logging.
  ///
  /// The file system decides where the log file is, since only it knows the
  /// folder behind `user://`. It hands the native path straight to this
  /// interface, so the path never passes through the rest of the engine. See
  /// FileSystem::PlaceLogFile.
  class LogFileTarget
  {
  protected:
    ~LogFileTarget() = default;

  public:
    /// Opens the log file at a native path, adding to it if it exists, and
    /// writes into it what was logged before. Returns false and logs why if
    /// it cannot be opened. Logging then goes on without a file.
    virtual bool OpenLogFile(const std::string &native_path) = 0;

    /// Gives up on a log file, because there is no place for one. What was
    /// held back for it is let go, and logging goes on without a file.
    virtual void GoWithoutLogFile() = 0;
  };
} // neon

#endif //LOG_FILE_TARGET_HPP
