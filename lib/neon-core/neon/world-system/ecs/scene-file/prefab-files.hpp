#ifndef PREFAB_FILES_HPP
#define PREFAB_FILES_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>

#include "prefab-file.hpp"

namespace neon
{
  /// The prefabs a scene places, read once each and kept for the life of
  /// the scene, so that a wall placed forty times is read from its file
  /// once, and a prefab spawned while the game plays is not read again.
  /// Clear() forgets them when another scene is loaded, so that a file that
  /// changed is read anew.
  ///
  /// It also keeps which prefabs are being placed at the moment, from the
  /// scene down, so that a prefab that places itself, through others or
  /// not, is refused instead of read without end.
  class PrefabFiles final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;

    // every file that was asked for, readable or not, so that none is read
    // twice and a file that cannot be read is reported where it is named
    std::vector<std::pair<std::string, std::unique_ptr<PrefabFile>>> _files;

    // the paths of the prefabs being placed, outermost first, each with the
    // document and line that placed it
    std::vector<std::pair<std::string, std::string>> _placing;

    // the paths of the prefabs that were placed once already, whose own
    // problems were reported then
    std::vector<std::string> _placed;

  public:
    PrefabFiles(FileSystemContext *file_system, DocumentFormat *format);

    /// The prefab at a path, read the first time it is asked for. Returns
    /// nullptr when the file cannot be read, every time it is asked for;
    /// what is wrong with the file is in `errors` once, with its line.
    [[nodiscard]] const PrefabFile *Find(const std::string &path, std::vector<std::string> &errors);

    /// Whether the prefab is being placed at the moment, so that placing it
    /// again would be a loop.
    [[nodiscard]] bool IsPlacing(const std::string &path) const;

    /// Whether the prefab was placed before, since the last Clear().
    [[nodiscard]] bool WasPlaced(const std::string &path) const;

    /// Forgets every prefab that was read and placed, so that the next
    /// Find() reads the file again: at the start of a scene load.
    void Clear();

    /// Says that the prefab is being placed, from a line of a document,
    /// until EndPlacing().
    void BeginPlacing(const std::string &path, const std::string &document, std::size_t line);

    void EndPlacing();

    /// The loop that placing the prefab again would close, for a message:
    /// `demo.scene.yml:4 places P, P:7 places Q, Q:5 places P again`, where
    /// the document and the line are those that would place it again.
    [[nodiscard]] std::string DescribeLoop(
      const std::string &path,
      const std::string &document,
      std::size_t line) const;
  };
} // neon

#endif //PREFAB_FILES_HPP
