#ifndef OS_NATIVE_FILES_HPP
#define OS_NATIVE_FILES_HPP

#include <memory>
#include <string>

#include <neon/filesystem/native-file-context.hpp>
#include <neon/logging/logger.hpp>

namespace neon
{
  /// The files of this machine through the standard library, which is what
  /// every platform has, and the home folder from the environment: `HOME`
  /// on macOS and Linux, `USERPROFILE` on Windows. SDL2 can neither create a
  /// folder nor say where the home folder is, so this is a backend of its
  /// own, as the entropy is.
  // ReSharper disable once CppInconsistentNaming
  class OS_NativeFiles final : public NativeFileContext
  {
    std::shared_ptr<Logger> _logger;

  public:
    explicit OS_NativeFiles(const std::shared_ptr<Logger> &logger)
      : _logger(logger) {}

    bool HomeDirectory(std::string &native_directory) override;

    [[nodiscard]] std::string Join(const std::string &native_directory, const std::string &name) override;

    bool Exists(const std::string &native_path) override;

    bool IsDirectory(const std::string &native_path) override;

    bool MakeDirectories(const std::string &native_directory) override;

    bool WriteText(const std::string &native_path, const std::string &contents) override;
  };
} // neon

#endif //OS_NATIVE_FILES_HPP
