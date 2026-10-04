#ifndef LOADED_EXTENSION_HPP
#define LOADED_EXTENSION_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <neon/extension/neon-extension.h>
#include <neon/logging/logger.hpp>
#include <neon/world-system/ecs/entity-store.hpp>
#include <neon/world-system/ecs/scene-file/component-format.hpp>

#include "extension-field.hpp"
#include "extension-physics-listener.hpp"
#include "extension-services.hpp"
#include "native-library.hpp"

namespace neon
{
  /// An extension whose library is open, as the ExtensionHost keeps it.
  struct LoadedExtension
  {
    std::string name;

    /// What the extension logs under.
    std::shared_ptr<Logger> logger;

    std::unique_ptr<NativeLibrary> library;

    /// What the extension was handed. Its `context` is this object, which
    /// therefore stays where it is while the library is open.
    NeonExtensionHost host{};

    /// What the extension filled in.
    NeonExtension table{};

    /// The systems the extension added, in the order it did.
    std::vector<NeonSystemDescription> systems;

    /// What of the extension is told what began and ended to touch, in the
    /// order it asked to be.
    std::vector<ExtensionPhysicsListener> physics_listeners;

    /// The files of the user interface the extension shows, by their
    /// paths, as what the user interface knows each as.
    std::map<std::string, int> ui_documents;

    /// What of the extension listens to elements of the user interface, as
    /// the user interface knows each. They are taken away before the
    /// library is closed, so that nothing calls into it afterwards.
    std::vector<int> ui_listeners;

    /// Whether `start` was reached, after which no system is added: the
    /// world runs.
    bool started = false;

    /// The fields the extension found, which it names by their place here
    /// plus one.
    std::vector<ExtensionField> fields;

    /// What of the engine the extension reaches besides the store. It is
    /// the host's, and the same for every extension.
    const ExtensionServices *services = nullptr;

    /// The store of the world while it is up, which is when the extension
    /// may reach it, and nullptr otherwise.
    EntityStore *store = nullptr;

    /// Where the components the extension registers are added, so that
    /// recipes read them. Only while `register_components` of the extension
    /// is called, which is the one time it may register any.
    ComponentFormats *formats = nullptr;
  };
} // neon

#endif //LOADED_EXTENSION_HPP
