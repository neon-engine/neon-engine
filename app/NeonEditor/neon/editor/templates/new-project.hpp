#ifndef NEW_PROJECT_HPP
#define NEW_PROJECT_HPP

#include <string>

namespace neon::editor
{
  /// What a new project is made from: what the dialog of the editor asks
  /// for, or the command line is given. The paths are native, since the
  /// project goes where the person says, outside every scheme of the
  /// engine, see ProjectTemplate.
  struct NewProject
  {
    /// What the project is called, and the name of its folder, of its
    /// extension, and of the game: a plain name, see ProjectFile::IsPlainName.
    std::string name;

    /// Who makes it, in the same plain form. With the name it places
    /// `user://`, see docs/projects.md.
    std::string organization;

    /// The folder the project's folder is made in. Empty stands for
    /// ProjectTemplate::DefaultParentDirectory, `Dev` under the home folder.
    std::string parent_directory;

    /// The checkout of the engine the project is built against: the
    /// folder with `cmake/NeonSdk.cmake`. The generated CMakeLists.txt
    /// names it as the default of NEON_ENGINE_DIRECTORY.
    std::string engine_directory;
  };
} // neon::editor

#endif //NEW_PROJECT_HPP
