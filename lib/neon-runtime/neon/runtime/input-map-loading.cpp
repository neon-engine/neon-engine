#include "input-map-loading.hpp"

#include <neon/input/input-map-file.hpp>

namespace neon
{
  InputMapLoading::InputMapLoading(
    FileSystemContext *file_system,
    DocumentFormat *format,
    LoggingContext *logging,
    const std::string &in_prose)
    : _file_system(file_system), _format(format), _logging(logging), _in_prose(in_prose) {}

  bool InputMapLoading::Load(
    const std::string &project_input,
    InputSystem &input_system,
    std::vector<std::string> &remarks,
    std::vector<std::string> &errors) const
  {
    std::string path = project_input;
    if (path.empty())
    {
      path = std::string(InputMapFile::default_path);
      if (!_file_system->Exists(path))
      {
        remarks.push_back(
          path + " is not next to " + _in_prose + ", so the input map built into the engine is used");
        return true;
      }
    }

    const InputMapFile file(_file_system, _format, _logging->CreateLogger("InputMapFile"));
    InputMap map;
    if (!file.Read(path, map, errors)) { return false; }

    input_system.SetInputMap(map);
    return true;
  }
} // neon
