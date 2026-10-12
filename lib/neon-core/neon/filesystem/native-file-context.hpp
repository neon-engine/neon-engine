#ifndef NATIVE_FILE_CONTEXT_HPP
#define NATIVE_FILE_CONTEXT_HPP

#include <string>

namespace neon
{
  /// The files of this machine, by native path: what a tool of the editor
  /// sees when it writes somewhere no scheme of FileSystemContext reaches,
  /// such as a new project under the home folder of the user.
  ///
  /// Nothing a game runs takes this. A game reads and writes virtual paths
  /// through FileSystemContext, whose rules keep it inside its folders; a
  /// tool that makes a folder for a person to work in is the one thing that
  /// is handed a path of the operating system, by that person. The paths are
  /// UTF-8, as every native path the engine holds, see NativePath.
  class NativeFileContext
  {
  protected:
    ~NativeFileContext() = default;

  public:
    /// The home folder of the user, without a separator at the end. Returns
    /// false when the operating system does not say where it is.
    virtual bool HomeDirectory(std::string &native_directory) = 0;

    /// A path joined from a folder and a name, with the separator of the
    /// platform between them.
    [[nodiscard]] virtual std::string Join(const std::string &native_directory, const std::string &name) = 0;

    /// Whether something is at the path: a file or a folder.
    virtual bool Exists(const std::string &native_path) = 0;

    /// Whether a folder is at the path.
    virtual bool IsDirectory(const std::string &native_path) = 0;

    /// Creates a folder and every folder that leads to it. A folder that is
    /// there already is left as it is. Returns false when one cannot be
    /// created, and says why in the log.
    virtual bool MakeDirectories(const std::string &native_directory) = 0;

    /// Writes a whole file as text, replacing it if it exists. The folder it
    /// goes in has to be there. Returns false when it cannot be written, and
    /// says why in the log.
    virtual bool WriteText(const std::string &native_path, const std::string &contents) = 0;
  };
} // neon

#endif //NATIVE_FILE_CONTEXT_HPP
