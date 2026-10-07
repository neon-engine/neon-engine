#include "file-system.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace neon
{
  // Helpers of FileSystem, for this file alone.
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

  // Helpers of FileSystem, for this file alone.
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

  // Helpers of FileSystem, for this file alone.
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
    } else if (path.starts_with(engine_scheme))
    {
      scheme = engine_scheme;
      scheme_directory = _engine_directory;
      writable = false;
    } else if (path.starts_with(user_scheme))
    {
      if (_user_directory.empty())
      {
        _logger->Error(
          "Invalid path '{}': {} has no folder yet. It is placed once the project says who makes it and what it is called",
          path,
          user_scheme);
        return false;
      }

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
    } else if (path.starts_with(extensions_scheme))
    {
      scheme = extensions_scheme;
      scheme_directory = _extensions_directory;
      writable = false;
    } else
    {
      const std::string known =
        std::string(assets_scheme) + ", " + std::string(engine_scheme) + ", " + std::string(user_scheme) + ", " +
        std::string(output_scheme) + " or " + std::string(extensions_scheme);
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

  bool FileSystem::PlaceLogFile(const std::string &path, LogFileTarget &target)
  {
    std::string native_path;
    if (!LocateForWriting(path, native_path))
    {
      _logger->Error("The log file cannot be placed at '{}'", path);
      target.GoWithoutLogFile();
      return false;
    }

    return target.OpenLogFile(native_path);
  }

  bool FileSystem::OpenLibrary(const std::string &path, NativeLibrary &library, std::string &error)
  {
    std::string native_path;
    if (!Locate(path, native_path))
    {
      error = "the file is not there";
      return false;
    }

    return library.OpenAt(native_path, error);
  }

  bool FileSystem::ListFiles(const std::string &directory, std::vector<std::string> &paths)
  {
    // a folder may be written with a slash at its end, which names nothing
    // on its own; a scheme alone keeps its two
    std::string folder = directory;
    while (!folder.empty() && folder.back() == '/' && !folder.ends_with("://")) { folder.pop_back(); }

    // a scheme alone is the folder behind it, which Locate has no name for
    std::string native_directory;
    if (folder == assets_scheme) { native_directory = _assets_directory; }
    else if (folder == engine_scheme) { native_directory = _engine_directory; }
    else if (folder == user_scheme) { native_directory = _user_directory; }
    else if (folder == output_scheme) { native_directory = _output_directory; }
    else if (folder == extensions_scheme) { native_directory = _extensions_directory; }
    else if (!Locate(folder, native_directory)) { return false; }
    if (native_directory.empty()) { return false; }
    while (native_directory.size() > 1 && native_directory.back() == _native_separator) { native_directory.pop_back(); }

    std::vector<std::string> names;
    if (!ListDirectory(native_directory, names)) { return false; }
    std::ranges::sort(names);

    const std::string prefix = folder.ends_with('/') ? folder : folder + '/';

    // the files of this folder first, then the folders below it, each in turn
    std::vector<std::string> folders;
    for (const auto &name : names)
    {
      std::vector<std::string> inside;
      if (ListDirectory(native_directory + _native_separator + name, inside)) { folders.push_back(name); }
      else { paths.push_back(prefix + name); }
    }
    for (const auto &name : folders)
    {
      if (!ListFiles(prefix + name, paths)) { return false; }
    }
    return true;
  }

} // neon
