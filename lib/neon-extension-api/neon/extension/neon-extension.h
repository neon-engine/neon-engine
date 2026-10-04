#ifndef NEON_EXTENSION_H
#define NEON_EXTENSION_H

/*
 * What a native extension and the application that loads it see of each
 * other. See docs/extensions.md.
 *
 * This is the whole of the boundary, and it is C: plain types, structs, and
 * pointers to functions. Nothing of C++ crosses it, no class, no exception,
 * and no type of the standard library, so that an extension need not be
 * built with the compiler, the standard library, or the settings the
 * application was built with. An extension is written in C, in C++, or in
 * anything else that can export a C function.
 *
 * The application hands the extension a table of what it offers,
 * NeonExtensionHost, and the extension fills in a table of what it brings,
 * NeonExtension. Both begin with the version of this file they were built
 * with.
 *
 * How the tables change: a new version adds fields at the end of a table
 * and never moves, changes, or removes one. An extension built with an
 * older version then finds everything it knows where it was. Whoever reads
 * a table looks at its version first and touches nothing past what that
 * version holds.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The version of this file. It goes up by one whenever a table grows. */
#define NEON_EXTENSION_ABI_VERSION 7

/* Marks the function an extension exports. Everything else of an extension
 * stays hidden, which neon_add_extension sees to. */
#if defined(_WIN32)
#define NEON_EXTENSION_EXPORT __declspec(dllexport)
#else
#define NEON_EXTENSION_EXPORT __attribute__((visibility("default")))
#endif

/* How much a message matters, as the loggers of the engine have it. */
typedef enum NeonLogLevel
{
  NEON_LOG_TRACE = 0,
  NEON_LOG_DEBUG = 1,
  NEON_LOG_INFO = 2,
  NEON_LOG_WARN = 3,
  NEON_LOG_ERROR = 4,
  NEON_LOG_CRITICAL = 5
} NeonLogLevel;

/* Names one thing in the world. 0 stands for none. */
typedef uint64_t NeonEntity;

/* Names a kind of component, as returned when it was registered. 0 stands
 * for none. */
typedef uint64_t NeonComponent;

/* Names a query, as returned when it was created. 0 stands for none. */
typedef uint64_t NeonQuery;

/* Names a field of a kind of component, as returned when it was found.
 * 0 stands for none. */
typedef uint64_t NeonField;

/* What the fields of a component hold, as they lie in memory. A component
 * of an extension is a struct of these and nothing else, in the order its
 * fields are described. */
typedef struct NeonVector2 { float x, y; } NeonVector2;
typedef struct NeonVector3 { float x, y, z; } NeonVector3;
typedef struct NeonVector4 { float x, y, z, w; } NeonVector4;
typedef struct NeonIntegerVector2 { int32_t x, y; } NeonIntegerVector2;
typedef struct NeonIntegerVector3 { int32_t x, y, z; } NeonIntegerVector3;
typedef struct NeonColor { float r, g, b, a; } NeonColor;
typedef struct NeonQuaternion { float x, y, z, w; } NeonQuaternion;

/* The kind of a field, and the type it has in the struct. Kinds are added
 * at the end. Text and lists are not among them yet. */
typedef enum NeonFieldKind
{
  NEON_FIELD_BOOLEAN = 0,          /* bool of C and C++, one byte */
  NEON_FIELD_INTEGER = 1,          /* int32_t */
  NEON_FIELD_FLOAT = 2,            /* float */
  NEON_FIELD_DOUBLE = 3,           /* double */
  NEON_FIELD_VECTOR2 = 4,          /* NeonVector2 */
  NEON_FIELD_VECTOR3 = 5,          /* NeonVector3 */
  NEON_FIELD_VECTOR4 = 6,          /* NeonVector4 */
  NEON_FIELD_COLOR = 7,            /* NeonColor */
  NEON_FIELD_QUATERNION = 8,       /* NeonQuaternion */
  NEON_FIELD_BYTE = 9,             /* uint8_t */
  NEON_FIELD_SHORT = 10,           /* int16_t */
  NEON_FIELD_UNSIGNED_SHORT = 11,  /* uint16_t */
  NEON_FIELD_UNSIGNED_INTEGER = 12,/* uint32_t */
  NEON_FIELD_LONG = 13,            /* int64_t */
  NEON_FIELD_UNSIGNED_LONG = 14,   /* uint64_t */
  NEON_FIELD_INTEGER_VECTOR2 = 15, /* NeonIntegerVector2 */
  NEON_FIELD_INTEGER_VECTOR3 = 16  /* NeonIntegerVector3 */
} NeonFieldKind;

