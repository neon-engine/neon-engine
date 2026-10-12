#ifndef TOOLCHAINS_HPP
#define TOOLCHAINS_HPP

#include <map>
#include <string>
#include <vector>

#include "toolchain.hpp"

namespace neon
{
  /// The toolchains the editor knows, and where the editors it opens are:
  /// what one toolchain file declares, or what several say together.
  ///
  /// A team shares `editor.yml` and one machine adds `user-editor.yml` on
  /// top, see ToolchainFile. Apply() lays one set over another: a toolchain
  /// of the same name replaces the whole entry, a name the file switched
  /// off with `off` is dropped, and an editor of the same name replaces
  /// the path. Nothing is merged field by field, so that an entry reads in
  /// one place.
  class Toolchains final
  {
    std::vector<Toolchain> _toolchains;
    std::vector<std::string> _dropped;
    std::map<std::string, std::string> _editors;

  public:
    /// Adds a toolchain, or replaces the one of its name.
    void Set(const Toolchain &toolchain);

    /// Removes the toolchain of the name, when there is one, and remembers
    /// the name, so that Apply() drops it from what is underneath.
    void Drop(const std::string &name);

    /// Where an editor is, under its name, such as `vscode`.
    void SetEditor(const std::string &name, const std::string &path);

    /// The toolchain of a name, or nullptr.
    [[nodiscard]] const Toolchain *Find(const std::string &name) const;

    /// Every toolchain, in the order it was declared.
    [[nodiscard]] const std::vector<Toolchain> &All() const;

    /// The names that were switched off.
    [[nodiscard]] const std::vector<std::string> &Dropped() const;

    /// Where an editor is, or an empty text when it is not named.
    [[nodiscard]] std::string Editor(const std::string &name) const;

    [[nodiscard]] const std::map<std::string, std::string> &Editors() const;

    /// Lays `over` on top of this set, see the class.
    void Apply(const Toolchains &over);
  };
} // neon

#endif //TOOLCHAINS_HPP
