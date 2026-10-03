#include "sdl2-library-loader.hpp"

#include "sdl2-native-library.hpp"

namespace neon
{
  SDL2_LibraryLoader::SDL2_LibraryLoader(FileSystem *file_system)
  {
    _file_system = file_system;
  }

  std::string SDL2_LibraryLoader::GetPlatform() const
  {
    // the build says it, since the build is what names the libraries
    return NEON_PLATFORM;
  }

  std::unique_ptr<NativeLibrary> SDL2_LibraryLoader::Open(const std::string &path, std::string &error)
  {
    auto library = std::make_unique<SDL2_NativeLibrary>();
    if (!_file_system->OpenLibrary(path, *library, error)) { return nullptr; }

    return library;
  }
} // neon
