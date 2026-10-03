/* The least an extension can be, in C: it starts, and says so. */

#include <neon/extension/neon-extension.h>

NEON_EXTENSION_EXPORT int neon_extension_initialize(const NeonExtensionHost *host, NeonExtension *extension)
{
  extension->abi_version = NEON_EXTENSION_ABI_VERSION;

  host->log(host->context, NEON_LOG_INFO, "Hello from an extension in C");
  return 1;
}
