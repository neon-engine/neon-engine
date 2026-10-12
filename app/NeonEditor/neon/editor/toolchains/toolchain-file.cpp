#include "toolchain-file.hpp"

#include <format>

#include <neon/data/data-reader.hpp>
#include <neon/project/project-file.hpp>

#include "toolchain-placeholders.hpp"

namespace neon
{
  // Helpers of ToolchainFile: one reader per part of the file.
  namespace
  {
    const std::string what_is_read = "the toolchains";

    /// Reads a command of a toolchain, and checks its placeholders. Returns
    /// false when it is not written or is wrong, which was reported.
    bool read_command(const DataReader &entry, const std::string &key, std::string &command)
    {
      if (!entry.Read(key, command))
      {
        if (!entry.Has(key))
        {
          entry.Report(std::format("'{}' is missing. It is the command, as one would type it", key));
        }
        return false;
      }
      if (command.empty())
      {
        entry.Report(*entry.ReadValue(key), std::format("'{}' of {} is empty", key, entry.GetWhere()));
        return false;
      }
      if (std::string problem; !ToolchainPlaceholders::Check(command, problem))
      {
        entry.Report(*entry.ReadValue(key), std::format("'{}' of {}: {}", key, entry.GetWhere(), problem));
        return false;
      }
      return true;
    }

    /// One toolchain under its name: a map with its kind, its platform,
    /// what it requires, and its commands.
    void read_toolchain(const DataReader &part, const std::string &name, const DataValue &written, Toolchains &toolchains)
    {
      if (!ProjectFile::IsPlainName(name))
      {
        part.Report(written, std::format(
                      "'{}' of {} is not a plain name: lowercase letters, digits, and dashes, starting with a "
                      "letter, as a toolchain is named in the editor",
                      name, part.GetWhere()));
        return;
      }

      // off drops a toolchain of the file underneath
      if (std::string text; written.GetText(text) && text == "off")
      {
        toolchains.Drop(name);
        return;
      }

      if (!written.IsMap())
      {
        part.Report(written, std::format(
                      "'{}' of {} is {}, where a map with the kind, the platform, and the commands of the "
                      "toolchain was expected, or off to drop it",
                      name, part.GetWhere(), DataValue::Describe(written.GetKind())));
        return;
      }

      const DataReader entry(written, part.GetDocument(), std::format("toolchain '{}'", name), part.GetErrors());
      const std::size_t before = entry.GetErrors().size();
      Toolchain toolchain;
      toolchain.name = name;

      std::string kind;
      if (!entry.Read("kind", kind))
      {
        if (!entry.Has("kind")) { entry.Report("'kind' is missing. It is native or docker"); }
      } else if (!toolchain_kind_of(kind, toolchain.kind))
      {
        entry.Report(*entry.ReadValue("kind"), std::format(
                       "'kind' of {} is '{}', where native or docker was expected", entry.GetWhere(), kind));
      }

      if (!entry.Read("platform", toolchain.platform))
      {
        if (!entry.Has("platform"))
        {
          entry.Report("'platform' is missing. It is what the toolchain builds for, such as darwin-arm64 or "
            "linux-x86_64");
        }
      } else if (toolchain.platform.empty())
      {
        entry.Report(*entry.ReadValue("platform"), std::format("'platform' of {} is empty", entry.GetWhere()));
      }

      // the image belongs to a docker toolchain and to no other
      const bool has_image = entry.Read("image", toolchain.image);
      if (toolchain.kind == ToolchainKind::Docker)
      {
        if (!has_image && !entry.Has("image"))
        {
          entry.Report("'image' is missing. It is the Docker image the toolchain runs in");
        } else if (has_image && toolchain.image.empty())
        {
          entry.Report(*entry.ReadValue("image"), std::format("'image' of {} is empty", entry.GetWhere()));
        }
      } else if (entry.Has("image"))
      {
        entry.Report(*entry.ReadValue("image"), std::format(
                       "'image' of {} is not taken: a native toolchain runs in no Docker image", entry.GetWhere()));
      }

      if (entry.Read("requires", toolchain.required))
      {
        for (const auto &program : toolchain.required)
        {
          if (program.empty())
          {
            entry.Report(*entry.ReadValue("requires"), std::format(
                           "'requires' of {} holds an empty name, where a program or a path was expected",
                           entry.GetWhere()));
            break;
          }
        }
      }

      read_command(entry, "configure", toolchain.configure);
      read_command(entry, "build", toolchain.build);

      // run is off when the result cannot be started here
      if (const DataValue *run = entry.ReadValue("run"); run != nullptr)
      {
        if (std::string text; run->GetText(text) && text == "off")
        {
          toolchain.run.clear();
        } else
        {
          read_command(entry, "run", toolchain.run);
        }
      }

      entry.Finish();
      if (entry.GetErrors().size() > before) { return; }

      toolchains.Set(toolchain);
    }

