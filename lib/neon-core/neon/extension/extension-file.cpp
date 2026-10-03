#include "extension-file.hpp"

#include <algorithm>
#include <format>

#include <neon/data/data-reader.hpp>
#include <neon/filesystem/file-system.hpp>
#include <neon/project/project-file.hpp>

namespace neon
{
  // Helpers of ExtensionFile, for this file alone.
  namespace
  {
    const std::string what_is_read = "the extension";
  }

  ExtensionFile::ExtensionFile(FileSystemContext *file_system, DocumentFormat *format)
  {
    _file_system = file_system;
    _format = format;
  }

  const std::vector<std::string> &ExtensionFile::GetPlatforms()
  {
    static const std::vector<std::string> platforms = {
      "macos-arm64",
      "macos-x86_64",
      "linux-x86_64",
      "linux-arm64",
      "windows-x86_64",
      "windows-arm64"
    };
    return platforms;
  }

  std::string ExtensionFile::PathOf(const std::string &folder)
  {
    return std::string(FileSystem::extensions_scheme) + folder + "/" + std::string(file_name);
  }

  bool ExtensionFile::Read(
    const std::string &folder,
    ExtensionRecipe &recipe,
    std::vector<std::string> &errors) const
  {
    const std::string path = PathOf(folder);

    std::string text;
    if (!_file_system->ReadText(path, text))
    {
      errors.push_back(std::format("{}: the file cannot be read", path));
      return false;
    }

    DataValue document;
    if (std::string error; !_format->Read(path, text, document, error))
    {
      errors.push_back(error);
      return false;
    }

    const std::size_t before = errors.size();
    const DataReader reader(document, path, what_is_read, errors);

    // The version is the first thing read, so that a file of a later layout
    // says so before its names are reported as unknown.
    int written = 0;
    if (!reader.Read("version", written))
    {
      if (!reader.Has("version"))
      {
        reader.Report(std::format("'version' is missing. It holds the version of the layout, which is {}", version));
      }
    } else if (written > version)
    {
      reader.Report(*document.Find("version"), std::format(
                      "the extension has version {}, and this engine reads up to version {}",
                      written, version));
    }

    ExtensionRecipe read;

    if (!reader.Read("name", read.name))
    {
      if (!reader.Has("name")) { reader.Report("'name' is missing. It is what the extension is called"); }
    } else if (!ProjectFile::IsPlainName(read.name))
    {
      reader.Report(*document.Find("name"), std::format(
                      "'name' is '{}', where a plain name was expected: lowercase letters, digits, and dashes, "
                      "starting with a letter. It is a folder name on every platform",
                      read.name));
    } else if (read.name != folder)
    {
      reader.Report(*document.Find("name"), std::format(
                      "'name' is '{}', and the folder of the extension is '{}'. The two are the same",
                      read.name, folder));
    }

    if (const DataValue *libraries = reader.ReadValue("libraries"); libraries != nullptr)
    {
      if (!libraries->IsMap())
      {
        reader.Report(*libraries, "'libraries' holds a library for each platform, as 'macos-arm64: name.dylib'");
      } else
      {
        const auto &platforms = GetPlatforms();
        for (const auto &[platform, file] : libraries->GetEntries())
        {
          std::string file_name;
          if (std::ranges::find(platforms, platform) == platforms.end())
          {
            std::string known;
            for (const auto &name : platforms) { known += (known.empty() ? "" : ", ") + name; }
            reader.Report(file, std::format("'{}' is not a platform. Those are {}", platform, known));
          } else if (!file.GetText(file_name) || file_name.empty())
          {
            reader.Report(file, std::format(
                            "the library for '{}' is a file in the folder of the extension, such as {}-{}.so",
                            platform, folder, platform));
          } else
          {
            read.libraries[platform] = file_name;
          }
        }
      }
    }

    reader.Finish();

    if (errors.size() > before) { return false; }

    recipe = read;
    return true;
  }
} // neon
