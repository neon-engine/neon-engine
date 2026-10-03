#ifndef LIBRARY_LOADER_HPP
#define LIBRARY_LOADER_HPP

#include <memory>
#include <string>

#include "native-library.hpp"

namespace neon
{
  /// Opens libraries of the platform while the application runs. How that
  /// is done is a backend behind this interface.
  class LibraryLoader
  {
  protected:
    ~LibraryLoader() = default;

  public:
    /// What the platform the application runs on is called where a library
    /// is named for it, such as `macos-arm64`, `linux-x86_64`, or
    /// `windows-x86_64`: the system, a dash, and the processor.
    [[nodiscard]] virtual std::string GetPlatform() const = 0;

    /// Opens the library at a virtual path, such as
    /// `extensions://quake/quake-macos-arm64.dylib`. Returns nullptr when
    /// it cannot be opened, and says why in `error`.
    virtual std::unique_ptr<NativeLibrary> Open(const std::string &path, std::string &error) = 0;
  };
} // neon

#endif //LIBRARY_LOADER_HPP
