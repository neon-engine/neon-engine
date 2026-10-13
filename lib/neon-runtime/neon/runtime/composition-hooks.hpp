#ifndef COMPOSITION_HOOKS_HPP
#define COMPOSITION_HOOKS_HPP

#include <string>

#include <neon/command-line/command-line.hpp>
#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system.hpp>
#include <neon/logging/logger.hpp>
#include <neon/logging/logging-context.hpp>
#include <neon/project/project.hpp>
#include <neon/runtime/runtime.hpp>
#include <neon/runtime/settings-config.hpp>

#include "runtime-systems.hpp"

namespace neon
{
  /// Where an application differs from the start-up every application
  /// shares, see RuntimeComposition. Each hook is called once, at the step
  /// of the start-up it is named for, in the order they are declared here.
  /// What is not overridden does what the runtime does: NeonRuntime
  /// overrides two of them, NeonEditor every one. The object is owned by the
  /// application's main() with its real type, and holds what the
  /// application makes for itself, as the Runtime it runs.
  class CompositionHooks
  {
  protected:
    ~CompositionHooks() = default;

  public:
    /// Registers the sets of options the application owns next to the
    /// runtime's, and the argument it takes, before the command line is
    /// parsed. The runtime's sets are registered already.
    virtual void RegisterOptions(CommandLine &) {}

    /// Applies the sets registered above to the settings, after the
    /// runtime's. Called twice, as the runtime's are: before the files are
    /// read, and again after them, so that the command line wins. False
    /// with the reason refuses the command line.
    virtual bool ApplyOptions(const CommandLineContext &, SettingsConfig &, std::string &) { return true; }

    /// Chooses the folder of the project once the command line is parsed:
    /// `settings.project_directory`, which the file system opens as
    /// `assets://` and `extensions://`. Left empty, the project is the one
    /// next to the executable. False with the reason stops the start-up
    /// with the help text.
    virtual bool ChooseProjectFolder(const CommandLine &, SettingsConfig &, std::string &) { return true; }

    /// What to say, after the errors, when there is neither a game nor a
    /// project where the application looked for one: what to start instead.
    virtual std::string AdviseWithoutAProject(const SettingsConfig &) const = 0;

    /// Places a folder of the application's own, once `user://` is placed
    /// by the project's names and the log has its file: the editor's
    /// `editor://`, which holds its layout and its settings, read here.
    /// The runtime has none. `format` reads the files of the engine, so
    /// that what is read here is read as every recipe is. False stops the
    /// start-up, once the reason is in the log.
    virtual bool PlaceApplicationFolder(FileSystem &, DocumentFormat &, LoggingContext &) { return true; }

    /// The application's last word on the settings, after every layer is
    /// applied and before the window and the systems are made: the window
    /// the editor insists on, or the title it gives it. The title is the
    /// project's name unless it is set here or by the settings.
    virtual void AdjustSettings(SettingsConfig &, const Project &, const CommandLineContext &, Logger &) {}

    /// Makes the Runtime the application runs, NeonRuntime or Editor, on
    /// the systems it is given, and keeps it. The composition cleans it up
    /// before the systems it stands on.
    virtual Runtime &MakeRuntime(const RuntimeSystems &systems) = 0;

    /// Adds what the application puts on the world after the engine's
    /// systems and the Runtime are made, and before anything is
    /// initialized: the editor's tools, its camera, how the world treats
    /// the pointer. False stops the start-up with exit code 1, once the
    /// reason is in the log: a script of the editor that cannot be read.
    virtual bool AddSystems(const RuntimeSystems &) { return true; }
  };
} // neon

#endif //COMPOSITION_HOOKS_HPP
