// Makes a new project from the command line, as the editor's dialog will
// (#573): the tool a test and a person use until NeonEditor has
// --new-project. See docs/new-projects.md.
//
//     new-project <name> [--organization NAME] [--parent FOLDER] [--engine FOLDER]
//
// The organization is the name unless given, the parent folder is ~/Dev,
// and the engine is the checkout this tool was built from. The folder that
// was made is printed on its own line at the end.

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <neon/editor/templates/project-template.hpp>
#include <neon/filesystem/os-native-files.hpp>
#include <neon/logging/logging-system.hpp>
#include <neon/runtime/settings-config.hpp>

namespace
{
  constexpr const char *usage =
    "usage: new-project <name> [--organization NAME] [--parent FOLDER] [--engine FOLDER]";
}

int main(const int argc, char **argv)
{
  const std::vector<std::string> arguments(argv + 1, argv + argc);
  if (arguments.empty())
  {
    std::cerr << usage << "\n";
    return EXIT_FAILURE;
  }

  neon::editor::NewProject project{.name = arguments.front(), .engine_directory = NEON_ENGINE_DIRECTORY};
  for (std::size_t at = 1; at < arguments.size(); at += 2)
  {
    if (at + 1 >= arguments.size())
    {
      std::cerr << "The option " << arguments[at] << " needs a value\n" << usage << "\n";
      return EXIT_FAILURE;
    }
    const std::string &option = arguments[at];
    const std::string &value = arguments[at + 1];
    if (option == "--organization")
    {
      project.organization = value;
    } else if (option == "--parent")
    {
      project.parent_directory = value;
    } else if (option == "--engine")
    {
      project.engine_directory = value;
    } else
    {
      std::cerr << "Unknown option " << option << "\n" << usage << "\n";
      return EXIT_FAILURE;
    }
  }
  if (project.organization.empty()) { project.organization = project.name; }

  const SettingsConfig settings_config{};
  neon::LoggingSystem logging_system(settings_config);
  logging_system.Initialize();
  logging_system.GoWithoutLogFile();

  neon::OS_NativeFiles files(logging_system.CreateLogger("OS_NativeFiles"));
  const neon::editor::ProjectTemplate project_template(&files, logging_system.CreateLogger("ProjectTemplate"));

  std::string directory;
  std::string error;
  const bool made = project_template.Create(project, directory, error);
  if (!made) { std::cerr << error << "\n"; }
  logging_system.CleanUp();
  if (!made) { return EXIT_FAILURE; }

  std::cout << directory << "\n";
  return EXIT_SUCCESS;
}
