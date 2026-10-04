#include "extension-host.hpp"

#include <algorithm>
#include <cstdint>

#include <neon/filesystem/file-system.hpp>

#include "extension-file.hpp"
#include "extension-host-table.hpp"

namespace neon
{
  ExtensionHost::ExtensionHost(
    FileSystemContext *file_system,
    DocumentFormat *format,
    LibraryLoader *library_loader,
    LoggingContext *logging,
    const std::shared_ptr<Logger> &logger)
  {
    _file_system = file_system;
    _format = format;
    _library_loader = library_loader;
    _logging = logging;
    _logger = logger;
    _services.file_system = file_system;
  }

  void ExtensionHost::SetInput(InputContext *input)
  {
    _services.input = input;
  }

  void ExtensionHost::SetPhysics(PhysicsContext *physics)
  {
    _services.physics = physics;
  }

  void ExtensionHost::SetRender(RenderContext *render)
  {
    _services.render = render;
  }

  void ExtensionHost::SetAudio(AudioContext *audio)
  {
    _services.audio = audio;
  }

  void ExtensionHost::SetUi(UiContext *ui)
  {
    _services.ui = ui;
  }

  void ExtensionHost::SetWindow(WindowContext *window)
  {
    _services.window = window;
  }

  void ExtensionHost::SetWorld(WorldSystem *world)
  {
    _services.world = world;
  }

  ExtensionHost::~ExtensionHost()
  {
    CleanUp();
  }

  void ExtensionHost::Initialize()
  {
    const std::string scheme(FileSystem::extensions_scheme);

    // a folder that is not there cannot be listed, and an application
    // without it has no extensions
    std::vector<std::string> paths;
    if (!_file_system->ListFiles(scheme, paths) || paths.empty())
    {
      _logger->Info("No extensions: nothing is in {}", scheme);
      return;
    }

    // The folders at the top, each once. The paths come in the order of
    // their names with the files at the top first, which are no extensions.
    std::vector<std::string> folders;
    for (const auto &path : paths)
    {
      const std::string relative = path.substr(scheme.size());
      const std::size_t slash = relative.find('/');
      if (slash == std::string::npos)
      {
        _logger->Warn("{} is not in the folder of an extension, and is left alone", path);
        continue;
      }

      const std::string folder = relative.substr(0, slash);
      if (std::ranges::find(folders, folder) == folders.end()) { folders.push_back(folder); }
    }
    std::ranges::sort(folders);

    for (const auto &folder : folders)
    {
      if (std::ranges::find(paths, ExtensionFile::PathOf(folder)) == paths.end())
      {
        const std::string file_name(ExtensionFile::file_name);
        _logger->Error(
          "{}{} has no {}, which says what the extension is. It is left out",
          scheme,
          folder,
          file_name);
        continue;
      }

      Load(folder);
    }

    const std::size_t started = _extensions.size();
    const std::size_t found = folders.size();
    _logger->Info("Started {} of {} extensions", started, found);
  }

  void ExtensionHost::Load(const std::string &folder)
  {
    ExtensionRecipe recipe;
    const ExtensionFile file(_file_system, _format);
    if (std::vector<std::string> errors; !file.Read(folder, recipe, errors))
    {
      for (const auto &error : errors) { _logger->Error("{}", error); }
      _logger->Error("The extension '{}' is left out until its recipe is corrected", folder);
      return;
    }

    // what brings no library has nothing to start. Its files are reached
    // under extensions:// all the same
    if (recipe.libraries.empty())
    {
      _logger->Info("The extension '{}' brings no library", folder);
      _present.push_back(folder);
      return;
    }

    const std::string platform = _library_loader->GetPlatform();
    const auto library = recipe.libraries.find(platform);
    if (library == recipe.libraries.end())
    {
      _logger->Error(
        "The extension '{}' has no library for this platform, {}. It is left out",
        folder,
        platform);
      return;
    }

    const std::string path = std::string(FileSystem::extensions_scheme) + folder + "/" + library->second;

    auto extension = std::make_unique<LoadedExtension>();
    extension->name = recipe.name;
    extension->logger = _logging->CreateLogger("extension:" + recipe.name);
    extension->services = &_services;

    std::string error;
    extension->library = _library_loader->Open(path, error);
    if (extension->library == nullptr)
    {
      _logger->Error("The library {} of the extension '{}' cannot be opened: {}", path, folder, error);
      return;
    }

    const std::string entry_name = NEON_EXTENSION_INITIALIZE_NAME;
    // a library hands out its functions as pointers to data, which is what
    // the platforms do, and the standard allows the cast where it works
    const auto initialize = reinterpret_cast<NeonExtensionInitialize>(extension->library->FindFunction(entry_name));
    if (initialize == nullptr)
    {
      _logger->Error(
        "The library {} exports no function '{}', so it is no extension. It is left out",
        path,
        entry_name);
      return;
    }

    FillExtensionHostTable(*extension);

    if (initialize(&extension->host, &extension->table) == 0)
    {
      _logger->Error("The extension '{}' did not start. It is left out", folder);
      return;
    }

    // What was built with a later version may have filled in what this
    // application does not know to be there, so nothing of it is called,
    // not even its clean_up.
    const std::uint32_t built_with = extension->table.abi_version;
    if (built_with == 0 || built_with > NEON_EXTENSION_ABI_VERSION)
    {
      const std::uint32_t ours = NEON_EXTENSION_ABI_VERSION;
      _logger->Error(
        "The extension '{}' was built with version {} of the interface of extensions, and this application has "
        "version {}. It is left out",
        folder,
        built_with,
        ours);
      return;
    }

    _logger->Info("Started the extension '{}' from {}", folder, path);
    _present.push_back(folder);
    _extensions.push_back(std::move(extension));
  }

