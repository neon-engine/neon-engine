/* An extension that cannot start, and says why. */

#include <neon/extension/neon-extension.h>

NEON_EXTENSION_EXPORT int neon_extension_initialize(const NeonExtensionHost *host, NeonExtension *extension)
{
  extension->abi_version = NEON_EXTENSION_ABI_VERSION;

  host->log(host->context, NEON_LOG_ERROR, "The data of the game is not here");
  return 0;
}
