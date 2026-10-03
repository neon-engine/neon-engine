/* An extension whose structs are not what it describes: one holds more than
 * its fields, one has them in another order, one is of a kind nobody knows,
 * and one takes a name that is taken. None is registered. */

#include <stddef.h>

#include <neon/extension/neon-extension.h>

typedef struct Padded
{
  float speed;
  void *something_of_its_own;
} Padded;

typedef struct Swapped
{
  double seconds;
  float speed;
} Swapped;

static const NeonExtensionHost *host;

static void register_components(void *context)
{
  (void) context;

  const NeonFieldDescription padded_fields[] = {
    {"speed", NEON_FIELD_FLOAT, offsetof(Padded, speed), {0.0}, 0},
  };
  const NeonComponentDescription padded = {"Padded", 0, sizeof(Padded), padded_fields, 1};

  /* described in the other order than they lie */
  const NeonFieldDescription swapped_fields[] = {
    {"speed", NEON_FIELD_FLOAT, offsetof(Swapped, speed), {0.0}, 0},
    {"seconds", NEON_FIELD_DOUBLE, offsetof(Swapped, seconds), {0.0}, 0},
  };
  const NeonComponentDescription swapped = {"Swapped", 0, sizeof(Swapped), swapped_fields, 2};

  const NeonFieldDescription unknown_fields[] = {
    {"what", (NeonFieldKind) 999, 0, {0.0}, 0},
  };
  const NeonComponentDescription unknown = {"Unknown", 0, 4, unknown_fields, 1};

  /* a component of the engine */
  const NeonComponentDescription taken = {"Transform", 0, 0, 0, 0};

  const NeonComponentDescription nameless = {0, 0, 0, 0, 0};

  if (host->register_component(host->context, &padded) != 0 ||
      host->register_component(host->context, &swapped) != 0 ||
      host->register_component(host->context, &unknown) != 0 ||
      host->register_component(host->context, &taken) != 0 ||
      host->register_component(host->context, &nameless) != 0)
  {
    host->log(host->context, NEON_LOG_CRITICAL, "A component that is not what it describes was registered");
  }
}

NEON_EXTENSION_EXPORT int neon_extension_initialize(const NeonExtensionHost *the_host, NeonExtension *extension)
{
  host = the_host;

  extension->abi_version = NEON_EXTENSION_ABI_VERSION;
  extension->register_components = &register_components;
  return 1;
}
