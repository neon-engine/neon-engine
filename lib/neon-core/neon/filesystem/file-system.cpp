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

  bool FileSystem::Locate(const std::string &path, std::string &native_path)
  {
    if (!path.starts_with(assets_scheme))
    {
      const std::string scheme(assets_scheme);
      _logger->Error("Invalid path '{}': it does not start with a known scheme such as {}", path, scheme);
      return false;
    }

    const std::string_view relative = std::string_view(path).substr(assets_scheme.size());

    std::vector<std::string_view> segments;
    if (const std::string reason = check_and_split(relative, segments); !reason.empty())
    {
      _logger->Error("Invalid path '{}': {}", path, reason);
      return false;
    }

    // walk down one name at a time and compare it with what is on disk
    std::string current = _assets_directory;
    for (size_t i = 0; i < segments.size(); i++)
    {
      std::vector<std::string> names;
      if (!ListDirectory(current, names)) { return false; }

      const std::string_view wanted = segments[i];
      bool found = false;
      std::string differs_in_case;

      for (const auto &name : names)
      {
        if (name == wanted)
        {
          found = true;
          break;
        }
        if (equals_ignoring_case(name, wanted)) { differs_in_case = name; }
      }

      if (!found)
      {
        if (!differs_in_case.empty())
        {
          const std::string written(wanted);
          _logger->Error(
            "Invalid path '{}': '{}' is named '{}' on disk, letter case has to match on every platform",
            path,
            written,
            differs_in_case);
        }
        return false;
      }

      current += wanted;
      if (i + 1 < segments.size()) { current += _native_separator; }
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
} // neon
