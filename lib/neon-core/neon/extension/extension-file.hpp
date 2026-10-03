#ifndef EXTENSION_FILE_HPP
#define EXTENSION_FILE_HPP

#include <string>
#include <string_view>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>

#include "extension-recipe.hpp"

namespace neon
{
  /// The recipe at the root of the folder of an extension, which says what
  /// the extension is.
  ///
  ///     version: 1
  ///     name: quake
  ///
  ///     libraries:
  ///       macos-arm64: quake-macos-arm64.dylib
  ///       linux-x86_64: quake-linux-x86_64.so
  ///       windows-x86_64: quake-windows-x86_64.dll
  ///
  /// `version` is the version of this layout. `name` is the name of the
  /// folder. `libraries` names the library for each platform, as a file in
  /// the folder. A name that is not known is an error, so that a name that
  /// was misspelled does not go unnoticed, and so is a platform that is not
  /// known.
  ///
  /// Which format the file has is up to the DocumentFormat that is handed in.
  class ExtensionFile final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;

  public:
    /// The version of the layout of the file that is read, and the highest
    /// that is understood.
    static constexpr int version = 1;

    /// What the file is called, in the folder of every extension.
    static constexpr std::string_view file_name = "extension.yml";

    /// The platforms a library can be named for.
    static const std::vector<std::string> &GetPlatforms();

    ExtensionFile(FileSystemContext *file_system, DocumentFormat *format);

    /// Where the recipe of the extension in a folder of `extensions://` is.
    [[nodiscard]] static std::string PathOf(const std::string &folder);

    /// Reads the recipe of the extension in a folder of `extensions://`
    /// into `recipe`. Returns false when the file cannot be read or
    /// something in it is wrong, and every problem that was found is in
    /// `errors` with its line, not only the first.
    bool Read(const std::string &folder, ExtensionRecipe &recipe, std::vector<std::string> &errors) const;
  };
} // neon

#endif //EXTENSION_FILE_HPP
