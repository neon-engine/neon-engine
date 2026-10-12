#ifndef BUILD_TARGET_HPP
#define BUILD_TARGET_HPP

#include <string>

namespace neon
{
  /// What a toolchain is run on: the project the editor has open, and the
  /// engine it is built against. Native paths, as the person who opened the
  /// project chose them; they go into the commands of the toolchain and
  /// nowhere else.
  struct BuildTarget
  {
    /// The folder of the project, where its CMakeLists.txt and editor.yml
    /// are.
    std::string project;

    /// The name of the project, from its project.yml, which is what the
    /// result is called.
    std::string name;

    /// The checkout of the engine the project is built against: the folder
    /// with cmake/NeonSdk.cmake and the runtime that was built. Empty when
    /// there is none to name.
    std::string engine;
  };
} // neon

#endif //BUILD_TARGET_HPP