  void ExtensionHost::RegisterComponents(EntityStore &store, ComponentFormats &formats)
  {
    _services.formats = &formats;

    for (const auto &extension : _extensions)
    {
      extension->store = &store;

      // what was built before there were components has none to register,
      // and its table is read no further than its version holds
      if (extension->table.abi_version < 2 || extension->table.register_components == nullptr) { continue; }

      extension->formats = &formats;
      extension->table.register_components(extension->table.context);
      extension->formats = nullptr;
    }
  }

  void ExtensionHost::Start(EntityStore &store)
  {
    for (const auto &extension : _extensions)
    {
      extension->store = &store;

      // from here on the world runs, and takes no more systems
      extension->started = true;

      if (extension->table.abi_version < 2 || extension->table.start == nullptr) { continue; }

      extension->table.start(extension->table.context);
    }
  }

  void ExtensionHost::Update(const double delta_time) const
  {
    // what touched what reaches the extensions before their update, so
    // that it sees the frame it happened in, as with the scripts
    if (_services.physics != nullptr)
    {
      for (const auto &event : _services.physics->GetFrameEvents())
      {
        NeonPhysicsEvent told{};
        told.began = event.kind == PhysicsEventKind::Began ? 1 : 0;
        told.trigger = event.trigger ? 1 : 0;
        told.first = event.first;
        told.second = event.second;
        told.point = {event.point.x, event.point.y, event.point.z};
        told.normal = {event.normal.x, event.normal.y, event.normal.z};

        for (const auto &extension : _extensions)
        {
          for (const auto &listener : extension->physics_listeners) { listener.listen(listener.user, &told); }
        }
      }
    }

    for (const auto &extension : _extensions)
    {
      for (const auto &system : extension->systems)
      {
        if (system.update != nullptr) { system.update(system.user, delta_time); }
      }
    }
  }

  void ExtensionHost::FixedUpdate(const double fixed_delta_time) const
  {
    for (const auto &extension : _extensions)
    {
      for (const auto &system : extension->systems)
      {
        if (system.fixed_update != nullptr) { system.fixed_update(system.user, fixed_delta_time); }
      }
    }
  }

  void ExtensionHost::Interpolate(const double blend) const
  {
    for (const auto &extension : _extensions)
    {
      for (const auto &system : extension->systems)
      {
        if (system.interpolate != nullptr) { system.interpolate(system.user, blend); }
      }
    }
  }

  void ExtensionHost::LeaveWorld()
  {
    for (const auto &extension : _extensions) { extension->store = nullptr; }
  }

  void ExtensionHost::CleanUp()
  {
    // the world is gone, or was never there
    LeaveWorld();

    while (!_extensions.empty())
    {
      const auto &extension = _extensions.back();
      _logger->Info("Cleaning up the extension '{}'", extension->name);

      // What the extension listens to the user interface with is taken away
      // first: the user interface outlives the library, and would call into
      // what is closed.
      if (_services.ui != nullptr)
      {
        for (const int listening : extension->ui_listeners) { _services.ui->Off(listening); }
      }
      extension->ui_listeners.clear();

      if (extension->table.clean_up != nullptr) { extension->table.clean_up(extension->table.context); }
      extension->library->Close();

      _extensions.pop_back();
    }

    _present.clear();
  }

  std::vector<std::string> ExtensionHost::GetAssetFolders() const
  {
    std::vector<std::string> folders;
    for (const auto &name : _present)
    {
      folders.push_back(std::string(FileSystem::extensions_scheme) + name + "/assets/");
    }
    return folders;
  }

  std::vector<std::string> ExtensionHost::GetLoaded() const
  {
    std::vector<std::string> names;
    for (const auto &extension : _extensions) { names.push_back(extension->name); }
    return names;
  }
} // neon
