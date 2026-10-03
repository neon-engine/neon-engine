/* An extension that says it was built with version 1 of the interface, from
 * before there were components. What lies in its table past what version 1
 * holds is not the application's to read, and is not called. */

#include <neon/extension/neon-extension.h>

static const NeonExtensionHost *host;

static void must_not_be_called(void *context)
{
  (void) context;
  host->log(host->context, NEON_LOG_ERROR, "A function past version 1 was called");
}

NEON_EXTENSION_EXPORT int neon_extension_initialize(const NeonExtensionHost *the_host, NeonExtension *extension)
{
  host = the_host;

  extension->abi_version = 1;
  extension->register_components = &must_not_be_called;
  extension->start = &must_not_be_called;

  host->log(host->context, NEON_LOG_INFO, "Hello from an extension of version 1");
  return 1;
}
