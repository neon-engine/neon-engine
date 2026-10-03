/* A library that is no extension: it exports a function, and not the one an
 * extension is started by. */

#include <neon/extension/neon-extension.h>

NEON_EXTENSION_EXPORT int something_else(void);

NEON_EXTENSION_EXPORT int something_else(void)
{
  return 1;
}
