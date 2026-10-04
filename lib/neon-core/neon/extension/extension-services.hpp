#ifndef EXTENSION_SERVICES_HPP
#define EXTENSION_SERVICES_HPP

#include <neon/audio/audio-context.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/input/input-context.hpp>
#include <neon/physics/physics-context.hpp>
#include <neon/render/render-context.hpp>
#include <neon/ui/ui-context.hpp>
#include <neon/window/window-context.hpp>
#include <neon/world-system/ecs/scene-file/component-format.hpp>
#include <neon/world-system/world-system.hpp>

namespace neon
{
  /// What of the engine the extensions reach besides the store, each
  /// through its interface. One that is nullptr is not there for them, and
  /// a call that needs it says so in the log.
  struct ExtensionServices
  {
    FileSystemContext *file_system = nullptr;

    /// What the game reads of the input, which is what the user interface
    /// left of it.
    InputContext *input = nullptr;

    /// For placing prefabs and asking for another scene.
    WorldSystem *world = nullptr;

    /// For what touched what, and for rays.
    PhysicsContext *physics = nullptr;

    /// For the pictures an extension makes, which materials read, and for
    /// the size of what is drawn to.
    RenderContext *render = nullptr;

    /// For the sounds an extension hands over in memory, which sound
    /// sources play.
    AudioContext *audio = nullptr;

    /// For the values the files of the user interface show, and for its
    /// elements.
    UiContext *ui = nullptr;

    /// For asking the application to close, as its own pause menu does.
    WindowContext *window = nullptr;

    /// Every kind of component a recipe reads, with its description, which
    /// is how a field is found by its name. Known once the world comes up.
    const ComponentFormats *formats = nullptr;
  };
} // neon

#endif //EXTENSION_SERVICES_HPP
