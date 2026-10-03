#ifndef SDL2_NATIVE_LIBRARY_HPP
#define SDL2_NATIVE_LIBRARY_HPP

#include <string>

#include <neon/extension/native-library.hpp>

namespace neon
{
  /// A library of the platform, opened through SDL2.
  // ReSharper disable once CppInconsistentNaming
  class SDL2_NativeLibrary final : public NativeLibrary
  {
    // what SDL knows the library by while it is open
    void *_object = nullptr;

  public:
    SDL2_NativeLibrary() = default;

    // one object stands for one open library
    SDL2_NativeLibrary(const SDL2_NativeLibrary &) = delete;

    SDL2_NativeLibrary &operator=(const SDL2_NativeLibrary &) = delete;

    ~SDL2_NativeLibrary() override;

    bool OpenAt(const std::string &native_path, std::string &error) override;

    void *FindFunction(const std::string &name) override;

    void Close() override;
  };
} // neon

#endif //SDL2_NATIVE_LIBRARY_HPP
