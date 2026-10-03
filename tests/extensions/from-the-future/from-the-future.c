/* An extension that was built with a later version of the interface than
 * the application has, and does not look at what the application has. */

#include <neon/extension/neon-extension.h>

NEON_EXTENSION_EXPORT int neon_extension_initialize(const NeonExtensionHost *host, NeonExtension *extension)
{
  (void) host;

  extension->abi_version = 99;
  return 1;
}