/* One field of a component. */
typedef struct NeonFieldDescription
{
  /* What the field is called in a recipe and in a script: letters, digits,
   * and underscores, not starting with a digit. */
  const char *name;

  NeonFieldKind kind;

  /* Where the field lies in the struct, as offsetof says. It is checked
   * against where the application puts it, so that the two cannot differ
   * unnoticed. */
  uint64_t offset;

  /* What the field starts with, which is also what a recipe leaves out.
   * As many numbers as the kind holds, in the order of its members; a
   * boolean is 0 or 1. */
  double standard[4];

  /* What the field is for, in a sentence, for the editor. May be zero. */
  const char *description;
} NeonFieldDescription;

/* A kind of component an extension brings. */
typedef struct NeonComponentDescription
{
  /* Unique among components, such as `Spinner`. It is what a recipe and a
   * script call the component. */
  const char *name;

  /* What the component is for, in a sentence, for the editor. May be
   * zero. */
  const char *description;

  /* sizeof the struct, which is checked like the offsets. */
  uint64_t size;

  /* The fields, in the order they lie in the struct. The struct holds
   * nothing else. */
  const NeonFieldDescription *fields;
  uint64_t field_count;
} NeonComponentDescription;

/* The order a query hands its entities over in. */
typedef enum NeonQueryOrder
{
  NEON_QUERY_ANY = 0,
  /* an entity comes after its parent */
  NEON_QUERY_PARENTS_FIRST = 1
} NeonQueryOrder;

/* The most components one query can ask for. */
#define NEON_QUERY_MAX_COMPONENTS 8

/* A run of entities that matched a query, with their components. The
 * pointers are valid during the call only. */
typedef struct NeonEntityBlock
{
  uint64_t count;
  const NeonEntity *entities;

  /* see EntityBlock::parent of the engine. Only for
   * NEON_QUERY_PARENTS_FIRST. */
  NeonEntity parent;

  /* One array for each component of the query, in the order the query
   * named them, each of `count` components. */
  void *columns[NEON_QUERY_MAX_COMPONENTS];
} NeonEntityBlock;

/* Behaviour an extension brings to the world. A function that is left zero
 * is not called. `user` is the extension's own and is handed to each as it
 * is. See EntitySystem of the engine for what belongs where. */
typedef struct NeonSystemDescription
{
  /* What the system is called, for messages. */
  const char *name;

  void *user;

  /* Once per frame, with the time the frame took, in seconds. */
  void (*update)(void *user, double delta_time);

  /* Once per step of the world, with the length of a step, which is
   * always the same. For what the game is decided by. */
  void (*fixed_update)(void *user, double fixed_delta_time);

  /* Once per frame before it is drawn. `blend` says how far the frame
   * lies between the last two steps, from 0 to 1. */
  void (*interpolate)(void *user, double blend);
} NeonSystemDescription;

/* Where a field lies in a component, for reading and writing it in place
 * in the column of a block: the component of entity `i` starts at
 * `column + i * stride`, and the field `offset` bytes into it, as the type
 * its kind says. */
typedef struct NeonFieldLayout
{
  NeonFieldKind kind;
  uint64_t offset;
  uint64_t stride;
} NeonFieldLayout;

/* One corner of a mesh an extension hands over to be drawn, as the engine
 * keeps its own: in metres, with y up, and with the triangles wound
 * anticlockwise seen from outside. */
typedef struct NeonVertex
{
  NeonVector3 position;
  NeonVector3 normal;

  /* Where in the texture the corner is, from 0 to 1, with 0, 0 at the top
   * left. */
  NeonVector2 texture;

  /* Multiplied into the colour of the surface. White leaves it as it is. */
  NeonColor color;
} NeonVertex;

/* Two bodies of the physics began or ended to touch. */
typedef struct NeonPhysicsEvent
{
  /* 1 when they began to touch, 0 when they ended. */
  int began;

  /* Whether one of the two is a trigger. Then `first` is the trigger and
   * `second` what entered or left it. */
  int trigger;

  NeonEntity first;
  NeonEntity second;

  /* Where they touch, and the direction from the first to the second.
   * Both are 0 when the touch ended. */
  NeonVector3 point;
  NeonVector3 normal;
} NeonPhysicsEvent;

