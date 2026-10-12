#include "project-template.hpp"

#include <algorithm>
#include <format>
#include <utility>
#include <vector>

#include "project-template-text.hpp"

namespace neon::editor
{
  // Helpers of ProjectTemplate: one file of the project, by its path from
  // the root with forward slashes, which is joined with the separator of the
  // platform when it is written.
  namespace
  {
    struct TemplateFile
    {
      std::string relative_path;
      std::string contents;
    };

    std::vector<TemplateFile> files_of(const NewProject &project)
    {
      const std::string extension = "extensions/" + project.name + "/";
      return {
        {"CMakeLists.txt", ProjectTemplateText::CMakeLists(project)},
        {"CMakePresets.json", ProjectTemplateText::CMakePresets()},
        {".gitignore", ProjectTemplateText::GitIgnore()},
        {"README.md", ProjectTemplateText::ReadMe(project)},
        {"assets/project.yml", ProjectTemplateText::ProjectFile(project)},
        {"assets/settings.yml", ProjectTemplateText::Settings(project)},
        {"assets/scenes/start.scene.yml", ProjectTemplateText::StartScene(project)},
        {"assets/input/game.input.yml", ProjectTemplateText::InputMap(project)},
        {extension + "extension.yml", ProjectTemplateText::ExtensionRecipe(project)},
        {extension + project.name + ".cpp", ProjectTemplateText::ExtensionSource(project)},
      };
    }
  }

  ProjectTemplate::ProjectTemplate(NativeFileContext *files, const std::shared_ptr<Logger> &logger)
    : _files(files),
      _logger(logger) {}

  bool ProjectTemplate::DefaultParentDirectory(std::string &native_directory) const
  {
    std::string home;
    if (!_files->HomeDirectory(home)) { return false; }

    native_directory = _files->Join(home, std::string(default_parent_name));
    return true;
  }

  bool ProjectTemplate::IsPlainName(const std::string_view name)
  {
    if (name.empty() || name.front() < 'a' || name.front() > 'z') { return false; }

    return std::ranges::all_of(name, [](const char character)
    {
      return (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '-';
    });
  }

  bool ProjectTemplate::Write(const std::string &native_path, const std::string &contents, std::string &error) const
  {
    if (!_files->WriteText(native_path, contents))
    {
      error = std::format("The file {} could not be written", native_path);
      return false;
    }
    _logger->Info("Wrote {}", native_path);
    return true;
  }

  bool ProjectTemplate::Create(const NewProject &given, std::string &project_directory, std::string &error) const
  {
    NewProject project = given;

    if (!IsPlainName(project.name))
    {
      error = std::format(
        "'{}' is not a plain name for a project: lowercase letters, digits, and dashes, starting with a letter",
        project.name);
      return false;
    }
    if (!IsPlainName(project.organization))
    {
      error = std::format(
        "'{}' is not a plain name for an organization: lowercase letters, digits, and dashes, starting with a "
        "letter",
        project.organization);
      return false;
    }

    const std::string sdk = _files->Join(_files->Join(project.engine_directory, "cmake"), "NeonSdk.cmake");
    if (project.engine_directory.empty() || !_files->Exists(sdk))
    {
      error = std::format(
        "The engine is not at '{}': there is no cmake/NeonSdk.cmake in it. Say where the checkout of the engine is",
        project.engine_directory);
      return false;
    }

    if (project.parent_directory.empty() && !DefaultParentDirectory(project.parent_directory))
    {
      error = "The home folder is not known, so there is no default for where the project goes. Say where";
      return false;
    }

    project_directory = _files->Join(project.parent_directory, project.name);
    if (_files->Exists(project_directory))
    {
      error = std::format("There is already something at {}. A project is only made in a folder that is new",
                          project_directory);
      return false;
    }

    // the parent folder is made when it is missing: ~/Dev on a fresh machine
    if (!_files->MakeDirectories(project_directory))
    {
      error = std::format("The folder {} could not be created", project_directory);
      return false;
    }

    for (const auto &[relative_path, contents] : files_of(project))
    {
      // the folders of the file first, from the root of the project
      std::string directory = project_directory;
      std::size_t start = 0;
      for (std::size_t slash = relative_path.find('/'); slash != std::string::npos;
           slash = relative_path.find('/', start))
      {
        directory = _files->Join(directory, relative_path.substr(start, slash - start));
        start = slash + 1;
      }
      if (!_files->MakeDirectories(directory))
      {
        error = std::format("The folder {} could not be created", directory);
        return false;
      }

      if (!Write(_files->Join(directory, relative_path.substr(start)), contents, error)) { return false; }
    }

    _logger->Info("Made the project {} of {} in {}", project.name, project.organization, project_directory);
    return true;
  }
} // neon::editor
