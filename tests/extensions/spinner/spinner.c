/* An extension that brings a component, in C: it registers it, and once the
 * world is up it gives it to an entity, finds the entity again with a
 * query, and changes the component in place. */

#include <stddef.h>
#include <stdio.h>

#include <neon/extension/neon-extension.h>

/* The component, as it lies in memory. Its fields are described below in
 * the same order. */
typedef struct Spinner
{
  float speed;
  NeonVector3 axis;
  int32_t turns;
  bool enabled;
} Spinner;

static const NeonFieldDescription spinner_fields[] = {
  {"speed", NEON_FIELD_FLOAT, offsetof(Spinner, speed), {90.0}, "How far it turns in a second, in degrees"},
  {"axis", NEON_FIELD_VECTOR3, offsetof(Spinner, axis), {0.0, 1.0, 0.0}, "What it turns around"},
  {"turns", NEON_FIELD_INTEGER, offsetof(Spinner, turns), {0.0}, 0},
  {"enabled", NEON_FIELD_BOOLEAN, offsetof(Spinner, enabled), {1.0}, 0},
};

static const NeonExtensionHost *host;
static NeonComponent spinner;
static NeonQuery spinners;

static void register_components(void *context)
{
  (void) context;

  const NeonComponentDescription description = {
    "Spinner",
    "Turns what carries it",
    sizeof(Spinner),
    spinner_fields,
    sizeof(spinner_fields) / sizeof(spinner_fields[0]),
  };
  spinner = host->register_component(host->context, &description);
}

/* Counts the entities of a block, and turns each once more. */
static void visit(void *user, const NeonEntityBlock *block)
{
  uint64_t *count = user;
  Spinner *components = block->columns[0];

  for (uint64_t i = 0; i < block->count; i++) { components[i].turns += 1; }
  *count += block->count;
}

static void start(void *context)
{
  (void) context;

  /* found by its name as well, as another extension would */
  if (host->find_component(host->context, "Spinner") != spinner)
  {
    host->log(host->context, NEON_LOG_ERROR, "Spinner is not found by its name");
    return;
  }

  const NeonEntity top = host->create_entity(host->context, "made-by-spinner", 0);
  const NeonEntity child = host->create_entity(host->context, "wheel", top);

  const Spinner fast = {360.0f, {1.0f, 0.0f, 0.0f}, 3, true};
  host->set_component(host->context, child, spinner, &fast);

  spinners = host->create_query(host->context, &spinner, 1, NEON_QUERY_ANY);

  uint64_t count = 0;
  host->each(host->context, spinners, &visit, &count);

  const Spinner *kept = host->get_component(host->context, child, spinner);

  char message[160];
  snprintf(
    message,
    sizeof(message),
    "%d entity spins, at %.0f degrees, after %d turns, under its parent: %d, without a Spinner at the top: %d",
    (int) count,
    kept->speed,
    kept->turns,
    host->get_parent(host->context, child) == top && host->find_entity(host->context, "made-by-spinner/wheel") == child,
    !host->has_component(host->context, top, spinner));
  host->log(host->context, NEON_LOG_INFO, message);
}

/* A system in C: turns every spinner once more in each frame. */
static void update(void *user, double delta_time)
{
  (void) delta_time;

  uint64_t count = 0;
  host->each(host->context, spinners, &visit, &count);
  *(uint64_t *) user += count;
}

/* how many spinners the system has turned, which it is handed as its own */
static uint64_t turned;

NEON_EXTENSION_EXPORT int neon_extension_initialize(const NeonExtensionHost *the_host, NeonExtension *extension)
{
  if (the_host->abi_version < 3)
  {
    the_host->log(the_host->context, NEON_LOG_ERROR, "This application has no systems for extensions yet");
    return 0;
  }

  host = the_host;

  extension->abi_version = NEON_EXTENSION_ABI_VERSION;
  extension->register_components = &register_components;
  extension->start = &start;

  const NeonSystemDescription turning = {"Turning", &turned, &update, 0, 0};
  return host->register_system(host->context, &turning);
}
