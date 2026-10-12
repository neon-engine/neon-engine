#ifndef INPUT_MAP_LOADING_HPP
#define INPUT_MAP_LOADING_HPP

#include <string>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/input/input-system.hpp>
#include <neon/logging/logging-context.hpp>

namespace neon
{
  /// The step of the start-up that gives the input system the input map of
  /// the project, see RuntimeComposition and input.md: the actions the game
  /// reads, and what is bound to each. A project that names none is played
  /// with the map the engine brings, `engine://input/default.input.yml`,
  /// and only without that file, which ships with the application, with
  /// the one built in. A mistake in a map stops the application, since the
  /// game could not be played as meant.
  class InputMapLoading final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    LoggingContext *_logging;
    std::string _in_prose;

  public:
    /// `in_prose` is how the remarks name the application: "the runtime".
    InputMapLoading(
      FileSystemContext *file_system,
      DocumentFormat *format,
      LoggingContext *logging,
      const std::string &in_prose);

    /// Reads the map `project_input` names, or the engine's without one,
    /// and hands it to `input_system`. `remarks` is what to say on the
    /// console without stopping: that the map built in is used. False with
    /// the errors when the map cannot be read.
    bool Load(
      const std::string &project_input,
      InputSystem &input_system,
      std::vector<std::string> &remarks,
      std::vector<std::string> &errors) const;
  };
} // neon

#endif //INPUT_MAP_LOADING_HPP
