#ifndef FILE_SYSTEM_CONTEXT_HPP
#define FILE_SYSTEM_CONTEXT_HPP

#include <string>
#include <vector>

namespace neon
{
  /// What the rest of the engine sees of the file system.
  ///
  /// Files are named by virtual paths such as `assets://models/cube.obj`.
  /// The scheme says where a file lives:
  ///   - `assets://` is what ships with the application. It is read-only.
  ///   - `user://` is a folder of the current user, for saves, settings, and
  ///     anything else the application writes.
  ///
  /// A virtual path is written the same way on every platform, so content
  /// that refers to files, such as a scene file, works everywhere unchanged.
  ///
  /// The rules are enforced on all platforms, including those whose own file
  /// system would accept more:
  ///   - it starts with a known scheme
  ///   - folders are separated by forward slashes, never backslashes
  ///   - it has no `..` segments, so it cannot leave the scheme's folder
  ///   - it avoids characters that some platform forbids in file names
  ///   - its letter case matches the names on disk exactly, so
  ///     `assets://Models/Cube.obj` does not find `models/cube.obj`
  ///
  /// A path that breaks a rule is reported as an error and treated as a file
  /// that does not exist. Paths of the underlying operating system never
  /// appear in this interface.
  class FileSystemContext
  {
  protected:
    ~FileSystemContext() = default;

  public:
    /// Whether a file can be opened for reading.
    virtual bool Exists(const std::string &path) = 0;

    /// Reads a whole file as raw bytes. Returns false if it cannot be read.
    virtual bool ReadBytes(const std::string &path, std::vector<unsigned char> &contents) = 0;

    /// Reads a whole file as text. Returns false if it cannot be read.
    virtual bool ReadText(const std::string &path, std::string &contents) = 0;

    /// Writes a whole file, replacing it if it exists. Folders in the path
    /// that do not exist yet are created. Returns false if it cannot be
    /// written, which includes every path outside a writable scheme.
    virtual bool WriteBytes(const std::string &path, const std::vector<unsigned char> &contents) = 0;

    /// Writes a whole file as text. See WriteBytes.
    virtual bool WriteText(const std::string &path, const std::string &contents) = 0;
  };
}

#endif //FILE_SYSTEM_CONTEXT_HPP