/* A ray cast into the world of the physics. */
typedef struct NeonRay
{
  NeonVector3 origin;

  /* Of any length but 0. */
  NeonVector3 direction;

  /* How far the ray reaches. */
  float distance;

  /* A body is looked at when it is in one of these layers, one bit for
   * each of the 32, the lowest for layer 1. 0xFFFFFFFF looks at all. */
  uint32_t layers;

  /* Whether triggers are looked at. */
  int triggers;

  /* The bodies of this entity are skipped, such as the one that asks. 0
   * skips none. */
  NeonEntity ignore;
} NeonRay;

/* What a ray hit first. */
typedef struct NeonRayHit
{
  NeonEntity entity;
  int trigger;
  NeonVector3 point;

  /* Points away from what was hit. */
  NeonVector3 normal;

  /* From the origin of the ray to the point. */
  float distance;
} NeonRayHit;

/* What the application offers an extension. It stays valid until the
 * extension was cleaned up, so the extension may keep the pointer.
 *
 * Every function takes `context` as its first argument, which is the
 * application's own and means nothing to the extension. */
typedef struct NeonExtensionHost
{
  /* NEON_EXTENSION_ABI_VERSION of the application. An extension that
   * needs more than the application has says so and returns 0. */
  uint32_t abi_version;

  void *context;

  /* Writes a line to the log, under the name of the extension. `message`
   * is UTF-8, ends with a zero, and is copied. Since version 1. */
  void (*log)(void *context, NeonLogLevel level, const char *message);

  /* Since version 2: components and the entity store.
   *
   * register_component is for `register_components` of the extension
   * alone. Everything below it is for `start` and what runs after it,
   * until the world is cleaned up. A call at another time, or one that
   * cannot be done, is said in the log as an error and returns 0. */

  /* Makes a kind of component known to the store, to recipes, and to
   * scripts. Returns 0 when the description has a problem, or the name is
   * taken. */
  NeonComponent (*register_component)(void *context, const NeonComponentDescription *description);

  /* The component registered under a name, by anyone, or 0. A component
   * of the engine is found too, but how it lies in memory is the engine's
   * own and not part of this file. */
  NeonComponent (*find_component)(void *context, const char *name);

  /* Creates an entity. A name is unique among the children of the parent,
   * and may be empty or zero; an entity of that name under the parent is
   * returned when there is one. A parent of 0 puts it at the top. */
  NeonEntity (*create_entity)(void *context, const char *name, NeonEntity parent);

  /* Destroys an entity together with its children. */
  void (*destroy_entity)(void *context, NeonEntity entity);

  int (*is_alive)(void *context, NeonEntity entity);

  /* Finds an entity by its names from the top, as `player/camera`. */
  NeonEntity (*find_entity)(void *context, const char *path);

  void (*set_parent)(void *context, NeonEntity entity, NeonEntity parent);

  NeonEntity (*get_parent)(void *context, NeonEntity entity);

  /* Gives the entity the component, or replaces the one it has. `value`
   * points to a component, which is copied. */
  void (*set_component)(void *context, NeonEntity entity, NeonComponent component, const void *value);

  /* The component of the entity, or zero. Valid until the entity gains or
   * loses a component. */
  void *(*get_component)(void *context, NeonEntity entity, NeonComponent component);

  int (*has_component)(void *context, NeonEntity entity, NeonComponent component);

  void (*remove_component)(void *context, NeonEntity entity, NeonComponent component);

  /* Prepares a query for the entities that carry all of the components.
   * Create it once, in `start`, and keep it. */
  NeonQuery (*create_query)(
    void *context,
    const NeonComponent *components,
    uint64_t component_count,
    NeonQueryOrder order);

  /* Hands over every entity that matches the query, block by block.
   * `user` is the caller's own and is handed to `visit` as it is. What is
   * created, destroyed, set, or removed meanwhile takes effect when the
   * query is done; writing to the components of a block takes effect at
   * once. */
  void (*each)(
    void *context,
    NeonQuery query,
    void (*visit)(void *user, const NeonEntityBlock *block),
    void *user);

  /* Since version 3: systems. */

  /* Adds a system to the world. The systems of an extension run in the
   * order they were added, the extensions in the order of their names,
   * before the scripts and the physics. Call it while the extension is
   * started or in `register_components`, not later. Returns 1, or 0 after
   * saying why in the log. */
  int (*register_system)(void *context, const NeonSystemDescription *description);

  /* Since version 4: the fields of any component, the input, files, and
   * scenes. Like the store, they are for `start` and what runs after it. */

  /* Finds a field of a component by the names a recipe writes it with,
   * such as `position` of `Transform`, or `material.color` for a field of
   * a group. It is how an extension reads and changes a component that is
   * not its own, the engine's, a script's, or another extension's, without
   * knowing how it lies in memory. Find it once, in `start`, and keep it.
   * Returns 0 when there is no such component or field. */
  NeonField (*find_field)(void *context, const char *component, const char *path);

  /* Reads a field of numbers of the component of an entity into
   * `numbers`, which has room for four: as many as the kind of the field
   * holds, in the order of its members, a boolean as 0 or 1. Returns how
   * many were written, or 0 when the entity has no such component or the
   * field holds no numbers. */
  int (*get_field)(void *context, NeonEntity entity, NeonField field, double *numbers);

  /* Changes a field of numbers. `numbers` holds as many as the kind of
   * the field does. Returns 1, or 0 after saying why: the entity has no
   * such component, or the field refuses the value. */
  int (*set_field)(void *context, NeonEntity entity, NeonField field, const double *numbers);

  /* Reads a field of text, or the word of a choice, into `buffer` of
   * `capacity` bytes, as UTF-8 that ends with a zero and is cut where it
   * does not fit. Returns the length of the whole text without the zero,
   * so that a buffer that was too small can be made larger. */
  uint64_t (*get_field_text)(void *context, NeonEntity entity, NeonField field, char *buffer, uint64_t capacity);

  /* Changes a field of text, or a choice by its word. Returns as
   * set_field does. */
  int (*set_field_text)(void *context, NeonEntity entity, NeonField field, const char *text);

  /* The actions of the input map, as the game reads them: what the user
   * interface left of the input. */
  int (*is_action_down)(void *context, const char *action);
  int (*was_action_pressed)(void *context, const char *action);
  float (*action_axis)(void *context, const char *action);
  NeonVector2 (*action_axis2)(void *context, const char *action);
  NeonVector3 (*action_axis3)(void *context, const char *action);

  /* Whether a file can be read, at a virtual path such as
   * `assets://models/cube.obj` or `extensions://quake/palette.lmp`. */
  int (*file_exists)(void *context, const char *path);

  /* Reads a whole file and hands its bytes to `receive`, which copies
   * what it keeps: the bytes are gone when the call returns. `user` is
   * the caller's own. Returns 1, or 0 when the file cannot be read, and
   * `receive` is then not called. */
  int (*read_file)(
    void *context,
    const char *path,
    void (*receive)(void *user, const uint8_t *bytes, uint64_t size),
    void *user);

  /* Places a prefab in the world, under a parent or at the top, and
   * returns its entity, or 0 after the world said why not. */
  NeonEntity (*spawn)(void *context, const char *prefab_path, NeonEntity parent);

  /* Asks for another scene, which is loaded when the frame is done. */
  void (*load_scene)(void *context, const char *scene_path);

  /* Since version 5: the physics. A body is a component, `RigidBody`,
   * `CharacterBody`, or `Trigger` with a `Collider`, and is moved through
   * its fields, see find_field. These are what a component cannot say. */

  /* Asks to be told what began and ended to touch. `listen` is called
   * for every event of a frame, before the `update` of the systems, with
   * `user` as it was given. Call it where systems are added, not later.
   * Returns 1, or 0 after saying why. */
  int (*listen_to_physics)(
    void *context,
    void (*listen)(void *user, const NeonPhysicsEvent *event),
    void *user);

  /* Casts a ray and fills in what it hit first. Returns 1 when it hit
   * something, and 0 when not, or when there is no physics. */
  int (*cast_ray)(void *context, const NeonRay *ray, NeonRayHit *hit);

  /* Since version 6: fields in place, values of the user interface, and
   * a prefab placed where it is spawned. */

  /* Says where a field that was found lies in its component, so that a
   * system that runs over thousands of entities reads and writes it in
   * the columns of a query, without a call for each. Asked once, in
   * `start`, of the application that runs, so that nothing of the layout
   * is compiled into the extension. Returns 1, or 0 for a field that has
   * no place of its own, one that is worked out when it is read, or is of
   * a kind this file has no type for; such a field is read with get_field.
   * What is written in place is not checked. */
  int (*get_field_layout)(void *context, NeonField field, NeonFieldLayout *layout);

  /* Sets a value that the files of the user interface refer to as
   * `{name}`. What shows it changes with it. */
  void (*set_ui_number)(void *context, const char *name, double number);
  void (*set_ui_text)(void *context, const char *name, const char *text);

  /* Places a prefab as spawn does, at a position and turned by pitch,
   * yaw, and roll in degrees, both relative to the parent. Either may be
   * zero, and is then what the prefab says. */
  NeonEntity (*spawn_at)(
    void *context,
    const char *prefab_path,
    NeonEntity parent,
    const NeonVector3 *position,
    const NeonVector3 *rotation);

  /* Since version 7: components of the engine given to an entity, lists of
   * text, and meshes and pictures made by the extension, which is how what
   * reads a format of its own shows what it read. */

  /* Gives the entity a component by its name, as a recipe that writes
   * `Renderable: Default` does: with what its fields start with. It is how
   * an entity gets a component of the engine, which an extension has no
   * struct for; its fields are then set with set_field. An entity that has
   * the component keeps it as it is. Returns 1, or 0 after saying why. */
  int (*add_component)(void *context, NeonEntity entity, const char *component);

  /* Changes a field that is a list of text, such as `textures` of
   * `Renderable`, to `count` texts. Returns as set_field does. */
  int (*set_field_texts)(
    void *context,
    NeonEntity entity,
    NeonField field,
    const char *const *texts,
    uint64_t count);

  /* Makes a picture in memory known to the renderer: `width` by `height`
   * pixels of four bytes, red, green, blue, and alpha, row after row from
   * the top, which are copied. A material reads it as the texture
   * `image://<extension>/<name>`, the name of the extension in front, so
   * that two extensions cannot take each other's. A name is set once,
   * before anything that shows it is first drawn. Returns 1, or 0 after
   * saying why. */
  int (*set_image)(void *context, const char *name, uint32_t width, uint32_t height, const uint8_t *pixels);

  /* Gives the `Renderable` of an entity a mesh to draw, in place of a
   * model file: `vertex_count` corners, and three indices into them for
   * every triangle. Both are copied. An entity that is drawn already is
   * drawn with the new mesh from the next frame. Returns 1, or 0 after
   * saying why: the entity has no Renderable, an index names no corner. */
  int (*set_mesh)(
    void *context,
    NeonEntity entity,
    const NeonVertex *vertices,
    uint64_t vertex_count,
    const uint32_t *indices,
    uint64_t index_count);

  /* The name of the extension, as its recipe has it. It is what stands in
   * front of the names of its pictures, and of its files under
   * `extensions://`. Since version 7. */
  const char *name;
} NeonExtensionHost;

