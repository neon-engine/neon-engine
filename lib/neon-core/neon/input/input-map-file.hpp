#ifndef INPUT_MAP_FILE_HPP
#define INPUT_MAP_FILE_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

#include "input-map.hpp"

namespace neon
{
  /// A file of an input map, `assets://input/<name>.input.yml`, which a
  /// project names in its `project.yml` as `input`.
  ///
  ///     version: 1
  ///     actions:
  ///       move:  { type: axis2, keys: [w, s, a, d], stick: left }
  ///       look:  { type: axis2, mouse: motion, stick: right }
  ///       jump:  { type: button, keys: [space], buttons: [south] }
  ///       dash:  { type: button, keys: [[left-shift, space]] }
  ///       shoot: { type: button, mouse: left, buttons: [right-trigger] }
  ///       pause: { type: button, keys: [escape], buttons: [start] }
  ///     states:
  ///       walking: [move, look, jump, shoot, pause]
  ///       menu:    [pause]
  ///
  /// `version` is the version of this layout. A key or a button in a binding
  /// is a name, or a list of names held together, a chord. A name that is
  /// not known is an error, as is a key, a button, or a stick that is not
  /// known, and an
  /// action a state names that there is not, so that a name that was
  /// misspelled does not go unnoticed. Every problem is reported with its
  /// line, not only the first.
  ///
  /// Which format the file has is up to the DocumentFormat that is handed in.
  class InputMapFile final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    std::shared_ptr<Logger> _logger;

  public:
    /// The version of the layout of the file that is read, and the highest
    /// that is understood.
    static constexpr int version = 1;

    /// The map the engine brings, which a project that names none is
    /// played with.
    static constexpr std::string_view default_path = "engine://input/default.input.yml";

    InputMapFile(FileSystemContext *file_system, DocumentFormat *format, const std::shared_ptr<Logger> &logger);

    /// Reads the file at `path` into `map`. Returns false when the file
    /// cannot be read or something in it is wrong, and every problem that
    /// was found is in `errors` with its line, not only the first.
    bool Read(const std::string &path, InputMap &map, std::vector<std::string> &errors) const;
  };
} // neon

#endif //INPUT_MAP_FILE_HPP
