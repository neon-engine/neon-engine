#ifndef EXTENSION_POSSESSION_LISTENER_HPP
#define EXTENSION_POSSESSION_LISTENER_HPP

#include <neon/extension/neon-extension.h>

namespace neon
{
  /// A function of an extension that is told what was possessed and
  /// released, with what it is handed as its own. See docs/possession.md.
  struct ExtensionPossessionListener
  {
    void (*listen)(void *user, NeonEntity entity, int possessed) = nullptr;
    void *user = nullptr;
  };
} // neon

#endif //EXTENSION_POSSESSION_LISTENER_HPP
