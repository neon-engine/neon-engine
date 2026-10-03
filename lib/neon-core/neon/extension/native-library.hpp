#ifndef NATIVE_LIBRARY_HPP
#define NATIVE_LIBRARY_HPP

#include <string>

namespace neon
{
  /// A library of the platform that is opened while the application runs: a
  /// `.dylib`, a `.so`, or a `.dll`.
  ///
  /// The file system decides where the file is, and hands the native path
  /// straight to this interface, so the path never passes through the rest
  /// of the engine. See FileSystem::OpenLibrary.
  class NativeLibrary
  {
  public:
    /// Closes the library when it is still open.
    virtual ~NativeLibrary() = default;

    /// Opens the library at a native path. Returns false when it cannot be
    /// opened, and says why in `error`.
    virtual bool OpenAt(const std::string &native_path, std::string &error) = 0;

    /// The function the library exports under a name, or nullptr when it
    /// exports none, or is not open.
    virtual void *FindFunction(const std::string &name) = 0;

    /// Closes the library. What it handed out is gone with it: no function
    /// of it may be called afterwards. Safe to call more than once.
    virtual void Close() = 0;
  };
} // neon

#endif //NATIVE_LIBRARY_HPP
