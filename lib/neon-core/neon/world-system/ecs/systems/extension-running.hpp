#ifndef EXTENSION_RUNNING_HPP
#define EXTENSION_RUNNING_HPP

#include <neon/extension/extension-host.hpp>
#include <neon/world-system/ecs/entity-system.hpp>
#include <neon/world-system/ecs/scene-file/component-format.hpp>

namespace neon
{
  /// Lets the extensions of the application take part in the world: they
  /// register their components when every system does, are told when all
  /// are registered, and the systems they added run with the engine's own. See ExtensionHost and docs/extensions.md.
  ///
  /// It is added before the system that runs the scripts, so that a script
  /// finds the components of an extension.
  class ExtensionRunning final : public EntitySystem
  {
    ExtensionHost *_extensions;
    ComponentFormats *_formats;

  public:
    /// `formats` is where the extensions' components are added, so that the
    /// scene reads them.
    ExtensionRunning(ExtensionHost *extensions, ComponentFormats *formats);

    void Register(EntityStore &store) override;

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;

    void FixedUpdate(EntityStore &store, double fixed_delta_time) override;

    void Interpolate(EntityStore &store, double blend) override;
  };
} // neon

#endif //EXTENSION_RUNNING_HPP
