#include "file-system.hpp"

#include <cctype>
#include <vector>

namespace neon
{
  namespace
  {
    // Characters that at least one supported platform forbids in a file name.
    // The colon also rules out drive letters.
    constexpr std::string_view forbidden_characters = "<>:\"|?*";

    /// Returns why a virtual path, with its scheme already removed, is not
    /// portable. Fills in its segments and returns an empty string if it is.
    std::string check_and_split(const std::string_view relative, std::vector<std::string_view> &segments)
    {
      if (relative.find('\\') != std::string_view::npos)
      {
        return "it contains a backslash, folders are separated by forward slashes on every platform";
      }

      for (const char character : relative)
      {
        if (static_cast<unsigned char>(character) < 0x20 ||
            forbidden_characters.find(character) != std::string_view::npos)
        {
          return "it contains a character that is not allowed in file names on every platform";
        }
      }

      size_t start = 0;
      while (start <= relative.size())
      {
        size_t end = relative.find('/', start);
        if (end == std::string_view::npos) { end = relative.size(); }

        const std::string_view segment = relative.substr(start, end - start);
        start = end + 1;

        // doubled slashes and "." add nothing
        if (segment.empty() || segment == ".") { continue; }

        if (segment == "..")
        {
          return "it contains '..', a path may not leave the folder of its scheme";
        }

        if (segment.back() == '.' || segment.back() == ' ')
        {
          return "a file or folder name ends with a dot or a space, which not every platform keeps";
        }

        segments.push_back(segment);
      }

      if (segments.empty())
      {
        return "it names no file";
      }

      return "";
    }
  }

  namespace
  {
    // Good enough to tell the author what went wrong. Whether a name matches
    // is decided by an exact comparison, not by this.
    bool equals_ignoring_case(const std::string_view left, const std::string_view right)
    {
      if (left.size() != right.size()) { return false; }

      for (size_t i = 0; i < left.size(); i++)
      {
        const auto a = static_cast<unsigned char>(left[i]);
        const auto b = static_cast<unsigned char>(right[i]);
        if (std::tolower(a) != std::tolower(b)) { return false; }
      }
      return true;
    }
  }

  namespace
  {
    enum class Match
    {
      Exact,
      DiffersInCase,
      None
    };

    /// Looks for a name among the entries of a folder. `on_disk` receives the
    /// spelling found when only the letter case differs.
    Match find_name(const std::vector<std::string> &names, const std::string_view wanted, std::string &on_disk)
    {
      Match match = Match::None;
      for (const auto &name : names)
      {
        if (name == wanted) { return Match::Exact; }

        if (equals_ignoring_case(name, wanted))
        {
          on_disk = name;
          match = Match::DiffersInCase;
        }
      }
      return match;
    }
  }

  bool FileSystem::Parse(
    const std::string &path,
    std::string &scheme_directory,
    bool &writable,
    std::vector<std::string_view> &segments) const
  {
    std::string_view scheme;
    if (path.starts_with(assets_scheme))
    {
      scheme = assets_scheme;
      scheme_directory = _assets_directory;
      writable = false;
    } else if (path.starts_with(user_scheme))
    {
      scheme = user_scheme;
      scheme_directory = _user_directory;
      writable = true;
    } else if (path.starts_with(output_scheme))
    {
      if (_output_directory.empty())
      {
        _logger->Error(
          "Invalid path '{}': {} has no folder. None was chosen with --output-dir, or the chosen one cannot be used",
          path,
          output_scheme);
        return false;
      }

      scheme = output_scheme;
      scheme_directory = _output_directory;
      writable = true;
    } else
    {
      const std::string known =
        std::string(assets_scheme) + ", " + std::string(user_scheme) + " or " + std::string(output_scheme);
      _logger->Error("Invalid path '{}': it does not start with a known scheme, which are {}", path, known);
      return false;
    }

    const std::string_view relative = std::string_view(path).substr(scheme.size());
    if (const std::string reason = check_and_split(relative, segments); !reason.empty())
    {
      _logger->Error("Invalid path '{}': {}", path, reason);
      return false;
    }

    return true;
  }

  bool FileSystem::Locate(const std::string &path, std::string &native_path)
  {
    std::string current;
    bool writable;
    std::vector<std::string_view> segments;
    if (!Parse(path, current, writable, segments)) { return false; }

    // walk down one name at a time and compare it with what is on disk
    for (size_t i = 0; i < segments.size(); i++)
    {
      std::vector<std::string> names;
      if (!ListDirectory(current, names)) { return false; }

      std::string on_disk;
      const Match match = find_name(names, segments[i], on_disk);

      if (match == Match::DiffersInCase)
      {
        const std::string written(segments[i]);
        _logger->Error(
          "Invalid path '{}': '{}' is named '{}' on disk, letter case has to match on every platform",
          path,
          written,
          on_disk);
      }
      if (match != Match::Exact) { return false; }

      current += segments[i];
      if (i + 1 < segments.size()) { current += _native_separator; }
    }

    native_path = current;
    return true;
  }

  bool FileSystem::LocateForWriting(const std::string &path, std::string &native_path)
  {
    std::string current;
    bool writable;
    std::vector<std::string_view> segments;
    if (!Parse(path, current, writable, segments)) { return false; }

    if (!writable)
    {
      _logger->Error("Invalid path '{}': its scheme is read-only", path);
      return false;
    }

    for (size_t i = 0; i < segments.size(); i++)
    {
      const bool is_file = i + 1 == segments.size();

      std::vector<std::string> names;
      if (!ListDirectory(current, names))
      {
        _logger->Error("Could not write '{}': a folder on the way to it cannot be listed", path);
        return false;
      }

      std::string on_disk;
      const Match match = find_name(names, segments[i], on_disk);

      if (match == Match::DiffersInCase)
      {
        const std::string written(segments[i]);
        _logger->Error(
          "Invalid path '{}': '{}' is named '{}' on disk, letter case has to match on every platform",
          path,
          written,
          on_disk);
        return false;
      }

      current += segments[i];

      if (!is_file)
      {
        if (match == Match::None && !MakeDirectory(current))
        {
          _logger->Error("Could not write '{}': a folder on the way to it cannot be created", path);
          return false;
        }
        current += _native_separator;
      }
    }

    native_path = current;
    return true;
  }

  bool FileSystem::ReadText(const std::string &path, std::string &contents)
  {
    std::vector<unsigned char> bytes;
    if (!ReadBytes(path, bytes)) { return false; }

    contents.assign(bytes.begin(), bytes.end());
    return true;
  }

  bool FileSystem::WriteText(const std::string &path, const std::string &contents)
  {
    return WriteBytes(path, {contents.begin(), contents.end()});
  }
} // neon
