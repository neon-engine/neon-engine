#include "sdl2-native-library.hpp"

#include <SDL.h>

namespace neon
{
  SDL2_NativeLibrary::~SDL2_NativeLibrary()
  {
    SDL2_NativeLibrary::Close();
  }

  bool SDL2_NativeLibrary::OpenAt(const std::string &native_path, std::string &error)
  {
    Close();

    _object = SDL_LoadObject(native_path.c_str());
    if (_object == nullptr)
    {
      error = SDL_GetError();
      return false;
    }

    return true;
  }

  void *SDL2_NativeLibrary::FindFunction(const std::string &name)
  {
    if (_object == nullptr) { return nullptr; }

    return SDL_LoadFunction(_object, name.c_str());
  }

  void SDL2_NativeLibrary::Close()
  {
    if (_object == nullptr) { return; }

    SDL_UnloadObject(_object);
    _object = nullptr;
  }
} // neon
