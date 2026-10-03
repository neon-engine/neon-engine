#ifndef EXTENSION_PHYSICS_LISTENER_HPP
#define EXTENSION_PHYSICS_LISTENER_HPP

#include <neon/extension/neon-extension.h>

namespace neon
{
  /// A function of an extension that is told what began and ended to touch,
  /// with what it is handed as its own.
  struct ExtensionPhysicsListener
  {
    void (*listen)(void *user, const NeonPhysicsEvent *event) = nullptr;
    void *user = nullptr;
  };
} // neon

#endif //EXTENSION_PHYSICS_LISTENER_HPP