/* What an extension brings. The application hands it over with every field
 * zero, and the extension fills in what it has. A function that is left
 * zero is not called. */
typedef struct NeonExtension
{
  /* NEON_EXTENSION_ABI_VERSION the extension was built with. */
  uint32_t abi_version;

  /* The extension's own, handed back as the first argument of every
   * function below. May be zero. */
  void *context;

  /* Called once before the library is closed, after everything the
   * extension brought was let go of. Since version 1. */
  void (*clean_up)(void *context);

  /* Called once when the world comes up, before any system is
   * initialized. The place to register the components the extension
   * brings, and the only one. Since version 2. */
  void (*register_components)(void *context);

  /* Called once after every component is registered, the engine's, the
   * extensions', and the scripts', and before the scene is read. The
   * place to find components and to create queries. Since version 2. */
  void (*start)(void *context);
} NeonExtension;

/* The function every extension exports, under this name. It is called once,
 * right after the library was opened. Returns 1 when the extension has
 * started, and 0 when it cannot, after saying why through `host->log`; its
 * library is then closed and nothing else of it is called.
 *
 *     NEON_EXTENSION_EXPORT int neon_extension_initialize(
 *       const NeonExtensionHost *host, NeonExtension *extension)
 *     {
 *       extension->abi_version = NEON_EXTENSION_ABI_VERSION;
 *       host->log(host->context, NEON_LOG_INFO, "started");
 *       return 1;
 *     }
 */
typedef int (*NeonExtensionInitialize)(const NeonExtensionHost *host, NeonExtension *extension);

#define NEON_EXTENSION_INITIALIZE_NAME "neon_extension_initialize"

#ifdef __cplusplus
}
#endif

#endif /* NEON_EXTENSION_H */
