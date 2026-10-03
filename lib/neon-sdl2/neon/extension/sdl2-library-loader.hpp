#ifndef SDL2_LIBRARY_LOADER_HPP
#define SDL2_LIBRARY_LOADER_HPP

#include <memory>
#include <string>

#include <neon/extension/library-loader.hpp>
#include <neon/filesystem/file-system.hpp>

namespace neon
{
  /// Opens libraries of the platform through SDL2. The file system finds
  /// the file a virtual path names.
  // ReSharper disable once CppInconsistentNaming
  class SDL2_LibraryLoader final : public LibraryLoader
  {
    FileSystem *_file_system;

  public:
    explicit SDL2_LibraryLoader(FileSystem *file_system);

    [[nodiscard]] std::string GetPlatform() const override;

    std::unique_ptr<NativeLibrary> Open(const std::string &path, std::string &error) override;
  };
} // neon

#endif //SDL2_LIBRARY_LOADER_HPP
