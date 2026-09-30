#ifndef TEMPORARY_DIRECTORY_HPP
#define TEMPORARY_DIRECTORY_HPP

#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>

namespace neon::testing
{
  /// A folder of its own for one test, in the place the operating system
  /// keeps temporary files. It is removed with everything in it when the
  /// object goes away.
  ///
  /// This is the one place tests reach the disk directly, to prepare what a
  /// backend is expected to find and to look at what it left behind. The
  /// engine itself never does, see the development guide.
  class TemporaryDirectory final
  {
    std::filesystem::path _directory;

  public:
    TemporaryDirectory()
    {
      std::random_device random;
      const std::filesystem::path parent = std::filesystem::temp_directory_path();

      // a name that is taken is tried again with another number
      for (int attempt = 0; attempt < 100; attempt++)
      {
        const auto candidate = parent / ("neon-engine-test-" + std::to_string(random()));

        std::error_code error;
        if (std::filesystem::create_directories(candidate, error) && !error)
        {
          _directory = std::filesystem::canonical(candidate);
          return;
        }
      }

      throw std::runtime_error("Could not create a temporary folder in " + parent.string());
    }

    TemporaryDirectory(const TemporaryDirectory &) = delete;

    TemporaryDirectory &operator=(const TemporaryDirectory &) = delete;

    ~TemporaryDirectory()
    {
      std::error_code error;
      std::filesystem::remove_all(_directory, error);
    }

    [[nodiscard]] const std::filesystem::path &Path() const
    {
      return _directory;
    }

    /// The native path of something inside the folder, named with forward
    /// slashes, such as `saves/slot-1.dat`. Empty gives the folder itself.
    [[nodiscard]] std::string Native(const std::string &relative = "") const
    {
      const auto path = relative.empty() ? _directory : _directory / std::filesystem::path(relative);

      auto preferred = path;
      const std::u8string native = preferred.make_preferred().u8string();
      return {native.begin(), native.end()};
    }

    /// Writes a file, and creates the folders that lead to it.
    void Write(const std::string &relative, const std::string &contents) const
    {
      const auto path = _directory / std::filesystem::path(relative);
      std::filesystem::create_directories(path.parent_path());

      std::ofstream file(path, std::ios::binary | std::ios::trunc);
      file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
      if (!file) { throw std::runtime_error("Could not write " + path.string()); }
    }

    /// The contents of a file. Throws when it cannot be read.
    [[nodiscard]] std::string Read(const std::string &relative) const
    {
      const auto path = _directory / std::filesystem::path(relative);

      std::ifstream file(path, std::ios::binary);
      if (!file) { throw std::runtime_error("Could not read " + path.string()); }
      return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    [[nodiscard]] bool HasFile(const std::string &relative) const
    {
      return std::filesystem::is_regular_file(_directory / std::filesystem::path(relative));
    }

    [[nodiscard]] bool HasDirectory(const std::string &relative) const
    {
      return std::filesystem::is_directory(_directory / std::filesystem::path(relative));
    }
  };
} // neon::testing

#endif //TEMPORARY_DIRECTORY_HPP
