#include "toolchain-files.hpp"

#include <vector>

namespace neon
{
  ToolchainFiles::ToolchainFiles(
    const ToolchainFile *file,
    FileSystemContext *file_system,
    const std::shared_ptr<Logger> &logger)
    : _file(file),
      _file_system(file_system),
      _logger(logger) {}

  bool ToolchainFiles::ReadOne(const std::string &path, Toolchains &toolchains) const
  {
    if (!_file_system->Exists(path))
    {
      _logger->Debug("There is no {}", path);
      return true;
    }

    std::vector<std::string> errors;
    if (_file->Read(path, toolchains, errors)) { return true; }
    for (const auto &error : errors) { _logger->Error("{}", error); }
    return false;
  }

  bool ToolchainFiles::Read(Toolchains &toolchains) const
  {
    toolchains = Toolchains::Defaults();

    const bool shared = ReadOne(std::string(shared_path), toolchains);
    const bool user = ReadOne(std::string(user_path), toolchains);
    return shared && user;
  }
} // neon
