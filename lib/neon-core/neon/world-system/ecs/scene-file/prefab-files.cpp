#include "prefab-files.hpp"

#include <algorithm>
#include <format>

namespace neon
{
  PrefabFiles::PrefabFiles(FileSystemContext *file_system, DocumentFormat *format)
  {
    _file_system = file_system;
    _format = format;
  }

  const PrefabFile *PrefabFiles::Find(const std::string &path, std::vector<std::string> &errors)
  {
    for (const auto &[known, file] : _files)
    {
      if (known == path) { return file.get(); }
    }

    auto file = std::make_unique<PrefabFile>(_file_system, _format, path);
    const bool readable = file->Read(errors);

    auto &entry = _files.emplace_back(path, readable ? std::move(file) : nullptr);
    return entry.second.get();
  }

  bool PrefabFiles::IsPlacing(const std::string &path) const
  {
    return std::ranges::any_of(_placing, [&](const auto &entry) { return entry.first == path; });
  }

  bool PrefabFiles::WasPlaced(const std::string &path) const
  {
    return std::ranges::find(_placed, path) != _placed.end();
  }

  void PrefabFiles::BeginPlacing(const std::string &path, const std::string &document, const std::size_t line)
  {
    _placing.emplace_back(path, std::format("{}:{}", document, line));
    if (!WasPlaced(path)) { _placed.push_back(path); }
  }

  void PrefabFiles::EndPlacing()
  {
    _placing.pop_back();
  }

  std::string PrefabFiles::DescribeLoop(
    const std::string &path,
    const std::string &document,
    const std::size_t line) const
  {
    // from where the prefab was first placed down to where it is placed again
    std::string described;
    bool inside = false;
    for (const auto &[placed, from] : _placing)
    {
      inside = inside || placed == path;
      if (!inside) { continue; }

      described += std::format("{} places {}, ", from, placed);
    }

    return described + std::format("{}:{} places {} again", document, line, path);
  }
} // neon
