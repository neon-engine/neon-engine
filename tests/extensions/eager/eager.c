/* An extension that does things at the wrong time: it registers a component
 * and reaches the store while it is started, when there is no world, and
 * registers a component in `start`, when it is too late. Each is refused. */

#include <neon/extension/neon-extension.h>

static const NeonExtensionHost *host;

static const NeonComponentDescription marker = {"Marker", 0, 0, 0, 0};

static void nothing(void *user, double delta_time)
{
  (void) user;
  (void) delta_time;
}

static void nobody(void *user, const NeonPhysicsEvent *event)
{
  (void) user;
  (void) event;
}

static void start(void *context)
{
  (void) context;

  if (host->register_component(host->context, &marker) != 0)
  {
    host->log(host->context, NEON_LOG_CRITICAL, "A component was registered in start");
  }

  /* the world runs, and takes no more systems */
  const NeonSystemDescription late = {"Late", 0, &nothing, 0, 0};
  const NeonSystemDescription empty = {"Empty", 0, 0, 0, 0};
  if (host->register_system(host->context, &late) != 0 || host->register_system(host->context, &empty) != 0)
  {
    host->log(host->context, NEON_LOG_CRITICAL, "A system was added in start");
  }

  /* nor anyone who wants to be told what touches */
  if (host->listen_to_physics(host->context, &nobody, 0) != 0 || host->listen_to_physics(host->context, 0, 0) != 0)
  {
    host->log(host->context, NEON_LOG_CRITICAL, "A listener was taken in start");
  }

  /* a ray that points nowhere */
  const NeonRay nowhere = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 10.0f, 0xFFFFFFFFu, 0, 0};
  NeonRayHit hit;
  host->cast_ray(host->context, &nowhere, &hit);

  /* a marker of its own is fine where it belongs, see spinner */
  const NeonEntity entity = host->create_entity(host->context, "made-by-eager", 0);
  host->set_component(host->context, entity, 0, &marker);
  host->get_component(host->context, entity + 1000, 1);
  host->create_query(host->context, 0, 0, NEON_QUERY_ANY);
  host->each(host->context, 0, 0, 0);
}

NEON_EXTENSION_EXPORT int neon_extension_initialize(const NeonExtensionHost *the_host, NeonExtension *extension)
{
  host = the_host;

  extension->abi_version = NEON_EXTENSION_ABI_VERSION;
  extension->start = &start;

  /* a system without a function, and one without a name */
  const NeonSystemDescription empty = {"Empty", 0, 0, 0, 0};
  const NeonSystemDescription nameless = {0, 0, &nothing, 0, 0};
  if (host->register_system(host->context, &empty) != 0 || host->register_system(host->context, &nameless) != 0)
  {
    host->log(host->context, NEON_LOG_CRITICAL, "A system that is none was added");
  }

  if (host->register_component(host->context, &marker) != 0 || host->create_entity(host->context, "too-early", 0) != 0)
  {
    host->log(host->context, NEON_LOG_CRITICAL, "The store was reached before there was a world");
  }
  return 1;
}