    void read_toolchains(const DataReader &reader, Toolchains &toolchains)
    {
      const DataValue *written = reader.ReadValue("toolchains");
      if (written == nullptr) { return; }

      if (!written->IsMap())
      {
        reader.Report(*written, std::format(
                        "'toolchains' is {}, where a map of toolchains by name was expected",
                        DataValue::Describe(written->GetKind())));
        return;
      }

      const DataReader part(*written, reader.GetDocument(), "'toolchains'", reader.GetErrors());
      for (const auto &[name, value] : written->GetEntries())
      {
        read_toolchain(part, name, value, toolchains);
      }
    }

    /// Where the editors are, each a path under its name.
    void read_editors(const DataReader &reader, Toolchains &toolchains)
    {
      const DataValue *written = reader.ReadValue("editors");
      if (written == nullptr) { return; }

      if (!written->IsMap())
      {
        reader.Report(*written, std::format(
                        "'editors' is {}, where a map of paths by the name of the editor was expected",
                        DataValue::Describe(written->GetKind())));
        return;
      }

      for (const auto &[name, value] : written->GetEntries())
      {
        std::string path;
        if (!value.GetText(path) || path.empty())
        {
          reader.Report(value, std::format(
                          "'{}' of 'editors' is {}, where the path of the editor was expected",
                          name, path.empty() && value.GetKind() == DataValue::Kind::Text
                            ? "empty"
                            : DataValue::Describe(value.GetKind())));
          continue;
        }
        toolchains.SetEditor(name, path);
      }
    }
  }

  ToolchainFile::ToolchainFile(
    FileSystemContext *file_system,
    DocumentFormat *format,
    const std::shared_ptr<Logger> &logger)
  {
    _file_system = file_system;
    _format = format;
    _logger = logger;
  }

  bool ToolchainFile::Read(const std::string &path, Toolchains &toolchains, std::vector<std::string> &errors) const
  {
    _logger->Info("Reading the toolchains from {}", path);

    std::string text;
    if (!_file_system->ReadText(path, text))
    {
      errors.push_back(std::format("{}: the file cannot be read", path));
      return false;
    }

    return ReadText(path, text, toolchains, errors);
  }

  bool ToolchainFile::ReadText(
    const std::string &name,
    const std::string &text,
    Toolchains &toolchains,
    std::vector<std::string> &errors) const
  {
    DataValue document;
    if (std::string error; !_format->Read(name, text, document, error))
    {
      errors.push_back(error);
      return false;
    }

    const std::size_t before = errors.size();
    const DataReader reader(document, name, what_is_read, errors);

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
                      "the file has version {}, and this editor reads up to version {}", written, version));
    }

    // read apart, so that a file with a mistake adds nothing
    Toolchains read;
    read_toolchains(reader, read);
    read_editors(reader, read);

    reader.Finish();

    if (errors.size() > before) { return false; }

    toolchains.Apply(read);
    return true;
  }
} // neon
