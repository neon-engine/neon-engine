#ifndef EXTENSION_HOST_HPP
#define EXTENSION_HOST_HPP

#include <memory>
#include <string>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logging-context.hpp>

#include "library-loader.hpp"
#include "loaded-extension.hpp"

namespace neon
{
  /// Finds the extensions of the application and starts them.
  ///
  /// An extension is a folder of `extensions://` with a recipe,
  /// `extension.yml`, see ExtensionFile. Nothing lists them: every folder
  /// that is there is an extension, and an application without the folder
  /// has none, which is not an error.
  ///
  /// A native extension brings a library for the platform. The library is
  /// opened, and its `neon_extension_initialize` is handed what the
  /// application offers, see neon-extension.h. That file is the whole of
  /// what the two see of each other.
  ///
  /// The extensions take part in the world through the system
  /// ExtensionRunning, which calls RegisterComponents() and Start(), and runs the
  /// systems the extensions added.
  ///
  /// An extension with a problem is reported as an error and left out; the
  /// rest are started. A game that needs the extension then fails where it
  /// uses what the extension would have brought.
  class ExtensionHost final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    LibraryLoader *_library_loader;
    LoggingContext *_logging;
    std::shared_ptr<Logger> _logger;

    // what the extensions reach besides the store
    ExtensionServices _services;

    // in the order they were started, which is the order of their names
    std::vector<std::unique_ptr<LoadedExtension>> _extensions;

    // the extensions that are there to be used, those that were started and
    // those that bring no library, in the order of their names
    std::vector<std::string> _present;

    /// Reads the recipe in a folder, and starts the library it names for
    /// this platform.
    void Load(const std::string &folder);

  public:
    ExtensionHost(
      FileSystemContext *file_system,
      DocumentFormat *format,
      LibraryLoader *library_loader,
      LoggingContext *logging,
      const std::shared_ptr<Logger> &logger);

    ~ExtensionHost();

    /// Starts every extension under `extensions://`, in the order of their
    /// names.
    void Initialize();

    /// Gives the extensions the input the game reads. Without it, a call
    /// that asks for an action says that there is none.
    void SetInput(InputContext *input);

    /// Gives the extensions the physics, for what touched what and for
    /// rays.
    void SetPhysics(PhysicsContext *physics);

    /// Gives the extensions the renderer, for the pictures they make.
    void SetRender(RenderContext *render);

    /// Gives the extensions the audio, for the sounds they hand over in
    /// memory.
    void SetAudio(AudioContext *audio);

    /// Gives the extensions the user interface, for the values its files
    /// show.
    void SetUi(UiContext *ui);

    /// Gives the extensions the window, which is what one that asks the
    /// application to close tells, as the pause menu does.
    void SetWindow(WindowContext *window);

    /// Gives the extensions the world, for placing prefabs and asking for
    /// another scene.
    void SetWorld(WorldSystem *world);

    /// Lets every extension register the components it brings, in the order
    /// the extensions were started, with the store and so that recipes read
    /// them. Called when the world comes up, before any system is
    /// initialized. From here on the extensions reach the store.
    void RegisterComponents(EntityStore &store, ComponentFormats &formats);

    /// Tells every extension that every component is registered, which is
    /// when it finds components and creates its queries.
    void Start(EntityStore &store);

    /// Tells the extensions what the physics reported in the frame, and
    /// then runs the `update` of every system they added. Once per frame.
    void Update(double delta_time) const;

    /// Runs the `fixed_update` of every system, once per step of the world.
    void FixedUpdate(double fixed_delta_time) const;

    /// Runs the `interpolate` of every system, once per frame before it is
    /// drawn.
    void Interpolate(double blend) const;

    /// Takes the store from the extensions, which reach it no more. Called
    /// before the store is cleaned up for good.
    void LeaveWorld();

    /// Tells every extension to clean up, the last that was started first,
    /// and closes its library. Safe to call more than once.
    void CleanUp();

    /// The folders the extensions keep their own assets in,
    /// `extensions://<name>/assets/`, in the order of their names: of those
    /// that were started and of those that bring no library, whether the
    /// folder is there or not. It is where the scripts of an extension are
    /// looked for.
    [[nodiscard]] std::vector<std::string> GetAssetFolders() const;

    /// The names of the extensions that were started, in the order they
    /// were.
    [[nodiscard]] std::vector<std::string> GetLoaded() const;
  };
} // neon

#endif //EXTENSION_HOST_HPP
