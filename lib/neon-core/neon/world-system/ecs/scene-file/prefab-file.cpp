#include "prefab-file.hpp"

#include <format>

#include <neon/data/data-reader.hpp>

namespace neon
{
  // Helpers of PrefabFile, for this file alone.
  namespace
  {
    const std::string what_is_read = "the prefab";
  }

  PrefabFile::PrefabFile(FileSystemContext *file_system, DocumentFormat *format, const std::string &path)
  {
    _file_system = file_system;
    _format = format;
    _path = path;
  }

  bool PrefabFile::Read(std::vector<std::string> &errors)
  {
    _name.clear();
    _entity = DataValue{};

    // whoever named the file says where it was named, with that line, so
    // nothing is said here
    std::string text;
    if (!_file_system->ReadText(_path, text)) { return false; }

    DataValue document;
    if (std::string error; !_format->Read(_path, text, document, error))
    {
      errors.push_back(error);
      return false;
    }

    const DataReader reader(document, _path, what_is_read, errors);

    reader.Read("prefab", _name);

    if (float written = 0.0f; reader.Read("version", written) && written > static_cast<float>(version))
    {
      reader.Report(*document.Find("version"), std::format(
                      "the prefab has version {}, and this engine reads up to version {}",
                      written, version));
    }

    if (const auto *entity = reader.ReadValue("entity"); entity == nullptr)
    {
      reader.Report("'entity' is missing. It holds the entity the prefab describes");
    } else if (!entity->IsMap())
    {
      reader.Report(*entity, std::format(
                      "'entity' of the prefab is {}, where a map was expected",
                      DataValue::Describe(entity->GetKind())));
    } else
    {
      _entity = *entity;
    }

    reader.Finish();
    return true;
  }

  const std::string &PrefabFile::GetPath() const
  {
    return _path;
  }

  const std::string &PrefabFile::GetName() const
  {
    return _name;
  }

  const DataValue &PrefabFile::GetEntity() const
  {
    return _entity;
  }
} // neon
