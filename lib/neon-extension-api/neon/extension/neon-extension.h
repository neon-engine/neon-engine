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
#define NEON_EXTENSION_ABI_VERSION 19

/* What get_input_device answers. */
#define NEON_DEVICE_KEYBOARD_AND_MOUSE 0
#define NEON_DEVICE_GAMEPAD 1 /* a gamepad of no family that is known */
#define NEON_DEVICE_XBOX 2
#define NEON_DEVICE_PLAYSTATION_4 3
#define NEON_DEVICE_PLAYSTATION_5 4
#define NEON_DEVICE_SWITCH 5

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

/* Names an element of the user interface, as returned when it was found or
 * created. 0 stands for none. A number is given once and never again, so the
 * name of an element that is gone, removed or closed with its file, names
 * nothing from then on: a call that is handed it does nothing and returns 0. */
typedef uint64_t NeonUiElement;

/* Names what listens to an element, as returned when it began to. 0 stands
 * for none. */
typedef uint64_t NeonUiListener;

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

/* Something that happened to an element of the user interface. */
typedef struct NeonUiEvent
{
  /* The element it happened to, which is the one that is listened to or
   * one inside of it. */
  NeonUiElement target;

  /* What happened, such as `click`. Valid during the call only. */
  const char *name;

  /* Where the pointer is, in units of the file of the element. */
  float x;
  float y;
} NeonUiEvent;

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
   * that two extensions cannot take each other's. A name that is
   * set again with pixels of the same size is drawn anew from the next
   * frame on, wherever it is shown. Returns 1, or 0 after saying why. */
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

  /* Since version 8: light that was worked out ahead. */

  /* Gives the mesh of an entity, as set_mesh set it, a second set of
   * coordinates: where in the lightmap each corner is, from 0 to 1, one
   * for every corner of the mesh, in their order. The lightmap itself is
   * a texture the material names, the field `material.lightmap` of
   * `Renderable`, such as a picture of set_image. Returns 1, or 0 after
   * saying why: the entity has no mesh, or `count` is not its number of
   * corners. */
  int (*set_mesh_lightmap)(void *context, NeonEntity entity, const NeonVector2 *coordinates, uint64_t count);

  /* Since version 9: sounds from memory. */

  /* Hands the audio the bytes of a sound file under a name: WAV, FLAC, MP3,
   * or Ogg Vorbis, as a file would hold them. They are copied. A sound
   * source plays them as the sound `sound://<extension>/<name>`, the field
   * `sound` of `SoundSource`. A name that is set again names the new bytes
   * for the sources made from then on. Returns 1, or 0 after saying why. */
  int (*set_sound)(void *context, const char *name, const uint8_t *bytes, uint64_t size);

  /* Since version 10: files of the player, and closing the application. */

  /* Writes a whole file under `user://`, the folder of the player and the
   * one place a game may write: a saved game, the settings of its menu.
   * The file is replaced if it is there, and the folders in the path that
   * are not there are created. `size` may be 0, for a file that holds
   * nothing. Returns 1, or 0 after saying why: the path is empty, is not
   * under `user://`, or climbs out with `..`, or the file cannot be
   * written. It is read back with read_file. */
  int (*write_file)(void *context, const char *path, const uint8_t *bytes, uint64_t size);

  /* Calls `visit` with the name of every file directly in a folder, such
   * as `user://saves`, in the order of the names: `slot-1.sav`, without
   * the folder in front. Folders in it, and what is in them, are left
   * out. The name is gone when `visit` returns, which copies what it
   * keeps. `user` is the caller's own. Any folder that can be read can be
   * listed. Returns 1, or 0 when there is no such folder, and `visit` is
   * then not called. */
  int (*list_files)(
    void *context,
    const char *folder,
    void (*visit)(void *user, const char *name),
    void *user);

  /* Asks the application to close, as the quit of its own pause menu
   * does: the frame is finished, and then everything is cleaned up as it
   * is when the window is closed. An extension never ends the process
   * itself, which would skip that. */
  void (*request_quit)(void *context);

  /* Since version 11: numbers for the shaders an extension brings. */

  /* Sets four numbers every shader reads, as `scene.numbers[place]`, at one
   * of 8 places, 0 to 7. They are for the shaders an extension brings and
   * names in a material as `extensions://<name>/assets/shaders/<shader>`:
   * how thick its fog is, a colour, whatever they are written to read. The
   * shaders of the engine read none of them. The numbers stay until they
   * are set again; two extensions that bring shaders agree on their places
   * among themselves. The time needs no setting: every shader reads the
   * seconds the world has run as `scene.time.x`. Returns 1, or 0 after
   * saying why. */
  int (*set_shader_numbers)(void *context, int32_t place, float x, float y, float z, float w);

  /* Since version 12: the elements of the user interface, and the size of
   * the view. The values a file shows as `{name}` are set with
   * set_ui_number and set_ui_text; these are for what a file cannot write
   * ahead: a picture at a place that changes with the game, a row for
   * every saved game, a menu that is shown and closed. Like the store,
   * they are for `start` and what runs after it. See
   * docs/user-interface.md for the files, the elements, and their fields. */

  /* Shows the user interface of a file at a virtual path, such as
   * `extensions://quake/assets/ui/hud.ui.yml`, on top of what is shown
   * already. A file the extension shows already stays as it is. Returns 1,
   * or 0 when the file cannot be used, and every problem of it is logged. */
  int (*ui_show)(void *context, const char *path);

  /* Stops showing a file that ui_show showed. Its elements are gone, and
   * their names name nothing. Returns 1, or 0 after saying that the
   * extension shows no such file. */
  int (*ui_close)(void *context, const char *path);

  /* The first element of a name, which is its `id` in a selector, in every
   * file that is shown, the topmost first. Returns 0 when there is none,
   * which is not an error. */
  NeonUiElement (*ui_find)(void *context, const char *name);

  /* Makes an element from text in the format of the files, such as
   *
   *     type: image
   *     name: face
   *     src: image://quake/face
   *
   * and puts it at the end of what `parent` holds. A parent of 0 is the
   * element at the top of the topmost file. Returns 0 after saying why:
   * the text is empty or wrong, no file is shown, or the parent is gone or
   * takes nothing inside. */
  NeonUiElement (*ui_create)(void *context, const char *yaml, NeonUiElement parent);

  /* Removes an element with everything inside it. Returns 1, or 0 for an
   * element that is gone, and for the element at the top of a file, which
   * ui_close removes. */
  int (*ui_remove)(void *context, NeonUiElement element);

  /* Sets a field of an element, such as `text` of a label, `src` of an
   * image, `value` of a bar, and `checked` of a checkbox. The value is
   * written as a file writes it, whatever the field holds: `Hello`, `0.5`,
   * `true`. Returns 1, or 0 after saying why: the element is gone, has no
   * such field, or the field cannot hold the text. */
  int (*ui_set_field)(void *context, NeonUiElement element, const char *field, const char *text);

  /* Sets a property of the style of an element, as CSS writes both:
   * `left` and `12px`, `background-color` and `#334`. It counts as written
   * for the element itself, on top of what its file and its style sheets
   * write. An empty value takes back what was set. Returns 1, or 0 after
   * saying why. */
  int (*ui_set_style)(void *context, NeonUiElement element, const char *property, const char *text);

  /* Hides an element and shows it again, as `hidden` of its file does.
   * `visible` is 1 or 0. Returns 1, or 0 for an element that is gone. */
  int (*ui_set_visible)(void *context, NeonUiElement element, int visible);

  /* Asks to be told whenever something of that name happens to the
   * element, or to an element inside of it for an event that goes up:
   * `click`, `pointer_enter`, `changed`, and the others of
   * docs/user-interface.md. `listen` is called where the user interface is
   * updated in a frame, with `user` as it was given, and is free to change
   * anything. Returns what ui_unlisten takes it away with, or 0 after
   * saying why. What an extension listens to is taken away when the
   * extension is cleaned up. */
  NeonUiListener (*ui_listen)(
    void *context,
    NeonUiElement element,
    const char *event,
    void (*listen)(void *user, const NeonUiEvent *event),
    void *user);

  /* Takes away what ui_listen returned. `listen` is not called again. */
  void (*ui_unlisten)(void *context, NeonUiListener listener);

  /* The size in pixels of what the camera of the window draws to: the
   * window, or the frame of a run without one. It is what the user
   * interface on the window is laid out in, before its scale. Either
   * pointer may be zero. Returns 1, or 0 when there is no renderer. Since
   * version 12. */
  int (*get_view_size)(void *context, int32_t *width, int32_t *height);

  /* Since version 13: how loud the groups of sounds are. */

  /* Sets how loud every sound of a group is: `effects`, `music`,
   * `ambience`, `voices`, `ui`, or a group of the game. 1 leaves the
   * sounds as they are, 0 is silence, and a number below 0 counts as 0.
   * It is what a menu of settings an extension brings sets with a slider.
   * Returns 1, or 0 after saying why: the name is empty, or the
   * application has no audio. A group that is not there is reported by
   * the audio and left alone. */
  int (*set_group_volume)(void *context, const char *group, float volume);

  /* How loud a group is, as set_group_volume set it or as it started.
   * 0 for a group that is not there, and when there is no audio. */
  float (*get_group_volume)(void *context, const char *group);

  /* Since version 14: freeing what nothing shows any more. */

  /* Asks the renderer to free the materials and textures nothing draws
   * with any more. They stay loaded once nothing shows them, since what
   * showed them may show them again, and the application frees them when
   * one scene takes the place of another. An extension that changes its
   * levels itself, within one scene, calls this when a level is over and
   * the next one was shown. They are freed when the next frame begins.
   * Returns 1, or 0 when there is no renderer. */
  int (*free_unused)(void *context);

  /* Since version 15: components that are turned off and on. */

  /* Turns a component of an entity off, or on again. One that is off keeps
   * what it holds and is not there for whoever asks: no query hands it
   * over, has_component says 0, and get_component gives nothing. Its
   * fields can still be read and set. An entity is hidden by turning its
   * `Renderable` off, and walked through by turning its `Collider` or its
   * `RigidBody` off. `enabled` is 1 or 0. Returns 1, or 0 when the entity
   * has no such component. */
  int (*set_component_enabled)(void *context, NeonEntity entity, NeonComponent component, int enabled);

  /* Whether the entity has the component and it is turned on, 1 or 0. */
  int (*is_component_enabled)(void *context, NeonEntity entity, NeonComponent component);

  /* Since version 16: pools. A pool is an entity with the component
   * `PoolManager`: it holds a fixed number of instances of one or more kinds,
   * made ahead and turned off, and hands them out and takes them back, so
   * that what comes and goes often is never made or destroyed while the
   * game runs. A kind is named as a file is: the path of the prefab its
   * instances were spawned from, or `instance://` and a name for instances an
   * extension made itself. See docs/pools.md. */

  /* Hands a pool instances that are there already, under a kind such as
   * `instance://nail`. They are turned off and wait. Returns how many it took:
   * one that is gone, or belongs to a pool already, is left out. 0 as well
   * when `pool` has no `PoolManager`, or the kind has no name. */
  uint64_t (*pool_add)(
    void *context,
    NeonEntity pool,
    const char *kind,
    const NeonEntity *instances,
    uint64_t count);

  /* Hands out an instance of a kind, turned on: the one that has waited
   * longest. Returns 0 when the pool holds no such kind, and when all of
   * the kind are out, which the application says once in its log. A pool
   * never makes more than it holds. */
  NeonEntity (*pool_acquire)(void *context, NeonEntity pool, const char *kind);

  /* Takes an instance back: it is turned off and waits at the end of the
   * queue of its kind. Returns 1, or 0 for an entity that belongs to no
   * pool or waits already. */
  int (*pool_release)(void *context, NeonEntity instance);

  /* How many instances of a kind a pool holds, and how many of them wait. */
  uint64_t (*pool_count)(void *context, NeonEntity pool, const char *kind);
  uint64_t (*pool_free_count)(void *context, NeonEntity pool, const char *kind);

  /* Since version 17: how the window is shown, while the application
   * runs. It is what the video settings of a menu an extension brings set.
   * What a player chose is kept by the game, as it keeps its other
   * settings: these change what is shown now. */

  /* Shows the window another way: 0 as a window, 1 without borders over
   * the whole display, 2 as the one thing on a display that is switched to
   * the size of the window. Returns 1, or 0 when it cannot be shown that
   * way, which is said, and when there is no window. */
  int (*set_window_mode)(void *context, int32_t mode);

  /* How the window is shown now: 0, 1, or 2, as set_window_mode takes it. */
  int32_t (*get_window_mode)(void *context);

  /* Gives the window another size, in points. A window has that size; a
   * display the window takes over is switched to the size it offers that
   * is nearest; a window without borders covers its display and keeps the
   * size for when it is a window again. Returns 1, or 0 for a size that
   * is not above zero and when there is no window to size. */
  int (*set_window_size)(void *context, int32_t width, int32_t height);

  /* The size of the window in points. Either pointer may be zero. Returns
   * 1, or 0 when there is no window. get_view_size gives the pixels that
   * are drawn to, which are more on a display of high density. */
  int (*get_window_size)(void *context, int32_t *width, int32_t *height);

  /* Calls `visit` with every size the display of the window offers, in
   * points, the largest first and each once: what a menu lets a player
   * choose from. `user` is the caller's own. Returns how many there are,
   * which is 0 without a display. */
  uint64_t (*list_display_sizes)(
    void *context,
    void (*visit)(void *user, int32_t width, int32_t height),
    void *user);

  /* Has a frame wait for the screen before it is shown, 1, or not, 0.
   * With it no frame is torn and no more frames are drawn than the screen
   * shows. Without it frames are shown as soon as they are done, where the
   * driver can. Returns 1, or 0 when nothing is shown on a screen. */
  int (*set_vertical_sync)(void *context, int enabled);

  /* Whether a frame waits for the screen, 1 or 0. */
  int (*get_vertical_sync)(void *context);

  /* Holds the frames to at most that many a second: a number from 30 to
   * 300, or 0 for as many as can be drawn. A number beyond them is held
   * to the nearest. It holds with vertical sync on and off. Returns 1, or
   * 0 when there is no window. */
  int (*set_frame_limit)(void *context, int32_t frames_per_second);

  /* The limit that holds, or 0 for none. */
  int32_t (*get_frame_limit)(void *context);

  /* --- Since version 18 --- */

  /* What the player used last, for the buttons a game shows: one of the
   * NEON_DEVICE_ values. A button or a stick of a gamepad makes it the
   * gamepad; a key makes it the keyboard and the mouse, and so does the
   * mouse unless the setting input.mouse_switches_device is false. A
   * gamepad says which family of controller it is as far as it knows. */
  int32_t (*get_input_device)(void *context);

  /* --- Since version 19: folders, and the focus of the user interface --- */

  /* Calls `visit` with the name of every folder directly in a folder, such
   * as `assets://basedirs`, in the order of the names: `librequake`,
   * without the folder in front, as list_files does for files. A folder
   * that holds no file, not even below it, is not seen. The name is gone
   * when `visit` returns, which copies what it keeps. `user` is the
   * caller's own. Returns 1, or 0 when there is no such folder, and
   * `visit` is then not called. */
  int (*list_folders)(void *context, const char *folder, void (*visit)(void *user, const char *name), void *user);

  /* Moves the focus of the user interface to an element, as a direction
   * or a click would: what accept then presses. For a menu whose buttons
   * the extension made after the file was shown, which the focus a file
   * takes when it is shown does not reach. Returns 1, or 0 when the
   * element is gone or takes no focus. */
  int (*ui_focus)(void *context, NeonUiElement element);
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
