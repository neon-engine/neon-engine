#include "sdl2-file-system.hpp"

#include <filesystem>
#include <stdexcept>
#include <system_error>
#include <SDL.h>

namespace neon
{
  void SDL2_FileSystem::Initialize()
  {
    _logger->Info("Initializing SDL2 file system");

    // the directory holding the executable, ending with a path separator
    char *base_path = SDL_GetBasePath();
    if (base_path == nullptr)
    {
      auto error = std::string(SDL_GetError());
      _logger->Critical("Failed to find the application directory: {}", error);
      throw std::runtime_error("Failed to find the application directory");
    }

    // SDL ends the base path with the platform's separator, which tells us
    // what to put between folders without asking which platform this is
    const std::string application_directory(base_path);
    SDL_free(base_path);

    _native_separator = application_directory.back();
    _assets_directory = application_directory + "assets" + _native_separator;

    _logger->Info("assets:// is {}", _assets_directory);
  }

  void SDL2_FileSystem::CleanUp()
  {
    _logger->Info("Cleaning up SDL2 file system");
    _assets_directory.clear();
  }

  bool SDL2_FileSystem::Exists(const std::string &path)
  {
    std::string native_path;
    if (!Locate(path, native_path)) { return false; }

    SDL_RWops *file = SDL_RWFromFile(native_path.c_str(), "rb");
    if (file == nullptr) { return false; }

    SDL_RWclose(file);
    return true;
  }

  bool SDL2_FileSystem::ListDirectory(const std::string &native_directory, std::vector<std::string> &names)
  {
    // SDL2 cannot list a folder, that arrives with SDL3. The standard library
    // does it here, which stays a detail of this backend. Paths are UTF-8, as
    // they are everywhere else SDL is involved.
    const std::filesystem::path directory(std::u8string(native_directory.begin(), native_directory.end()));

    std::error_code error;
    std::filesystem::directory_iterator entries(directory, error);
    if (error) { return false; }

    for (const std::filesystem::directory_iterator end; entries != end; entries.increment(error))
    {
      if (error) { return false; }

      const std::u8string name = entries->path().filename().u8string();
      names.emplace_back(name.begin(), name.end());
    }

    return true;
  }

  bool SDL2_FileSystem::ReadBytes(const std::string &path, std::vector<unsigned char> &contents)
  {
    std::string native_path;
    if (!Locate(path, native_path)) { return false; }

    SDL_RWops *file = SDL_RWFromFile(native_path.c_str(), "rb");
    if (file == nullptr)
    {
      auto error = std::string(SDL_GetError());
      _logger->Error("Could not open {}: {}", path, error);
      return false;
    }

    const Sint64 size = SDL_RWsize(file);
    if (size < 0)
    {
      auto error = std::string(SDL_GetError());
      _logger->Error("Could not determine the size of {}: {}", path, error);
      SDL_RWclose(file);
      return false;
    }

    contents.resize(static_cast<size_t>(size));

    // a single read may return fewer bytes than asked for
    size_t total = 0;
    while (total < contents.size())
    {
      const size_t read = SDL_RWread(file, contents.data() + total, 1, contents.size() - total);
      if (read == 0) { break; }
      total += read;
    }

    SDL_RWclose(file);

    if (total != contents.size())
    {
      // the logger formats lvalues only
      const size_t expected = contents.size();
      _logger->Error("Could not read all of {}: got {} of {} bytes", path, total, expected);
      contents.clear();
      return false;
    }

    return true;
  }
} // neon
