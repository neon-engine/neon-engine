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

    // next to the executable as well. The folder need not be there
    _extensions_directory = application_directory + "extensions" + _native_separator;

    _logger->Info("assets:// is {}", _assets_directory);
    _logger->Info("extensions:// is {}", _extensions_directory);

    InitializeOutputDirectory();
  }

  bool SDL2_FileSystem::PlaceUserDirectory(const std::string &organization, const std::string &application)
  {
    // a folder of the current user, which SDL creates if it is missing
    char *user_path = SDL_GetPrefPath(organization.c_str(), application.c_str());
    if (user_path == nullptr)
    {
      auto error = std::string(SDL_GetError());
      _logger->Error("Failed to find the user directory of {}/{}: {}", organization, application, error);
      return false;
    }

    _user_directory = user_path;
    SDL_free(user_path);

    _logger->Info("user:// is {}", _user_directory);
    return true;
  }

  void SDL2_FileSystem::InitializeOutputDirectory()
  {
    _output_directory.clear();

    const std::string &wanted = _settings_config.output_directory;
    if (wanted.empty()) { return; }

    // The folder arrives as a native path, relative to the working directory
    // or absolute. It is made absolute once, so that it keeps pointing at the
    // same place. Paths are UTF-8, see ListDirectory.
    std::error_code error;
    std::filesystem::path directory =
      std::filesystem::absolute(std::filesystem::path(std::u8string(wanted.begin(), wanted.end())), error);

    if (!error)
    {
      directory = directory.lexically_normal().make_preferred();
      std::filesystem::create_directories(directory, error);
    }

    // a file of that name is in the way
    if (!error && !std::filesystem::is_directory(directory, error))
    {
      error = std::make_error_code(std::errc::not_a_directory);
    }

    if (error)
    {
      const std::string reason = error.message();
      _logger->Error("The output folder '{}' cannot be used, output:// stays without a folder: {}", wanted, reason);
      return;
    }

    const std::u8string native = directory.u8string();
    _output_directory.assign(native.begin(), native.end());
    if (_output_directory.back() != _native_separator) { _output_directory += _native_separator; }

    _logger->Info("output:// is {}", _output_directory);
  }

  void SDL2_FileSystem::CleanUp()
  {
    _logger->Info("Cleaning up SDL2 file system");
    _assets_directory.clear();
    _extensions_directory.clear();
    _user_directory.clear();
    _output_directory.clear();
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

  bool SDL2_FileSystem::MakeDirectory(const std::string &native_directory)
  {
    // SDL2 cannot create folders either, see ListDirectory
    const std::filesystem::path directory(std::u8string(native_directory.begin(), native_directory.end()));

    std::error_code error;
    std::filesystem::create_directory(directory, error);
    return !error;
  }

  bool SDL2_FileSystem::WriteBytes(const std::string &path, const std::vector<unsigned char> &contents)
  {
    std::string native_path;
    if (!LocateForWriting(path, native_path)) { return false; }

    SDL_RWops *file = SDL_RWFromFile(native_path.c_str(), "wb");
    if (file == nullptr)
    {
      auto error = std::string(SDL_GetError());
      _logger->Error("Could not open {} for writing: {}", path, error);
      return false;
    }

    size_t total = 0;
    while (total < contents.size())
    {
      const size_t written = SDL_RWwrite(file, contents.data() + total, 1, contents.size() - total);
      if (written == 0) { break; }
      total += written;
    }

    // closing flushes, so a failure here means the file is incomplete
    const bool closed = SDL_RWclose(file) == 0;

    if (total != contents.size() || !closed)
    {
      auto error = std::string(SDL_GetError());
      _logger->Error("Could not write all of {}: {}", path, error);
      return false;
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
