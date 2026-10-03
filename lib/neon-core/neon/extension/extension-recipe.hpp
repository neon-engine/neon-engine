#ifndef EXTENSION_RECIPE_HPP
#define EXTENSION_RECIPE_HPP

#include <map>
#include <string>

namespace neon
{
  /// What an extension says about itself, as its `extension.yml` holds it.
  struct ExtensionRecipe
  {
    /// What the extension is called, which is the name of its folder. A
    /// plain name, see ProjectFile::IsPlainName.
    std::string name;

    /// The library of the extension for each platform it has one for: what
    /// the platform is called, see LibraryLoader::GetPlatform, and the file
    /// in the folder of the extension. Empty for an extension that brings
    /// no library.
    std::map<std::string, std::string> libraries;
  };
} // neon

#endif //EXTENSION_RECIPE_HPP
